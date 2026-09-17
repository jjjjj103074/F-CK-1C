#pragma once

#include "../ModeAndGainScheduling/ModeAndGainScheduling.h"
#include "../Contracts/FlightControlReferences.h"
#include "../../../Contracts/AircraftData.h"

namespace Systems
{
struct ManagedFlightControlSignals
{
	double dt_s = 0.0;
	Core::FlightControlObservation observation;
	double pilot_roll_raw_normalized = 0.0;
	double pilot_pitch_raw_normalized = 0.0;
	double pilot_yaw_raw_normalized = 0.0;
	double pilot_roll_normalized = 0.0;
	double pilot_pitch_normalized = 0.0;
	double pilot_yaw_normalized = 0.0;
	bool roll_pitch_in_deadband = true;
	bool pitch_in_deadband = true;
	double gear_position_normalized = 0.0;
	bool landing_gear_handle_down = false;
	bool weight_on_wheels = false;
	double symmetric_stabilator_position_rad = 0.0;
	Core::FlightControlPositionLimit symmetric_stabilator_position_limit =
		Core::FlightControlPositionLimit::None;
	bool symmetric_stabilator_at_position_limit = false;
	bool symmetric_stabilator_saturated = false;
	double differential_flaperon_position_rad = 0.0;
	double rudder_position_rad = 0.0;
	bool actuator_saturated = false;
};

struct ComputedFlightState
{
	double dt_s = 0.0;
	double dynamic_pressure_pa = 0.0;
	double pressure_altitude_ft = 0.0;
	double vertical_speed_ft_s = 0.0;
	bool flight_path_angle_available = false;
	double flight_path_angle_rad = 0.0;
	bool magnetic_heading_available = false;
	double magnetic_heading_deg = 0.0;
	double roll_attitude_rad = 0.0;
	double pitch_attitude_rad = 0.0;
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
	double angle_of_attack_rad = 0.0;
	double sideslip_rad = 0.0;
	double indicated_airspeed_mps = 0.0;
	double mach = 0.0;
	double normal_acceleration_g = 1.0;
	bool pressure_altitude_available = false;
};

struct ControlSurfaceDemandSet
{
	double symmetric_stabilator_demand_rad = 0.0;
	double differential_flaperon_demand_rad = 0.0;
	double rudder_demand_rad = 0.0;
};

struct FlightControlLawsStatus
{
	bool angle_of_attack_limit_active = false;
	bool load_factor_limit_active = false;
	bool body_rate_limit_active = false;
	bool anti_windup_active = false;
	bool longitudinal_mode_transition_active = false;
	bool electronic_command_saturated = false;
};

struct FlightControlLawsDiagnostics
{
	Core::Systems::LongitudinalCommandMode longitudinal_command_mode =
		Core::Systems::LongitudinalCommandMode::NormalAcceleration;
	double requested_normal_acceleration_reference_g = 1.0;
	double effective_normal_acceleration_reference_g = 1.0;
	double normal_acceleration_command_decrement_g = 0.0;
	double requested_pitch_rate_command_rad_s = 0.0;
	double effective_pitch_rate_command_rad_s = 0.0;
	double angle_of_attack_blend_0_1 = 0.0;
	double angle_of_attack_maximum_normal_acceleration_g = 1.0;
	double normal_acceleration_error_g = 0.0;
	double pitch_rate_error_rad_s = 0.0;
	double pitch_rate_washout_feedback_effort = 0.0;
	double angle_of_attack_stability_feedback_effort = 0.0;
	double longitudinal_integral_effort = 0.0;
	double unsaturated_pitch_effort = 0.0;
	double limited_pitch_effort = 0.0;
};

struct FlightControlLawsResult
{
	ControlSurfaceDemandSet normal_surface_demand;
	ControlSurfaceDemandSet developer_direct_surface_demand;
	FlightControlLawsStatus status;
	FlightControlLawsDiagnostics diagnostics;
};

struct FlightControlLawsInput
{
	ComputedFlightState flight;
	ManagedFlightControlSignals signals;
	Core::Systems::CoordinatedManeuverReference maneuver;
	ActiveFlightControlConfiguration configuration;
};
}
