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
	double mach = 0.0;
	double dynamic_pressure = 0.0;
	double g_load = 0.0;
	double alpha_deg = 0.0;
	double beta_deg = 0.0;
	double roll = 0.0;
	double pitch = 0.0;
	double roll_rate = 0.0;
	double pitch_rate = 0.0;
	double yaw_rate = 0.0;
};

struct FlightControlDemand
{
	double pitch = 0.0;
	double roll = 0.0;
	double yaw = 0.0;
};

struct PrimaryControlPosition
{
	double elevator = 0.0;
	double aileron = 0.0;
	double rudder = 0.0;
};

struct PilotControlState
{
	double pitch = 0.0;
	double roll = 0.0;
	double yaw = 0.0;
};

struct EngineControlDemand
{
	double left_throttle = 0.0;
	double right_throttle = 0.0;
};

struct SecondaryControlPosition
{
	double flaps = 0.0;
	double slats = 0.0;
	double airbrake = 0.0;
};

struct LandingGearData
{
	double position = 0.0;
	double nose_wheel_steering = 0.0;
	double brake_left = 0.0;
	double brake_right = 0.0;
	std::array<double, kFrameSuspensionWheelCount> wheel_spin = {};
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
};

struct AirframeIntegrity
{
	double left_wing = 1.0;
	double right_wing = 1.0;
	double tail = 1.0;
};

enum class AircraftDataId
{
	FrameInput,
	AircraftObservation,
	PilotControlState,
	FlightControlDemand,
	PrimaryControlPosition,
	EngineControlDemand,
	SecondaryControlPosition,
	LandingGearData,
	EngineData,
	FuelDemand,
	FuelData,
	AirframeIntegrity,
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

inline constexpr AircraftDataKey<PilotControlState> kPilotControlState = {
	AircraftDataId::PilotControlState,
	"pilot_control_state"
};

inline constexpr AircraftDataKey<FlightControlDemand> kFlightControlDemand = {
	AircraftDataId::FlightControlDemand,
	"flight_control_demand"
};

inline constexpr AircraftDataKey<PrimaryControlPosition> kPrimaryControlPosition = {
	AircraftDataId::PrimaryControlPosition,
	"primary_control_position"
};

inline constexpr AircraftDataKey<EngineControlDemand> kEngineControlDemand = {
	AircraftDataId::EngineControlDemand,
	"engine_control_demand"
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
}

using AircraftDataValue = std::variant<
	FrameInput,
	AircraftObservation,
	PilotControlState,
	FlightControlDemand,
	PrimaryControlPosition,
	EngineControlDemand,
	SecondaryControlPosition,
	LandingGearData,
	EngineData,
	FuelDemand,
	FuelData,
	AirframeIntegrity>;

inline constexpr std::size_t kAircraftDataSlotCount =
	static_cast<std::size_t>(AircraftDataId::Count);
}
