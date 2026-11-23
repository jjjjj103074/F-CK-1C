#include "SensorControlTelemetry.h"

#include "../../SystemPipeline.h"
#include "Common/CommandValue.h"
#include "Common/Units.h"
#include <iomanip>
#include <sstream>

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
		return "30X20 (RESERVED)";
	case TargetAcquisitionMode::VerticalScan:
		return "10X60 (RESERVED)";
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
	const auto declare = [&setup](const char* name, const char* label)
	{
		return setup.declare_debug_channel<std::string>({ name, label, DebugTelemetryValueType::Text, "", label });
	};
	channels_.display_mode = declare("mc_display_mode", "MC / Display Mode");
	channels_.mfd_left = declare("mc_mfd_left", "MFD Left / Source");
	channels_.mfd_right = declare("mc_mfd_right", "MFD Right / Source");
	channels_.soi_source = declare("mc_soi_source", "SOI Source");
	channels_.soi_location = declare("mc_soi_location", "SOI Display");
	channels_.display_context = declare("mc_display_context", "Display Context");
	channels_.tms_input = declare("mc_tms_input", "TMS Input");
	channels_.tms_receiver = declare("mc_tms_receiver", "TMS Receiver");
	channels_.tms_request = declare("mc_tms_request", "TMS Request");
	channels_.tms_result = declare("mc_tms_result", "TMS Result");
	channels_.radar_requested = declare("mc_radar_requested", "Radar Requested");
	channels_.radar_observed = declare("mc_radar_observed", "Radar Observed");
	channels_.dms_format_step = declare("mc_dms_format_step", "DMS Format Step (Shell)");
	channels_.radar_active = declare("mc_radar_active", "Radar Active");
	channels_.radar_scan = declare("mc_radar_scan", "Radar Scan / Limit");
	channels_.radar_acquisition = declare("mc_radar_acquisition", "Radar Auto Capture");
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
	std::ostringstream scan;
	scan << std::fixed << std::setprecision(1) << Common::deg(radar.scan.azimuth_width_rad) << " x "
	     << Common::deg(radar.scan.elevation_height_rad) << " deg / " << radar.scan.maximum_search_range_m / 1852.0
	     << " NM";
	channels_.display_mode.publish(time,
	                               std::string(mode_name(mode.operating_mode())) + " / " +
	                                   mode_name(display.operating_mode));
	channels_.mfd_left.publish(time,
	                           std::string(function_name(display.left.function)) + " / " +
	                               source_name(display.left.control_source));
	channels_.mfd_right.publish(time,
	                            std::string(function_name(display.right.function)) + " / " +
	                                source_name(display.right.control_source));
	channels_.soi_source.publish(time, source_name(selection.source));
	channels_.soi_location.publish(time, location_name(selection.location));
	channels_.display_context.publish(time, selection.display_context_ready ? "READY" : "WAITING FOR DISPLAY");
	channels_.tms_input.publish(time, last_input_);
	channels_.tms_receiver.publish(time, source_name(last_receiver_));
	channels_.tms_request.publish(time, last_request_);
	channels_.tms_result.publish(time, last_result_);
	channels_.radar_requested.publish(time,
	                                  std::string(search_name(targeting.search_mode)) + " / " +
	                                      pattern_name(targeting.acquisition_mode));
	channels_.radar_observed.publish(time, observed_mode_name(radar.observation));
	channels_.dms_format_step.publish(time, location_name(display.last_format_step));
	channels_.radar_active.publish(time,
	                               !radar.scan.enabled                                      ? "IDLE"
	                               : radar.active_search_mode == RadarSearchMode::AirCombat ? "ACM BORE"
	                                                                                        : "CRM RWS");
	channels_.radar_scan.publish(time, scan.str());
	channels_.radar_acquisition.publish(time, radar.automatic_acquisition_requested ? "ENABLED" : "IDLE");
}
}
