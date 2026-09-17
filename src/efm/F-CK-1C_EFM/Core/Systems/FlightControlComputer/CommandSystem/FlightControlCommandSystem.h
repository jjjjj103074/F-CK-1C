#pragma once

#include "../ControlLaws/ControlLawSignals.h"
#include "../../../Contracts/CockpitContracts.h"
#include "../../../Contracts/Commands.h"

#include <memory>

namespace Core
{
namespace Systems
{
class SystemSetup;
struct FlightControlComputerConfig;

struct FlightControlCommandSystemInput
{
	::Systems::ComputedFlightState flight;
	::Systems::ManagedFlightControlSignals signals;
	::Systems::ActiveFlightControlConfiguration configuration;
};

struct FlightControlCommandSystemResult
{
	SelectedFlightReference selected;
	GuidanceCoordinationResult coordinated;
	AutomaticFlightGuidanceReference automatic;
};

struct FlightControlCommandMonitorInput
{
	double dt_s = 0.0;
	bool vertical_active = false;
	bool lateral_active = false;
	VerticalGuidanceReferenceType vertical_type =
		VerticalGuidanceReferenceType::None;
	double pitch_tracking_error_rad = 0.0;
	double vertical_speed_tracking_error_ft_s = 0.0;
	double lateral_tracking_error_rad = 0.0;
	GuidanceConstraint constraint;
	bool control_path_saturated = false;
	ConstraintReason hard_protection_reason = ConstraintReason::None;
};

// Public command-and-guidance boundary of the FLCC. Selection, pilot mapping,
// AP implementation and cross-axis coordination stay private to this module.
class FlightControlCommandSystem final
{
public:
	FlightControlCommandSystem(
		const FlightControlComputerConfig& config,
		bool initial_weight_on_wheels);
	~FlightControlCommandSystem();
	FlightControlCommandSystem(const FlightControlCommandSystem&) = delete;
	FlightControlCommandSystem& operator=(
		const FlightControlCommandSystem&) = delete;

	void register_commands(SystemSetup& setup);
	bool handles(CommandId id) const;
	void handle_command(const Command& command);
	const FlightControlCommandSystemResult& update(
		const FlightControlCommandSystemInput& input);
	void observe_control_result(
		const FlightControlCommandMonitorInput& observation);
	const AutomaticFlightControlSnapshot& automatic_snapshot() const;

private:
	class Implementation;
	std::unique_ptr<Implementation> implementation_;
};
}
}
