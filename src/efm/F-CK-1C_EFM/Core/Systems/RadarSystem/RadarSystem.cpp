#include "RadarSystem.h"
#include "Internal/SearchProfiles.h"
#include "../SystemPipeline.h"
#include <algorithm>
#include "Common/CommandValue.h"

namespace Core::Systems
{
void RadarSystem::setup(SystemSetup& setup)
{
	setup.update_rate_hz(32);
	setup.read(AircraftDataKeys::kFrameInput);
	setup.read(AircraftDataKeys::kRadarRequests);
	setup.publish(AircraftDataKeys::kRadarSnapshot, output_);
	const std::array<CommandId, 4> commands = { CommandId::SetRadarCursorUp,
		                                        CommandId::SetRadarCursorDown,
		                                        CommandId::SetRadarCursorLeft,
		                                        CommandId::SetRadarCursorRight };
	for (std::size_t index = 0; index < commands.size(); ++index)
		setup.register_command_handler(
		    commands[index],
		    [this, index](const SystemActionContext&, const Command& command)
		    {
			    const bool pressed = Common::command_value_is_pressed(command.value_normalized);
			    if (!pressed ||
			        (output_.scan.enabled && output_.active_search_mode == RadarSearchMode::RangeWhileSearch))
				    cursor_held_[index] = pressed;
		    });
}

void RadarSystem::set_scan(RadarSearchMode mode, std::uint64_t parent)
{
	if (output_.scan.enabled && output_.active_search_mode == mode)
		return;
	output_.automatic_acquisition_requested = false;
	if (acquisition_.active())
		finish_acquisition(parent);
	cursor_held_.fill(false);
	cursor_initialized_ = false;
	const auto revision = output_.scan.revision + 1;
	const bool acm = mode == RadarSearchMode::AirCombat;
	output_.scan = acm ? RadarProfiles::boresight() : RadarProfiles::range_while_search();
	output_.scan.revision = revision;
	output_.active_search_mode = mode;
	output_.requested_acquisition_mode =
	    acm ? TargetAcquisitionMode::Boresight : TargetAcquisitionMode::BeyondVisualRange;
	if (acm)
		actions_.append(MissionActionKind::ModeBoresight, parent);
	else
	{
		actions_.append(MissionActionKind::ModeBeyondVisualRange, parent);
		actions_.append(MissionActionKind::ModeFi0, parent);
	}
}

bool RadarSystem::apply_mode(TargetAcquisitionMode mode, std::uint64_t parent)
{
	switch (mode)
	{
	case TargetAcquisitionMode::Boresight:
		set_scan(RadarSearchMode::AirCombat, parent);
		return true;
	case TargetAcquisitionMode::BeyondVisualRange:
		set_scan(RadarSearchMode::RangeWhileSearch, parent);
		return true;
	default:
		return false;
	}
}

void RadarSystem::begin_acquisition(std::uint64_t parent, AcquisitionInput input)
{
	if (output_.observation.status.available && output_.observation.mode == RadarMode::SingleTargetTrack)
		return;
	if (acquisition_.active())
		return;
	acquisition_.begin(actions_.append(MissionActionKind::LockStart, parent));
	if (input == AcquisitionInput::Pulse)
		acquisition_.request_release();
}

void RadarSystem::finish_acquisition(std::uint64_t parent)
{
	actions_.append(MissionActionKind::LockFinish, parent);
	acquisition_.clear();
}

void RadarSystem::cancel_acquisition(std::uint64_t parent)
{
	output_.automatic_acquisition_requested = false;
	// 原生 509 在 STT 時返回搜尋；510 結束仍在進行的捕獲輸入。
	if (output_.observation.status.available && output_.observation.mode == RadarMode::SingleTargetTrack &&
	    !stt_cancel_requested_)
	{
		actions_.append(MissionActionKind::LockStart, parent);
		stt_cancel_requested_ = true;
	}
	finish_acquisition(parent);
}

void RadarSystem::execute(const RadarRequest& request)
{
	const auto previous = actions_.latest_sequence();
	RadarCommandResult response{ request.sequence };
	response.operation = request.command.operation;
	switch (request.command.operation)
	{
	case RadarOperation::EnsureOn:
		output_.power_requested = true;
		if (output_.power_on)
			radar_on_requested_ = false;
		else if (!radar_on_requested_)
		{
			actions_.append(MissionActionKind::EnsureRadarOn, request.sequence);
			radar_on_requested_ = true;
		}
		break;
	case RadarOperation::SetAcquisitionMode:
		response.implementation = apply_mode(request.command.mode, request.sequence) ? RadarImplementation::Simplified
		                                                                             : RadarImplementation::Reserved;
		break;
	case RadarOperation::BeginAcquisition:
		if (!output_.adapter_available || !output_.scan.enabled || stt_cancel_requested_)
		{
			response.accepted = false;
			break;
		}
		output_.automatic_acquisition_requested = output_.active_search_mode == RadarSearchMode::AirCombat;
		begin_acquisition(request.sequence,
		                  output_.automatic_acquisition_requested ? AcquisitionInput::Hold : request.command.input);
		break;
	case RadarOperation::ReleaseAcquisition:
		if (acquisition_.active() && !output_.automatic_acquisition_requested)
			acquisition_.request_release();
		break;
	case RadarOperation::CancelAcquisition:
		cancel_acquisition(request.sequence);
		break;
	case RadarOperation::SelectNextTarget:
		actions_.append(MissionActionKind::ChangeLock, request.sequence);
		break;
	case RadarOperation::BeginSpotlight:
	case RadarOperation::EndSpotlight:
		output_.spotlight_requested = request.command.operation == RadarOperation::BeginSpotlight;
		response.implementation = RadarImplementation::Reserved;
		break;
	case RadarOperation::SetSearchMode:
		output_.requested_search_mode = request.command.search_mode;
		if (request.command.search_mode == RadarSearchMode::RangeWhileSearch ||
		    request.command.search_mode == RadarSearchMode::AirCombat)
		{
			set_scan(request.command.search_mode, request.sequence);
			response.implementation = RadarImplementation::Simplified;
		}
		else
			response.implementation = RadarImplementation::Reserved;
		break;
	case RadarOperation::ExpandTargetData:
	case RadarOperation::StepPriorityTarget:
	case RadarOperation::CycleSightingPoint:
	case RadarOperation::DesignateGroundTarget:
	case RadarOperation::RejectGroundTarget:
		response.implementation = RadarImplementation::Reserved;
		break;
	case RadarOperation::SelectNavigation:
		if (acquisition_.active() || output_.automatic_acquisition_requested ||
		    (output_.observation.status.available && output_.observation.mode == RadarMode::SingleTargetTrack &&
		     !stt_cancel_requested_))
			cancel_acquisition(request.sequence);
		output_.automatic_acquisition_requested = false;
		output_.scan.enabled = false;
		cursor_held_.fill(false);
		output_.requested_acquisition_mode = TargetAcquisitionMode::None;
		actions_.append(MissionActionKind::ModeCannon, request.sequence);
		break;
	}
	const auto last = actions_.latest_sequence();
	if (last > previous)
	{
		response.first_action_sequence = previous + 1;
		response.last_action_sequence = last;
	}
	output_.command_results.push_back(response);
}

void RadarSystem::update_cursor(const SystemStepContext& context)
{
	// 每個設備 tick 積分固定時間，補算時沿用相同游標速度。
	const double elapsed_s = context.dt_s;
	if (!output_.scan.enabled || output_.active_search_mode != RadarSearchMode::RangeWhileSearch ||
	    (output_.observation.status.available && output_.observation.mode == RadarMode::SingleTargetTrack))
		return;
	const auto& observation = output_.observation;
	const bool moving = std::any_of(cursor_held_.begin(),
	                                cursor_held_.end(),
	                                [](bool held)
	                                {
		                                return held;
	                                });
	const bool initialize = !cursor_initialized_ && !observation.tdc_available;
	if (!cursor_initialized_)
	{
		cursor_azimuth_rad_ = observation.tdc_available ? observation.tdc_azimuth_rad : 0.0;
		cursor_range_m_ = observation.tdc_available ? observation.tdc_range_m : output_.scan.display_range_m / 2.0;
		cursor_initialized_ = true;
	}
	else if (!moving && observation.tdc_available)
	{
		cursor_azimuth_rad_ = observation.tdc_azimuth_rad;
		cursor_range_m_ = observation.tdc_range_m;
	}
	if (moving)
	{
		cursor_azimuth_rad_ += (static_cast<double>(cursor_held_[3]) - static_cast<double>(cursor_held_[2])) *
		                       Common::rad(30.0) * elapsed_s;
		cursor_range_m_ += (static_cast<double>(cursor_held_[0]) - static_cast<double>(cursor_held_[1])) *
		                   output_.scan.display_range_m * 0.25 * elapsed_s;
	}
	cursor_azimuth_rad_ =
	    std::clamp(cursor_azimuth_rad_, -output_.scan.azimuth_width_rad / 2.0, output_.scan.azimuth_width_rad / 2.0);
	cursor_range_m_ = std::clamp(cursor_range_m_, 300.0, output_.scan.display_range_m);
	output_.scan.tdc_azimuth_rad = cursor_azimuth_rad_;
	output_.scan.tdc_range_m = cursor_range_m_;
	output_.scan.override_tdc = moving || initialize;
}

void RadarSystem::step(const SystemStepContext& context, const AircraftDataView& aircraft, SystemResult& result)
{
	const auto& cockpit = aircraft.read(AircraftDataKeys::kFrameInput).cockpit;
	const auto& requests = aircraft.read(AircraftDataKeys::kRadarRequests);
	actions_.acknowledge(cockpit.external_action_feedback.radar_acknowledged_through);
	output_.observation = cockpit.radar;
	output_.adapter_available = cockpit.mission_adapter.available;
	output_.power_on = cockpit.mission_adapter.radar_power_on;
	if (output_.power_on)
		radar_on_requested_ = false;
	if (output_.observation.status.available && output_.observation.mode != RadarMode::SingleTargetTrack)
		stt_cancel_requested_ = false;
	output_.command_results.erase(std::remove_if(output_.command_results.begin(),
	                                             output_.command_results.end(),
	                                             [&](const auto& response)
	                                             {
		                                             return response.request_sequence <= requests.acknowledged_through;
	                                             }),
	                              output_.command_results.end());
	for (const auto& request : requests.items)
	{
		if (request.sequence <= last_request_)
			continue;
		execute(request);
		last_request_ = request.sequence;
	}
	const bool tracking =
	    output_.observation.status.available && output_.observation.mode == RadarMode::SingleTargetTrack;
	if (tracking && acquisition_.active())
		acquisition_.request_release();
	if (acquisition_.release_due(context.input_time, actions_))
		finish_acquisition(0);
	if (output_.automatic_acquisition_requested && output_.scan.enabled && output_.adapter_available &&
	    output_.observation.status.available && output_.observation.mode == RadarMode::Search)
		begin_acquisition(0, AcquisitionInput::Hold);
	if (output_.scan.enabled)
	{
		// 捕獲距離門隨 STT 接手恢復至既有感測器距離校準。
		output_.scan.maximum_search_range_m = output_.active_search_mode == RadarSearchMode::AirCombat && !tracking
		                                          ? RadarProfiles::kBoresightAcquisitionRangeM
		                                          : RadarProfiles::kDetectionModelRangeM;
		output_.scan.override_tdc = output_.active_search_mode == RadarSearchMode::AirCombat && !tracking;
	}
	update_cursor(context);
	output_.status = { true, output_.status.revision + 1 };
	output_.action_requests = actions_.pending();
	result.publish(AircraftDataKeys::kRadarSnapshot, output_);
}
}
