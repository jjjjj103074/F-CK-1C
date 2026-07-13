#pragma once

#include "../Common/Actuator.h"
#include "../Common/Clamp.h"
#include "../Common/Table.h"

namespace Systems
{
struct EngineSystemConfig
{
	double start_time = 0.0;
	double spool_up_tau = 0.0;
	double spool_down_tau = 0.0;
	const double* mach_table = nullptr;
	const double* max_thrust_table = nullptr;
	unsigned mach_table_size = 0;
	const double* throttle_input_table = nullptr;
	const double* power_table = nullptr;
	unsigned throttle_table_size = 0;
};

struct EngineChannelState
{
	bool switch_on = false;
	double throttle_input = 0.0;
	double throttle_output = 0.0;
	double power_readout = 0.0;
	double thrust_force = 0.0;
	double afterburner_ratio = 0.0;
	bool afterburner_lit = false;
	double nozzle_aperture = 0.80;
};

struct AfterburnerConfig
{
	double detent = 0.70;
	double thrust_factor = 1.73;
	double fuel_factor = 2.2;
	double core_rpm = 0.94;
	double core_drop_time = 0.80;
	double spool_in_tau = 2.0;
	double spool_out_tau = 0.6;
	double light_throttle_output_min = 0.88;
};

struct EngineSystemState
{
	EngineChannelState left;
	EngineChannelState right;
	double throttle_cmd_left = 0.0;
	double throttle_cmd_right = 0.0;
	AfterburnerConfig afterburner;
};

inline double max_dry_thrust(const EngineSystemConfig& config, double mach)
{
	return Common::lerp(
		config.mach_table,
		config.max_thrust_table,
		config.mach_table_size,
		mach);
}

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

inline double engine_first_order(double current, double target, double tau, double dt)
{
	if (tau <= 1e-6)
	{
		return target;
	}
	const double k = Common::limit(dt / (tau + dt), 0.0, 1.0);
	return current + (target - current) * k;
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
		engine_first_order(engine.afterburner_ratio, target, tau, dt),
		0.0,
		1.0);
}

inline void update_afterburners(EngineSystemState& engines, double dt)
{
	update_afterburner(engines.left, engines.afterburner, dt);
	update_afterburner(engines.right, engines.afterburner, dt);
}

inline void update_dry_engine_channel(
	EngineChannelState& engine,
	double dt,
	double engine_start_time,
	const double* throttle_input_table,
	const double* engine_power_table,
	unsigned engine_table_size,
	double engine_spool_up_tau,
	double engine_spool_down_tau,
	const AfterburnerConfig& afterburner)
{
	if (engine.switch_on == false)
	{
		engine.throttle_output = Common::actuator(engine.throttle_output, 0.0, -0.01, 0.01);
		engine.power_readout = Common::actuator(
			engine.power_readout,
			0.0,
			-dt / (engine_start_time / 2),
			dt / (engine_start_time / 2));
		engine.throttle_input = Common::limit(engine.throttle_input, 0.0, 0.0);
	}

	if (engine.switch_on == true && engine.power_readout < 0.5)
	{
		engine.power_readout = Common::actuator(
			engine.power_readout,
			0.5,
			-dt / (engine_start_time / 2),
			dt / (engine_start_time / 2));
		engine.throttle_input = Common::limit(engine.throttle_input, 0.0, 0.1);
	}

	if (engine.switch_on == true && engine.power_readout >= 0.5)
	{
		const double mil_cmd = Common::limit(engine.throttle_input / afterburner.detent, 0.0, 1.0);
		const double throttle_target = Common::limit(
			Common::lerp(throttle_input_table, engine_power_table, engine_table_size, mil_cmd),
			0.1,
			1.0);
		const double spool_tau = (throttle_target > engine.throttle_output)
			? engine_spool_up_tau
			: engine_spool_down_tau;
		engine.throttle_output = engine_first_order(engine.throttle_output, throttle_target, spool_tau, dt);
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

		const double core_step = dt * ((1.0 - afterburner.core_rpm) / afterburner.core_drop_time);
		engine.power_readout = Common::actuator(engine.power_readout, target_core, -core_step, core_step);
	}
}

