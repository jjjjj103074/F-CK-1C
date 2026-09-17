#pragma once

#include "DigitalEngineControlModel.h"
#include "EngineConfig.h"
#include "Common/Actuator.h"
#include "Common/Clamp.h"
#include "Common/Table.h"
#include <cstddef>
#include <vector>

namespace Systems
{
inline constexpr double kEngineShutdownAltitudeM = 20000.0;

struct EngineChannelState
{
	bool switch_on = false;
	double throttle_input_normalized = 0.0;
	double throttle_output_normalized = 0.0;
	double power_readout_normalized = 0.0;
	double afterburner_ratio_0_1 = 0.0;
	bool afterburner_lit = false;
	double nozzle_aperture_normalized = 0.80;
};

struct EngineSystemState
{
	EngineChannelState left;
	EngineChannelState right;
	double throttle_cmd_left_normalized = 0.0;
	double throttle_cmd_right_normalized = 0.0;
};

struct FirstOrderInput
{
	double target_normalized = 0.0;
	double tau_s = 0.0;
	double dt_s = 0.0;
};

struct NozzleTargetInput
{
	double throttle_input_normalized = 0.0;
	double power_readout_normalized = 0.0;
	double afterburner_ratio_0_1 = 0.0;
	bool engine_on = false;
};

struct EngineStartState
{
	bool engine_on = false;
	double throttle_input_normalized = 0.0;
	double throttle_output_normalized = 0.0;
	double power_readout_normalized = 0.0;
};

struct NozzleUpdateInput
{
	double target_normalized = 0.0;
	double power_readout_normalized = 0.0;
	double afterburner_ratio_0_1 = 0.0;
	bool engine_on = false;
	double dt_s = 0.0;
};

inline void set_engine_switch(EngineChannelState& engine, bool enabled)
{
	engine.switch_on = enabled;
}

inline void set_both_engine_switches(EngineSystemState& engines, bool enabled)
{
	set_engine_switch(engines.left, enabled);
	set_engine_switch(engines.right, enabled);
}

inline void set_left_engine_switch(EngineSystemState& engines, bool enabled)
{
	set_engine_switch(engines.left, enabled);
}

inline void set_right_engine_switch(EngineSystemState& engines, bool enabled)
{
	set_engine_switch(engines.right, enabled);
}

inline void apply_engine_throttle_commands(
	EngineSystemState& engines,
	double left_command_normalized,
	double right_command_normalized)
{
	engines.throttle_cmd_left_normalized = Common::limit(
		left_command_normalized, 0.0, 1.0);
	engines.throttle_cmd_right_normalized = Common::limit(
		right_command_normalized, 0.0, 1.0);
	engines.left.throttle_input_normalized =
		engines.throttle_cmd_left_normalized;
	engines.right.throttle_input_normalized =
		engines.throttle_cmd_right_normalized;
}

inline double engine_first_order(double current, const FirstOrderInput& input)
{
	if (input.tau_s <= 1e-6)
	{
		return input.target_normalized;
	}
	const double k = Common::limit(
		input.dt_s / (input.tau_s + input.dt_s), 0.0, 1.0);
	return current + (input.target_normalized - current) * k;
}

inline void update_afterburner(
	EngineChannelState& engine,
	const AfterburnerConfig& afterburner,
	double dt_s)
{
	const DigitalEngineControlAfterburnerCommand command =
		command_afterburner(
			{
				engine.throttle_input_normalized,
				engine.throttle_output_normalized,
				engine.afterburner_ratio_0_1,
				engine.switch_on
			},
			engine.afterburner_lit,
			afterburner);
	engine.afterburner_lit = command.lit;
	engine.afterburner_ratio_0_1 = Common::limit(
		engine_first_order(
			engine.afterburner_ratio_0_1,
			{ command.ratio_target_0_1,
				command.spool_time_constant_s,
				dt_s }),
		0.0,
		1.0);
}

inline void update_afterburners(
	EngineSystemState& engines,
	const EngineConfig& config,
	double dt_s)
{
	update_afterburner(engines.left, config.afterburner, dt_s);
	update_afterburner(engines.right, config.afterburner, dt_s);
}

inline void update_stopped_engine(
	EngineChannelState& engine,
	const EngineConfig& config,
	double dt_s)
{
	engine.throttle_output_normalized = Common::actuator(
		engine.throttle_output_normalized, { 0.0, -0.01, 0.01 });
	engine.power_readout_normalized = Common::actuator(
		engine.power_readout_normalized,
		{
			0.0,
			-dt_s / (config.start_time_s / 2),
			dt_s / (config.start_time_s / 2)
		});
	engine.throttle_input_normalized = Common::limit(
		engine.throttle_input_normalized, 0.0, 0.0);
}

inline void update_starting_engine(
	EngineChannelState& engine,
	const EngineConfig& config,
	double dt_s)
{
	engine.power_readout_normalized = Common::actuator(
		engine.power_readout_normalized,
		{
			0.5,
			-dt_s / (config.start_time_s / 2),
			dt_s / (config.start_time_s / 2)
		});
	engine.throttle_input_normalized = Common::limit(
		engine.throttle_input_normalized, 0.0, 0.1);
}

inline void update_running_dry_engine(
	EngineChannelState& engine,
	const EngineConfig& config,
	double dt_s)
{
	const DigitalEngineControlDryCommand command = command_dry_engine(
		engine.throttle_input_normalized,
		engine.throttle_output_normalized,
		config);
	engine.throttle_output_normalized = engine_first_order(
		engine.throttle_output_normalized,
		{ command.throttle_output_target_normalized,
			command.spool_time_constant_s,
			dt_s });
	engine.throttle_output_normalized = Common::limit(
		engine.throttle_output_normalized, 0.1, 1.0);
	const double core_step_0_1 =
		dt_s * command.core_speed_rate_0_1_per_s;
	engine.power_readout_normalized = Common::actuator(
		engine.power_readout_normalized,
		{ command.core_speed_target_0_1, -core_step_0_1, core_step_0_1 });
}

inline void update_dry_engine_channel(
	EngineChannelState& engine,
	const EngineConfig& config,
	double dt_s)
{
	if (!engine.switch_on)
	{
		update_stopped_engine(engine, config, dt_s);
	}
	if (engine.switch_on && engine.power_readout_normalized < 0.5)
	{
		update_starting_engine(engine, config, dt_s);
	}
	if (engine.switch_on && engine.power_readout_normalized >= 0.5)
	{
		update_running_dry_engine(engine, config, dt_s);
	}
}

inline void update_dry_engine_channels(
	EngineSystemState& engines,
	const EngineConfig& config,
	double dt_s)
{
	update_dry_engine_channel(engines.left, config, dt_s);
	update_dry_engine_channel(engines.right, config, dt_s);
}

inline void clamp_engine_throttle_inputs(EngineSystemState& engines)
{
	engines.left.throttle_input_normalized = Common::limit(
		engines.left.throttle_input_normalized, 0.0, 1.0);
	engines.right.throttle_input_normalized = Common::limit(
		engines.right.throttle_input_normalized, 0.0, 1.0);
}

inline void apply_engine_readout_integrity(
	EngineSystemState& engines,
	double left_engine_integrity,
	double right_engine_integrity)
{
	engines.left.power_readout_normalized *= left_engine_integrity;
	engines.right.power_readout_normalized *= right_engine_integrity;
}

inline bool should_shutdown_engines(
	double internal_fuel_kg,
	double altitude_asl_m)
{
	return internal_fuel_kg <= 0.0 ||
		altitude_asl_m > kEngineShutdownAltitudeM;
}

inline void shutdown_engines(EngineSystemState& engines, double dt_s)
{
	engines.left.afterburner_ratio_0_1 = 0.0;
	engines.right.afterburner_ratio_0_1 = 0.0;
	engines.left.afterburner_lit = false;
	engines.right.afterburner_lit = false;
	engines.left.switch_on = false;
	engines.right.switch_on = false;
	engines.left.power_readout_normalized = Common::actuator(
		engines.left.power_readout_normalized,
		{ 0.0, -dt_s / 10, dt_s / 10 });
	engines.right.power_readout_normalized = Common::actuator(
		engines.right.power_readout_normalized,
		{ 0.0, -dt_s / 10, dt_s / 10 });
}

inline double estimate_nozzle_aperture_target(
	const NozzleTargetInput& input,
	const AfterburnerConfig& afterburner)
{
	return command_nozzle_aperture(
		{
			input.throttle_input_normalized,
			input.power_readout_normalized,
			input.afterburner_ratio_0_1,
			input.engine_on
		},
		afterburner);
}

inline void configure_engine_start_channel(
	EngineChannelState& engine,
	const EngineStartState& start,
	const AfterburnerConfig& afterburner)
{
	engine.switch_on = start.engine_on;
	engine.throttle_input_normalized = start.throttle_input_normalized;
	engine.throttle_output_normalized = start.throttle_output_normalized;
	engine.power_readout_normalized = start.power_readout_normalized;
	engine.nozzle_aperture_normalized = estimate_nozzle_aperture_target(
		{ start.throttle_input_normalized,
			start.power_readout_normalized,
			0.0,
			start.engine_on },
		afterburner);
}

inline void configure_cold_start_engines(
	EngineSystemState& engines,
	const EngineConfig& config)
{
	configure_engine_start_channel(
		engines.left, { false, 0.0, 0.0, 0.0 }, config.afterburner);
	configure_engine_start_channel(
		engines.right, { false, 0.0, 0.0, 0.0 }, config.afterburner);
}

inline void configure_hot_ground_start_engines(
	EngineSystemState& engines,
	const EngineConfig& config)
{
	configure_engine_start_channel(
		engines.left, { true, 0.0, 0.5, 0.5 }, config.afterburner);
	configure_engine_start_channel(
		engines.right, { true, 0.0, 0.5, 0.5 }, config.afterburner);
}

inline void configure_hot_air_start_engines(
	EngineSystemState& engines,
	const EngineConfig& config)
{
	configure_engine_start_channel(
		engines.left, { true, 0.5, 0.5, 0.5 }, config.afterburner);
	configure_engine_start_channel(
		engines.right, { true, 0.5, 0.5, 0.5 }, config.afterburner);
}

inline void reset_engine_release_state(EngineSystemState& engines)
{
	engines.throttle_cmd_left_normalized = 0.0;
	engines.throttle_cmd_right_normalized = 0.0;
	engines.left.afterburner_ratio_0_1 = 0.0;
	engines.right.afterburner_ratio_0_1 = 0.0;
	engines.left.nozzle_aperture_normalized = 0.80;
	engines.right.nozzle_aperture_normalized = 0.80;
}

inline double update_nozzle_aperture(
	double current,
	const NozzleUpdateInput& input)
{
	double aperture_rate = 0.50;

	if (!input.engine_on || input.power_readout_normalized < 0.50)
	{
		aperture_rate = 0.40;
	}
	else if (input.afterburner_ratio_0_1 > 0.0 &&
		input.target_normalized > current)
	{
		aperture_rate = 0.45;
	}
	else if (input.target_normalized < current)
	{
		aperture_rate = 0.35;
	}

	return Common::actuator(
		current,
		{
			input.target_normalized,
			-aperture_rate * input.dt_s,
			aperture_rate * input.dt_s
		});
}

inline void update_nozzle_aperture(
	EngineChannelState& engine,
	const AfterburnerConfig& afterburner,
	double dt_s)
{
	const double nozzle_throttle_normalized =
		engine.throttle_output_normalized * afterburner.detent_normalized;
	const double target = estimate_nozzle_aperture_target(
		{
			nozzle_throttle_normalized,
			engine.power_readout_normalized,
			engine.afterburner_ratio_0_1,
			engine.switch_on
		},
		afterburner);
	engine.nozzle_aperture_normalized = update_nozzle_aperture(
		engine.nozzle_aperture_normalized,
		{
			target,
			engine.power_readout_normalized,
			engine.afterburner_ratio_0_1,
			engine.switch_on,
			dt_s
		});
}

inline void update_nozzle_apertures(
	EngineSystemState& engines,
	const EngineConfig& config,
	double dt_s)
{
	update_nozzle_aperture(engines.left, config.afterburner, dt_s);
	update_nozzle_aperture(engines.right, config.afterburner, dt_s);
}
}
