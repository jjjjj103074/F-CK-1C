#pragma once

#include "EngineConfig.h"
#include "Common/Clamp.h"
#include "Common/Table.h"

namespace Systems
{
// Responsibility seam for the TFE1042 all-digital electronic control.
// Public sources confirm digital control, but not the exact F-CK-1C control
// laws or the DEEC hardware name. These commands are project approximations;
// the physical spool, afterburner, and nozzle dynamics remain in EngineModel.
struct DigitalEngineControlDryCommand
{
	double throttle_output_target_normalized = 0.0;
	double spool_time_constant_s = 0.0;
	double core_speed_target_0_1 = 0.0;
	double core_speed_rate_0_1_per_s = 0.0;
};

struct DigitalEngineControlAfterburnerCommand
{
	bool lit = false;
	double ratio_target_0_1 = 0.0;
	double spool_time_constant_s = 0.0;
};

struct DigitalEngineControlObservation
{
	double throttle_input_normalized = 0.0;
	double power_readout_normalized = 0.0;
	double afterburner_ratio_0_1 = 0.0;
	bool engine_on = false;
};

struct DigitalEngineControlFuelInput
{
	double left_throttle_output_normalized = 0.0;
	double right_throttle_output_normalized = 0.0;
	double left_afterburner_ratio_0_1 = 0.0;
	double right_afterburner_ratio_0_1 = 0.0;
};

struct DigitalEngineControlFuelCommand
{
	double flow_rate_kg_s = 0.0;
};

struct DigitalEngineControlRangeMapping
{
	double input_min = 0.0;
	double input_max = 0.0;
	double output_min = 0.0;
	double output_max = 0.0;
};

inline DigitalEngineControlDryCommand command_dry_engine(
	double throttle_input_normalized,
	double throttle_output_normalized,
	const Core::Systems::EngineConfig& config)
{
	const ::Systems::AfterburnerConfig& afterburner =
		config.afterburner;
	const double mil_command = Common::limit(
		throttle_input_normalized / afterburner.detent_normalized, 0.0, 1.0);
	const double throttle_target = Common::limit(
		Common::lerp({
			config.throttle_input_table_normalized.data(),
			config.power_table_normalized.data(),
			static_cast<unsigned>(
				config.throttle_input_table_normalized.size())
		}, mil_command),
		0.1,
		1.0);
	const double spool_time_constant =
		throttle_target > throttle_output_normalized
		? config.spool_up_tau_s : config.spool_down_tau_s;
	const double core_target =
		throttle_input_normalized <= afterburner.detent_normalized
		? Common::limit(0.5 + 0.5 * mil_command, 0.0, 1.0)
		: afterburner.core_rpm_0_1;
	return {
		throttle_target,
		spool_time_constant,
		core_target,
		(1.0 - afterburner.core_rpm_0_1) / afterburner.core_drop_time_s
	};
}

inline DigitalEngineControlAfterburnerCommand command_afterburner(
	const DigitalEngineControlObservation& input,
	bool currently_lit,
	const ::Systems::AfterburnerConfig& config)
{
	const double demand = Common::limit(
		(input.throttle_input_normalized - config.detent_normalized) /
			(1.0 - config.detent_normalized),
		0.0,
		1.0);
	bool lit = currently_lit;
	if (input.engine_on &&
		input.throttle_input_normalized > config.detent_normalized &&
		input.power_readout_normalized >=
			config.light_throttle_output_min_normalized)
	{
		lit = true;
	}
	if (!input.engine_on ||
		input.throttle_input_normalized <= config.detent_normalized)
	{
		lit = false;
	}
	const double target = lit ? demand : 0.0;
	return {
		lit,
		target,
		target > input.afterburner_ratio_0_1
			? config.spool_in_tau_s : config.spool_out_tau_s
	};
}

inline double remap_deec_range(
	double value,
	const DigitalEngineControlRangeMapping& mapping)
{
	if (mapping.input_max <= mapping.input_min)
	{
		return mapping.output_max;
	}
	const double normalized = Common::limit(
		(value - mapping.input_min) /
			(mapping.input_max - mapping.input_min),
		0.0,
		1.0);
	return mapping.output_min +
		(mapping.output_max - mapping.output_min) * normalized;
}

inline double command_dry_nozzle_aperture(
	double limited_power,
	double limited_throttle,
	const ::Systems::AfterburnerConfig& config)
{
	// These named mappings preserve the legacy calibrated visual schedule.
	constexpr DigitalEngineControlRangeMapping kLowPowerMapping = {
		0.0, 0.50, 0.80, 0.40
	};
	constexpr DigitalEngineControlRangeMapping kIdleMapping = {
		0.0, 0.15, 0.40, 0.30
	};
	constexpr DigitalEngineControlRangeMapping kLowDryMapping = {
		0.15, 0.45, 0.30, 0.18
	};
	constexpr DigitalEngineControlRangeMapping kMediumDryMapping = {
		0.45, 0.75, 0.18, 0.08
	};
	constexpr DigitalEngineControlRangeMapping kHighDryMapping = {
		0.75, 1.0, 0.08, 0.00
	};
	if (limited_power < kLowPowerMapping.input_max)
	{
		return remap_deec_range(limited_power, kLowPowerMapping);
	}
	const double dry_ratio = Common::limit(
		limited_throttle / config.detent_normalized, 0.0, 1.0);
	if (dry_ratio <= kIdleMapping.input_max)
	{
		return remap_deec_range(dry_ratio, kIdleMapping);
	}
	if (dry_ratio <= kLowDryMapping.input_max)
	{
		return remap_deec_range(dry_ratio, kLowDryMapping);
	}
	if (dry_ratio <= kMediumDryMapping.input_max)
	{
		return remap_deec_range(dry_ratio, kMediumDryMapping);
	}
	return remap_deec_range(dry_ratio, kHighDryMapping);
}

inline double command_afterburner_nozzle_aperture(double ratio)
{
	constexpr DigitalEngineControlRangeMapping kLowMapping = {
		0.0, 0.25, 0.00, 0.18
	};
	constexpr DigitalEngineControlRangeMapping kMediumMapping = {
		0.25, 0.60, 0.18, 0.55
	};
	constexpr DigitalEngineControlRangeMapping kHighMapping = {
		0.60, 1.0, 0.55, 1.0
	};
	if (ratio <= kLowMapping.input_max)
	{
		return remap_deec_range(ratio, kLowMapping);
	}
	if (ratio <= kMediumMapping.input_max)
	{
		return remap_deec_range(ratio, kMediumMapping);
	}
	return remap_deec_range(ratio, kHighMapping);
}

inline double command_nozzle_aperture(
	const DigitalEngineControlObservation& input,
	const ::Systems::AfterburnerConfig& config)
{
	if (!input.engine_on)
	{
		return 0.80;
	}
	const double power = Common::limit(
		input.power_readout_normalized, 0.0, 1.0);
	const double throttle = Common::limit(
		input.throttle_input_normalized, 0.0, 1.0);
	const double afterburner = Common::limit(
		input.afterburner_ratio_0_1, 0.0, 1.0);
	return afterburner <= 0.0
		? command_dry_nozzle_aperture(power, throttle, config)
		: command_afterburner_nozzle_aperture(afterburner);
}

inline DigitalEngineControlFuelCommand command_fuel_flow(
	const DigitalEngineControlFuelInput& input,
	const Core::Systems::EngineConfig& config)
{
	constexpr double kChannelBias = 1.0;
	constexpr double kChannelDivisor = 3.0;
	const double afterburner_average = 0.5 *
		(input.left_afterburner_ratio_0_1 +
			input.right_afterburner_ratio_0_1);
	const double multiplier = 1.0 + afterburner_average *
		(config.afterburner.fuel_factor - 1.0);
	return {
		config.fuel_consumption_rate_kg_s *
		((input.left_throttle_output_normalized +
			input.right_throttle_output_normalized +
			kChannelBias) / kChannelDivisor) *
		multiplier
	};
}
}