inline void update_dry_engine_channels(
	EngineSystemState& engines,
	const EngineSystemConfig& config,
	double dt)
{
	update_dry_engine_channel(
		engines.left,
		dt,
		config.start_time,
		config.throttle_input_table,
		config.power_table,
		config.throttle_table_size,
		config.spool_up_tau,
		config.spool_down_tau,
		engines.afterburner);
	update_dry_engine_channel(
		engines.right,
		dt,
		config.start_time,
		config.throttle_input_table,
		config.power_table,
		config.throttle_table_size,
		config.spool_up_tau,
		config.spool_down_tau,
		engines.afterburner);
}

inline void clamp_engine_throttle_inputs(EngineSystemState& engines)
{
	engines.left.throttle_input = Common::limit(engines.left.throttle_input, 0.0, 1.0);
	engines.right.throttle_input = Common::limit(engines.right.throttle_input, 0.0, 1.0);
}

inline void update_engine_thrust_outputs(
	EngineSystemState& engines,
	double max_dry_thrust,
	double engine_alt_effect,
	double left_engine_integrity,
	double right_engine_integrity)
{
	const double max_ab_thrust = max_dry_thrust * engines.afterburner.thrust_factor;

	const double left_dry_force = engines.left.throttle_output
		* max_dry_thrust
		* engine_alt_effect
		* left_engine_integrity
		* 0.5;
	const double right_dry_force = engines.right.throttle_output
		* max_dry_thrust
		* engine_alt_effect
		* right_engine_integrity
		* 0.5;
	const double left_ab_extra = engines.left.afterburner_ratio
		* (max_ab_thrust - max_dry_thrust)
		* engine_alt_effect
		* left_engine_integrity
		* 0.5;
	const double right_ab_extra = engines.right.afterburner_ratio
		* (max_ab_thrust - max_dry_thrust)
		* engine_alt_effect
		* right_engine_integrity
		* 0.5;

	engines.left.thrust_force = left_dry_force + left_ab_extra;
	engines.right.thrust_force = right_dry_force + right_ab_extra;
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
	engines.left.thrust_force = 0.0;
	engines.right.thrust_force = 0.0;
	engines.left.afterburner_ratio = 0.0;
	engines.right.afterburner_ratio = 0.0;
	engines.left.afterburner_lit = false;
	engines.right.afterburner_lit = false;
	engines.left.switch_on = false;
	engines.right.switch_on = false;
	engines.left.power_readout = Common::actuator(engines.left.power_readout, 0.0, -dt / 10, dt / 10);
	engines.right.power_readout = Common::actuator(engines.right.power_readout, 0.0, -dt / 10, dt / 10);
}

inline void apply_thrust_cut(EngineSystemState& engines, bool cut_thrust)
{
	if (!cut_thrust)
	{
		return;
	}

	engines.left.thrust_force = 0.0;
	engines.right.thrust_force = 0.0;
}

inline double remap_engine_range(double value, double in_min, double in_max, double out_min, double out_max)
{
	if (in_max <= in_min)
	{
		return out_max;
	}

	const double normalized = Common::limit((value - in_min) / (in_max - in_min), 0.0, 1.0);
	return out_min + (out_max - out_min) * normalized;
}

