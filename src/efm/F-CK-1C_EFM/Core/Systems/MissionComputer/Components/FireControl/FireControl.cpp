#include "FireControl.h"

#include "Common/CommandValue.h"

#include <algorithm>

namespace Core::Systems::Mc
{
FireControl::FireControl(const MissionInputs& input,
                         const ModeData& mode,
                         const StoresData& stores,
                         FireControlData& data,
                         ExternalActionQueue& actions,
                         StoresCommandQueue& stores_commands)
    : input_(input),
      mode_(mode),
      data_(data),
      actions_(actions),
      target_designation_(data.target_designation),
      weapon_operation_(input, mode, data.weapon, stores_commands),
      infrared_missile_support_(input, mode, stores, data.target_designation, data.weapon, data.infrared_missile)
{
}

void FireControl::handle(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetMissileUncage:
	case CommandId::SetTmsUp:
	case CommandId::PressTmsDown:
	case CommandId::PressTmsLeft:
	case CommandId::PressTmsRight:
		handle_target_input(command);
		break;
	case CommandId::SetWeaponRelease:
		if (Common::command_value_is_pressed(command.value_normalized))
			reapply_acquisition_mode();
		weapon_operation_.handle(command);
		break;
	default:
		weapon_operation_.handle(command);
		break;
	}
}

void FireControl::on_operating_mode_selected(const OperatingModeSelected&)
{
	if (mode_.operating_mode == OperatingMode::Navigation)
	{
		data_.acquisition_mode = AirToAirSubmode::None;
		cancel_target_designation();
		actions_.append(MissionActionKind::ModeCannon);
		return;
	}
	const bool dogfight = mode_.operating_mode == OperatingMode::Dogfight;
	if (!dogfight)
		cancel_target_designation();
	request_radar_on();
	apply_acquisition_mode(dogfight ? AirToAirSubmode::Helmet : AirToAirSubmode::BeyondVisualRange);
	if (dogfight)
		acquire_target(AcquisitionInput::Pulse);
}

void FireControl::on_master_arm_selected(const MasterArmSelected& event)
{
	if (event.selected.master_arm != MasterArmMode::On)
		weapon_operation_.stop();
}

void FireControl::step(std::chrono::nanoseconds time, double dt_s)
{
	time = (std::max)(time, input_.observed_at);
	// 先安排武器停止與目標接續，再推進捕獲操作的執行計時。
	if (weapon_operation_.step(time, dt_s) == Internal::WeaponOperationEvent::ReleaseInputEnded)
		cancel_target_designation();
	if (input_.cockpit.mission_adapter.radar_power_on)
		radar_on_requested_ = false;
	if (acquisition_input_.release_due(time, actions_))
		finish_acquisition_input();
	target_designation_.observe(input_.cockpit.radar);
	infrared_missile_support_.step(time, dt_s);
}
}
