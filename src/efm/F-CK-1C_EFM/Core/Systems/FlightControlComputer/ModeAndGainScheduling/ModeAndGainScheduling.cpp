#include "ModeAndGainScheduling.h"

#include "Internal/ModeAndGainMath.h"
#include "Common/Clamp.h"
#include "Common/CommandValue.h"
#include "Common/Table.h"

#include <cmath>
#include <stdexcept>

namespace
{
constexpr double kMinimumTimeConstantS = 1.0e-6;

struct TransitionInput
{
	double current = 0.0;
	double target = 0.0;
	double time_constant_s = 0.0;
	double dt_s = 0.0;
};

double transition_value(const TransitionInput& input)
{
	if (input.time_constant_s <= kMinimumTimeConstantS) return input.target;
	const double gain = Common::limit(
		input.dt_s / (input.time_constant_s + input.dt_s), 0.0, 1.0);
	return input.current + (input.target - input.current) * gain;
}

double scheduled_angle_of_attack_limit_rad(
	const Systems::ModeAndGainSchedulingConfig& config,
	double mach)
{
	return Common::lerp({ config.angle_of_attack_mach.data(),
		config.angle_of_attack_limit_rad.data(),
		static_cast<unsigned>(config.angle_of_attack_mach.size()) }, mach);
}

double configuration_value(double cruise, double landing, bool landing_mode)
{
	return landing_mode ? landing : cruise;
}

bool has_positive_physical_limits(const Systems::ManeuverEnvelope& envelope)
{
	return envelope.guidance.bank_limit_rad > 0.0 &&
		envelope.guidance.roll_rate_limit_rad_s > 0.0 &&
		envelope.hard_protection.bank_limit_rad > 0.0 &&
		envelope.hard_protection.angle_of_attack_blend_start_rad > 0.0 &&
		envelope.hard_protection.angle_of_attack_limit_rad > 0.0 &&
		envelope.hard_protection.roll_rate_limit_rad_s > 0.0 &&
		envelope.hard_protection.pitch_rate_limit_rad_s > 0.0 &&
		envelope.hard_protection.yaw_rate_limit_rad_s > 0.0;
}

bool has_ordered_angle_of_attack_range(
	const Systems::ManeuverEnvelope& envelope)
{
	return envelope.hard_protection.angle_of_attack_blend_start_rad <
		envelope.hard_protection.angle_of_attack_limit_rad;
}

bool has_ordered_load_ranges(const Systems::ManeuverEnvelope& envelope)
{
	return envelope.guidance.minimum_normal_acceleration_g <
			envelope.guidance.maximum_normal_acceleration_g &&
		envelope.hard_protection.minimum_normal_acceleration_g <
			envelope.hard_protection.maximum_normal_acceleration_g;
}

bool guidance_is_inside_protection(const Systems::ManeuverEnvelope& envelope)
{
	return envelope.guidance.bank_limit_rad <=
			envelope.hard_protection.bank_limit_rad &&
		envelope.guidance.roll_rate_limit_rad_s <=
			envelope.hard_protection.roll_rate_limit_rad_s &&
		envelope.guidance.minimum_normal_acceleration_g >=
			envelope.hard_protection.minimum_normal_acceleration_g &&
		envelope.guidance.maximum_normal_acceleration_g <=
			envelope.hard_protection.maximum_normal_acceleration_g;
}
}

