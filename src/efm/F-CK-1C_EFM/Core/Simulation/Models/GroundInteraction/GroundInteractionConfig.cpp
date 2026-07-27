#include "GroundInteractionConfig.h"

#include "../../../../Common/ConfigValidation.h"

#include <stdexcept>

namespace
{
Core::Simulation::GroundInteractionConfig make_fck1c_config()
{
	Core::Simulation::GroundInteractionConfig config;
	config.gear_points = {
		Common::Vec3(4.12, -1.912, 0.0),
		Common::Vec3(-1.185, -1.913, -0.7905),
		Common::Vec3(-1.185, -1.913, 0.7905)
	};
	config.spring = { 1000000.0, 3200000.0, 3200000.0 };
	config.damping = { 12000.0, 20000.0, 20000.0 };
	config.contact_band = { 0.015, 0.055, 0.055 };
	config.belly_point = { 0.0, -1.05, 0.0 };
	config.enable_fallback_ground_forces = false;
	return config;
}
}

namespace Core
{
namespace Simulation
{
void validate_ground_interaction_config(
	const GroundInteractionConfig& config)
{
	const bool finite_positions = Common::all_finite({
		config.gear_points[0].x, config.gear_points[0].y,
		config.gear_points[0].z, config.gear_points[1].x,
		config.gear_points[1].y, config.gear_points[1].z,
		config.gear_points[2].x, config.gear_points[2].y,
		config.gear_points[2].z, config.belly_point.x,
		config.belly_point.y, config.belly_point.z
	});
	const bool finite_contact = Common::all_finite(config.spring) &&
		Common::all_finite(config.damping) &&
		Common::all_finite(config.contact_band);
	if (!finite_positions || !finite_contact)
	{
		throw std::invalid_argument(
			"GroundInteractionConfig requires finite geometry and contact data.");
	}
	for (std::size_t index = 0;
		index < kFrameSuspensionWheelCount;
		++index)
	{
		if (config.spring[index] <= 0.0 ||
			config.damping[index] < 0.0 ||
			config.contact_band[index] < 0.0)
		{
			throw std::invalid_argument(
				"GroundInteractionConfig requires valid wheel contact data.");
		}
	}
}

const GroundInteractionConfig& fck1c_ground_interaction_config()
{
	static const GroundInteractionConfig config = make_fck1c_config();
	return config;
}
}
}
