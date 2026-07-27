#include "LandingGearConfig.h"

#include "../../../Common/ConfigValidation.h"

#include <stdexcept>

namespace Core
{
namespace Systems
{
void validate_landing_gear_config(const LandingGearConfig& config)
{
	if (!Common::all_finite(config.wheel_radius))
	{
		throw std::invalid_argument(
			"LandingGearConfig requires finite wheel radii.");
	}
	for (const double radius : config.wheel_radius)
	{
		if (radius <= 0.0)
		{
			throw std::invalid_argument(
				"LandingGearConfig requires positive wheel radii.");
		}
	}
}

const LandingGearConfig& fck1c_landing_gear_config()
{
	static const LandingGearConfig config = {
		{ 0.2286, 0.3048, 0.3048 }
	};
	return config;
}
}
}
