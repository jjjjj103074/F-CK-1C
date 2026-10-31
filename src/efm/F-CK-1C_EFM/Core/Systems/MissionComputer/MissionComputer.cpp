#include "MissionComputer.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"
#include "Common/CommandValue.h"

#include <stdexcept>

namespace
{
// 依 group 順序先停止火控，再中止 CMS。
constexpr sigslot::group_id kFireControlArmOrder = 0;
constexpr sigslot::group_id kCountermeasureArmOrder = 1;
}

namespace Core::Systems
{
MissionComputer::MissionComputer()
    : system_monitor_(input_output_data_, system_status_tables_.health),
      failure_manager_(system_status_tables_.health, system_status_tables_.failure_response),
      stores_management_(input_output_data_.input,
                         mode_data_,
                         fire_control_data_.weapon,
                         stores_data_,
                         external_actions_),
      fire_control_(input_output_data_.input,
                    mode_data_,
                    stores_data_,
                    fire_control_data_,
                    external_actions_,
                    stores_management_),
      mode_management_(mode_data_, mode_events_),
      countermeasure_control_(mode_data_, external_actions_),
      executive_(executive_tables_,
                 { { "stores_management.step",
                     Mc::TaskFrequency(kMissionComputerProjectUpdateRateHz),
                     stores_management_,
                     &Mc::StoresManagement::step },
                   { "fire_control.step",
                     Mc::TaskFrequency(kMissionComputerProjectUpdateRateHz),
                     fire_control_,
                     &Mc::FireControl::step },
                   { "system_monitor.step",
                     Mc::TaskFrequency(kMissionComputerProjectUpdateRateHz),
                     system_monitor_,
                     &Mc::SystemMonitor::step },
                   { "failure_manager.step",
                     Mc::TaskFrequency(kMissionComputerProjectUpdateRateHz),
                     failure_manager_,
                     &Mc::FailureManager::step } }),
      startup_and_restart_(executive_, system_status_tables_.lifecycle)
{
	// 組裝本台 MC 的主保險與模式事件訂閱。
	fire_control_arm_connection_ = mode_events_.master_arm_selected.connect(
	    [this](const Mc::MasterArmSelected& event)
	    {
		    fire_control_.on_master_arm_selected(event);
	    },
	    kFireControlArmOrder);
	countermeasure_arm_connection_ = mode_events_.master_arm_selected.connect(
	    [this](const Mc::MasterArmSelected& event)
	    {
		    countermeasure_control_.on_master_arm_selected(event);
	    },
	    kCountermeasureArmOrder);
	fire_control_mode_connection_ = mode_events_.operating_mode_selected.connect(
	    [this](const Mc::OperatingModeSelected& event)
	    {
		    fire_control_.on_operating_mode_selected(event);
	    });
	startup_and_restart_.start();
}

void MissionComputer::set_powered(bool powered) noexcept
{
	powered_ = powered;
}

bool MissionComputer::powered() const noexcept
{
	return powered_;
}

void MissionComputer::setup(SystemSetup& setup)
{
	// MC 以 32 Hz 更新。
	setup.update_rate_hz(kMissionComputerProjectUpdateRateHz);
	setup.read(AircraftDataKeys::kFrameInput);
	setup.publish(AircraftDataKeys::kMissionComputerSnapshot, input_output_data_.output.combat_avionics);
	// 各類指令先進入同一 FIFO，待本 tick 讀取觀測後才執行，保留跨元件的操作順序。
	const CommandId mode_commands[] = { CommandId::SetMasterArmOn,     CommandId::SetMasterArmOff,
		                                CommandId::SetMasterArmSim,    CommandId::SelectNavigationMode,
		                                CommandId::SelectDogfightMode, CommandId::SelectMissileOverride };
	for (CommandId id : mode_commands)
	{
		setup.register_command_handler(id,
		                               [this](const SystemActionContext&, const Command& command)
		                               {
			                               input_output_data_.input.pending_commands.push_back(
			                                   { Mc::MissionCommandTarget::Mode, command });
		                               });
	}
	const CommandId countermeasure_commands[] = { CommandId::StartCmsForward,
		                                          CommandId::StartCmsAft,
		                                          CommandId::StartCmsLeft,
		                                          CommandId::StartCmsRight,
		                                          CommandId::PressCms };
	for (CommandId id : countermeasure_commands)
	{
		setup.register_command_handler(id,
		                               [this](const SystemActionContext&, const Command& command)
		                               {
			                               input_output_data_.input.pending_commands.push_back(
			                                   { Mc::MissionCommandTarget::Countermeasure, command });
		                               });
	}
	const CommandId fire_control_commands[] = { CommandId::SetTriggerFirstStage,
		                                        CommandId::SetTriggerSecondStage,
		                                        CommandId::SetWeaponRelease,
		                                        CommandId::SetMissileUncage,
		                                        CommandId::SetTmsUp,
		                                        CommandId::PressTmsDown,
		                                        CommandId::PressTmsLeft,
		                                        CommandId::PressTmsRight };
	for (CommandId id : fire_control_commands)
	{
		setup.register_command_handler(id,
		                               [this](const SystemActionContext&, const Command& command)
		                               {
			                               input_output_data_.input.pending_commands.push_back(
			                                   { Mc::MissionCommandTarget::FireControl, command });
		                               });
	}
}

void MissionComputer::step(const SystemStepContext& context, const AircraftDataView& aircraft, SystemResult& result)
{
	if (!powered_)
	{
		return;
	}

	read_inputs(context, aircraft);
	consume_commands();
	executive_.step(context.scheduled_time);
	publish_outputs(result);
}

void MissionComputer::read_inputs(const SystemStepContext& context, const AircraftDataView& aircraft)
{
	input_output_data_.input.cockpit = aircraft.read(AircraftDataKeys::kFrameInput).cockpit;
	input_output_data_.input.observed_at = context.input_time;
	const MissionAdapterObservation& adapter = input_output_data_.input.cockpit.mission_adapter;
	if (adapter.available)
	{
		external_actions_.acknowledge(adapter.acknowledged_sequence);
	}
}

void MissionComputer::publish_outputs(SystemResult& result)
{
	CombatAvionicsSnapshot& output = input_output_data_.output.combat_avionics;
	output.status = { true, output.status.revision + 1 };
	// 模式由獨立快照對外發布；Bridge 仍映射到原有座艙參數名稱與數值。
	output.mode.status = output.status;
	output.fire_control.status = output.status;
	output.mode.master_arm_mode = mode_data_.master_arm;
	output.mode.operating_mode = mode_data_.operating_mode;
	output.fire_control.air_to_air_submode = fire_control_data_.targeting.submode;
	output.mode.dogfight_mode = mode_data_.operating_mode == OperatingMode::Dogfight;
	output.fire_control.gun_fire_requested = fire_control_data_.weapon.gun_requested;
	output.fire_control.gun_firing_indication = fire_control_data_.weapon.gun_firing_indication;
	output.fire_control.pickle_requested = fire_control_data_.weapon.pickle_requested;
	output.stores.status = output.status;
	output.stores.air_to_air_missile_count = stores_data_.missile_count;
	output.stores.gun_count = stores_data_.gun_count;
	output.stores.count_source = stores_data_.count_source;
	output.stores.selected_station = stores_data_.selected_station;
	output.stores.requested_station = stores_data_.requested_station;
	output.stores.selection_request_revision = stores_data_.selection_request_revision;
	output.infrared_missile.uncage_held = fire_control_data_.targeting.uncage_held;
	output.infrared_missile.target_designated = fire_control_data_.targeting.designation_requested;
	output.infrared_missile.status = output.status;
	output.infrared_missile.readiness = fire_control_data_.infrared_missile.readiness;
	output.infrared_missile.active = fire_control_data_.infrared_missile.active;
	output.infrared_missile.contact = fire_control_data_.infrared_missile.contact;
	output.infrared_missile.locked = fire_control_data_.infrared_missile.locked;
	output.infrared_missile.seeker_state = fire_control_data_.infrared_missile.seeker_state;
	output.infrared_missile.requested_tone = fire_control_data_.infrared_missile.requested_tone;
	output.infrared_missile.seeker_azimuth_rad = fire_control_data_.infrared_missile.seeker_azimuth_rad;
	output.infrared_missile.seeker_elevation_rad = fire_control_data_.infrared_missile.seeker_elevation_rad;
	output.infrared_missile.lock_range_m = fire_control_data_.infrared_missile.lock_range_m;
	output.control_inputs = input_output_data_.input.control_inputs;
	output.external_actions.status = output.status;
	output.external_actions.batch = external_actions_.batch();
	result.publish(AircraftDataKeys::kMissionComputerSnapshot, output);
}

void MissionComputer::consume_commands()
{
	auto& pending = input_output_data_.input.pending_commands;
	while (!pending.empty())
	{
		const auto& intent = pending.front();
		record_control_input(intent.command);
		switch (intent.target)
		{
		case Mc::MissionCommandTarget::Mode:
			mode_management_.handle(intent.command);
			break;
		case Mc::MissionCommandTarget::Countermeasure:
			countermeasure_control_.handle(intent.command);
			break;
		case Mc::MissionCommandTarget::FireControl:
			fire_control_.handle(intent.command);
			break;
		}
		// sigslot 的同步回呼已全部返回，這時才結束本筆指令並移往下一筆。
		pending.pop_front();
	}
}
void MissionComputer::record_control_input(const Command& command)
{
	auto& input = input_output_data_.input;
	if (input.command_sequence == kMaximumExactCockpitInteger)
		throw std::overflow_error("MC input sequence exhausted its exact numeric range.");
	const auto sequence = ++input.command_sequence;
	auto& controls = input.control_inputs;
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	if (command.id == CommandId::SetTriggerSecondStage)
		controls.trigger_second_stage_held = pressed;
	if (!pressed)
		return;
	switch (command.id)
	{
	case CommandId::SetMasterArmOn:
	case CommandId::SetMasterArmOff:
	case CommandId::SetMasterArmSim:
		controls.master_arm_press_sequence = sequence;
		break;
	case CommandId::SelectNavigationMode:
		controls.navigation_press_sequence = sequence;
		break;
	case CommandId::SelectDogfightMode:
		controls.dogfight_press_sequence = sequence;
		break;
	case CommandId::SelectMissileOverride:
		controls.missile_override_press_sequence = sequence;
		break;
	case CommandId::SetMissileUncage:
		controls.missile_uncage_press_sequence = sequence;
		break;
	case CommandId::SetWeaponRelease:
		controls.weapon_release_press_sequence = sequence;
		break;
	default:
		break;
	}
}

}
