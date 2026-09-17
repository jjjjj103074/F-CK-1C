#include "PropulsionModel.h"

#include "../../../../Common/Table.h"

namespace
{
constexpr std::size_t kEngineEffectCapacity = 2;
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
	const double dry_force = engine.throttle_output_normalized
		* conditions.max_dry_thrust
		* conditions.altitude_effect
		* engine.condition_0_1
		* kPerEngineThrustShare;
	const double afterburner_extra = engine.afterburner_ratio_0_1
		* conditions.max_dry_thrust
		* (conditions.afterburner_thrust_factor - 1.0)
		* conditions.altitude_effect
		* engine.condition_0_1
		* kPerEngineThrustShare;
	return dry_force + afterburner_extra;
}

}

namespace Core
{
namespace Simulation
{
PropulsionModel::PropulsionModel(
	const PropulsionConfig& config)
	: config_(config)
{
	validate_propulsion_config(config_);
	result_.effects.reserve(kEngineEffectCapacity);
}

const PropulsionResult& PropulsionModel::step(
	const PropulsionModelInput& input)
{
	const double dry_thrust = Common::lerp(
		{
			config_.mach_table.data(),
			config_.max_thrust_table_n.data(),
			static_cast<unsigned>(config_.mach_table.size())
		},
		input.observation.mach);
	const ThrustConditions conditions = {
		dry_thrust,
		input.observation.engine_altitude_factor,
		config_.afterburner_thrust_factor
	};
	result_.left_thrust_force_n =
		calculate_channel_thrust(input.engines.left, conditions);
	result_.right_thrust_force_n =
		calculate_channel_thrust(input.engines.right, conditions);
	if (input.engines.thrust_inhibited ||
		input.diagnostics.thrust_cut_requested)
	{
		result_.left_thrust_force_n = 0.0;
		result_.right_thrust_force_n = 0.0;
	}
	result_.effects.clear();
	result_.effects.push_back(make_local_force_effect(
		{ result_.left_thrust_force_n, 0.0, 0.0 },
		config_.left_engine_position_body_m));
	result_.effects.push_back(make_local_force_effect(
		{ result_.right_thrust_force_n, 0.0, 0.0 },
		config_.right_engine_position_body_m));
	return result_;
}
}
}
