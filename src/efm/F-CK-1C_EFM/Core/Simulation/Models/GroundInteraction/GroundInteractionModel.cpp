#include "GroundInteractionModel.h"

#include "../../../../Common/Clamp.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
constexpr std::size_t kGroundEffectCapacity = 5;
constexpr std::size_t kNoseWheelIndex = 0;
constexpr std::size_t kLeftMainWheelIndex = 1;
constexpr std::size_t kRightMainWheelIndex = 2;
constexpr double kMaximumSinkRate = 80.0;
constexpr double kGearSupportStart = 0.2;
constexpr double kGearSupportRange = 0.8;
constexpr double kGravity = 9.81;
constexpr double kNoseWheelLoadLimit = 0.45;
constexpr double kMainWheelLoadLimit = 1.15;
constexpr double kMinimumMainNormalForce = 1.0;
constexpr double kThrottleAverageFactor = 0.5;
constexpr double kRollingResistanceFactor = 0.035;
constexpr double kBrakeResistanceFactor = 0.85;
constexpr double kLowSpeedThreshold = 1.5;
constexpr double kIdleThrottleThreshold = 0.05;
constexpr double kStaticThrustResistanceFactor = 0.12;
constexpr double kMovingSpeedThreshold = 0.05;
constexpr double kBellyContactBand = 0.03;
constexpr double kBellySpring = 260000.0;
constexpr double kBellyDamping = 40000.0;
constexpr double kBellyLoadLimit = 2.0;
const Common::Vec3 kGroundResistancePoint(-0.9, -1.6, 0.0);

using Core::Simulation::GroundInteractionModelInput;
using Core::Simulation::GroundInteractionResult;

struct FallbackContext
{
	const Core::Simulation::GroundInteractionConfig& config;
	const GroundInteractionModelInput& input;
	double sink_rate = 0.0;
	double gear_support = 0.0;
};

struct FallbackGearLoads
{
	double total_force = 0.0;
	double left_main_normal = 0.0;
	double right_main_normal = 0.0;
	bool gear_contact = false;

	double total_main_normal() const
	{
		return left_main_normal + right_main_normal;
	}
};

bool has_missing_suspension_feedback(
	const Core::FrameDataAvailability& availability)
{
	return std::any_of(
		availability.suspension.begin(),
		availability.suspension.end(),
		[](bool available) { return !available; });
}

bool has_current_dcs_gear_contact(
	const GroundInteractionModelInput& input)
{
	for (std::size_t index = 0;
		index < Core::kFrameSuspensionWheelCount;
		++index)
	{
		if (input.availability.suspension[index] &&
			input.landing_gear.suspension[index].weight_on_wheel)
		{
			return true;
		}
	}
	return false;
}

FallbackContext make_fallback_context(
	const Core::Simulation::GroundInteractionConfig& config,
	const GroundInteractionModelInput& input)
{
	return {
		config,
		input,
		Common::limit(
			-input.observation.velocity_world.y, 0.0, kMaximumSinkRate),
		Common::limit(
			(input.landing_gear.position - kGearSupportStart) /
				kGearSupportRange,
			0.0,
			1.0)
	};
}

double world_vertical_offset(
	const Common::Vec3& point,
	const Core::AircraftState& observation)
{
	const double cos_pitch = std::cos(observation.pitch);
	const double sin_pitch = std::sin(observation.pitch);
	const double cos_roll = std::cos(observation.roll);
	const double sin_roll = std::sin(observation.roll);
	const double y_after_pitch =
		point.x * sin_pitch + point.y * cos_pitch;
	return y_after_pitch * cos_roll - point.z * sin_roll;
}

double wheel_force(
	const FallbackContext& context,
	std::size_t index,
	double compression)
{
	double force = compression * context.config.spring[index] *
		context.gear_support;
	force += context.sink_rate * context.config.damping[index] *
		context.gear_support;
	const double weight = context.input.observation.current_mass * kGravity;
	const double load_limit = index == kNoseWheelIndex
		? kNoseWheelLoadLimit
		: kMainWheelLoadLimit;
	return Common::limit(force, 0.0, weight * load_limit);
}

void record_wheel_load(
	std::size_t index,
	double force,
	FallbackGearLoads& loads)
{
	loads.total_force += force;
	loads.gear_contact = true;
	if (index == kLeftMainWheelIndex)
	{
		loads.left_main_normal = force;
	}
	if (index == kRightMainWheelIndex)
	{
		loads.right_main_normal = force;
	}
}

