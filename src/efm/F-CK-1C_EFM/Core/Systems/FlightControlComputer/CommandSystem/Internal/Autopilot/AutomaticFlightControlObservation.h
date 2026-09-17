#pragma once

namespace Core
{
namespace Systems
{
struct AutomaticFlightControlObservation
{
	double dt_s = 0.0;
	double indicated_airspeed_mps = 0.0;
	double pressure_altitude_ft = 0.0;
	double vertical_speed_ft_s = 0.0;
	double mach = 0.0;
	bool magnetic_heading_available = false;
	double magnetic_heading_deg = 0.0;
	double pitch_rad = 0.0;
	double roll_rad = 0.0;
	double conditioned_pitch_input_normalized = 0.0;
	double conditioned_roll_input_normalized = 0.0;
	bool weight_on_wheels = false;
	bool pressure_altitude_available = false;
};
}
}
