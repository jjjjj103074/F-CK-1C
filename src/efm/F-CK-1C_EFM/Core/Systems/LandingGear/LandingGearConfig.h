#pragma once

#include "../../Contracts/FrameContracts.h"

#include <array>

namespace Core
{
namespace Systems
{
struct LandingGearConfig
{
	std::array<double, kFrameSuspensionWheelCount> wheel_radius = {};
};

void validate_landing_gear_config(const LandingGearConfig& config);
const LandingGearConfig& fck1c_landing_gear_config();
}
}