FallbackGearLoads apply_wheel_contacts(
	const FallbackContext& context,
	GroundInteractionResult& result)
{
	FallbackGearLoads loads;
	loads.gear_contact =
		has_current_dcs_gear_contact(context.input);
	if (context.gear_support <= 0.0)
	{
		return loads;
	}
	for (std::size_t index = 0;
		index < Core::kFrameSuspensionWheelCount;
		++index)
	{
		if (context.input.availability.suspension[index])
		{
			continue;
		}
		const double wheel_bottom_agl =
			context.input.observation.altitude_agl +
			world_vertical_offset(
				context.config.gear_points[index],
				context.input.observation) -
			context.input.landing_gear.wheel_radius[index];
		const double compression =
			context.config.contact_band[index] - wheel_bottom_agl;
		if (compression <= 0.0)
		{
			continue;
		}
		const double force = wheel_force(context, index, compression);
		result.effects.push_back(Core::Simulation::make_local_force_effect(
			{ 0.0, force, 0.0 },
			context.config.gear_points[index]));
		record_wheel_load(index, force, loads);
	}
	return loads;
}

void apply_longitudinal_resistance(
	const FallbackContext& context,
	const FallbackGearLoads& loads,
	GroundInteractionResult& result)
{
	const double total_main_normal = loads.total_main_normal();
	if (total_main_normal <= kMinimumMainNormalForce)
	{
		return;
	}
	const auto& input = context.input;
	const double forward_speed = input.observation.velocity_body.x;
	const double speed_abs = std::fabs(forward_speed);
	const double speed_sign = forward_speed >= 0.0 ? 1.0 : -1.0;
	const double average_throttle = kThrottleAverageFactor *
		(input.engines.left.throttle_input +
			input.engines.right.throttle_input);
	const double brake_normal =
		loads.left_main_normal *
			Common::limit(input.landing_gear.brake_left, 0.0, 1.0) +
		loads.right_main_normal *
			Common::limit(input.landing_gear.brake_right, 0.0, 1.0);
	double resistance = total_main_normal * kRollingResistanceFactor +
		brake_normal * kBrakeResistanceFactor;
	if (speed_abs < kLowSpeedThreshold &&
		average_throttle < kIdleThrottleThreshold)
	{
		resistance += Common::limit(
			input.propulsion.left_thrust_force +
				input.propulsion.right_thrust_force,
			0.0,
			total_main_normal * kStaticThrustResistanceFactor);
	}
	if (speed_abs > kMovingSpeedThreshold)
	{
		result.effects.push_back(
			Core::Simulation::make_local_force_effect(
				{ -speed_sign * resistance, 0.0, 0.0 },
				kGroundResistancePoint));
		return;
	}
	if (average_throttle < kIdleThrottleThreshold ||
		brake_normal > kMinimumMainNormalForce)
	{
		result.effects.push_back(
			Core::Simulation::make_local_force_effect(
				{ -resistance, 0.0, 0.0 },
				kGroundResistancePoint));
	}
}

void apply_belly_contact(
	const FallbackContext& context,
	const FallbackGearLoads& loads,
	GroundInteractionResult& result)
{
	if (loads.gear_contact)
	{
		return;
	}
	const double belly_bottom_agl =
		context.input.observation.altitude_agl +
		world_vertical_offset(
			context.config.belly_point,
			context.input.observation);
	const double compression = kBellyContactBand - belly_bottom_agl;
	if (compression <= 0.0)
	{
		return;
	}
	double force = compression * kBellySpring +
		context.sink_rate * kBellyDamping;
	force = Common::limit(
		force,
		0.0,
		context.input.observation.current_mass *
			kGravity * kBellyLoadLimit);
	result.effects.push_back(Core::Simulation::make_local_force_effect(
		{ 0.0, force, 0.0 },
		context.config.belly_point));
}
}

namespace Core
{
namespace Simulation
{
struct GroundInteractionModel::Implementation
{
	explicit Implementation(const GroundInteractionConfig& model_config)
		: config(model_config)
	{
		validate_ground_interaction_config(config);
		result.effects.reserve(kGroundEffectCapacity);
	}

	const GroundInteractionResult& step(
		const GroundInteractionModelInput& input);

	const GroundInteractionConfig config;
	GroundInteractionResult result;
};

const GroundInteractionResult&
	GroundInteractionModel::Implementation::step(
		const GroundInteractionModelInput& input)
{
	result.effects.clear();
	result.used_fallback =
		config.enable_fallback_ground_forces &&
		has_missing_suspension_feedback(input.availability);
	if (!result.used_fallback)
	{
		return result;
	}
	const FallbackContext context =
		make_fallback_context(config, input);
	const FallbackGearLoads loads =
		apply_wheel_contacts(context, result);
	apply_longitudinal_resistance(context, loads, result);
	apply_belly_contact(context, loads, result);
	return result;
}

GroundInteractionModel::GroundInteractionModel(
	const GroundInteractionConfig& config)
	: implementation_(std::make_unique<Implementation>(config))
{
}

GroundInteractionModel::~GroundInteractionModel() = default;

const GroundInteractionResult& GroundInteractionModel::step(
	const GroundInteractionModelInput& input)
{
	return implementation_->step(input);
}
}
}
