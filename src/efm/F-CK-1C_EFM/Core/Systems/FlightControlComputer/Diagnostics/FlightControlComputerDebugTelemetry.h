#pragma once

#include "../../../Contracts/AircraftData.h"
#include "../../../Contracts/Diagnostics/DebugTelemetry.h"

#include <memory>

namespace Core
{
namespace Systems
{
class SystemSetup;

struct FlightControlComputerDebugFrame
{
	DebugSimulationTime time;
	const FlightControlObservation& observation;
	const FlightControlComputerSnapshot& flight_control;
	const AutomaticFlightControlSnapshot& automatic;
};

class FlightControlComputerDebugTelemetry final
{
public:
	FlightControlComputerDebugTelemetry();
	~FlightControlComputerDebugTelemetry();
	FlightControlComputerDebugTelemetry(
		const FlightControlComputerDebugTelemetry&) = delete;
	FlightControlComputerDebugTelemetry& operator=(
		const FlightControlComputerDebugTelemetry&) = delete;

	void declare_channels(SystemSetup& setup);
	void publish_initial(
		const FlightControlComputerSnapshot& flight_control,
		const AutomaticFlightControlSnapshot& automatic) const;
	void publish_step(const FlightControlComputerDebugFrame& frame) const;

private:
	class Implementation;
	std::unique_ptr<Implementation> implementation_;
};
}
}
