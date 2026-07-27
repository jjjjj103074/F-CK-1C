#pragma once

#include "../../../../Common/Vec3.h"
#include "../../../Contracts/FrameContracts.h"

#include <array>

namespace Core
{
namespace Simulation
{
struct GroundInteractionConfig
{
	std::array<Common::Vec3, kFrameSuspensionWheelCount> gear_points = {};
	std::array<double, kFrameSuspensionWheelCount> spring = {};
	std::array<double, kFrameSuspensionWheelCount> damping = {};
	std::array<double, kFrameSuspensionWheelCount> contact_band = {};
	Common::Vec3 belly_point;
	bool enable_fallback_ground_forces = false;
};

void validate_ground_interaction_config(
	const GroundInteractionConfig& config);
const GroundInteractionConfig& fck1c_ground_interaction_config();
}
}
