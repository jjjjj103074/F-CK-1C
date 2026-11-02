#pragma once

#include "../Database/InputOutputData.h"
#include "../Database/StoresData.h"
#include "../Database/MissionData.h"
#include "Core/Contracts/Diagnostics/DebugTelemetry.h"

#include <array>

namespace Core::Systems
{
class SystemSetup;
}

namespace Core::Systems::Mc
{
/// @brief 將掛載輸入、庫存及彈量來源發布至共用診斷通道。
class StoresTelemetry final
{
public:
	void setup(SystemSetup& setup);
	void publish(DebugSimulationTime time,
	             const MissionInputs& input,
	             const StoresData& stores,
	             const WeaponOperationData& weapon) const;

private:
	struct StationChannels
	{
		DebugTelemetryChannel<std::string> state;
		DebugTelemetryChannel<std::string> identity;
		DebugTelemetryChannel<std::string> input;
	};

	std::array<StationChannels, kWeaponStationCount> stations_;
	DebugTelemetryChannel<bool> observation_available_;
	DebugTelemetryChannel<std::int64_t> observation_reason_;
	DebugTelemetryChannel<std::string> observation_revision_;
	DebugTelemetryChannel<std::string> inventory_revision_;
	DebugTelemetryChannel<std::string> rearm_revision_;
	DebugTelemetryChannel<double> observation_age_s_;
	DebugTelemetryChannel<std::int64_t> missile_count_;
	DebugTelemetryChannel<bool> missile_count_complete_;
	DebugTelemetryChannel<std::int64_t> aa_count_;
	DebugTelemetryChannel<bool> aa_count_complete_;
	DebugTelemetryChannel<std::int64_t> aa_summary_count_;
	DebugTelemetryChannel<std::int64_t> aa_summary_source_;
	DebugTelemetryChannel<bool> gun_api_available_;
	DebugTelemetryChannel<std::int64_t> gun_api_count_;
	DebugTelemetryChannel<double> gun_count_;
	DebugTelemetryChannel<bool> gun_requested_;
};
}