namespace Systems
{
ModeAndGainScheduling::ModeAndGainScheduling(
	const ModeAndGainSchedulingConfig& config)
	: config_(config)
{
	active_.stores = config_.cat1;
	active_.gains = evaluate_gain_schedule(config_, 0.0);
}

std::vector<Core::Systems::FlightControlCommandBinding>
ModeAndGainScheduling::command_bindings()
{
	return {
		{ Core::CommandId::ToggleFbwCat,
			[this](const Core::Command& command)
			{
				toggle_stores_configuration(
					Common::command_value_is_pressed(command.value_normalized));
			} },
		{ Core::CommandId::SetFbwCat1,
			[this](const Core::Command& command)
			{
				if (Common::command_value_is_pressed(command.value_normalized))
					set_stores_configuration(StoresConfiguration::Cat1);
			} },
		{ Core::CommandId::SetFbwCat3,
			[this](const Core::Command& command)
			{
				if (Common::command_value_is_pressed(command.value_normalized))
					set_stores_configuration(StoresConfiguration::Cat3);
			} },
		{ Core::CommandId::SetGLimiterOverride,
			[this](const Core::Command& command)
			{
				g_limiter_override_active_ = config_.g_limiter_override.available &&
					Common::command_value_is_pressed(command.value_normalized);
			} },
		{ Core::CommandId::ToggleGLimiterOverride,
			[this](const Core::Command& command)
			{
				if (config_.g_limiter_override.available &&
					Common::command_value_is_pressed(command.value_normalized))
				{
					g_limiter_override_active_ = !g_limiter_override_active_;
				}
			} }
	};
}

const ActiveFlightControlConfiguration& ModeAndGainScheduling::update(
	const ModeAndGainSchedulingStepInput& input)
{
	const double target_blend =
		target_ == StoresConfiguration::Cat3 ? 1.0 : 0.0;
	active_.stores_transition_0_1 = transition_value({
		active_.stores_transition_0_1,
		target_blend,
		config_.stores_transition_time_constant_s,
		input.dt_s });
	active_.stores_configuration = target_;
	active_.stores = blend_stores_schedule(
		config_.cat1, config_.cat3, active_.stores_transition_0_1);
	if (input.update_slow_gain_schedule)
	{
		active_.gains = evaluate_gain_schedule(
			config_, input.dynamic_pressure_pa);
	}
	active_.envelope = make_maneuver_envelope({
		config_, active_.stores, input.mach,
		g_limiter_override_active_, config_.g_limiter_override.margin_g,
		input.landing_gear_handle_down });
	return active_;
}

void ModeAndGainScheduling::set_stores_configuration(
	StoresConfiguration configuration)
{
	target_ = configuration;
}

void ModeAndGainScheduling::toggle_stores_configuration(bool command_pressed)
{
	if (!command_pressed) return;
	target_ = target_ == StoresConfiguration::Cat1
		? StoresConfiguration::Cat3
		: StoresConfiguration::Cat1;
}

StoresConfiguration ModeAndGainScheduling::target_stores_configuration() const
{
	return target_;
}

double ModeAndGainScheduling::transition_0_1() const
{
	return active_.stores_transition_0_1;
}

bool ModeAndGainScheduling::g_limiter_override_active() const
{
	return g_limiter_override_active_;
}

ManeuverEnvelope make_maneuver_envelope(
	const ManeuverEnvelopeInput& input)
{
	const double cruise_limit_rad =
		scheduled_angle_of_attack_limit_rad(input.config, input.mach);
	const double blend_start_rad = configuration_value(
		input.config.cruise_angle_of_attack_blend_start_rad,
		input.config.landing_angle_of_attack_blend_start_rad,
		input.landing_gear_handle_down);
	const double limit_rad = configuration_value(cruise_limit_rad,
		input.config.landing_angle_of_attack_limit_rad,
		input.landing_gear_handle_down);
	const ManeuverEnvelope result = {
		{ input.config.guidance_bank_limit_rad,
			input.config.guidance_roll_rate_limit_rad_s,
			input.config.guidance_minimum_load_factor_g,
			input.config.guidance_maximum_load_factor_g },
		{ input.config.hard_bank_limit_rad,
			input.config.hard_minimum_load_factor_g,
			input.stores.envelope.hard_positive_load_factor_g +
				(input.developer_g_limiter_override_active
					? input.developer_g_limiter_override_margin_g : 0.0),
			blend_start_rad,
			limit_rad,
			input.stores.envelope.roll_rate_limit_rad_s,
			input.stores.envelope.pitch_rate_limit_rad_s,
			input.stores.envelope.yaw_rate_limit_rad_s }
	};
	validate_maneuver_envelope(result);
	return result;
}

void validate_maneuver_envelope(const ManeuverEnvelope& envelope)
{
	const double values[] = {
		envelope.guidance.bank_limit_rad,
		envelope.guidance.roll_rate_limit_rad_s,
		envelope.guidance.minimum_normal_acceleration_g,
		envelope.guidance.maximum_normal_acceleration_g,
		envelope.hard_protection.bank_limit_rad,
		envelope.hard_protection.minimum_normal_acceleration_g,
		envelope.hard_protection.maximum_normal_acceleration_g,
		envelope.hard_protection.angle_of_attack_blend_start_rad,
		envelope.hard_protection.angle_of_attack_limit_rad,
		envelope.hard_protection.roll_rate_limit_rad_s,
		envelope.hard_protection.pitch_rate_limit_rad_s,
		envelope.hard_protection.yaw_rate_limit_rad_s
	};
	for (double value : values)
	{
		if (!std::isfinite(value))
			throw std::invalid_argument("Non-finite maneuver envelope.");
	}
	if (!has_positive_physical_limits(envelope))
		throw std::invalid_argument("Maneuver envelope limit is not positive.");
	if (!has_ordered_load_ranges(envelope))
		throw std::invalid_argument("Maneuver envelope load range is reversed.");
	if (!has_ordered_angle_of_attack_range(envelope))
		throw std::invalid_argument("Maneuver envelope AOA range is reversed.");
	if (!guidance_is_inside_protection(envelope))
		throw std::invalid_argument("Guidance envelope exceeds protection.");
}
}
