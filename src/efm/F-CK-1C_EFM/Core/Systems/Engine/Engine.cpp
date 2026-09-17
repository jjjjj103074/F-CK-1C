#include "Engine.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
}

namespace Core
{
namespace Systems
{
Engine::Engine(
	const EngineConfig& config,
	StartMode start_mode)
	: config_(config)
{
	validate_engine_config(config_);
	configure_start(start_mode);
	refresh_outputs();
}

void Engine::configure_start(StartMode start_mode)
{
	switch (start_mode)
	{
	case StartMode::ColdGround:
		::Systems::configure_cold_start_engines(engines_, config_); break;
	case StartMode::HotGround:
		::Systems::configure_hot_ground_start_engines(engines_, config_); break;
	case StartMode::HotAir:
		::Systems::configure_hot_air_start_engines(engines_, config_); break;
	}
}

void Engine::setup(SystemSetup& setup)
{
	setup.update_rate_hz(kProjectDefinedFallbackUpdateRateHz);
	setup.read(AircraftDataKeys::kAircraftObservation);
	setup.read(AircraftDataKeys::kEngineThrottleCommand);
	setup.read(AircraftDataKeys::kFuelData);
	setup.publish(AircraftDataKeys::kEngineData, data_);
	setup.publish(AircraftDataKeys::kFuelDemand, fuel_demand_);
	register_handlers(setup);
}

void Engine::register_handlers(SystemSetup& setup)
{
	const CommandId commands[] = {
		CommandId::SetBothEngines,
		CommandId::SetLeftEngine,
		CommandId::SetRightEngine
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const SystemActionContext&, const Command& command)
			{ handle_command(command); });
	}
	setup.register_damage_handler(
		DamageArea::LeftEngine,
		[this](const SystemActionContext&, const DamageEvent& event)
		{ apply_damage(event); });
	setup.register_damage_handler(
		DamageArea::RightEngine,
		[this](const SystemActionContext&, const DamageEvent& event)
		{ apply_damage(event); });
	setup.register_repair_handler(
		[this](const SystemActionContext&, const RepairEvent& event)
		{ repair(event); });
}

void Engine::step(
	const SystemStepContext& context,
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	const AircraftObservation& observation =
		aircraft.read(AircraftDataKeys::kAircraftObservation);
	const FuelData& fuel = aircraft.read(AircraftDataKeys::kFuelData);
	step({
		context.dt_s,
		aircraft.read(AircraftDataKeys::kEngineThrottleCommand),
		fuel.internal_fuel_kg,
		observation.altitude_asl_m
	});
	result.publish(AircraftDataKeys::kEngineData, data_);
	result.publish(AircraftDataKeys::kFuelDemand, fuel_demand_);
}

const EngineData& Engine::step(const EngineFrameInput& input)
{
	::Systems::apply_engine_throttle_commands(
		engines_,
		input.throttle_command.left_normalized,
		input.throttle_command.right_normalized);
	::Systems::clamp_engine_throttle_inputs(engines_);
	::Systems::update_dry_engine_channels(engines_, config_, input.dt_s);
	::Systems::update_afterburners(engines_, config_, input.dt_s);
	::Systems::update_nozzle_apertures(engines_, config_, input.dt_s);
	::Systems::apply_engine_readout_integrity(
		engines_, left_integrity(), right_integrity());
	thrust_inhibited_ = ::Systems::should_shutdown_engines(
		input.internal_fuel_kg, input.altitude_asl_m);
	if (thrust_inhibited_)
	{
		::Systems::shutdown_engines(engines_, input.dt_s);
	}
	refresh_outputs();
	return data_;
}

void Engine::refresh_outputs()
{
	fuel_demand_.flow_rate_kg_s =
		::Systems::command_fuel_flow({
			engines_.left.throttle_output_normalized,
			engines_.right.throttle_output_normalized,
			engines_.left.afterburner_ratio_0_1,
			engines_.right.afterburner_ratio_0_1
		}, config_).flow_rate_kg_s;
	data_.left = {
		engines_.left.switch_on,
		engines_.left.throttle_input_normalized,
		engines_.left.throttle_output_normalized,
		engines_.left.power_readout_normalized,
		engines_.left.afterburner_ratio_0_1,
		engines_.left.afterburner_lit,
		engines_.left.nozzle_aperture_normalized,
		left_integrity()
	};
	data_.right = {
		engines_.right.switch_on,
		engines_.right.throttle_input_normalized,
		engines_.right.throttle_output_normalized,
		engines_.right.power_readout_normalized,
		engines_.right.afterburner_ratio_0_1,
		engines_.right.afterburner_lit,
		engines_.right.nozzle_aperture_normalized,
		right_integrity()
	};
	data_.thrust_inhibited = thrust_inhibited_;
}

void Engine::handle_command(const Command& command)
{
	const bool enabled = command.value_normalized > kEnabledCommandThreshold;
	switch (command.id)
	{
	case CommandId::SetBothEngines:
		::Systems::set_both_engine_switches(engines_, enabled); break;
	case CommandId::SetLeftEngine:
		::Systems::set_left_engine_switch(engines_, enabled); break;
	case CommandId::SetRightEngine:
		::Systems::set_right_engine_switch(engines_, enabled); break;
	default:
		break;
	}
	refresh_outputs();
}

void Engine::apply_damage(const DamageEvent& event)
{
	if (event.area == DamageArea::LeftEngine)
	{
		left_integrity_.apply(event.segment, event.integrity);
	}
	if (event.area == DamageArea::RightEngine)
	{
		right_integrity_.apply(event.segment, event.integrity);
	}
	refresh_outputs();
}

void Engine::repair(const RepairEvent& event)
{
	(void)event;
	left_integrity_.reset();
	right_integrity_.reset();
	refresh_outputs();
}

const EngineData& Engine::data() const
{
	return data_;
}

const FuelDemand& Engine::fuel_demand() const
{
	return fuel_demand_;
}

const ::Systems::EngineSystemState& Engine::state() const
{
	return engines_;
}

double Engine::left_integrity() const
{
	return left_integrity_.value();
}

double Engine::right_integrity() const
{
	return right_integrity_.value();
}
}
}
