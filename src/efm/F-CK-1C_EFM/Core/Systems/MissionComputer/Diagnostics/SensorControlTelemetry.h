#pragma once

#include "../Database/MissionData.h"
#include "../../../Contracts/Commands.h"
#include "../../../Contracts/Diagnostics/DebugTelemetry.h"

#include <array>

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
	static constexpr std::size_t kChannelCount = 16;
	std::array<DebugTelemetryChannel<std::string>, kChannelCount> channels_;
	std::string last_input_ = "NONE";
	SensorControlSource last_receiver_ = SensorControlSource::None;
	std::string last_request_ = "NONE";
	std::string last_result_ = "NO INPUT";
	std::uint64_t request_sequence_ = 0;
};
}
