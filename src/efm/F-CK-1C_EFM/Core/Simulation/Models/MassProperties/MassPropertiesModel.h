#pragma once

#include "../../../Contracts/AircraftData.h"
#include "../../../Contracts/FrameContracts.h"

namespace Core
{
namespace Simulation
{
class MassPropertiesModel final
{
public:
	const MassDeltaResult& step(const FuelData& fuel);

private:
	MassDeltaResult result_;
};
}
}
