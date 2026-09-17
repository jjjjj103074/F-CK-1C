#pragma once

#include "../../ControlLaws/ControlLawConfig.h"
#include "Common/Clamp.h"

namespace Systems
{
inline double schedule_blend(double a, double b, double amount)
{
	return a + (b - a) * Common::limit(amount, 0.0, 1.0);
}

inline PilotInputShapingConfig blend_pilot_input(
	const PilotInputShapingConfig& cat1,
	const PilotInputShapingConfig& cat3,
	double amount)
{
	PilotInputShapingConfig result;
	result.deadband_normalized = schedule_blend(
		cat1.deadband_normalized, cat3.deadband_normalized, amount);
	result.command_time_constant_s = schedule_blend(
		cat1.command_time_constant_s, cat3.command_time_constant_s, amount);
	result.command_rate_normalized_s = schedule_blend(
		cat1.command_rate_normalized_s,
		cat3.command_rate_normalized_s, amount);
	result.cubic_weight = schedule_blend(
		cat1.cubic_weight, cat3.cubic_weight, amount);
	return result;
}

inline ManeuverEnvelopeSchedule blend_envelope_schedule(
	const ManeuverEnvelopeSchedule& cat1,
	const ManeuverEnvelopeSchedule& cat3,
	double amount)
{
	return {
		schedule_blend(cat1.maximum_roll_command_rad_s,
			cat3.maximum_roll_command_rad_s, amount),
		schedule_blend(cat1.maximum_yaw_command_rad_s,
			cat3.maximum_yaw_command_rad_s, amount),
		schedule_blend(cat1.soft_positive_load_factor_g,
			cat3.soft_positive_load_factor_g, amount),
		schedule_blend(cat1.hard_positive_load_factor_g,
			cat3.hard_positive_load_factor_g, amount),
		schedule_blend(cat1.roll_rate_limit_rad_s,
			cat3.roll_rate_limit_rad_s, amount),
		schedule_blend(cat1.pitch_rate_limit_rad_s,
			cat3.pitch_rate_limit_rad_s, amount),
		schedule_blend(cat1.yaw_rate_limit_rad_s,
			cat3.yaw_rate_limit_rad_s, amount)
	};
}

inline DirectionalControlSchedule blend_directional_schedule(
	const DirectionalControlSchedule& cat1,
	const DirectionalControlSchedule& cat3,
	double amount)
{
	return {
		schedule_blend(cat1.sideslip_damping_s_inv,
			cat3.sideslip_damping_s_inv, amount),
		schedule_blend(cat1.yaw_rate_damping,
			cat3.yaw_rate_damping, amount)
	};
}

inline StoresControlLawSchedule blend_stores_schedule(
	const StoresControlLawSchedule& cat1,
	const StoresControlLawSchedule& cat3,
	double amount)
{
	return {
		blend_pilot_input(cat1.pilot_input, cat3.pilot_input, amount),
		blend_envelope_schedule(cat1.envelope, cat3.envelope, amount),
		blend_directional_schedule(cat1.directional, cat3.directional, amount)
	};
}

inline GainScheduleValues gain_values(const GainSchedulePoint& point)
{
	return { point.command_gain, point.damping_gain, point.limiter_gain };
}

inline GainScheduleValues evaluate_gain_schedule(
	const ModeAndGainSchedulingConfig& config,
	double dynamic_pressure_pa)
{
	if (dynamic_pressure_pa <= config.gain_schedule[0].dynamic_pressure_pa)
		return gain_values(config.gain_schedule[0]);
	for (unsigned index = 1; index < kGainScheduleSize; ++index)
	{
		const GainSchedulePoint& upper = config.gain_schedule[index];
		if (dynamic_pressure_pa > upper.dynamic_pressure_pa) continue;
		const GainSchedulePoint& lower = config.gain_schedule[index - 1];
		const double amount = Common::limit(
			(dynamic_pressure_pa - lower.dynamic_pressure_pa) /
				(upper.dynamic_pressure_pa - lower.dynamic_pressure_pa),
			0.0, 1.0);
		return {
			schedule_blend(lower.command_gain, upper.command_gain, amount),
			schedule_blend(lower.damping_gain, upper.damping_gain, amount),
			schedule_blend(lower.limiter_gain, upper.limiter_gain, amount)
		};
	}
	return gain_values(config.gain_schedule[kGainScheduleSize - 1]);
}
}
