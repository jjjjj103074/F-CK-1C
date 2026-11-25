#include "WeaponProcedures.h"
#include <cmath>

namespace Core::Systems::Mc::Internal
{
WeaponReleaseState evaluate_weapon_release(const ModeData& mode,
                                           const SelectedWeaponState& selected,
                                           const RadarSnapshot& radar,
                                           bool uncage_button_held)
{
	if (mode.master_arm != MasterArmMode::On || !mode.air_to_air())
		return WeaponReleaseState::Inhibited;
	if (!selected.available)
		return selected.pending ? WeaponReleaseState::SelectionPending : WeaponReleaseState::NoWeapon;
	const auto& weapon = selected.weapon;
	if (!weapon.registered || weapon.role != WeaponRole::AirToAir)
		return WeaponReleaseState::Unsupported;
	const auto& observation = radar.observation;
	const bool tracking = observation.status.available && observation.mode == RadarMode::SingleTargetTrack &&
	                      std::isfinite(observation.stt_range_m) && observation.stt_range_m > 0.0;
	switch (weapon.procedure)
	{
	case WeaponProcedure::Sidewinder:
		// AIM-9 沿用原生投放條件，IR 捕獲與自主追蹤各自回報。
		return WeaponReleaseState::Infrared;
	case WeaponProcedure::Sparrow:
		// AIM-7 正常程序使用有效 STT 提供目標支援。
		return tracking ? WeaponReleaseState::Targeted : WeaponReleaseState::SparrowTrackRequired;
	case WeaponProcedure::Amraam:
		if (tracking)
			return WeaponReleaseState::Targeted;
		return uncage_button_held ? WeaponReleaseState::Visual : WeaponReleaseState::VisualConsentRequired;
	default:
		return WeaponReleaseState::Unsupported;
	}
}

bool permits_release(WeaponReleaseState state) noexcept
{
	return state == WeaponReleaseState::Infrared || state == WeaponReleaseState::Targeted ||
	       state == WeaponReleaseState::Visual;
}
}
