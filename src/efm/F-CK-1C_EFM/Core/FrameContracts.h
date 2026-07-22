#pragma once

#include "../Common/Vec3.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace Core
{
inline constexpr std::size_t kFrameEngineCount = 2;
inline constexpr std::size_t kFrameSuspensionWheelCount = 3;

enum class StartMode
{
	ColdGround,
	HotGround,
	HotAir
};

inline bool is_valid_frame_dt(double dt_s) noexcept
{
	return std::isfinite(dt_s) && dt_s > 0.0;
}

struct Quaternion
{
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	double w = 1.0;
};

struct AtmosphereInput
{
	double altitude_asl = 0.0;
	double temperature = 0.0;
	double speed_of_sound = 0.0;
	double density = 0.0;
	double pressure = 0.0;
	Common::Vec3 wind;
};

struct SurfaceInput
{
	double surface_height = 0.0;
	double surface_height_with_objects = 0.0;
	unsigned surface_type = 0;
	Common::Vec3 normal;
};

struct MassStateInput
{
	double mass = 0.0;
	Common::Vec3 center_of_mass;
	Common::Vec3 moment_of_inertia;
};

struct WorldKinematicsInput
{
	Common::Vec3 acceleration;
	Common::Vec3 velocity;
	Common::Vec3 position;
	Common::Vec3 angular_acceleration;
	Common::Vec3 angular_velocity;
	Quaternion orientation;
};

struct BodyKinematicsInput
{
	Common::Vec3 acceleration;
	Common::Vec3 velocity;
	Common::Vec3 wind_velocity;
	Common::Vec3 angular_acceleration;
	Common::Vec3 angular_velocity;
	double heading = 0.0;
	double pitch = 0.0;
	double roll = 0.0;
	double angle_of_attack = 0.0;
	double angle_of_slide = 0.0;
};

struct SuspensionFeedbackInput
{
	int index = 0;
	Common::Vec3 acting_force;
	Common::Vec3 acting_force_point;
	double integrity_factor = 0.0;
	double compression = 0.0;
	double wheel_speed_x = 0.0;
};

struct AutopilotCommand
{
	bool master = false;
	bool bypass = false;
	bool auto_throttle_engaged = false;
	double pitch_command = 0.0;
	double roll_command = 0.0;
	double throttle_command = 0.0;
};

struct MaxPowerCommand
{
	double ready = 0.0;
	double value = 1.0;
};

struct FrameDataAvailability
{
	bool atmosphere = false;
	bool surface = false;
	bool mass = false;
	bool world_kinematics = false;
	bool body_kinematics = false;
	std::array<bool, kFrameSuspensionWheelCount> suspension = {};
};

struct FrameInput
{
	double dt_s = 0.0;
	FrameDataAvailability availability;
	AtmosphereInput atmosphere;
	SurfaceInput surface;
	MassStateInput mass;
	WorldKinematicsInput world_kinematics;
	BodyKinematicsInput body_kinematics;
	std::array<SuspensionFeedbackInput, kFrameSuspensionWheelCount> suspension = {
		SuspensionFeedbackInput{ 0 },
		SuspensionFeedbackInput{ 1 },
		SuspensionFeedbackInput{ 2 }
	};
	AutopilotCommand autopilot;
	MaxPowerCommand max_power;
};

struct FlightOutput
{
	double altitude_asl_m = 0.0;
	double altitude_agl_m = 0.0;
	double position_world_z_m = 0.0;
	double mach = 0.0;
	double g_load = 0.0;
	double angle_of_attack_deg = 0.0;
	double angle_of_slide_deg = 0.0;
	double atmosphere_temperature_k = 0.0;
};

struct ForceMomentOutput
{
	Common::Vec3 force;
	Common::Vec3 moment;
	Common::Vec3 center_of_mass;
};

struct EngineOutput
{
	bool switch_on = false;
	double throttle_input = 0.0;
	double throttle_output = 0.0;
	double power_readout = 0.0;
	double thrust_force = 0.0;
	double afterburner_ratio = 0.0;
	bool afterburner_lit = false;
	double nozzle_aperture = 0.0;
};

struct ControlOutput
{
	double pitch_input = 0.0;
	double roll_input = 0.0;
	double yaw_input = 0.0;
	double elevator_command = 0.0;
	double aileron_command = 0.0;
	double rudder_command = 0.0;
	double flaps_position = 0.0;
	double slats_position = 0.0;
	double airbrake_position = 0.0;
};

struct LandingGearOutput
{
	double gear_position = 0.0;
	double nose_wheel_steering = 0.0;
	double brake_left = 0.0;
	double brake_right = 0.0;
	std::array<double, kFrameSuspensionWheelCount> wheel_spin = {};
};

struct SuspensionWheelOutput
{
	Common::Vec3 acting_force;
	double compression = 0.0;
	double force_magnitude = 0.0;
	bool weight_on_wheel = false;
};

struct SuspensionOutput
{
	std::array<SuspensionWheelOutput, kFrameSuspensionWheelCount> wheels = {};
	bool any_weight_on_wheels = false;
	bool on_ground = false;
};

struct FuelOutput
{
	double internal_fuel = 0.0;
	double external_fuel = 0.0;
	double total_fuel = 0.0;
};

struct FrameOutput
{
	double simulation_time_s = 0.0;
	FrameDataAvailability availability;
	FlightOutput flight;
	ForceMomentOutput force_moment;
	std::array<EngineOutput, kFrameEngineCount> engines = {};
	ControlOutput controls;
	LandingGearOutput landing_gear;
	SuspensionOutput suspension;
	FuelOutput fuel;
	double shake_amplitude = 0.0;
};
}