inline double estimate_nozzle_aperture_target(
	double throttle_input,
	double engine_power_readout,
	double afterburner_ratio,
	bool engine_on,
	const AfterburnerConfig& afterburner)
{
	if (!engine_on)
	{
		return 0.80;
	}

	const double limited_power = Common::limit(engine_power_readout, 0.0, 1.0);
	const double limited_throttle = Common::limit(throttle_input, 0.0, 1.0);
	const double limited_ab = Common::limit(afterburner_ratio, 0.0, 1.0);

	if (limited_power < 0.50)
	{
		return remap_engine_range(limited_power, 0.0, 0.50, 0.80, 0.40);
	}

	if (limited_ab <= 0.0)
	{
		const double dry_ratio = Common::limit(limited_throttle / afterburner.detent, 0.0, 1.0);

		if (dry_ratio <= 0.15)
		{
			return remap_engine_range(dry_ratio, 0.0, 0.15, 0.40, 0.30);
		}
		if (dry_ratio <= 0.45)
		{
			return remap_engine_range(dry_ratio, 0.15, 0.45, 0.30, 0.18);
		}
		if (dry_ratio <= 0.75)
		{
			return remap_engine_range(dry_ratio, 0.45, 0.75, 0.18, 0.08);
		}

		return remap_engine_range(dry_ratio, 0.75, 1.0, 0.08, 0.00);
	}

	if (limited_ab <= 0.25)
	{
		return remap_engine_range(limited_ab, 0.0, 0.25, 0.00, 0.18);
	}
	if (limited_ab <= 0.60)
	{
		return remap_engine_range(limited_ab, 0.25, 0.60, 0.18, 0.55);
	}

	return remap_engine_range(limited_ab, 0.60, 1.0, 0.55, 1.0);
}

inline void configure_engine_start_channel(
	EngineChannelState& engine,
	bool engine_on,
	double throttle_input,
	double throttle_output,
	double power_readout,
	const AfterburnerConfig& afterburner)
{
	engine.switch_on = engine_on;
	engine.throttle_input = throttle_input;
	engine.throttle_output = throttle_output;
	engine.power_readout = power_readout;
	engine.nozzle_aperture = estimate_nozzle_aperture_target(
		throttle_input,
		power_readout,
		0.0,
		engine_on,
		afterburner);
}

inline void configure_cold_start_engines(EngineSystemState& engines)
{
	configure_engine_start_channel(engines.left, false, 0.0, 0.0, 0.0, engines.afterburner);
	configure_engine_start_channel(engines.right, false, 0.0, 0.0, 0.0, engines.afterburner);
}

inline void configure_hot_ground_start_engines(EngineSystemState& engines)
{
	configure_engine_start_channel(engines.left, true, 0.0, 0.5, 0.5, engines.afterburner);
	configure_engine_start_channel(engines.right, true, 0.0, 0.5, 0.5, engines.afterburner);
}

inline void configure_hot_air_start_engines(EngineSystemState& engines)
{
	configure_engine_start_channel(engines.left, true, 0.5, 0.5, 0.5, engines.afterburner);
	configure_engine_start_channel(engines.right, true, 0.5, 0.5, 0.5, engines.afterburner);
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
	double target,
	double engine_power_readout,
	double afterburner_ratio,
	bool engine_on,
	double dt)
{
	double aperture_rate = 0.50;

	if (!engine_on || engine_power_readout < 0.50)
	{
		aperture_rate = 0.40;
	}
	else if (afterburner_ratio > 0.0 && target > current)
	{
		aperture_rate = 0.45;
	}
	else if (target < current)
	{
		aperture_rate = 0.35;
	}

	return Common::actuator(current, target, -aperture_rate * dt, aperture_rate * dt);
}

inline void update_nozzle_aperture(
	EngineChannelState& engine,
	const AfterburnerConfig& afterburner,
	double dt)
{
	const double nozzle_throttle = engine.throttle_output * afterburner.detent;
	const double target = estimate_nozzle_aperture_target(
		nozzle_throttle,
		engine.power_readout,
		engine.afterburner_ratio,
		engine.switch_on,
		afterburner);
	engine.nozzle_aperture = update_nozzle_aperture(
		engine.nozzle_aperture,
		target,
		engine.power_readout,
		engine.afterburner_ratio,
		engine.switch_on,
		dt);
}

inline void update_nozzle_apertures(EngineSystemState& engines, double dt)
{
	update_nozzle_aperture(engines.left, engines.afterburner, dt);
	update_nozzle_aperture(engines.right, engines.afterburner, dt);
}
}
