#include "WeaponOperation.h"
#include "WeaponProcedures.h"

#include "Common/CommandValue.h"

namespace
{
constexpr auto kMissileTypeHold = std::chrono::milliseconds(500);
}

namespace Core::Systems::Mc::Internal
{
WeaponOperation::WeaponOperation(const ModeData& mode,
                                 const RadarSnapshot& radar,
                                 const SelectedWeaponState& selected,
                                 const WeaponUseData& execution,
                                 WeaponOperationData& weapon,
                                 StoresCommandQueue& stores_commands)
    : mode_(mode),
      radar_(radar),
      selected_(selected),
      execution_(execution),
      weapon_(weapon),
      stores_commands_(stores_commands)
{
}

bool WeaponOperation::can_use_gun() const
{
	return mode_.master_arm == MasterArmMode::On && mode_.override_mode == OverrideMode::Dogfight;
}

bool WeaponOperation::can_release_weapon() const
{
	return permits_release(evaluate_weapon_release(mode_, selected_, radar_, weapon_.uncage_button_held));
}

void WeaponOperation::handle(const Command& command, std::chrono::nanoseconds input_time)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	switch (command.id)
	{
	case CommandId::ToggleMissileUncage:
		if (pressed && mode_.air_to_air() && mode_.master_arm != MasterArmMode::Off &&
		    selected_.weapon.procedure == WeaponProcedure::Sidewinder)
			stores_commands_.submit(SetSeekerUncaged{});
		break;
	case CommandId::SetMissileUncage:
	{
		const bool rising = pressed && !weapon_.uncage_button_held;
		weapon_.uncage_button_held = pressed;
		if (mode_.master_arm == MasterArmMode::Off || !mode_.air_to_air())
		{
			weapon_.uncage_button_held = false;
			stores_commands_.submit(SetSeekerUncaged{ false });
		}
		else if (rising && selected_.weapon.procedure == WeaponProcedure::Sidewinder)
			stores_commands_.submit(SetSeekerUncaged{});
		break;
	}
	case CommandId::SetTriggerSecondStage:
		handle_gun(pressed);
		break;
	case CommandId::SetWeaponRelease:
		handle_release(pressed);
		break;
	case CommandId::SetMissileStep:
		handle_missile_step(pressed, input_time);
		break;
	default:
		break;
	}
}

void WeaponOperation::handle_gun(bool pressed)
{
	if (pressed)
	{
		if (can_use_gun())
			stores_commands_.submit(BeginWeaponUse{ WeaponUseKind::Gun });
	}
	else if (execution_.gun.request_id != 0)
		stores_commands_.submit(EndWeaponUse{ { WeaponUseKind::Gun, execution_.gun.request_id } });
}

void WeaponOperation::handle_release(bool pressed)
{
	if (pressed)
	{
		weapon_.release_state = evaluate_weapon_release(mode_, selected_, radar_, weapon_.uncage_button_held);
		if (can_release_weapon())
			stores_commands_.submit(BeginWeaponUse{ WeaponUseKind::Release, LaunchPermission::Normal });
	}
	else if (execution_.release.request_id != 0)
		stores_commands_.submit(EndWeaponUse{ { WeaponUseKind::Release, execution_.release.request_id } });
}

void WeaponOperation::stop()
{
	stores_commands_.submit(AbortWeaponUse{});
}

void WeaponOperation::on_mode_changed()
{
	stop();
	weapon_.uncage_button_held = false;
	missile_step_pressed_at_.reset();
	missile_type_step_sent_ = false;
}

void WeaponOperation::submit_missile_step(MissileSelectionStep step)
{
	if (mode_.air_to_air())
		stores_commands_.submit(SelectStoresForMode{ mode_.operating_mode(), step });
}

void WeaponOperation::handle_missile_step(bool pressed, std::chrono::nanoseconds time)
{
	if (pressed)
	{
		if (mode_.air_to_air() && !missile_step_pressed_at_)
		{
			missile_step_pressed_at_ = time;
			missile_type_step_sent_ = false;
		}
	}
	else if (missile_step_pressed_at_)
	{
		if (!missile_type_step_sent_)
			submit_missile_step(time - *missile_step_pressed_at_ >= kMissileTypeHold ? MissileSelectionStep::WeaponType
			                                                                         : MissileSelectionStep::Station);
		missile_step_pressed_at_.reset();
		missile_type_step_sent_ = false;
	}
}

void WeaponOperation::step(std::chrono::nanoseconds time)
{
	if (missile_step_pressed_at_ && !missile_type_step_sent_ && time - *missile_step_pressed_at_ >= kMissileTypeHold)
	{
		submit_missile_step(MissileSelectionStep::WeaponType);
		missile_type_step_sent_ = true;
	}
	weapon_.release_state = evaluate_weapon_release(mode_, selected_, radar_, weapon_.uncage_button_held);
	const auto& target = radar_.observation;
	stores_commands_.submit(
	    AimIrSeeker{ mode_.air_to_air() && target.status.available && target.mode == RadarMode::SingleTargetTrack,
	                 target.stt_azimuth_rad,
	                 target.stt_elevation_rad });
	weapon_.gun_requested = execution_.gun.in_progress();
	weapon_.pickle_requested = execution_.release.in_progress();
	weapon_.gun_firing_indication =
	    weapon_.gun_requested && mode_.master_arm == MasterArmMode::On && mode_.override_mode == OverrideMode::Dogfight;
}
}
