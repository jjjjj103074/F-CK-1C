#pragma once

#include "../../Contracts/AircraftData.h"
#include "../../Contracts/Diagnostics/DebugTelemetry.h"

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
	void declare_channels(SystemSetup& setup);
	void publish_initial(
		const FlightControlComputerSnapshot& flight_control,
		const AutomaticFlightControlSnapshot& automatic) const;
	void publish_step(const FlightControlComputerDebugFrame& frame) const;

private:
	void declare_actual_channels(SystemSetup& setup);
	void declare_reference_channels(SystemSetup& setup);
	void declare_afcs_channels(SystemSetup& setup);
	void publish_actual(
		DebugSimulationTime time,
		const FlightControlObservation& value) const;
	void publish_selected(
		DebugSimulationTime time,
		const FlightControlComputerSnapshot& value) const;
	void publish_control(
		DebugSimulationTime time,
		const FlightControlComputerSnapshot& value) const;
	void publish_afcs_state(
		DebugSimulationTime time,
		const AutomaticFlightControlSnapshot& value) const;
	void publish_afcs_reasons(
		DebugSimulationTime time,
		const AutomaticFlightControlSnapshot& value) const;

	DebugTelemetryChannel<double> actual_ias_;
	DebugTelemetryChannel<double> actual_vertical_speed_;
	DebugTelemetryChannel<double> actual_heading_;
	DebugTelemetryChannel<double> actual_pitch_;
	DebugTelemetryChannel<double> actual_roll_;
	DebugTelemetryChannel<double> actual_roll_rate_;
	DebugTelemetryChannel<double> actual_pitch_rate_;
	DebugTelemetryChannel<double> actual_yaw_rate_;
	DebugTelemetryChannel<bool> g_override_available_;
	DebugTelemetryChannel<bool> g_override_active_;
	DebugTelemetryChannel<std::string> selected_longitudinal_source_;
	DebugTelemetryChannel<std::string> selected_lateral_source_;
	DebugTelemetryChannel<std::string> selected_directional_source_;
	DebugTelemetryChannel<std::string> selected_vertical_type_;
	DebugTelemetryChannel<double> selected_normal_acceleration_;
	DebugTelemetryChannel<double> selected_pitch_rate_;
	DebugTelemetryChannel<double> selected_pitch_attitude_;
	DebugTelemetryChannel<double> selected_vertical_speed_;
	DebugTelemetryChannel<double> selected_roll_rate_;
	DebugTelemetryChannel<double> selected_bank_angle_;
	DebugTelemetryChannel<double> selected_sideslip_;
	DebugTelemetryChannel<double> selected_yaw_rate_;
	DebugTelemetryChannel<double> normal_acceleration_reference_;
	DebugTelemetryChannel<double> pitch_rate_reference_;
	DebugTelemetryChannel<double> roll_rate_reference_;
	DebugTelemetryChannel<double> sideslip_reference_;
	DebugTelemetryChannel<double> yaw_rate_reference_;
	DebugTelemetryChannel<double> elevator_command_;
	DebugTelemetryChannel<double> aileron_command_;
	DebugTelemetryChannel<double> rudder_command_;
	DebugTelemetryChannel<std::string> constraint_reason_;
	DebugTelemetryChannel<bool> vertical_constrained_;
	DebugTelemetryChannel<bool> lateral_constrained_;
	DebugTelemetryChannel<bool> ap_master_;
	DebugTelemetryChannel<bool> ap_bypass_;
	DebugTelemetryChannel<bool> auto_throttle_;
	DebugTelemetryChannel<std::string> vertical_mode_;
	DebugTelemetryChannel<std::string> lateral_mode_;
	DebugTelemetryChannel<double> pitch_reference_;
	DebugTelemetryChannel<double> vertical_speed_reference_;
	DebugTelemetryChannel<double> bank_reference_;
	DebugTelemetryChannel<double> throttle_command_;
	DebugTelemetryChannel<double> target_altitude_;
	DebugTelemetryChannel<double> target_heading_;
	DebugTelemetryChannel<double> target_speed_;
	DebugTelemetryChannel<double> target_pitch_;
	DebugTelemetryChannel<double> target_vertical_speed_;
	DebugTelemetryChannel<std::string> longitudinal_authority_;
	DebugTelemetryChannel<std::string> lateral_authority_;
	DebugTelemetryChannel<std::string> ap_constraint_reason_;
	DebugTelemetryChannel<std::string> degradation_reason_;
	DebugTelemetryChannel<std::string> disconnect_reason_;
	DebugTelemetryChannel<bool> vertical_degraded_;
	DebugTelemetryChannel<bool> lateral_degraded_;
	DebugTelemetryChannel<std::string> ap_engage_rejection_;
	DebugTelemetryChannel<std::string> ap_disengage_;
	DebugTelemetryChannel<std::string> at_engage_rejection_;
	DebugTelemetryChannel<std::string> at_disengage_;
};
}
}
