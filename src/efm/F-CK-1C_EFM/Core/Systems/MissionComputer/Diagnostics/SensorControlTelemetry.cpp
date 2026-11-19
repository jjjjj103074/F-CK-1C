#include "SensorControlTelemetry.h"

#include "../../SystemPipeline.h"
#include "Common/CommandValue.h"

namespace
{
using namespace Core;

const char* source_name(SensorControlSource source)
{
	switch (source)
	{
	case SensorControlSource::Radar:
		return "FCR";
	case SensorControlSource::Hud:
		return "HUD";
	case SensorControlSource::Navigation:
		return "HSD";
	default:
		return "NONE";
	}
}

const char* location_name(DisplayLocation location)
{
	switch (location)
	{
	case DisplayLocation::Hud:
		return "HUD";
	case DisplayLocation::LeftMfd:
		return "LEFT MFD";
	case DisplayLocation::RightMfd:
		return "RIGHT MFD";
	default:
		return "NONE";
	}
}

const char* function_name(DisplayFunction function)
{
	switch (function)
	{
	case DisplayFunction::FireControlRadar:
		return "FCR";
	case DisplayFunction::StoresManagement:
		return "SMS";
	case DisplayFunction::HorizontalSituation:
		return "HSD (SHELL)";
	default:
		return "NONE";
	}
}

const char* mode_name(OperatingMode mode)
{
	switch (mode)
	{
	case OperatingMode::AirToAir:
		return "A-A";
	case OperatingMode::AirToGround:
		return "A-G";
	case OperatingMode::Dogfight:
		return "DGFT";
	case OperatingMode::MissileOverride:
		return "MSL OVRD";
	default:
		return "NAV";
	}
}

const char* search_name(RadarSearchMode mode)
{
	switch (mode)
	{
	case RadarSearchMode::TrackWhileScan:
		return "TWS (SHELL)";
	case RadarSearchMode::AirCombat:
		return "ACM";
	case RadarSearchMode::GroundMap:
		return "GROUND (SHELL)";
	default:
		return "RWS";
	}
}

const char* pattern_name(TargetAcquisitionMode mode)
{
	switch (mode)
	{
	case TargetAcquisitionMode::Boresight:
		return "BORE";
	case TargetAcquisitionMode::WideScan:
		return "30X20 (SIMPLIFIED)";
	case TargetAcquisitionMode::VerticalScan:
		return "10X60 (SIMPLIFIED)";
	case TargetAcquisitionMode::BeyondVisualRange:
		return "CRM";
	default:
		return "NONE";
	}
}

const char* operation_name(RadarOperation operation)
{
	switch (operation)
	{
	case RadarOperation::BeginSpotlight:
		return "SPOTLIGHT";
	case RadarOperation::EndSpotlight:
		return "END SPOTLIGHT";
	case RadarOperation::BeginAcquisition:
		return "DESIGNATE";
	case RadarOperation::CancelAcquisition:
		return "REJECT TARGET";
	case RadarOperation::SetSearchMode:
		return "SEARCH MODE";
	case RadarOperation::ExpandTargetData:
		return "EXPAND TARGET DATA";
	case RadarOperation::StepPriorityTarget:
		return "STEP PRIORITY TARGET";
	case RadarOperation::CycleSightingPoint:
		return "CYCLE SIGHTING POINT";
	case RadarOperation::DesignateGroundTarget:
		return "GROUND DESIGNATE";
	case RadarOperation::RejectGroundTarget:
		return "GROUND REJECT";
	default:
		return "RADAR REQUEST";
	}
}

const char* observed_mode_name(const RadarObservation& radar)
{
	if (!radar.status.available)
		return "UNAVAILABLE";
	switch (radar.mode)
	{
	case RadarMode::Off:
		return "OFF";
	case RadarMode::Search:
		return "SEARCH";
	case RadarMode::LockAttempt:
		return "LOCK ATTEMPT";
	case RadarMode::SingleTargetTrack:
		return "STT";
	default:
		return "UNKNOWN";
	}
}
}

