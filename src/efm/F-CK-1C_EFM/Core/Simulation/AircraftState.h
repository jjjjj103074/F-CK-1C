#pragma once

#include "../../Common/Clamp.h"
#include "../../Common/Units.h"
#include "../../Common/Vec3.h"
#include "../Contracts/AircraftData.h"
#include "../Contracts/FrameContracts.h"
#include <cmath>

namespace Core
{
inline constexpr double kStandardGravityMps2 = 9.81;
inline constexpr double kEngineAltitudeReferenceM = 30000.0;
inline constexpr double kEngineAltitudeExponent = 0.3;
inline constexpr double kMinimumEngineAltitudeFactor = 0.1;
inline constexpr double kSeaLevelAirDensityKgM3 = 1.225;
inline constexpr double kDynamicPressureCoefficient = 0.5;

struct AircraftState
{
	Common::Vec3 wind_world_mps;
	Common::Vec3 velocity_world_mps;
	Common::Vec3 velocity_body_mps;
	Common::Vec3 angular_velocity_world_rad_s;
	Common::Vec3 airspeed_world_mps;
	Common::Vec3 center_of_mass_body_m;

	double mass_kg = 9000.0;
	double atmosphere_density_kg_m3 = kSeaLevelAirDensityKgM3;
	double altitude_asl_m = 0.0;
	double altitude_agl_m = 0.0;
	double surface_height_m = 0.0;
	double surface_height_with_objects_m = 0.0;
	unsigned surface_type = 0;
	double position_world_z_m = 0.0;
	double true_airspeed_mps = 0.0;
	double speed_of_sound_mps = 320.0;
	double mach = 0.0;
	double engine_altitude_factor = 1.0;

	double angle_of_attack_rad = 0.0;
	double angle_of_attack_deg = 0.0;
	double angle_of_slide_rad = 0.0;
	double angle_of_slide_deg = 0.0;
	double normal_acceleration_g = 0.0;
	double atmosphere_temperature_k = 273.0;

	double pitch_rad = 0.0;
	double pitch_rate_rad_s = 0.0;
	double roll_rad = 0.0;
	double roll_rate_rad_s = 0.0;
	double world_yaw_rad = 0.0;
	double yaw_rate_rad_s = 0.0;
};

inline void set_atmosphere(
	AircraftState& state,
	const AtmosphereInput& input)
{
	state.wind_world_mps = input.wind_world_mps;
	state.atmosphere_density_kg_m3 = input.density_kg_m3;
	state.speed_of_sound_mps = input.speed_of_sound_mps;
	state.altitude_asl_m = input.altitude_asl_m;
	state.engine_altitude_factor = Common::limit(
		std::pow(
			1.0 - (input.altitude_asl_m / kEngineAltitudeReferenceM),
			kEngineAltitudeExponent),
		kMinimumEngineAltitudeFactor,
		1.0);
	state.atmosphere_temperature_k = input.temperature_k;
}

inline void set_surface(
	AircraftState& state,
	const SurfaceInput& input)
{
	state.surface_height_m = input.surface_height_m;
	state.surface_height_with_objects_m = input.surface_height_with_objects_m;
	state.surface_type = input.surface_type;
	state.altitude_agl_m = state.altitude_asl_m - input.surface_height_m;
}

inline void set_mass_state(
	AircraftState& state,
	const MassStateInput& input)
{
	state.mass_kg = input.mass_kg;
	state.center_of_mass_body_m = input.center_of_mass_body_m;
}

inline void set_world_kinematics(
	AircraftState& state,
	const WorldKinematicsInput& input)
{
	state.velocity_world_mps = input.velocity_world_mps;
	state.angular_velocity_world_rad_s = input.angular_velocity_world_rad_s;
	state.position_world_z_m = input.position_world_m.z;
}

inline void set_body_kinematics(
	AircraftState& state,
	const BodyKinematicsInput& input)
{
	state.velocity_body_mps = input.velocity_body_mps;

	state.angle_of_attack_rad = input.angle_of_attack_rad;
	state.angle_of_attack_deg = Common::deg(input.angle_of_attack_rad);
	state.angle_of_slide_rad = input.angle_of_slide_rad;
	state.angle_of_slide_deg = Common::deg(input.angle_of_slide_rad);
	state.normal_acceleration_g =
		(input.acceleration_body_mps2.y / kStandardGravityMps2) + 1.0;

	state.pitch_rad = input.pitch_rad;
	state.roll_rad = input.roll_rad;
	state.world_yaw_rad = input.world_yaw_rad;
	state.roll_rate_rad_s = input.angular.roll_rate_rad_s;
	state.pitch_rate_rad_s = input.angular.pitch_rate_rad_s;
	state.yaw_rate_rad_s = input.angular.yaw_rate_rad_s;
}

inline void update_airspeed(AircraftState& state)
{
	state.airspeed_world_mps.x =
		state.velocity_world_mps.x - state.wind_world_mps.x;
	state.airspeed_world_mps.y =
		state.velocity_world_mps.y - state.wind_world_mps.y;
	state.airspeed_world_mps.z =
		state.velocity_world_mps.z - state.wind_world_mps.z;

	state.true_airspeed_mps = std::sqrt(
		state.airspeed_world_mps.x * state.airspeed_world_mps.x +
		state.airspeed_world_mps.y * state.airspeed_world_mps.y +
		state.airspeed_world_mps.z * state.airspeed_world_mps.z);
	state.mach = state.true_airspeed_mps / state.speed_of_sound_mps;
}

inline double ground_speed_mps(const AircraftState& state)
{
	return std::sqrt(
		state.velocity_world_mps.x * state.velocity_world_mps.x +
		state.velocity_world_mps.z * state.velocity_world_mps.z);
}

inline double indicated_airspeed_mps(const AircraftState& state)
{
	return state.true_airspeed_mps * std::sqrt(
		state.atmosphere_density_kg_m3 / kSeaLevelAirDensityKgM3);
}

inline AircraftObservation make_aircraft_observation(
	const AircraftState& state)
{
	return {
		state.altitude_asl_m,
		state.altitude_agl_m,
		state.atmosphere_density_kg_m3,
		state.true_airspeed_mps,
		ground_speed_mps(state),
		indicated_airspeed_mps(state),
		state.velocity_world_mps.y,
		state.mach,
		kDynamicPressureCoefficient * state.atmosphere_density_kg_m3 *
			state.true_airspeed_mps * state.true_airspeed_mps,
		state.normal_acceleration_g,
		state.angle_of_attack_deg,
		state.angle_of_slide_deg,
		state.world_yaw_rad,
		state.roll_rad,
		state.pitch_rad,
		state.roll_rate_rad_s,
		state.pitch_rate_rad_s,
		state.yaw_rate_rad_s
	};
}

inline void apply_aircraft_observations(
	AircraftState& state,
	const FrameInput& input)
{
	if (input.availability.atmosphere)
	{
		set_atmosphere(state, input.atmosphere);
	}
	if (input.availability.surface)
	{
		set_surface(state, input.surface);
	}
	if (input.availability.mass)
	{
		set_mass_state(state, input.mass);
	}
	if (input.availability.world_kinematics)
	{
		set_world_kinematics(state, input.world_kinematics);
	}
	if (input.availability.body_kinematics)
	{
		set_body_kinematics(state, input.body_kinematics);
	}
}
}
