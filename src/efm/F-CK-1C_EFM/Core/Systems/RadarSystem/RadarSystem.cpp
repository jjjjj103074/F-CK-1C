#include "RadarSystem.h"
#include "../SystemPipeline.h"
#include <algorithm>

namespace Core::Systems
{
void RadarSystem::setup(SystemSetup& setup)
{
	setup.update_rate_hz(32);
	setup.read(AircraftDataKeys::kFrameInput);
	setup.read(AircraftDataKeys::kRadarRequests);
	setup.publish(AircraftDataKeys::kRadarSnapshot, output_);
}

void RadarSystem::apply_mode(TargetAcquisitionMode mode, std::uint64_t parent)
{
	output_.requested_acquisition_mode = mode;
	switch (mode)
	{
	case TargetAcquisitionMode::Helmet:
		actions_.append(MissionActionKind::ModeHelmet, parent);
		break;
	case TargetAcquisitionMode::VerticalScan:
		actions_.append(MissionActionKind::ModeVerticalScan, parent);
		break;
	case TargetAcquisitionMode::BeyondVisualRange:
		actions_.append(MissionActionKind::ModeBeyondVisualRange, parent);
		actions_.append(MissionActionKind::ModeFi0, parent);
		break;
	default:
		break;
	}
}

void RadarSystem::finish_acquisition(std::uint64_t parent)
{
	actions_.append(MissionActionKind::LockFinish, parent);
	actions_.append(MissionActionKind::ChangeLockUp, parent);
	acquisition_.clear();
}

void RadarSystem::execute(const RadarRequest& request)
{
	const auto previous = actions_.latest_sequence();
	RadarCommandResult response{ request.sequence };
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
		apply_mode(request.command.mode, request.sequence);
		break;
	case RadarOperation::BeginAcquisition:
		if (!output_.adapter_available)
		{
			response.accepted = false;
			break;
		}
		actions_.append(MissionActionKind::LockStart, request.sequence);
		acquisition_.begin(actions_.append(MissionActionKind::ChangeLock, request.sequence));
		if (request.command.input == AcquisitionInput::Pulse)
			acquisition_.request_release();
		break;
	case RadarOperation::ReleaseAcquisition:
		if (acquisition_.active())
			acquisition_.request_release();
		break;
	case RadarOperation::CancelAcquisition:
		if (acquisition_.active())
			acquisition_.request_release();
		else
			finish_acquisition(request.sequence);
		break;
	case RadarOperation::SelectNextTarget:
		actions_.append(MissionActionKind::ChangeLock, request.sequence);
		break;
	case RadarOperation::SelectNavigation:
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
	if (acquisition_.release_due(context.input_time, actions_))
		finish_acquisition(0);
	output_.status = { true, output_.status.revision + 1 };
	output_.action_requests = actions_.pending();
	result.publish(AircraftDataKeys::kRadarSnapshot, output_);
}
}
