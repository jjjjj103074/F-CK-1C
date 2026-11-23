#pragma once

#include "../Database/MissionData.h"
#include "../../../Contracts/Commands.h"
#include "../../../Contracts/Diagnostics/DebugTelemetry.h"

namespace Core::Systems
{
class SystemSetup;
}

namespace Core::Systems::Mc
{
/// @brief 將顯示對應、SOI、TMS 分派與設備實作狀態發布至共用 debug 通道。
class SensorControlTelemetry final
{
public:
	void setup(SystemSetup& setup);
	void record_tms(const Command& command, SensorControlSource source, const std::optional<RadarRequest>& previous);
	void publish(DebugSimulationTime time,
	             const ModeData& mode,
	             const DisplaySnapshot& display,
	             const SensorOfInterestSnapshot& selection,
	             const TargetingData& targeting,
	             const RadarSnapshot& radar);

private:
	struct Channels
	{
		DebugTelemetryChannel<std::string> display_mode;
		DebugTelemetryChannel<std::string> mfd_left;
		DebugTelemetryChannel<std::string> mfd_right;
		DebugTelemetryChannel<std::string> soi_source;
		DebugTelemetryChannel<std::string> soi_location;
		DebugTelemetryChannel<std::string> display_context;
		DebugTelemetryChannel<std::string> tms_input;
		DebugTelemetryChannel<std::string> tms_receiver;
		DebugTelemetryChannel<std::string> tms_request;
		DebugTelemetryChannel<std::string> tms_result;
		DebugTelemetryChannel<std::string> radar_requested;
		DebugTelemetryChannel<std::string> radar_observed;
		DebugTelemetryChannel<std::string> dms_format_step;
		DebugTelemetryChannel<std::string> radar_active;
		DebugTelemetryChannel<std::string> radar_scan;
		DebugTelemetryChannel<std::string> radar_acquisition;
	};
	Channels channels_;
	std::string last_input_ = "NONE";
	SensorControlSource last_receiver_ = SensorControlSource::None;
	std::string last_request_ = "NONE";
	std::string last_result_ = "NO INPUT";
	std::uint64_t request_sequence_ = 0;
};
}
