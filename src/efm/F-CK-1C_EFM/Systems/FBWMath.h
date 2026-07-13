#pragma once

#include "FBWControllerTypes.h"
#include "../Common/Clamp.h"
#include "../Common/Units.h"
#include <cmath>

namespace Systems
{
inline double fbw_blend_value(double a, double b, double t)
{
	return a + (b - a) * Common::limit(t, 0.0, 1.0);
}

inline double fbw_min(double a, double b)
{
	return (a < b) ? a : b;
}

inline double fbw_max(double a, double b)
{
	return (a > b) ? a : b;
}

inline double fbw_wrap_pi(double angle)
{
	double wrapped = angle;
	while (wrapped > Common::kPi)
	{
		wrapped -= 2.0 * Common::kPi;
	}
	while (wrapped < -Common::kPi)
	{
		wrapped += 2.0 * Common::kPi;
	}
	return wrapped;
}

inline double fbw_soft_limit_symmetric(double command, double limit_value)
{
	if (limit_value <= 1e-6)
	{
		return 0.0;
	}
	return limit_value * std::tanh(command / limit_value);
}

inline double fbw_smoothstep01(double value)
{
	const double limited = Common::limit(value, 0.0, 1.0);
	return limited * limited * (3.0 - 2.0 * limited);
}

inline double fbw_soft_clip_positive(double value, double soft_limit, double hard_limit)
{
	if (value <= soft_limit)
	{
		return value;
	}
	if (value >= hard_limit)
	{
		return hard_limit;
	}
	const double ratio = (value - soft_limit) / (hard_limit - soft_limit);
	return soft_limit + (hard_limit - soft_limit) * fbw_smoothstep01(ratio);
}

inline double fbw_soft_clip_negative(double value, double soft_limit, double hard_limit)
{
	if (value >= soft_limit)
	{
		return value;
	}
	if (value <= hard_limit)
	{
		return hard_limit;
	}
	const double ratio = (soft_limit - value) / (soft_limit - hard_limit);
	return soft_limit - (soft_limit - hard_limit) * fbw_smoothstep01(ratio);
}

inline FBWCatParams fbw_blend_cat_params(const FBWCatParams& cat1, const FBWCatParams& cat3, double blend)
{
	FBWCatParams out;
	out.deadband = fbw_blend_value(cat1.deadband, cat3.deadband, blend);
	out.hold_engage_time = fbw_blend_value(cat1.hold_engage_time, cat3.hold_engage_time, blend);
	out.hold_phi_kp = fbw_blend_value(cat1.hold_phi_kp, cat3.hold_phi_kp, blend);
	out.hold_theta_kp = fbw_blend_value(cat1.hold_theta_kp, cat3.hold_theta_kp, blend);
	out.hold_p_cmd_max = fbw_blend_value(cat1.hold_p_cmd_max, cat3.hold_p_cmd_max, blend);
	out.hold_q_cmd_max = fbw_blend_value(cat1.hold_q_cmd_max, cat3.hold_q_cmd_max, blend);
	out.alpha_hold_degrade_deg = fbw_blend_value(cat1.alpha_hold_degrade_deg, cat3.alpha_hold_degrade_deg, blend);
	out.qbar_min_hold = fbw_blend_value(cat1.qbar_min_hold, cat3.qbar_min_hold, blend);
	out.sat_time = fbw_blend_value(cat1.sat_time, cat3.sat_time, blend);
	out.hold_cmd_ratio_limit = fbw_blend_value(cat1.hold_cmd_ratio_limit, cat3.hold_cmd_ratio_limit, blend);
	out.hold_decay_tau = fbw_blend_value(cat1.hold_decay_tau, cat3.hold_decay_tau, blend);
	out.command_shape_tau = fbw_blend_value(cat1.command_shape_tau, cat3.command_shape_tau, blend);
	out.command_shape_rate = fbw_blend_value(cat1.command_shape_rate, cat3.command_shape_rate, blend);
	out.stick_expo = fbw_blend_value(cat1.stick_expo, cat3.stick_expo, blend);
	out.p_cmd_max = fbw_blend_value(cat1.p_cmd_max, cat3.p_cmd_max, blend);
	out.q_cmd_max = fbw_blend_value(cat1.q_cmd_max, cat3.q_cmd_max, blend);
	out.r_cmd_max = fbw_blend_value(cat1.r_cmd_max, cat3.r_cmd_max, blend);
	out.aoa_soft_deg = fbw_blend_value(cat1.aoa_soft_deg, cat3.aoa_soft_deg, blend);
	out.aoa_hard_deg = fbw_blend_value(cat1.aoa_hard_deg, cat3.aoa_hard_deg, blend);
	out.g_soft = fbw_blend_value(cat1.g_soft, cat3.g_soft, blend);
	out.g_hard = fbw_blend_value(cat1.g_hard, cat3.g_hard, blend);
	out.p_rate_limit = fbw_blend_value(cat1.p_rate_limit, cat3.p_rate_limit, blend);
	out.q_rate_limit = fbw_blend_value(cat1.q_rate_limit, cat3.q_rate_limit, blend);
	out.r_rate_limit = fbw_blend_value(cat1.r_rate_limit, cat3.r_rate_limit, blend);
	out.yaw_damper_beta = fbw_blend_value(cat1.yaw_damper_beta, cat3.yaw_damper_beta, blend);
	out.yaw_damper_r = fbw_blend_value(cat1.yaw_damper_r, cat3.yaw_damper_r, blend);
	return out;
}

inline FBWGainScheduleValues fbw_gain_values(const FBWGainSchedulePoint& point)
{
	return { point.cmd_gain, point.hold_gain, point.damping_gain, point.limiter_gain };
}

inline FBWGainScheduleValues fbw_eval_gain_schedule(
	const FBWControllerConfig& config,
	double qbar_value)
{
	if (qbar_value <= config.gain_schedule[0].qbar)
	{
		return fbw_gain_values(config.gain_schedule[0]);
	}

	for (unsigned index = 1; index < kFBWGainScheduleSize; ++index)
	{
		const FBWGainSchedulePoint& upper = config.gain_schedule[index];
		if (qbar_value > upper.qbar)
		{
			continue;
		}
		const FBWGainSchedulePoint& lower = config.gain_schedule[index - 1];
		const double blend = Common::limit(
			(qbar_value - lower.qbar) / (upper.qbar - lower.qbar),
			0.0,
			1.0);
		return {
			fbw_blend_value(lower.cmd_gain, upper.cmd_gain, blend),
			fbw_blend_value(lower.hold_gain, upper.hold_gain, blend),
			fbw_blend_value(lower.damping_gain, upper.damping_gain, blend),
			fbw_blend_value(lower.limiter_gain, upper.limiter_gain, blend)
		};
	}
	return fbw_gain_values(config.gain_schedule[kFBWGainScheduleSize - 1]);
}
}
