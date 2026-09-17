#pragma once

#include "CockpitContracts.h"
#include "../../Common/Vec3.h"

#include <array>
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

struct Quaternion
{
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	double w = 1.0;
};

struct AtmosphereInput
{
	double altitude_asl_m = 0.0;
	double temperature_k = 0.0;
	double speed_of_sound_mps = 0.0;
	double density_kg_m3 = 0.0;
	double pressure_pa = 0.0;
	Common::Vec3 wind_world_mps;
};

struct SurfaceInput
{
	double surface_height_m = 0.0;
	double surface_height_with_objects_m = 0.0;
	unsigned surface_type = 0;
	Common::Vec3 normal_world_unit;
};

struct MassStateInput
{
	double mass_kg = 0.0;
	Common::Vec3 center_of_mass_body_m;
	Common::Vec3 moment_of_inertia_body_kg_m2;
};

struct WorldKinematicsInput
{
	Common::Vec3 acceleration_world_mps2;
	Common::Vec3 velocity_world_mps;
	Common::Vec3 position_world_m;
	Common::Vec3 angular_acceleration_world_rad_s2;
	Common::Vec3 angular_velocity_world_rad_s;
	Quaternion orientation;
};

// Core local-body convention, preserved from the DCS EFM callback:
// +x/right roll, +z/nose-up pitch, +y/nose-left yaw. Angles are radians,
// angular rates are rad/s, and angular accelerations are rad/s^2. Aviation
// heading is a separate clockwise-positive magnetic observation.
struct BodyAngularKinematicsInput
{
	double roll_acceleration_rad_s2 = 0.0;
	double pitch_acceleration_rad_s2 = 0.0;
	double yaw_acceleration_rad_s2 = 0.0;
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
};

struct BodyKinematicsInput
{
	Common::Vec3 acceleration_body_mps2;
	Common::Vec3 velocity_body_mps;
	Common::Vec3 wind_velocity_body_mps;
	BodyAngularKinematicsInput angular;
	double world_yaw_rad = 0.0;
	double pitch_rad = 0.0;
	double roll_rad = 0.0;
	double angle_of_attack_rad = 0.0;
	double angle_of_slide_rad = 0.0;
};

struct SuspensionFeedbackInput
{
	int index = 0;
	Common::Vec3 acting_force_body_n;
	Common::Vec3 acting_force_point_body_m;
	double integrity_factor_0_1 = 0.0;
	double compression_m = 0.0;
	double wheel_speed_x_mps = 0.0;
};

struct ExternalFuelInput
{
	int station = 0;
	double fuel_kg = 0.0;
	Common::Vec3 position_body_m;
};

struct MassDelta
{
	double mass_kg = 0.0;
	Common::Vec3 position_body_m;
	Common::Vec3 moment_of_inertia_delta_kg_m2;
};

struct MassDeltaResult
{
	bool available = false;
	MassDelta delta;
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
	CockpitObservation cockpit;
};

struct FlightOutput
{
	double altitude_asl_m = 0.0;
	double altitude_agl_m = 0.0;
	double position_world_z_m = 0.0;
	double mach = 0.0;
	double normal_acceleration_g = 0.0;
	double angle_of_attack_deg = 0.0;
	double angle_of_slide_deg = 0.0;
	double atmosphere_temperature_k = 0.0;
	double indicated_airspeed_mps = 0.0;
	double vertical_speed_mps = 0.0;
	double world_yaw_rad = 0.0;
	double pitch_attitude_rad = 0.0;
	double roll_attitude_rad = 0.0;
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
};

struct ForceMomentOutput
{
	Common::Vec3 force_body_n;
	Common::Vec3 moment_body_nm;
	Common::Vec3 center_of_mass_body_m;
};

struct EngineOutput
{
	bool switch_on = false;
	double throttle_input_normalized = 0.0;
	double throttle_output_normalized = 0.0;
	double power_readout_normalized = 0.0;
	double thrust_force_n = 0.0;
	double afterburner_ratio_0_1 = 0.0;
	bool afterburner_lit = false;
	double nozzle_aperture_normalized = 0.0;
};

struct ControlOutput
{
	double pitch_input_normalized = 0.0;
	double roll_input_normalized = 0.0;
	double yaw_input_normalized = 0.0;
	double symmetric_stabilator_position_rad = 0.0;
	double differential_flaperon_position_rad = 0.0;
	double rudder_position_rad = 0.0;
	double flaps_position_normalized = 0.0;
	double slats_position_normalized = 0.0;
	double airbrake_position_normalized = 0.0;
};

struct LandingGearOutput
{
	double gear_position_normalized = 0.0;
	double nose_wheel_steering_normalized = 0.0;
	double brake_left_normalized = 0.0;
	double brake_right_normalized = 0.0;
	std::array<double, kFrameSuspensionWheelCount>
		wheel_spin_phase_0_1 = {};
};

struct SuspensionWheelOutput
{
	Common::Vec3 acting_force_body_n;
	double compression_m = 0.0;
	double force_magnitude_n = 0.0;
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
	double internal_fuel_kg = 0.0;
	double external_fuel_kg = 0.0;
	double total_fuel_kg = 0.0;
	double total_fuel_flow_kg_s = 0.0;
};

struct PropulsionDiagnosticsOutput
{
	bool thrust_cut_requested = false;
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
	PropulsionDiagnosticsOutput propulsion_diagnostics;
	MassDeltaResult mass_effect;
	CockpitSnapshot cockpit;
	double shake_amplitude_normalized = 0.0;
};
}
