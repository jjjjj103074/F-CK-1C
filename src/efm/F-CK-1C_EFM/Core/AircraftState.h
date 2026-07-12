#pragma once

#include "../Common/Clamp.h"
#include "../Common/Units.h"
#include "../Common/Vec3.h"
#include <cmath>

namespace Core
{
struct AircraftState
{
	Common::Vec3 wind;
	Common::Vec3 velocity_world;
	Common::Vec3 velocity_body;
	Common::Vec3 angular_velocity_world;
	Common::Vec3 angular_velocity_body;
	Common::Vec3 airspeed;

	double current_mass = 9000.0;
	double atmosphere_density = 101000.0;
	double altitude_asl = 0.0;
	double altitude_agl = 0.0;
	double surface_height_raw = 0.0;
	double surface_height_with_objects = 0.0;
	unsigned surface_type_raw = 0;
	double position_world_z = 0.0;
	double speed_scalar = 0.0;
	double speed_of_sound = 320.0;
	double mach = 0.0;
	double engine_alt_effect = 1.0;

	double aoa = 0.0;
	double alpha = 0.0;
	double aos = 0.0;
	double beta = 0.0;
	double g = 0.0;
	double atmosphere_temperature = 273.0;

	double pitch = 0.0;
	double pitch_rate = 0.0;
	double roll = 0.0;
	double roll_rate = 0.0;
	double heading = 0.0;
	double yaw_rate = 0.0;
};

inline void set_atmosphere(
	AircraftState& state,
	double altitude_asl,
	double temperature,
	double speed_of_sound,
	double density,
	double wind_vx,
	double wind_vy,
	double wind_vz)
{
	state.wind.x = wind_vx;
	state.wind.y = wind_vy;
	state.wind.z = wind_vz;
	state.atmosphere_density = density;
	state.speed_of_sound = speed_of_sound;
	state.altitude_asl = altitude_asl;
	state.engine_alt_effect = Common::limit(std::pow(1.0 - (altitude_asl / 30000.0), 0.3), 0.1, 1.0);
	state.atmosphere_temperature = temperature;
}

inline void set_surface(
	AircraftState& state,
	double surface_height,
	double surface_height_with_objects,
	unsigned surface_type)
{
	state.surface_height_raw = surface_height;
	state.surface_height_with_objects = surface_height_with_objects;
	state.surface_type_raw = surface_type;
	state.altitude_agl = state.altitude_asl - surface_height;
}

inline void set_current_mass(AircraftState& state, double mass)
{
	state.current_mass = mass;
}

inline void set_world_kinematics(
	AircraftState& state,
	double vx,
	double vy,
	double vz,
	double omegax,
	double omegay,
	double omegaz,
	double position_z)
{
	state.velocity_world.x = vx;
	state.velocity_world.y = vy;
	state.velocity_world.z = vz;
	state.angular_velocity_world.x = omegax;
	state.angular_velocity_world.y = omegay;
	state.angular_velocity_world.z = omegaz;
	state.position_world_z = position_z;
}

inline void set_body_kinematics(
	AircraftState& state,
	double vx,
	double vy,
	double vz,
	double omegax,
	double omegay,
	double omegaz,
	double heading,
	double pitch,
	double roll,
	double angle_of_attack,
	double angle_of_slide,
	double acceleration_y)
{
	state.velocity_body.x = vx;
	state.velocity_body.y = vy;
	state.velocity_body.z = vz;
	state.angular_velocity_body.x = omegax;
	state.angular_velocity_body.y = omegay;
	state.angular_velocity_body.z = omegaz;

	state.aoa = angle_of_attack;
	state.alpha = Common::deg(angle_of_attack);
	state.aos = angle_of_slide;
	state.beta = Common::deg(angle_of_slide);
	state.g = (acceleration_y / 9.81) + 1.0;

	state.pitch = pitch;
	state.roll = roll;
	state.heading = heading;
	state.roll_rate = omegax;
	state.yaw_rate = omegay;
	state.pitch_rate = omegaz;
}

inline void update_airspeed(AircraftState& state)
{
	state.airspeed.x = state.velocity_world.x - state.wind.x;
	state.airspeed.y = state.velocity_world.y - state.wind.y;
	state.airspeed.z = state.velocity_world.z - state.wind.z;

	state.speed_scalar = std::sqrt(
		state.airspeed.x * state.airspeed.x +
		state.airspeed.y * state.airspeed.y +
		state.airspeed.z * state.airspeed.z);
	state.mach = state.speed_scalar / state.speed_of_sound;
}

inline double ground_speed(const AircraftState& state)
{
	return std::sqrt(
		state.velocity_world.x * state.velocity_world.x +
		state.velocity_world.z * state.velocity_world.z);
}
}
