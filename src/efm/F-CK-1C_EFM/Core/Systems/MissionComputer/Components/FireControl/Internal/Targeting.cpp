#include "Targeting.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc::Internal
{
Targeting::Targeting(const ModeData& mode, const RadarSnapshot& radar, TargetingData& data, RadarRequestQueue& requests)
    : mode_(mode),
      radar_(radar),
      data_(data),
      requests_(requests)
{
}

void Targeting::select_mode(TargetAcquisitionMode mode)
{
	data_.acquisition_mode = mode;
	requests_.submit({ RadarOperation::SetAcquisitionMode, mode });
}

void Targeting::reapply_mode()
{
	if (mode_.air_to_air())
		requests_.submit({ RadarOperation::SetAcquisitionMode, data_.acquisition_mode });
}

void Targeting::submit_tms(const RadarCommand& command)
{
	const auto sequence = requests_.submit(command);
	data_.last_tms_request = RadarRequest{ sequence, command };
}

void Targeting::acquire(AcquisitionInput input)
{
	reapply_mode();
	submit_tms({ RadarOperation::BeginAcquisition, data_.acquisition_mode, input });
	latest_designation_request_ = data_.last_tms_request->sequence;
	data_.designation.requested = true;
}

void Targeting::cancel()
{
	data_.designation.requested = false;
	latest_designation_request_ = requests_.submit({ RadarOperation::CancelAcquisition });
}

bool Targeting::tracking() const
{
	return radar_.observation.status.available && radar_.observation.mode == RadarMode::SingleTargetTrack;
}

void Targeting::acquire_air_combat(TargetAcquisitionMode mode)
{
	// 同一次操作先拒絕舊目標，再提出指定掃描型式的捕獲意圖。
	if (tracking())
		cancel();
	select_mode(mode);
	acquire(AcquisitionInput::Pulse);
}

void Targeting::handle_tms_up(bool pressed)
{
	if (data_.search_mode == RadarSearchMode::AirCombat)
	{
		if (pressed && !tms_up_held_)
			acquire_air_combat(TargetAcquisitionMode::Boresight);
		tms_up_held_ = pressed;
		return;
	}
	if (pressed)
	{
		if (tms_up_held_)
			return;
		tms_up_held_ = true;
		submit_tms({ RadarOperation::BeginSpotlight });
	}
	else if (tms_up_held_)
	{
		tms_up_held_ = false;
		requests_.submit({ RadarOperation::EndSpotlight });
		// 手冊在放開 TMS 時提出追蹤要求；設備端處理實際捕獲與脈衝保持。
		acquire(AcquisitionInput::Pulse);
	}
}

void Targeting::handle_tms_down()
{
	if (data_.search_mode == RadarSearchMode::AirCombat)
	{
		acquire_air_combat(TargetAcquisitionMode::VerticalScan);
		return;
	}
	data_.designation.requested = false;
	tms_up_held_ = false;
	requests_.submit({ RadarOperation::EndSpotlight });
	submit_tms({ RadarOperation::CancelAcquisition });
	latest_designation_request_ = data_.last_tms_request->sequence;
}

void Targeting::apply_search_toggle()
{
	tms_right_long_ = true;
	data_.search_mode = data_.search_mode == RadarSearchMode::TrackWhileScan ? RadarSearchMode::RangeWhileSearch
	                                                                         : RadarSearchMode::TrackWhileScan;
	submit_tms({ RadarOperation::SetSearchMode, data_.acquisition_mode, AcquisitionInput::Pulse, data_.search_mode });
}

void Targeting::handle_tms_right(bool pressed, std::chrono::nanoseconds time)
{
	if (data_.search_mode == RadarSearchMode::AirCombat)
	{
		if (pressed && !tms_right_held_)
			acquire_air_combat(TargetAcquisitionMode::WideScan);
		tms_right_held_ = pressed;
		return;
	}
	if (pressed && !tms_right_held_)
	{
		tms_right_held_ = true;
		tms_right_long_ = false;
		tms_right_pressed_at_ = time;
	}
	else if (!pressed && tms_right_held_)
	{
		if (!tms_right_long_)
		{
			if (time - tms_right_pressed_at_ > std::chrono::seconds(1))
				apply_search_toggle();
			else
				submit_tms({ RadarOperation::StepPriorityTarget });
		}
		tms_right_held_ = false;
	}
}

void Targeting::handle(const Command& command, std::chrono::nanoseconds input_time)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	if (mode_.operating_mode() == OperatingMode::AirToGround)
	{
		if (pressed)
		{
			switch (command.id)
			{
			case CommandId::SetTmsUp:
				submit_tms({ RadarOperation::DesignateGroundTarget });
				break;
			case CommandId::PressTmsDown:
				submit_tms({ RadarOperation::RejectGroundTarget });
				break;
			case CommandId::PressTmsRight:
				submit_tms({ RadarOperation::CycleSightingPoint });
				break;
			default:
				break;
			}
		}
		return;
	}
	if (!mode_.air_to_air())
		return;
	switch (command.id)
	{
	case CommandId::SetTmsUp:
		handle_tms_up(pressed);
		break;
	case CommandId::PressTmsDown:
		if (pressed)
			handle_tms_down();
		break;
	case CommandId::PressTmsLeft:
		if (pressed)
			submit_tms({ RadarOperation::ExpandTargetData });
		break;
	case CommandId::PressTmsRight:
		handle_tms_right(pressed, input_time);
		break;
	default:
		break;
	}
}

