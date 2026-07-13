#pragma once

#include "../Common/Vec3.h"
#include "../Systems/FuelSystem.h"

namespace DcsBridge
{
struct MassDelta
{
	double mass = 0.0;
	Common::Vec3 position;
	Common::Vec3 moment_of_inertia;
};

struct MassDeltaResult
{
	bool available = false;
	MassDelta delta;
};

inline MassDeltaResult take_mass_delta(Systems::FuelSystem& fuel)
{
	MassDeltaResult result;
	result.available = Systems::change_mass(
		fuel,
		result.delta.mass,
		result.delta.position.x,
		result.delta.position.y,
		result.delta.position.z,
		result.delta.moment_of_inertia.x,
		result.delta.moment_of_inertia.y,
		result.delta.moment_of_inertia.z);
	return result;
}
}
