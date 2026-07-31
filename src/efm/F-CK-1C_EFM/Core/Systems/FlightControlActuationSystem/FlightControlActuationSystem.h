#pragma once

#include "FlightControlActuationModel.h"
#include "FlightControlActuationSystemConfig.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
class FlightControlActuationSystem final : public System
{
public:
	explicit FlightControlActuationSystem(
		const FlightControlActuationSystemConfig& config);

	void setup(SystemSetup& setup) override;
	void step(
		const SystemStepContext& context,
		const AircraftDataView& aircraft,
		SystemResult& result) override;
	const FlightControlActuatorState& update(
		const FlightControlActuatorCommand& command,
		double dt_s);
	const FlightControlActuatorState& state() const;

private:
	const FlightControlActuationSystemConfig config_;
	::Systems::FlightControlAxisModelState elevator_;
	::Systems::FlightControlAxisModelState aileron_;
	::Systems::FlightControlAxisModelState rudder_;
	FlightControlActuatorState state_;
};

SystemEntry make_flight_control_actuation_system_entry(
	const FlightControlActuationSystemConfig& config);
}
}
