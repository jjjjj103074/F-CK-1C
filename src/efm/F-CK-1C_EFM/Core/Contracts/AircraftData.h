#pragma once

#include "FrameContracts.h"

#include <array>
#include <cstddef>
#include <variant>

namespace Core
{
struct AircraftObservation
{
	double altitude_asl = 0.0;
	double altitude_agl = 0.0;
	double atmosphere_density = 0.0;
	double speed_scalar = 0.0;
	double ground_speed = 0.0;
	double indicated_airspeed_mps = 0.0;
	double vertical_speed_mps = 0.0;
	double mach = 0.0;
	double dynamic_pressure = 0.0;
	double g_load = 0.0;
	double alpha_deg = 0.0;
	double beta_deg = 0.0;
	double heading_rad = 0.0;
	double roll = 0.0;
	double pitch = 0.0;
	double roll_rate = 0.0;
	double pitch_rate = 0.0;
	double yaw_rate = 0.0;
};

struct PilotControlSignal
{
	double pitch_axis_normalized = 0.0;
	double roll_axis_normalized = 0.0;
	double yaw_axis_normalized = 0.0;
	double pitch_trim_normalized = 0.0;
	double roll_trim_normalized = 0.0;
	double yaw_trim_normalized = 0.0;
};

struct FlightControlObservation
{
	double altitude_asl_m = 0.0;
	double indicated_airspeed_mps = 0.0;
	double vertical_speed_mps = 0.0;
	double mach = 0.0;
	double dynamic_pressure_pa = 0.0;
	double normal_acceleration_g = 0.0;
	double alpha_deg = 0.0;
	double beta_deg = 0.0;
	double heading_rad = 0.0;
	double roll_rad = 0.0;
	double pitch_rad = 0.0;
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
};

inline FlightControlObservation make_flight_control_observation(
	const AircraftObservation& source)
{
	return {
		source.altitude_asl,
		source.indicated_airspeed_mps,
		source.vertical_speed_mps,
		source.mach,
		source.dynamic_pressure,
		source.g_load,
		source.alpha_deg,
		source.beta_deg,
		source.heading_rad,
		source.roll,
		source.pitch,
		source.roll_rate,
		source.pitch_rate,
		source.yaw_rate
	};
}

struct FlightControlActuatorCommand
{
	double elevator_normalized = 0.0;
	double aileron_normalized = 0.0;
	double rudder_normalized = 0.0;
};

struct FlightControlSurfaceState
{
	double position_rad = 0.0;
	double rate_rad_s = 0.0;
	double normalized_position = 0.0;
	bool saturated = false;
};

struct FlightControlActuatorState
{
	FlightControlSurfaceState elevator;
	FlightControlSurfaceState aileron;
	FlightControlSurfaceState rudder;
	bool any_saturated = false;
};

struct ThrottleLeverSignal
{
	double left_normalized = 0.0;
	double right_normalized = 0.0;
};

struct EngineThrottleCommand
{
	double left_normalized = 0.0;
	double right_normalized = 0.0;
};

struct SecondaryControlPosition
{
	double flaps = 0.0;
	double slats = 0.0;
	double airbrake = 0.0;
};

struct SuspensionWheelData
{
	Common::Vec3 acting_force;
	double compression = 0.0;
	double force_magnitude = 0.0;
	bool weight_on_wheel = false;
};

struct LandingGearData
{
	double position = 0.0;
	double nose_wheel_steering = 0.0;
	double brake_left = 0.0;
	double brake_right = 0.0;
	std::array<double, kFrameSuspensionWheelCount> wheel_radius = {};
	std::array<double, kFrameSuspensionWheelCount> wheel_spin = {};
	std::array<SuspensionWheelData, kFrameSuspensionWheelCount>
		suspension = {};
	bool any_weight_on_wheels = false;
	bool on_ground = false;
};

struct EngineChannelData
{
	bool switch_on = false;
	double throttle_input = 0.0;
	double throttle_output = 0.0;
	double power_readout = 0.0;
	double afterburner_ratio = 0.0;
	bool afterburner_lit = false;
	double nozzle_aperture = 0.0;
	double condition = 1.0;
};

struct EngineData
{
	EngineChannelData left;
	EngineChannelData right;
	bool thrust_inhibited = false;
};

struct FuelDemand
{
	double flow_rate_kg_s = 0.0;
};

struct FuelData
{
	double internal_fuel = 0.0;
	double external_fuel = 0.0;
	double total_fuel_flow = 0.0;
	double consumed_mass = 0.0;
};

struct AirframeIntegrity
{
	double left_wing = 1.0;
	double right_wing = 1.0;
	double tail = 1.0;
};

struct PropulsionTestIntent
{
	bool thrust_cut_requested = false;
};

enum class AircraftDataId
{
	FrameInput,
	AircraftObservation,
	FlightControlObservation,
	PilotControlSignal,
	FlightControlActuatorCommand,
	FlightControlActuatorState,
	ThrottleLeverSignal,
	EngineThrottleCommand,
	SecondaryControlPosition,
	LandingGearData,
	EngineData,
	FuelDemand,
	FuelData,
	AirframeIntegrity,
	FlightControlComputerSnapshot,
	AutomaticFlightControlSnapshot,
	PropulsionTestIntent,
	Count
};

template <typename T>
struct AircraftDataKey
{
	AircraftDataId id;
	const char* name;
};

namespace AircraftDataKeys
{
inline constexpr AircraftDataKey<FrameInput> kFrameInput = {
	AircraftDataId::FrameInput,
	"frame_input"
};

inline constexpr AircraftDataKey<AircraftObservation> kAircraftObservation = {
	AircraftDataId::AircraftObservation,
	"aircraft_observation"
};

inline constexpr AircraftDataKey<FlightControlObservation>
	kFlightControlObservation = {
		AircraftDataId::FlightControlObservation,
		"flight_control_observation"
};

inline constexpr AircraftDataKey<PilotControlSignal> kPilotControlSignal = {
	AircraftDataId::PilotControlSignal,
	"pilot_control_signal"
};

inline constexpr AircraftDataKey<FlightControlActuatorCommand>
	kFlightControlActuatorCommand = {
		AircraftDataId::FlightControlActuatorCommand,
		"flight_control_actuator_command"
};

inline constexpr AircraftDataKey<FlightControlActuatorState>
	kFlightControlActuatorState = {
		AircraftDataId::FlightControlActuatorState,
		"flight_control_actuator_state"
};

inline constexpr AircraftDataKey<ThrottleLeverSignal> kThrottleLeverSignal = {
	AircraftDataId::ThrottleLeverSignal,
	"throttle_lever_signal"
};

inline constexpr AircraftDataKey<EngineThrottleCommand>
	kEngineThrottleCommand = {
		AircraftDataId::EngineThrottleCommand,
		"engine_throttle_command"
};

inline constexpr AircraftDataKey<SecondaryControlPosition>
	kSecondaryControlPosition = {
		AircraftDataId::SecondaryControlPosition,
		"secondary_control_position"
	};

inline constexpr AircraftDataKey<LandingGearData> kLandingGearData = {
	AircraftDataId::LandingGearData,
	"landing_gear_data"
};

inline constexpr AircraftDataKey<EngineData> kEngineData = {
	AircraftDataId::EngineData,
	"engine_data"
};

inline constexpr AircraftDataKey<FuelDemand> kFuelDemand = {
	AircraftDataId::FuelDemand,
	"fuel_demand"
};

inline constexpr AircraftDataKey<FuelData> kFuelData = {
	AircraftDataId::FuelData,
	"fuel_data"
};

inline constexpr AircraftDataKey<AirframeIntegrity> kAirframeIntegrity = {
	AircraftDataId::AirframeIntegrity,
	"airframe_integrity"
};

inline constexpr AircraftDataKey<FlightControlComputerSnapshot>
	kFlightControlComputerSnapshot = {
		AircraftDataId::FlightControlComputerSnapshot,
		"flight_control_computer_snapshot"
};

inline constexpr AircraftDataKey<AutomaticFlightControlSnapshot>
	kAutomaticFlightControlSnapshot = {
		AircraftDataId::AutomaticFlightControlSnapshot,
		"automatic_flight_control_snapshot"
	};

inline constexpr AircraftDataKey<PropulsionTestIntent> kPropulsionTestIntent = {
	AircraftDataId::PropulsionTestIntent,
	"propulsion_test_intent"
};
}

using AircraftDataValue = std::variant<
	FrameInput,
	AircraftObservation,
	FlightControlObservation,
	PilotControlSignal,
	FlightControlActuatorCommand,
	FlightControlActuatorState,
	ThrottleLeverSignal,
	EngineThrottleCommand,
	SecondaryControlPosition,
	LandingGearData,
	EngineData,
	FuelDemand,
	FuelData,
	AirframeIntegrity,
	FlightControlComputerSnapshot,
	AutomaticFlightControlSnapshot,
	PropulsionTestIntent>;

inline constexpr std::size_t kAircraftDataSlotCount =
	static_cast<std::size_t>(AircraftDataId::Count);
}
