#pragma once

#include "AircraftDefinition.h"
#include "../Common/Vec3.h"
#include "../Core/Systems/Engine/EngineModel.h"
#include "../Core/Systems/FlightControlComputer/ControlLaws.h"
#include "../Core/Systems/Fuel/FuelModel.h"
#include "../Systems/SuspensionSystem.h"

namespace Data
{
struct AircraftConfig
{
	AerodynamicsDefinition aerodynamics;
	Systems::SuspensionSystemConfig suspension;
	Systems::FBWControllerConfig fbw;
	Systems::EngineSystemConfig engine;
	Systems::FuelSystemConfig fuel;
	FlightEnvelopeConfig flight_envelope;
	Common::Vec3 left_engine_position;
	Common::Vec3 right_engine_position;
};

GroundInteractionDefinition make_ground_interaction_definition(
		const Systems::SuspensionSystemConfig& suspension);
const AircraftConfig& fck1c_aircraft_config();
}
