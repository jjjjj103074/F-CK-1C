#include "FlightStateComputation.h"

#include "Common/Clamp.h"
#include "Common/Units.h"

#include <cmath>

namespace
{
constexpr double kMinimumFlightPathSpeedMps = 1.0e-3;

struct FlightPathAngle
{
	bool available = false;
	double rad = 0.0;
};

FlightPathAngle derive_flight_path_angle(
	double vertical_speed_ft_s,
	double indicated_airspeed_mps)
{
	if (indicated_airspeed_mps <= kMinimumFlightPathSpeedMps) return {};
	const double ratio = Common::limit(
		Common::metres(vertical_speed_ft_s) / indicated_airspeed_mps,
		-1.0, 1.0);
	return { true, std::asin(ratio) };
}
}

namespace Core
{
namespace Systems
{
::Systems::ComputedFlightState compute_flight_state(
	const ::Systems::ManagedFlightControlSignals& signals)
{
	const FlightControlObservation& value = signals.observation;
	const FlightPathAngle flight_path = derive_flight_path_angle(
		value.vertical_speed_ft_s, value.indicated_airspeed_mps);
	return {
		signals.dt_s,
		value.dynamic_pressure_pa,
		value.pressure_altitude_ft,
		value.vertical_speed_ft_s,
		flight_path.available,
		flight_path.rad,
		value.magnetic_heading_available,
		value.magnetic_heading_deg,
		value.roll_rad,
		value.pitch_rad,
		value.roll_rate_rad_s,
		value.pitch_rate_rad_s,
		value.yaw_rate_rad_s,
		value.angle_of_attack_rad,
		value.sideslip_rad,
		value.indicated_airspeed_mps,
		value.mach,
		value.normal_acceleration_g,
		value.pressure_altitude_available
	};
}
}
}
