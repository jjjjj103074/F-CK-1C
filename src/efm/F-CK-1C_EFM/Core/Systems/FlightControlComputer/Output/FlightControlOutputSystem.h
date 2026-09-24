#pragma once

#include "FlightControlOutputConfig.h"
#include "../ControlLaws/ControlLawConfig.h"
#include "../ControlLaws/ControlLawSignals.h"

namespace Core
{
namespace Systems
{
enum class ElectronicControlLawSelection
{
	Normal,
	DeveloperDirect
};

struct FlightControlOutputStatus
{
	ElectronicControlLawSelection selection =
		ElectronicControlLawSelection::Normal;
	bool selection_transition_active = false;
	bool command_valid = false;
	bool actuator_feedback_valid = false;
	bool electronic_command_saturated = false;
	bool actuator_saturated = false;
	bool protection_authority_exhausted = false;
	bool actuator_tracking_consistent = false;
	double maximum_tracking_error_rad = 0.0;
};

struct FlightControlOutputInput
{
	::Systems::FlightControlLawsResult laws;
	ElectronicControlLawSelection selection =
		ElectronicControlLawSelection::Normal;
	FlightControlActuatorState actuator;
	double dt_s = 0.0;
};

struct FlightControlOutputResult
{
	FlightControlActuatorCommand actuator_command;
	FlightControlOutputStatus status;
};

class FlightControlOutputSystem
{
public:
	// Electronic selection, bumpless transfer, feedback routing and consistency
	// live here. Servo rate/lag dynamics remain in the 256 Hz actuation system.
	FlightControlOutputSystem(
		const ::Systems::SurfaceCommandMixerConfig& limits,
		const ::Systems::FlightControlOutputConfig& config);
	FlightControlOutputResult update(const FlightControlOutputInput& input);

private:
	::Systems::ControlSurfaceDemandSet selected_demand(
		const FlightControlOutputInput& input) const;
	::Systems::ControlSurfaceDemandSet transition_demand(
		const ::Systems::ControlSurfaceDemandSet& target,
		double dt_s);
	FlightControlActuatorCommand bounded_command(
		const ::Systems::ControlSurfaceDemandSet& demand) const;
	FlightControlOutputStatus make_status(
		const FlightControlOutputInput& input,
		const FlightControlActuatorCommand& command) const;

	const ::Systems::SurfaceCommandMixerConfig limits_;
	const ::Systems::FlightControlOutputConfig config_;
	ElectronicControlLawSelection selection_ =
		ElectronicControlLawSelection::Normal;
	::Systems::ControlSurfaceDemandSet transition_source_;
	::Systems::ControlSurfaceDemandSet current_demand_;
	double transition_0_1_ = 1.0;
	bool initialized_ = false;
};
}
}
