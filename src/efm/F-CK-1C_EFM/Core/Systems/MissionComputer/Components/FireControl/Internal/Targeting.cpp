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

void Targeting::acquire(AcquisitionInput input)
{
	reapply_mode();
	latest_designation_request_ = requests_.submit({ RadarOperation::BeginAcquisition, data_.acquisition_mode, input });
	data_.designation.requested = true;
}

void Targeting::cancel()
{
	data_.designation.requested = false;
	latest_designation_request_ = requests_.submit({ RadarOperation::CancelAcquisition });
}

void Targeting::handle(const Command& command)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	const bool dogfight = mode_.operating_mode == OperatingMode::Dogfight;
	switch (command.id)
	{
	case CommandId::SetMissileUncage:
		if (pressed && mode_.air_to_air())
		{
			requests_.submit({ RadarOperation::EnsureOn });
			reapply_mode();
		}
		else if (!pressed && dogfight)
			cancel();
		break;
	case CommandId::SetTmsUp:
		if (pressed)
		{
			if (dogfight)
				select_mode(TargetAcquisitionMode::Helmet);
			acquire(dogfight ? AcquisitionInput::Pulse : AcquisitionInput::Hold);
		}
		else if (!dogfight)
			requests_.submit({ RadarOperation::ReleaseAcquisition });
		break;
	case CommandId::PressTmsDown:
		if (pressed)
		{
			if (dogfight)
			{
				select_mode(TargetAcquisitionMode::VerticalScan);
				acquire(AcquisitionInput::Pulse);
			}
			else
				cancel();
		}
		break;
	case CommandId::PressTmsRight:
		if (pressed)
		{
			if (dogfight)
				select_mode(TargetAcquisitionMode::Hud);
			else if (mode_.operating_mode == OperatingMode::MissileOverride)
			{
				latest_designation_request_ = requests_.submit({ RadarOperation::SelectNextTarget });
				data_.designation.requested = true;
			}
		}
		break;
	default:
		break;
	}
}

void Targeting::on_operating_mode_selected(const OperatingModeSelected&)
{
	if (mode_.operating_mode == OperatingMode::Navigation)
	{
		data_.acquisition_mode = TargetAcquisitionMode::None;
		cancel();
		requests_.submit({ RadarOperation::SelectNavigation });
		return;
	}
	const bool dogfight = mode_.operating_mode == OperatingMode::Dogfight;
	if (!dogfight)
		cancel();
	requests_.submit({ RadarOperation::EnsureOn });
	select_mode(dogfight ? TargetAcquisitionMode::Helmet : TargetAcquisitionMode::BeyondVisualRange);
	if (dogfight)
		acquire(AcquisitionInput::Pulse);
}

void Targeting::before_weapon_release()
{
	reapply_mode();
}

void Targeting::on_weapon_release_ended()
{
	cancel();
}

void Targeting::step()
{
	for (const auto& result : radar_.command_results)
		if (result.request_sequence == latest_designation_request_ && !result.accepted)
			data_.designation.requested = false;
	auto& target = data_.designation;
	const auto& observation = radar_.observation;
	target.radar_observation = observation.status;
	target.radar_tracking = observation.status.available && observation.mode == RadarMode::SingleTargetTrack;
	target.radar_azimuth_rad = target.radar_tracking ? observation.stt_azimuth_rad : 0.0;
	target.radar_elevation_rad = target.radar_tracking ? observation.stt_elevation_rad : 0.0;
	target.radar_range_m = target.radar_tracking ? observation.stt_range_m : 0.0;
}
}
