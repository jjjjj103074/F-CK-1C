#pragma once

#include "../FlightControlReferences.h"

namespace Systems
{
struct ConditionedFlightControlInput
{
	double dt_s = 0.0;
	double dynamic_pressure_pa = 0.0;
	double alpha_limit_deg = 0.0;
	double altitude_asl_m = 0.0;
	double vertical_speed_mps = 0.0;
	double heading_rad = 0.0;
	double roll_attitude_rad = 0.0;
	double pitch_attitude_rad = 0.0;
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
	double angle_of_attack_deg = 0.0;
	double sideslip_deg = 0.0;
	double indicated_airspeed_mps = 0.0;
	double mach = 0.0;
	double normal_acceleration_g = 1.0;
	double pilot_roll_raw_normalized = 0.0;
	double pilot_pitch_raw_normalized = 0.0;
	double pilot_yaw_raw_normalized = 0.0;
	double pilot_roll_normalized = 0.0;
	double pilot_pitch_normalized = 0.0;
	double pilot_yaw_normalized = 0.0;
	bool roll_pitch_in_deadband = true;
	bool pitch_in_deadband = true;
	double cat_mode_blend = 0.0;
	double gear_position_normalized = 0.0;
	bool weight_on_wheels = false;
	double elevator_position_normalized = 0.0;
	double aileron_position_normalized = 0.0;
	double rudder_position_normalized = 0.0;
	bool actuator_saturated = false;
};

struct NormalizedControlSurfaceDemand
{
	double elevator_command_normalized = 0.0;
	double aileron_command_normalized = 0.0;
	double rudder_command_normalized = 0.0;
};

struct FlightControlLawResult
{
	NormalizedControlSurfaceDemand surface_demand;
};

struct FlightControlLawResetInput
{
	double roll_attitude_rad = 0.0;
	double pitch_attitude_rad = 0.0;
	double angle_of_attack_deg = 0.0;
	double normal_acceleration_g = 1.0;
};

struct FlightControlLawStepInput
{
	ConditionedFlightControlInput flight;
	Core::Systems::CoordinatedManeuverReference maneuver;
};
}
