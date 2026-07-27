#pragma once

#include "../../Common/Clamp.h"
#include "../../Common/Units.h"
#include "../../Common/Vec3.h"
#include "../Contracts/AircraftData.h"
#include "../Contracts/FrameContracts.h"
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
	Common::Vec3 center_of_mass;

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
	const AtmosphereInput& input)
{
	state.wind = input.wind;
	state.atmosphere_density = input.density;
	state.speed_of_sound = input.speed_of_sound;
	state.altitude_asl = input.altitude_asl;
	state.engine_alt_effect = Common::limit(
		std::pow(1.0 - (input.altitude_asl / 30000.0), 0.3), 0.1, 1.0);
	state.atmosphere_temperature = input.temperature;
}

inline void set_surface(
	AircraftState& state,
	const SurfaceInput& input)
{
	state.surface_height_raw = input.surface_height;
	state.surface_height_with_objects = input.surface_height_with_objects;
	state.surface_type_raw = input.surface_type;
	state.altitude_agl = state.altitude_asl - input.surface_height;
}

inline void set_mass_state(
	AircraftState& state,
	const MassStateInput& input)
{
	state.current_mass = input.mass;
	state.center_of_mass = input.center_of_mass;
}

inline void set_world_kinematics(
	AircraftState& state,
	const WorldKinematicsInput& input)
{
	state.velocity_world = input.velocity;
	state.angular_velocity_world = input.angular_velocity;
	state.position_world_z = input.position.z;
}

inline void set_body_kinematics(
	AircraftState& state,
	const BodyKinematicsInput& input)
{
	state.velocity_body = input.velocity;
	state.angular_velocity_body = input.angular_velocity;

	state.aoa = input.angle_of_attack;
	state.alpha = Common::deg(input.angle_of_attack);
	state.aos = input.angle_of_slide;
	state.beta = Common::deg(input.angle_of_slide);
	state.g = (input.acceleration.y / 9.81) + 1.0;

	state.pitch = input.pitch;
	state.roll = input.roll;
	state.heading = input.heading;
	state.roll_rate = input.angular_velocity.x;
	state.yaw_rate = input.angular_velocity.y;
	state.pitch_rate = input.angular_velocity.z;
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

inline AircraftObservation make_aircraft_observation(
	const AircraftState& state)
{
	constexpr double kDynamicPressureCoefficient = 0.5;
	return {
		state.altitude_asl,
		state.altitude_agl,
		state.atmosphere_density,
		state.speed_scalar,
		ground_speed(state),
		state.mach,
		kDynamicPressureCoefficient * state.atmosphere_density *
			state.speed_scalar * state.speed_scalar,
		state.g,
		state.alpha,
		state.beta,
		state.roll,
		state.pitch,
		state.roll_rate,
		state.pitch_rate,
		state.yaw_rate
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
