#include "MissionComputer.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace Core::Systems
{
MissionComputer::MissionComputer()
    : system_monitor_(input_output_data_, system_status_tables_.health),
      failure_manager_(system_status_tables_.health, system_status_tables_.failure_response),
      stores_management_(input_output_data_.input, stores_data_),
      sensor_coordination_(input_output_data_.input, mode_data_, stores_data_, sensor_data_, external_actions_),
      weapon_control_(input_output_data_.input,
                      mode_data_,
                      sensor_data_,
                      weapon_data_,
                      external_actions_,
                      sensor_coordination_,
                      stores_management_),
      mode_management_(mode_data_, sensor_coordination_, weapon_control_, external_actions_),
      executive_(executive_tables_,
                 { { "stores_management.step",
                     Mc::TaskFrequency(kProjectDefinedFallbackUpdateRateHz),
                     stores_management_,
                     &Mc::StoresManagement::step },
                   { "weapon_control.step",
                     Mc::TaskFrequency(kProjectDefinedFallbackUpdateRateHz),
                     weapon_control_,
                     &Mc::WeaponControl::step },
                   { "sensor_coordination.step",
                     Mc::TaskFrequency(kProjectDefinedFallbackUpdateRateHz),
                     sensor_coordination_,
                     &Mc::SensorCoordination::step },
                   { "system_monitor.step",
                     Mc::TaskFrequency(kProjectDefinedFallbackUpdateRateHz),
                     system_monitor_,
                     &Mc::SystemMonitor::step },
                   { "failure_manager.step",
                     Mc::TaskFrequency(kProjectDefinedFallbackUpdateRateHz),
                     failure_manager_,
                     &Mc::FailureManager::step } }),
      startup_and_restart_(executive_, system_status_tables_.lifecycle)
{
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
	// 暫用專案 fallback；不借用 F-16XL 飛控參考值作為 MC 實機依據。
	setup.update_rate_hz(kProjectDefinedFallbackUpdateRateHz);
	setup.read(AircraftDataKeys::kFrameInput);
	setup.publish(AircraftDataKeys::kMissionComputerSnapshot, input_output_data_.output.combat_avionics);
	const CommandId mode_commands[] = { CommandId::SetMasterArmOn,     CommandId::SetMasterArmOff,
		                                CommandId::SetMasterArmSim,    CommandId::SelectNavigationMode,
		                                CommandId::SelectDogfightMode, CommandId::SelectMissileOverride,
		                                CommandId::StartCmsForward,    CommandId::StartCmsAft,
		                                CommandId::StartCmsLeft,       CommandId::StartCmsRight,
		                                CommandId::PressCms,           CommandId::SetTriggerFirstStage };
	for (CommandId id : mode_commands)
	{
		setup.register_command_handler(id,
		                               [this](const SystemActionContext&, const Command& command)
		                               {
			                               input_output_data_.input.pending_commands.push_back(
			                                   { Mc::MissionCommandTarget::Mode, command });
		                               });
	}
	const CommandId sensor_commands[] = { CommandId::SetMissileUncage,
		                                  CommandId::SetTmsUp,
		                                  CommandId::PressTmsDown,
		                                  CommandId::PressTmsLeft,
		                                  CommandId::PressTmsRight };
	for (CommandId id : sensor_commands)
	{
		setup.register_command_handler(id,
		                               [this](const SystemActionContext&, const Command& command)
		                               {
			                               input_output_data_.input.pending_commands.push_back(
			                                   { Mc::MissionCommandTarget::Sensor, command });
		                               });
	}
	for (CommandId id : { CommandId::SetTriggerSecondStage, CommandId::SetWeaponRelease })
	{
		setup.register_command_handler(id,
		                               [this](const SystemActionContext&, const Command& command)
		                               {
			                               input_output_data_.input.pending_commands.push_back(
			                                   { Mc::MissionCommandTarget::Weapon, command });
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
	output.fire_control.status = output.status;
	output.fire_control.master_arm_mode = mode_data_.master_arm;
	output.fire_control.fire_control_mode = mode_data_.fire_control;
	output.fire_control.air_to_air_submode = sensor_data_.submode;
	output.fire_control.dogfight_mode = mode_data_.fire_control == FireControlMode::Dogfight;
	output.fire_control.gun_fire_requested = weapon_data_.gun_requested;
	output.fire_control.gun_firing_indication = weapon_data_.gun_firing_indication;
	output.fire_control.pickle_requested = weapon_data_.pickle_requested;
	output.stores.status = output.status;
	output.stores.aim9_count = stores_data_.missile_count;
	output.aim9.uncage_held = sensor_data_.uncage_held;
	output.aim9.target_designated = sensor_data_.target_designated;
	output.aim9.readiness = sensor_data_.readiness;
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
		switch (intent.target)
		{
		case Mc::MissionCommandTarget::Mode:
			mode_management_.handle(intent.command);
			break;
		case Mc::MissionCommandTarget::Sensor:
			sensor_coordination_.handle(intent.command);
			break;
		case Mc::MissionCommandTarget::Weapon:
			weapon_control_.handle(intent.command);
			break;
		}
		pending.pop_front();
	}
}
}