namespace Core::Systems::Mc
{
void SensorControlTelemetry::setup(SystemSetup& setup)
{
	const std::array<const char*, kChannelCount> names = {
		"mc_display_mode",    "mc_mfd_left",       "mc_mfd_right",      "mc_soi_source",  "mc_soi_location",
		"mc_display_context", "mc_tms_input",      "mc_tms_receiver",   "mc_tms_request", "mc_tms_result",
		"mc_radar_requested", "mc_radar_observed", "mc_dms_format_step"
	};
	const std::array<const char*, kChannelCount> labels = {
		"MC / Display Mode", "MFD Left / Source", "MFD Right / Source",     "SOI Source",  "SOI Display",
		"Display Context",   "TMS Input",         "TMS Receiver",           "TMS Request", "TMS Result",
		"Radar Requested",   "Radar Observed",    "DMS Format Step (Shell)"
	};
	for (std::size_t i = 0; i < channels_.size(); ++i)
		channels_[i] = setup.declare_debug_channel<std::string>(
		    { names[i], labels[i], DebugTelemetryValueType::Text, "", labels[i] });
}

void SensorControlTelemetry::record_tms(const Command& command,
                                        SensorControlSource source,
                                        const std::optional<RadarRequest>& previous)
{
	switch (command.id)
	{
	case CommandId::SetTmsUp:
		last_input_ = "UP";
		break;
	case CommandId::PressTmsDown:
		last_input_ = "DOWN";
		break;
	case CommandId::PressTmsLeft:
		last_input_ = "LEFT";
		break;
	case CommandId::PressTmsRight:
		last_input_ = "RIGHT";
		break;
	default:
		return;
	}
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	const bool completing = !pressed && source == last_receiver_ && previous &&
	                        previous->sequence == request_sequence_ && last_request_ != "NONE";
	last_input_ += pressed ? " PRESS" : " RELEASE";
	last_receiver_ = source;
	if (completing)
		return;
	request_sequence_ = previous ? previous->sequence : 0;
	last_request_ = "NONE";
	last_result_ = source == SensorControlSource::Radar  ? "INPUT ACCEPTED"
	               : source == SensorControlSource::None ? "NO SOI"
	                                                     : "RESERVED";
}

void SensorControlTelemetry::publish(DebugSimulationTime time,
                                     const ModeData& mode,
                                     const DisplaySnapshot& display,
                                     const SensorOfInterestSnapshot& selection,
                                     const TargetingData& targeting,
                                     const RadarSnapshot& radar)
{
	if (last_receiver_ == SensorControlSource::Radar && targeting.last_tms_request &&
	    targeting.last_tms_request->sequence != request_sequence_)
	{
		request_sequence_ = targeting.last_tms_request->sequence;
		last_request_ = std::string(operation_name(targeting.last_tms_request->command.operation)) + " #" +
		                std::to_string(request_sequence_);
		last_result_ = "PENDING";
	}
	for (const auto& response : radar.command_results)
		if (last_receiver_ == SensorControlSource::Radar && response.request_sequence == request_sequence_)
		{
			last_result_ = !response.accepted                                           ? "REJECTED"
			               : response.implementation == RadarImplementation::Reserved   ? "RESERVED"
			               : response.implementation == RadarImplementation::Simplified ? "SIMPLIFIED"
			                                                                            : "NATIVE";
		}
	const std::array<std::string, kChannelCount> values = {
		std::string(mode_name(mode.operating_mode())) + " / " + mode_name(display.operating_mode),
		std::string(function_name(display.left.function)) + " / " + source_name(display.left.control_source),
		std::string(function_name(display.right.function)) + " / " + source_name(display.right.control_source),
		source_name(selection.source),
		location_name(selection.location),
		selection.display_context_ready ? "READY" : "WAITING FOR DISPLAY",
		last_input_,
		source_name(last_receiver_),
		last_request_,
		last_result_,
		std::string(search_name(targeting.search_mode)) + " / " + pattern_name(targeting.acquisition_mode),
		observed_mode_name(radar.observation),
		location_name(display.last_format_step)
	};
	for (std::size_t i = 0; i < channels_.size(); ++i)
		channels_[i].publish(time, values[i]);
}
}
