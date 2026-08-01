#pragma once

#include <cstdint>
#include <optional>

namespace Core
{
enum class ObservationInvalidReason
{
	None = 0,
	NotProvided = 1,
	ParameterUnavailable = 2,
	InvalidNumeric = 3,
	InvalidRevision = 4,
	StationApiError = 5
};

enum class RadarMode
{
	Off = 0,
	Search = 1,
	LockAttempt = 2,
	SingleTargetTrack = 3
};

enum class AutomaticFlightControlVerticalMode
{
	Off = 0,
	PitchHold = 1,
	VerticalSpeedHold = 2,
	AltitudeHold = 3
};

enum class AutomaticFlightControlLateralMode
{
	Off = 0,
	HeadingHold = 1,
	HeadingSelect = 2,
	NavigationTrack = 3
};

enum class AutomaticFlightControlReason
{
	None = 0,
	Commanded = 1,
	BelowMinimumIndicatedAirspeed = 2,
	WeightOnWheels = 3,
	RollLimit = 4,
	PitchLimit = 5,
	MachLimit = 6
};

enum class FlightControlAuthorityState
{
	Manual,
	Automatic,
	Bypassed,
	StickSteering
};

enum class FlightControlConstraintReason
{
	None,
	GuidanceBankLimit,
	GuidanceLoadFactorLimit,
	LateralConstrainedByVerticalAuthority,
	VerticalReferenceUnmaintainable,
	HardAngleOfAttackLimit,
	HardLoadFactorLimit,
	HardRateLimit,
	ActuatorAuthority
};

enum class FlightControlDegradationReason
{
	None,
	SustainedVerticalTrackingFailure,
	SustainedLateralTrackingFailure,
	SustainedActuatorSaturation
};

enum class FlightControlDisconnectReason
{
	None,
	PilotCommand,
	SafetyCondition,
	InvalidInput
};

enum class MasterArmMode
{
	Off = 0,
	Simulation = 1,
	On = 2
};

enum class FireControlMode
{
	Navigation = 0,
	Dogfight = 1,
	MissileOverride = 2
};

enum class AirToAirSubmode
{
	None = 0,
	Helmet = 1,
	VerticalScan = 2,
	Hud = 3,
	BeyondVisualRange = 4
};

enum class Aim9SeekerState
{
	Off = 0,
	SearchCaged = 1,
	SearchUncaged = 2,
	Track = 3
};

enum class Aim9Tone
{
	Off = 0,
	Seek = 1,
	Acquire = 2,
	Lock = 3
};

enum class ExternalSensorMode
{
	None = 0
};

enum class HmcsDisplayMode
{
	Screen = 0,
	VirtualReality = 1
};

struct ObservationStatus
{
	bool available = false;
	std::uint64_t revision = 0;
	ObservationInvalidReason invalid_reason =
		ObservationInvalidReason::NotProvided;
};

struct RadarObservation
{
	ObservationStatus status;
	RadarMode mode = RadarMode::Off;
	double stt_azimuth_rad = 0.0;
	double stt_elevation_rad = 0.0;
	double stt_range_m = 0.0;
	double stt_azimuth_stabilized_rad = 0.0;
	double stt_elevation_stabilized_rad = 0.0;
	double tdc_azimuth_rad = 0.0;
	double tdc_range_normalized = 0.0;
	double gate_range_normalized = 0.0;
	double contact_01_azimuth_rad = 0.0;
	double contact_01_range_normalized = 0.0;
};

struct IrSeekerObservation
{
	ObservationStatus status;
	bool locked = false;
	double desired_azimuth_rad = 0.0;
	double desired_elevation_rad = 0.0;
	double target_azimuth_rad = 0.0;
	double target_elevation_rad = 0.0;
	double target_range_m = 0.0;
};

struct WeaponStationObservation
{
	ObservationStatus status;
	int aim9_count = 0;
	std::optional<int> selected_station;
	int scanned_station_count = 0;
};

struct CockpitObservation
{
	RadarObservation radar;
	IrSeekerObservation ir_seeker;
	WeaponStationObservation weapon_stations;
};

struct SnapshotStatus
{
	bool available = false;
	std::uint64_t revision = 0;
};

struct FlightControlComputerSnapshot
{
	SnapshotStatus status;
	bool developer_g_limiter_override_available = false;
	bool developer_g_limiter_override_active = false;
	double normal_acceleration_reference_g = 1.0;
	double pitch_rate_feedforward_rad_s = 0.0;
	double roll_rate_reference_rad_s = 0.0;
	double sideslip_reference_rad = 0.0;
	double yaw_rate_feedforward_rad_s = 0.0;
	double elevator_command_normalized = 0.0;
	double aileron_command_normalized = 0.0;
	double rudder_command_normalized = 0.0;
	FlightControlConstraintReason constraint_reason =
		FlightControlConstraintReason::None;
	bool vertical_constrained = false;
	bool lateral_constrained = false;
};

struct AutomaticFlightControlSnapshot
{
	SnapshotStatus status;
	bool master_engaged = false;
	bool bypass_active = false;
	bool auto_throttle_engaged = false;
	AutomaticFlightControlVerticalMode vertical_mode =
		AutomaticFlightControlVerticalMode::Off;
	AutomaticFlightControlLateralMode lateral_mode =
		AutomaticFlightControlLateralMode::Off;
	double pitch_attitude_reference_rad = 0.0;
	double vertical_speed_reference_mps = 0.0;
	double bank_angle_reference_rad = 0.0;
	double throttle_command_normalized = 0.0;
	double target_altitude_m = 0.0;
	double target_heading_rad = 0.0;
	double target_speed_mps = 0.0;
	double target_pitch_rad = 0.0;
	double target_vertical_speed_mps = 0.0;
	FlightControlAuthorityState longitudinal_authority =
		FlightControlAuthorityState::Manual;
	FlightControlAuthorityState lateral_authority =
		FlightControlAuthorityState::Manual;
	AutomaticFlightControlReason autopilot_engage_rejection_reason =
		AutomaticFlightControlReason::None;
	AutomaticFlightControlReason autopilot_disengage_reason =
		AutomaticFlightControlReason::None;
	AutomaticFlightControlReason auto_throttle_engage_rejection_reason =
		AutomaticFlightControlReason::None;
	AutomaticFlightControlReason auto_throttle_disengage_reason =
		AutomaticFlightControlReason::None;
};

struct FireControlSnapshot
{
	SnapshotStatus status;
	MasterArmMode master_arm_mode = MasterArmMode::Off;
	FireControlMode fire_control_mode = FireControlMode::Navigation;
	AirToAirSubmode air_to_air_submode = AirToAirSubmode::None;
	bool dogfight_mode = false;
	bool gun_fire_requested = false;
	bool pickle_requested = false;
};

struct RadarSnapshot
{
	SnapshotStatus status;
	bool power_requested = false;
	RadarMode operating_mode = RadarMode::Off;
};

struct StoresSnapshot
{
	SnapshotStatus status;
	std::optional<int> selected_station;
	int aim9_count = 0;
};

struct Aim9Snapshot
{
	SnapshotStatus status;
	bool active = false;
	bool contact = false;
	bool locked = false;
	bool uncage_held = false;
	Aim9SeekerState seeker_state = Aim9SeekerState::Off;
	Aim9Tone requested_tone = Aim9Tone::Off;
};

struct CountermeasureSnapshot
{
	SnapshotStatus status;
	std::uint64_t flare_release_count = 0;
	std::uint64_t chaff_release_count = 0;
	std::uint64_t combined_release_count = 0;
};

struct ExternalActionIntentSnapshot
{
	SnapshotStatus status;
	ExternalSensorMode sensor_mode = ExternalSensorMode::None;
	std::uint64_t lock_start_count = 0;
	std::uint64_t lock_finish_count = 0;
	std::uint64_t weapon_change_count = 0;
};

struct CombatAvionicsSnapshot
{
	SnapshotStatus status;
	FireControlSnapshot fire_control;
	RadarSnapshot radar;
	StoresSnapshot stores;
	Aim9Snapshot aim9;
	CountermeasureSnapshot countermeasures;
	ExternalActionIntentSnapshot external_actions;
};

struct HmcsDisplaySnapshot
{
	SnapshotStatus status;
	bool enabled = false;
	HmcsDisplayMode display_mode = HmcsDisplayMode::Screen;
	double indicated_airspeed_mps = 0.0;
	double altitude_m = 0.0;
	double heading_rad = 0.0;
};

struct CockpitSnapshot
{
	SnapshotStatus status;
	double simulation_time_s = 0.0;
	FlightControlComputerSnapshot flight_control_computer;
	AutomaticFlightControlSnapshot automatic_flight_control;
	bool propulsion_test_thrust_cut_requested = false;
	CombatAvionicsSnapshot combat_avionics;
	HmcsDisplaySnapshot hmcs;
};
}
