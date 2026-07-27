#pragma once

#include "../Common/Vec3.h"
#include "../Core/Contracts/FrameContracts.h"

#include <array>
#include <cstddef>
#include <vector>

namespace Data
{
struct AerodynamicsDefinition
{
	double wing_area = 0.0;
	double wingspan = 0.0;
	double length = 0.0;
	double height = 0.0;
	double mach_max = 0.0;

	double cy_zero = 0.0;
	double cz_beta = 0.0;
	double cx_gear = 0.0;
	double cx_airbrake = 0.0;
	double cx_flap = 0.0;
	double cx_lift_k = 0.0;
	double cx_alpha_k = 0.0;
	double cx_elevator_k = 0.0;
	double cy_flap = 0.0;
	double airbrake_pitch_comp_k = 0.0;

	std::vector<double> mach_table;
	std::vector<double> cx_zero_table;
	std::vector<double> cy_alpha_table;
	std::vector<double> roll_rate_max_table;
	std::vector<double> alpha_max_table;
	std::vector<double> cy_max_table;
};

inline bool has_valid_aerodynamics_definition(
	const AerodynamicsDefinition& definition)
{
	const std::size_t table_size = definition.mach_table.size();
	return table_size > 0 &&
		definition.cx_zero_table.size() == table_size &&
		definition.cy_alpha_table.size() == table_size &&
		definition.roll_rate_max_table.size() == table_size &&
		definition.alpha_max_table.size() == table_size &&
		definition.cy_max_table.size() == table_size;
}

struct GroundInteractionDefinition
{
	std::array<Common::Vec3, Core::kFrameSuspensionWheelCount>
		gear_points = {};
	std::array<double, Core::kFrameSuspensionWheelCount>
		wheel_radius = {};
	std::array<double, Core::kFrameSuspensionWheelCount> spring = {};
	std::array<double, Core::kFrameSuspensionWheelCount> damping = {};
	std::array<double, Core::kFrameSuspensionWheelCount>
		contact_band = {};
	Common::Vec3 belly_point;
	bool enable_fallback_ground_forces = false;
};

struct FlightEnvelopeConfig
{
	std::vector<double> mach;
	std::vector<double> alpha_limit_deg;
};
}
