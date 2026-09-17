#include "MassPropertiesModel.h"

namespace
{
// Provisional local-aircraft fuel centroid in metres. These values preserve
// the legacy EFM mass callback until tank-specific mass distribution exists.
constexpr double kFuelMassPositionX = -1.0;
constexpr double kFuelMassPositionY = 1.0;
constexpr double kFuelMassPositionZ = 0.0;
}

namespace Core
{
namespace Simulation
{
const MassDeltaResult& MassPropertiesModel::step(const FuelData& fuel)
{
	result_ = {};
	if (fuel.consumed_mass_kg <= 0.0)
	{
		return result_;
	}
	result_.available = true;
	result_.delta.mass_kg = fuel.consumed_mass_kg;
	result_.delta.position_body_m = {
		kFuelMassPositionX,
		kFuelMassPositionY,
		kFuelMassPositionZ
	};
	return result_;
}
}
}
