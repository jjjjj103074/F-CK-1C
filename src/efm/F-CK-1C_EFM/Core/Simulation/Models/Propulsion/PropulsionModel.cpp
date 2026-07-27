#include "PropulsionModel.h"

#include "../../../../Common/Table.h"

namespace
{
constexpr std::size_t kEngineEffectCapacity = 2;
constexpr double kMaxPowerReadyThreshold = 0.5;
constexpr double kMaxPowerCutThreshold = 0.5;
constexpr double kPerEngineThrustShare = 0.5;

struct ThrustConditions
{
	double max_dry_thrust = 0.0;
	double altitude_effect = 1.0;
	double afterburner_thrust_factor = 1.0;
};

double calculate_channel_thrust(
	const Core::EngineChannelData& engine,
	const ThrustConditions& conditions)
{
	const double dry_force = engine.throttle_output
		* conditions.max_dry_thrust
		* conditions.altitude_effect
		* engine.condition
		* kPerEngineThrustShare;
	const double afterburner_extra = engine.afterburner_ratio
		* conditions.max_dry_thrust
		* (conditions.afterburner_thrust_factor - 1.0)
		* conditions.altitude_effect
		* engine.condition
		* kPerEngineThrustShare;
	return dry_force + afterburner_extra;
}

bool should_cut_thrust(const Core::MaxPowerCommand& command)
{
	return command.ready > kMaxPowerReadyThreshold &&
		command.value < kMaxPowerCutThreshold;
}
}

namespace Core
{
namespace Simulation
{
PropulsionModel::PropulsionModel(
	const PropulsionModelDefinition& definition)
	: mach_table_(definition.mach_table),
	max_thrust_table_(definition.max_thrust_table),
	afterburner_thrust_factor_(definition.afterburner_thrust_factor),
	left_engine_position_(definition.left_engine_position),
	right_engine_position_(definition.right_engine_position)
{
	result_.effects.reserve(kEngineEffectCapacity);
}

const PropulsionResult& PropulsionModel::step(
	const PropulsionModelInput& input)
{
	const double dry_thrust = Common::lerp(
		{
			mach_table_.data(),
			max_thrust_table_.data(),
			static_cast<unsigned>(mach_table_.size())
		},
		input.observation.mach);
	const ThrustConditions conditions = {
		dry_thrust,
		input.observation.engine_alt_effect,
		afterburner_thrust_factor_
	};
	result_.left_thrust_force =
		calculate_channel_thrust(input.engines.left, conditions);
	result_.right_thrust_force =
		calculate_channel_thrust(input.engines.right, conditions);
	if (input.engines.thrust_inhibited ||
		should_cut_thrust(input.max_power))
	{
		result_.left_thrust_force = 0.0;
		result_.right_thrust_force = 0.0;
	}
	result_.effects.clear();
	result_.effects.push_back(make_local_force_effect(
		{ result_.left_thrust_force, 0.0, 0.0 },
		left_engine_position_));
	result_.effects.push_back(make_local_force_effect(
		{ result_.right_thrust_force, 0.0, 0.0 },
		right_engine_position_));
	return result_;
}
}
}
