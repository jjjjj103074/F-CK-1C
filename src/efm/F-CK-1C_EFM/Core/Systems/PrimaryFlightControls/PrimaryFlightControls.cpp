#include "PrimaryFlightControls.h"

#include "../SystemPipeline.h"

namespace Core
{
namespace Systems
{
void PrimaryFlightControls::setup(SystemSetup& setup)
{
	setup.read(AircraftDataKeys::kFlightControlDemand);
	setup.publish(AircraftDataKeys::kPrimaryControlPosition, position_);
}

void PrimaryFlightControls::step(
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	result.publish(
		AircraftDataKeys::kPrimaryControlPosition,
		step(aircraft.read(AircraftDataKeys::kFlightControlDemand)));
}

const PrimaryControlPosition& PrimaryFlightControls::step(
	const FlightControlDemand& demand)
{
	position_.elevator = demand.pitch;
	position_.aileron = demand.roll;
	position_.rudder = demand.yaw;
	return position_;
}

const PrimaryControlPosition& PrimaryFlightControls::position() const
{
	return position_;
}
}
}
