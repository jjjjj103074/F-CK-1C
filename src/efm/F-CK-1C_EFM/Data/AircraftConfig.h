#pragma once

#include "../Common/Vec3.h"
#include "../Systems/AerodynamicsSystem.h"
#include "../Systems/EngineSystem.h"
#include "../Systems/FBWController.h"
#include "../Systems/FuelSystem.h"
#include "../Systems/SuspensionSystem.h"

namespace Data
{
struct AircraftConfig
{
	Systems::AerodynamicsSystemConfig aerodynamics;
	Systems::SuspensionSystemConfig suspension;
	Systems::FBWControllerConfig fbw;
	Systems::EngineSystemConfig engine;
	Systems::FuelSystemConfig fuel;
	Common::Vec3 left_engine_position;
	Common::Vec3 right_engine_position;
};

const AircraftConfig& fck1c_aircraft_config();
}
