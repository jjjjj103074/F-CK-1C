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
	std::array<Common::Vec3, kFrameSuspensionWheelCount> gear_points_body_m = {};
	std::array<double, kFrameSuspensionWheelCount> spring_rate_n_m = {};
	std::array<double, kFrameSuspensionWheelCount> damping_n_s_m = {};
	std::array<double, kFrameSuspensionWheelCount> contact_band_m = {};
	Common::Vec3 belly_point_body_m;
	bool enable_fallback_ground_forces = false;
};

void validate_ground_interaction_config(
	const GroundInteractionConfig& config);
const GroundInteractionConfig& fck1c_ground_interaction_config();
}
}
