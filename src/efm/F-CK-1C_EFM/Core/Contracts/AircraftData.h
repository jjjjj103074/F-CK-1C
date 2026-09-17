#pragma once

#include "../../Common/Units.h"
#include "FrameContracts.h"

#include <array>
#include <cstddef>
#include <variant>

namespace Core
{
// Retained SystemPipeline flight-state contract. Field suffixes are the unit
// authority. Local-body signs match DCS: right roll and nose-up pitch are
// positive; nose-left yaw is positive. Magnetic heading is independent and
// increases clockwise toward the right.
struct AircraftObservation
{
	double altitude_asl_m = 0.0;
	double altitude_agl_m = 0.0;
	double atmosphere_density_kg_m3 = 0.0;
	double true_airspeed_mps = 0.0;
	double ground_speed_mps = 0.0;
	double indicated_airspeed_mps = 0.0;
	double vertical_speed_mps = 0.0;
	double mach = 0.0;
	double dynamic_pressure_pa = 0.0;
	double normal_acceleration_g = 0.0;
	double alpha_deg = 0.0;
	double beta_deg = 0.0;
	double world_yaw_rad = 0.0;
	double roll_rad = 0.0;
	double pitch_rad = 0.0;
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
};

struct PilotControlSignal
{
	// Normalized Core control convention: pull/right-roll/left-yaw are positive.
	// DCS raw axis signs are adapted before this contract is published.
	double pitch_axis_normalized = 0.0;
	double roll_axis_normalized = 0.0;
	double yaw_axis_normalized = 0.0;
	double pitch_trim_normalized = 0.0;
	double roll_trim_normalized = 0.0;
	double yaw_trim_normalized = 0.0;
};

struct FlightControlObservation
{
	double pressure_altitude_ft = 0.0;
	double indicated_airspeed_mps = 0.0;
	double vertical_speed_ft_s = 0.0;
	double mach = 0.0;
	double dynamic_pressure_pa = 0.0;
	double normal_acceleration_g = 0.0;
	double angle_of_attack_rad = 0.0;
	double sideslip_rad = 0.0;
	bool magnetic_heading_available = false;
	double magnetic_heading_deg = 0.0;
	double roll_rad = 0.0;
	double pitch_rad = 0.0;
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
	bool pressure_altitude_available = false;
};

inline FlightControlObservation make_flight_control_observation(
	const AircraftObservation& source,
	const PressureAltitudeObservation& pressure_altitude,
	const MagneticHeadingObservation& heading)
{
	return {
		pressure_altitude.pressure_altitude_ft,
		source.indicated_airspeed_mps,
		Common::feet(source.vertical_speed_mps),
		source.mach,
		source.dynamic_pressure_pa,
		source.normal_acceleration_g,
		Common::rad(source.alpha_deg),
		Common::rad(source.beta_deg),
		heading.status.available,
		heading.magnetic_heading_deg,
		source.roll_rad,
		source.pitch_rad,
		source.roll_rate_rad_s,
		source.pitch_rate_rad_s,
		source.yaw_rate_rad_s,
		pressure_altitude.status.available
	};
}

struct FlightControlActuatorCommand
{
	double symmetric_stabilator_demand_rad = 0.0;
	double differential_flaperon_demand_rad = 0.0;
	double rudder_demand_rad = 0.0;
};

enum class FlightControlPositionLimit
{
	None,
	Negative,
	Positive
};

struct FlightControlSurfaceState
{
	double position_rad = 0.0;
	double rate_rad_s = 0.0;
	FlightControlPositionLimit position_limit =
		FlightControlPositionLimit::None;
	bool at_position_limit = false;
	bool rate_limited = false;
	bool saturated = false;
};

struct FlightControlActuatorState
{
	FlightControlSurfaceState symmetric_stabilator;
	FlightControlSurfaceState differential_flaperon;
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
	double flaps_position_normalized = 0.0;
	double slats_position_normalized = 0.0;
	double airbrake_position_normalized = 0.0;
};

struct SuspensionWheelData
{
	Common::Vec3 acting_force_body_n;
	double compression_m = 0.0;
	double force_magnitude_n = 0.0;
	bool weight_on_wheel = false;
};

struct LandingGearData
{
	double position_normalized = 0.0;
	double nose_wheel_steering_normalized = 0.0;
	double brake_left_normalized = 0.0;
	double brake_right_normalized = 0.0;
	std::array<double, kFrameSuspensionWheelCount> wheel_radius_m = {};
	std::array<double, kFrameSuspensionWheelCount>
		wheel_spin_phase_0_1 = {};
	std::array<SuspensionWheelData, kFrameSuspensionWheelCount>
		suspension = {};
	bool any_weight_on_wheels = false;
	bool on_ground = false;
	// The longitudinal flight-control law is selected from the cockpit gear
	// handle signal, not from delayed physical gear travel.
	bool handle_down = false;
};

struct EngineChannelData
{
	bool switch_on = false;
	double throttle_input_normalized = 0.0;
	double throttle_output_normalized = 0.0;
	double power_readout_normalized = 0.0;
	double afterburner_ratio_0_1 = 0.0;
	bool afterburner_lit = false;
	double nozzle_aperture_normalized = 0.0;
	double condition_0_1 = 1.0;
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
	double internal_fuel_kg = 0.0;
	double external_fuel_kg = 0.0;
	double total_fuel_flow_kg_s = 0.0;
	double consumed_mass_kg = 0.0;
};

struct AirframeIntegrity
{
	double left_wing_0_1 = 1.0;
	double right_wing_0_1 = 1.0;
	double tail_0_1 = 1.0;
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