void Targeting::on_operating_mode_selected(const OperatingModeSelected& event)
{
	if (event.previous.operating_mode() == event.selected.operating_mode())
		return;
	tms_up_held_ = false;
	tms_right_held_ = false;
	tms_right_long_ = false;
	requests_.submit({ RadarOperation::EndSpotlight });
	const bool retain_track = event.previous.air_to_air() && event.selected.air_to_air() && tracking();
	if (!retain_track)
		cancel();
	if (!mode_.air_to_air())
	{
		data_.acquisition_mode = TargetAcquisitionMode::None;
		data_.search_mode = mode_.master_mode == MasterMode::AirToGround ? RadarSearchMode::GroundMap
		                                                                 : RadarSearchMode::RangeWhileSearch;
		requests_.submit({ RadarOperation::SelectNavigation });
		return;
	}
	const bool dogfight = mode_.override_mode == OverrideMode::Dogfight;
	data_.search_mode = dogfight ? RadarSearchMode::AirCombat : RadarSearchMode::RangeWhileSearch;
	requests_.submit({ RadarOperation::EnsureOn });
	requests_.submit(
	    { RadarOperation::SetSearchMode, TargetAcquisitionMode::None, AcquisitionInput::Pulse, data_.search_mode });
	select_mode(dogfight ? TargetAcquisitionMode::Boresight : TargetAcquisitionMode::BeyondVisualRange);
}

void Targeting::before_weapon_release()
{
	reapply_mode();
}

void Targeting::step(std::chrono::nanoseconds time)
{
	if (data_.search_mode != RadarSearchMode::AirCombat && tms_right_held_ && !tms_right_long_ &&
	    time - tms_right_pressed_at_ > std::chrono::seconds(1))
		apply_search_toggle();
	for (const auto& result : radar_.command_results)
		if (result.request_sequence == latest_designation_request_ && !result.accepted)
			data_.designation.requested = false;
	auto& target = data_.designation;
	const auto& observation = radar_.observation;
	target.radar_observation = observation.status;
	target.radar_tracking = tracking();
	target.radar_azimuth_rad = target.radar_tracking ? observation.stt_azimuth_rad : 0.0;
	target.radar_elevation_rad = target.radar_tracking ? observation.stt_elevation_rad : 0.0;
	target.radar_range_m = target.radar_tracking ? observation.stt_range_m : 0.0;
}
}
