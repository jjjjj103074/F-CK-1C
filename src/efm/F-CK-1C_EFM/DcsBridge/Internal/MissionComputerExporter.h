#pragma once

#include "CockpitParameterEndpoint.h"
#include "../../Core/Contracts/CockpitContracts.h"

#include <array>

namespace DcsBridge::Internal
{
/// @brief Commits one MC action window using an odd/even generation around numeric cockpit parameters.
class MissionComputerExporter final
{
public:
	explicit MissionComputerExporter(cockpit_param_api api);
	void reset();
	CockpitParameterEvents export_snapshot(const Core::CombatAvionicsSnapshot& snapshot);

private:
	enum class Parameter : std::size_t
	{
		Generation,
		Available,
		FirstSequence,
		ActionCount,
		MasterArm,
		Dogfight,
		OperatingMode,
		Submode,
		GunFiring,
		Uncage,
		Designated,
		MissileStatus,
		MissileCount,
		RequestedStation,
		SelectionRequestRevision,
		SeekerState,
		Tone,
		Contact,
		Lock,
		Active,
		Azimuth,
		Elevation,
		LockRange,
		GunCount,
		TriggerHeld,
		MasterArmPressSequence,
		NavigationPressSequence,
		DogfightPressSequence,
		MissileOverridePressSequence,
		MissileUncagePressSequence,
		WeaponReleasePressSequence,
		Count
	};
	bool write(Parameter parameter, double value, CockpitParameterEvents& events);
	const cockpit_param_api api_;
	std::array<CockpitParameterEndpoint, static_cast<std::size_t>(Parameter::Count)> slots_;
	std::array<CockpitParameterEndpoint, Core::kMissionActionWindowSize> action_slots_;
	std::uint64_t generation_ = 0;
};
}
