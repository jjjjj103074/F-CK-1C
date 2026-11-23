#pragma once

#include "CockpitParameterEndpoint.h"
#include "../../Core/Contracts/CockpitContracts.h"

#include <array>

namespace DcsBridge::Internal
{
/// @brief 以奇／偶 generation 一併發布 MC 狀態、已確認選站與共用 DCS 動作視窗。
class MissionComputerExporter final
{
public:
	explicit MissionComputerExporter(cockpit_param_api api);
	void reset();
	CockpitParameterEvents export_snapshot(const Core::CombatAvionicsSnapshot& snapshot,
	                                       const Core::MissionActionBatch& actions);

private:
	enum class SelectedStoreState
	{
		Unavailable,
		None,
		Selecting,
		Selected
	};
	enum class Parameter : std::size_t
	{
		Generation,
		Available,
		FirstSequence,
		ActionCount,
		MasterArm,
		Dogfight,
		OperatingMode,
		MasterMode,
		OverrideMode,
		Submode,
		GunFiring,
		Uncage,
		IrAvailable,
		IrRevision,
		IrInvalidReason,
		IrLock,
		IrDesiredAzimuth,
		IrDesiredElevation,
		IrTargetAzimuth,
		IrTargetElevation,
		MissileCount,
		MissileCountAvailable,
		RequestedStation,
		SelectionRequestRevision,
		SelectionActionSequence,
		GunCount,
		GunCountAvailable,
		TriggerHeld,
		MasterArmPressSequence,
		NavigationPressSequence,
		DogfightPressSequence,
		MissileOverridePressSequence,
		MissileUncagePressSequence,
		WeaponReleasePressSequence,
		SelectedStoreState,
		SelectedStoreCount,
		SelectedStoreCountAvailable,
		SelectedStoreFamily,
		IrControlAvailable,
		IrSelfTrackEnabled,
		IrNativeMode,
		IrControlStatus,
		IrControlRevision,
		Count
	};
	bool write(Parameter parameter, double value, CockpitParameterEvents& events);
	bool export_selected_store(const Core::StoresSnapshot& stores, CockpitParameterEvents& events);
	const cockpit_param_api api_;
	std::array<CockpitParameterEndpoint, static_cast<std::size_t>(Parameter::Count)> slots_;
	std::array<CockpitParameterEndpoint, Core::kMissionActionWindowSize> action_slots_;
	CockpitParameterEndpoint selected_store_identifier_;
	CockpitParameterEndpoint selected_store_station_;
	std::uint64_t generation_ = 0;
};
}
