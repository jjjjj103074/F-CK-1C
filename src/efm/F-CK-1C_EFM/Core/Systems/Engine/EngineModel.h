#pragma once

#include "EngineConfig.h"
#include "Common/Actuator.h"
#include "Common/Clamp.h"
#include "Common/Table.h"
#include <cstddef>
#include <vector>

namespace Systems
{
struct EngineChannelState
{
	bool switch_on = false;
	double throttle_input = 0.0;
	double throttle_output = 0.0;
	double power_readout = 0.0;
	double afterburner_ratio = 0.0;
	bool afterburner_lit = false;
	double nozzle_aperture = 0.80;
};

struct EngineSystemState
{
	EngineChannelState left;
	EngineChannelState right;
	double throttle_cmd_left = 0.0;
	double throttle_cmd_right = 0.0;
};

struct FirstOrderInput
{
	double target = 0.0;
	double tau = 0.0;
	double dt = 0.0;
};

struct RangeRemap
{
	double input_min = 0.0;
	double input_max = 0.0;
	double output_min = 0.0;
	double output_max = 0.0;
};

struct NozzleTargetInput
{
	double throttle_input = 0.0;
	double power_readout = 0.0;
	double afterburner_ratio = 0.0;
	bool engine_on = false;
};

struct EngineStartState
{
	bool engine_on = false;
	double throttle_input = 0.0;
	double throttle_output = 0.0;
	double power_readout = 0.0;
};

struct NozzleUpdateInput
{
	double target = 0.0;
	double power_readout = 0.0;
	double afterburner_ratio = 0.0;
	bool engine_on = false;
	double dt = 0.0;
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
	double left_command,
	double right_command)
{
	engines.throttle_cmd_left = Common::limit(left_command, 0.0, 1.0);
	engines.throttle_cmd_right = Common::limit(right_command, 0.0, 1.0);
	engines.left.throttle_input = engines.throttle_cmd_left;
	engines.right.throttle_input = engines.throttle_cmd_right;
}

inline double engine_first_order(double current, const FirstOrderInput& input)
{
	if (input.tau <= 1e-6)
	{
		return input.target;
	}
	const double k = Common::limit(
		input.dt / (input.tau + input.dt), 0.0, 1.0);
	return current + (input.target - current) * k;
}

inline void update_afterburner(EngineChannelState& engine, const AfterburnerConfig& afterburner, double dt)
{
	const double demand = Common::limit(
		(engine.throttle_input - afterburner.detent) / (1.0 - afterburner.detent),
		0.0,
		1.0);

	if (engine.switch_on &&
		engine.throttle_input > afterburner.detent &&
		engine.throttle_output >= afterburner.light_throttle_output_min)
	{
		engine.afterburner_lit = true;
	}
	else if (!engine.switch_on || engine.throttle_input <= afterburner.detent)
	{
		engine.afterburner_lit = false;
	}

	const double target = engine.afterburner_lit ? demand : 0.0;
	const double tau = (target > engine.afterburner_ratio) ? afterburner.spool_in_tau : afterburner.spool_out_tau;
	engine.afterburner_ratio = Common::limit(
		engine_first_order(engine.afterburner_ratio, { target, tau, dt }),
		0.0,
		1.0);
}

inline void update_afterburners(
	EngineSystemState& engines,
	const EngineConfig& config,
	double dt)
{
	update_afterburner(engines.left, config.afterburner, dt);
	update_afterburner(engines.right, config.afterburner, dt);
}

inline void update_stopped_engine(
	EngineChannelState& engine,
	const EngineConfig& config,
	double dt)
{
	engine.throttle_output = Common::actuator(
		engine.throttle_output, { 0.0, -0.01, 0.01 });
	engine.power_readout = Common::actuator(
		engine.power_readout,
		{ 0.0, -dt / (config.start_time / 2), dt / (config.start_time / 2) });
	engine.throttle_input = Common::limit(engine.throttle_input, 0.0, 0.0);
}

inline void update_starting_engine(
	EngineChannelState& engine,
	const EngineConfig& config,
	double dt)
{
	engine.power_readout = Common::actuator(
		engine.power_readout,
		{ 0.5, -dt / (config.start_time / 2), dt / (config.start_time / 2) });
	engine.throttle_input = Common::limit(engine.throttle_input, 0.0, 0.1);
}

inline void update_running_dry_engine(
	EngineChannelState& engine,
	const EngineConfig& config,
	double dt)
{
	const AfterburnerConfig& afterburner = config.afterburner;
	const double mil_cmd = Common::limit(
		engine.throttle_input / afterburner.detent, 0.0, 1.0);
	const double throttle_target = Common::limit(
		Common::lerp(
			{
				config.throttle_input_table.data(),
				config.power_table.data(),
				static_cast<unsigned>(config.throttle_input_table.size())
			},
			mil_cmd),
		0.1,
		1.0);
	const double spool_tau = throttle_target > engine.throttle_output
		? config.spool_up_tau : config.spool_down_tau;
	engine.throttle_output = engine_first_order(
		engine.throttle_output, { throttle_target, spool_tau, dt });
	engine.throttle_output = Common::limit(engine.throttle_output, 0.1, 1.0);
	double target_core = 0.5 + 0.5 * mil_cmd;
	if (engine.throttle_input <= afterburner.detent)
	{
		target_core = Common::limit(target_core, 0.0, 1.0);
	}
	else
	{
		target_core = afterburner.core_rpm;
	}
	const double core_step = dt *
		((1.0 - afterburner.core_rpm) / afterburner.core_drop_time);
	engine.power_readout = Common::actuator(
		engine.power_readout, { target_core, -core_step, core_step });
}

inline void update_dry_engine_channel(
	EngineChannelState& engine,
	const EngineConfig& config,
	double dt)
{
	if (!engine.switch_on)
	{
		update_stopped_engine(engine, config, dt);
	}
	if (engine.switch_on && engine.power_readout < 0.5)
	{
		update_starting_engine(engine, config, dt);
	}
	if (engine.switch_on && engine.power_readout >= 0.5)
	{
		update_running_dry_engine(engine, config, dt);
	}
}

inline void update_dry_engine_channels(
	EngineSystemState& engines,
	const EngineConfig& config,
	double dt)
{
	update_dry_engine_channel(engines.left, config, dt);
	update_dry_engine_channel(engines.right, config, dt);
}

inline void clamp_engine_throttle_inputs(EngineSystemState& engines)
{
	engines.left.throttle_input = Common::limit(engines.left.throttle_input, 0.0, 1.0);
	engines.right.throttle_input = Common::limit(engines.right.throttle_input, 0.0, 1.0);
}

inline void apply_engine_readout_integrity(
	EngineSystemState& engines,
	double left_engine_integrity,
	double right_engine_integrity)
{
	engines.left.power_readout *= left_engine_integrity;
	engines.right.power_readout *= right_engine_integrity;
}

inline bool should_shutdown_engines(double internal_fuel, double altitude_asl)
{
	return internal_fuel <= 0.0 || altitude_asl > 20000.0;
}

inline void shutdown_engines(EngineSystemState& engines, double dt)
{
	engines.left.afterburner_ratio = 0.0;
	engines.right.afterburner_ratio = 0.0;
	engines.left.afterburner_lit = false;
	engines.right.afterburner_lit = false;
	engines.left.switch_on = false;
	engines.right.switch_on = false;
	engines.left.power_readout = Common::actuator(
		engines.left.power_readout, { 0.0, -dt / 10, dt / 10 });
	engines.right.power_readout = Common::actuator(
		engines.right.power_readout, { 0.0, -dt / 10, dt / 10 });
}

inline double remap_engine_range(double value, const RangeRemap& range)
{
	if (range.input_max <= range.input_min)
	{
		return range.output_max;
	}
	const double normalized = Common::limit(
		(value - range.input_min) / (range.input_max - range.input_min),
		0.0,
		1.0);
	return range.output_min +
		(range.output_max - range.output_min) * normalized;
}

inline double estimate_dry_nozzle_aperture(
	double limited_power,
	double limited_throttle,
	const AfterburnerConfig& afterburner)
{
	if (limited_power < 0.50)
	{
		return remap_engine_range(limited_power, { 0.0, 0.50, 0.80, 0.40 });
	}
	const double dry_ratio = Common::limit(
		limited_throttle / afterburner.detent, 0.0, 1.0);
	if (dry_ratio <= 0.15)
	{
		return remap_engine_range(dry_ratio, { 0.0, 0.15, 0.40, 0.30 });
	}
	if (dry_ratio <= 0.45)
	{
		return remap_engine_range(dry_ratio, { 0.15, 0.45, 0.30, 0.18 });
	}
	if (dry_ratio <= 0.75)
	{
		return remap_engine_range(dry_ratio, { 0.45, 0.75, 0.18, 0.08 });
	}
	return remap_engine_range(dry_ratio, { 0.75, 1.0, 0.08, 0.00 });
}

inline double estimate_afterburner_nozzle_aperture(double limited_afterburner)
{
	if (limited_afterburner <= 0.25)
	{
		return remap_engine_range(
			limited_afterburner, { 0.0, 0.25, 0.00, 0.18 });
	}
	if (limited_afterburner <= 0.60)
	{
		return remap_engine_range(
			limited_afterburner, { 0.25, 0.60, 0.18, 0.55 });
	}
	return remap_engine_range(
		limited_afterburner, { 0.60, 1.0, 0.55, 1.0 });
}

inline double estimate_nozzle_aperture_target(
	const NozzleTargetInput& input,
	const AfterburnerConfig& afterburner)
{
	if (!input.engine_on)
	{
		return 0.80;
	}
	const double limited_power = Common::limit(input.power_readout, 0.0, 1.0);
	const double limited_throttle = Common::limit(input.throttle_input, 0.0, 1.0);
	const double limited_afterburner = Common::limit(
		input.afterburner_ratio, 0.0, 1.0);
	return limited_afterburner <= 0.0
		? estimate_dry_nozzle_aperture(
			limited_power, limited_throttle, afterburner)
		: estimate_afterburner_nozzle_aperture(limited_afterburner);
}

inline void configure_engine_start_channel(
	EngineChannelState& engine,
	const EngineStartState& start,
	const AfterburnerConfig& afterburner)
{
	engine.switch_on = start.engine_on;
	engine.throttle_input = start.throttle_input;
	engine.throttle_output = start.throttle_output;
	engine.power_readout = start.power_readout;
	engine.nozzle_aperture = estimate_nozzle_aperture_target(
		{ start.throttle_input, start.power_readout, 0.0, start.engine_on },
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
	engines.throttle_cmd_left = 0.0;
	engines.throttle_cmd_right = 0.0;
	engines.left.afterburner_ratio = 0.0;
	engines.right.afterburner_ratio = 0.0;
	engines.left.nozzle_aperture = 0.80;
	engines.right.nozzle_aperture = 0.80;
}

inline double update_nozzle_aperture(
	double current,
	const NozzleUpdateInput& input)
{
	double aperture_rate = 0.50;

	if (!input.engine_on || input.power_readout < 0.50)
	{
		aperture_rate = 0.40;
	}
	else if (input.afterburner_ratio > 0.0 && input.target > current)
	{
		aperture_rate = 0.45;
	}
	else if (input.target < current)
	{
		aperture_rate = 0.35;
	}

	return Common::actuator(
		current,
		{ input.target, -aperture_rate * input.dt, aperture_rate * input.dt });
}

inline void update_nozzle_aperture(
	EngineChannelState& engine,
	const AfterburnerConfig& afterburner,
	double dt)
{
	const double nozzle_throttle = engine.throttle_output * afterburner.detent;
	const double target = estimate_nozzle_aperture_target(
		{
			nozzle_throttle,
			engine.power_readout,
			engine.afterburner_ratio,
			engine.switch_on
		},
		afterburner);
	engine.nozzle_aperture = update_nozzle_aperture(
		engine.nozzle_aperture,
		{
			target,
			engine.power_readout,
			engine.afterburner_ratio,
			engine.switch_on,
			dt
		});
}

inline void update_nozzle_apertures(
	EngineSystemState& engines,
	const EngineConfig& config,
	double dt)
{
	update_nozzle_aperture(engines.left, config.afterburner, dt);
	update_nozzle_aperture(engines.right, config.afterburner, dt);
}
}
