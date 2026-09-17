#include "FlightControlComputerDebugTelemetry.h"

#include "Common/Units.h"
#include "../../SystemPipeline.h"

#include <stdexcept>
#include <string>

namespace
{
struct ChannelDefinition
{
	const char* name;
	const char* label;
	const char* unit;
	const char* description;
};

Core::DebugTelemetryChannelDescriptor descriptor(
	const ChannelDefinition& definition)
{
	return { definition.name, definition.label,
		Core::DebugTelemetryValueType::Double,
		definition.unit, definition.description };
}

template <typename T>
Core::DebugTelemetryChannel<T> declare(
	Core::Systems::SystemSetup& setup,
	const ChannelDefinition& definition)
{
	return setup.declare_debug_channel<T>(descriptor(definition));
}

const char* reference_source(Core::FlightControlReferenceSource value)
{
	switch (value)
	{
	case Core::FlightControlReferenceSource::Manual: return "Manual";
	case Core::FlightControlReferenceSource::Automatic: return "Automatic";
	}
	throw std::logic_error("Unknown flight-control reference source.");
}

const char* vertical_reference_type(
	Core::FlightControlVerticalReferenceType value)
{
	switch (value)
	{
	case Core::FlightControlVerticalReferenceType::None: return "None";
	case Core::FlightControlVerticalReferenceType::PitchAttitude:
		return "Pitch Attitude";
	case Core::FlightControlVerticalReferenceType::VerticalSpeed:
		return "Vertical Speed";
	}
	throw std::logic_error("Unknown vertical reference type.");
}

const char* longitudinal_command_mode(
	Core::FlightControlLongitudinalCommandMode value)
{
	switch (value)
	{
	case Core::FlightControlLongitudinalCommandMode::NormalAcceleration:
		return "Normal Acceleration";
	case Core::FlightControlLongitudinalCommandMode::PitchRate:
		return "Pitch Rate";
	}
	throw std::logic_error("Unknown longitudinal command mode.");
}

const char* vertical_mode(Core::AutomaticFlightControlVerticalMode value)
{
	switch (value)
	{
	case Core::AutomaticFlightControlVerticalMode::Off: return "Off";
	case Core::AutomaticFlightControlVerticalMode::PitchAttitudeHold:
		return "Pitch Attitude Hold";
	case Core::AutomaticFlightControlVerticalMode::AltitudeHold:
		return "Altitude Hold";
	}
	throw std::logic_error("Unknown AFCS vertical mode.");
}

const char* lateral_mode(Core::AutomaticFlightControlLateralMode value)
{
	switch (value)
	{
	case Core::AutomaticFlightControlLateralMode::Off: return "Off";
	case Core::AutomaticFlightControlLateralMode::RollAttitudeHold:
		return "Roll Attitude Hold";
	case Core::AutomaticFlightControlLateralMode::HeadingSelect:
		return "Heading Select";
	}
	throw std::logic_error("Unknown AFCS lateral mode.");
}

const char* authority(Core::FlightControlAuthorityState value)
{
	switch (value)
	{
	case Core::FlightControlAuthorityState::Manual: return "Manual";
	case Core::FlightControlAuthorityState::Automatic: return "Automatic";
	case Core::FlightControlAuthorityState::Bypassed: return "Bypassed";
	case Core::FlightControlAuthorityState::StickSteering:
		return "Stick Steering";
	}
	throw std::logic_error("Unknown flight-control authority state.");
}

const char* constraint_reason(Core::FlightControlConstraintReason value)
{
	switch (value)
	{
	case Core::FlightControlConstraintReason::None: return "None";
	case Core::FlightControlConstraintReason::GuidanceBankLimit:
		return "Guidance Bank Limit";
	case Core::FlightControlConstraintReason::GuidanceLoadFactorLimit:
		return "Guidance Load Factor Limit";
	case Core::FlightControlConstraintReason::LateralConstrainedByVerticalAuthority:
		return "Lateral Constrained By Vertical Authority";
	case Core::FlightControlConstraintReason::VerticalReferenceUnmaintainable:
		return "Vertical Reference Unmaintainable";
	case Core::FlightControlConstraintReason::HardAngleOfAttackLimit:
		return "Hard Angle Of Attack Limit";
	case Core::FlightControlConstraintReason::HardLoadFactorLimit:
		return "Hard Load Factor Limit";
	case Core::FlightControlConstraintReason::HardRateLimit:
		return "Hard Rate Limit";
	case Core::FlightControlConstraintReason::ActuatorAuthority:
		return "Actuator Authority";
	}
	throw std::logic_error("Unknown flight-control constraint reason.");
}

const char* degradation_reason(Core::FlightControlDegradationReason value)
{
	switch (value)
	{
	case Core::FlightControlDegradationReason::None: return "None";
	case Core::FlightControlDegradationReason::SustainedVerticalTrackingFailure:
		return "Sustained Vertical Tracking Failure";
	case Core::FlightControlDegradationReason::SustainedLateralTrackingFailure:
		return "Sustained Lateral Tracking Failure";
	case Core::FlightControlDegradationReason::SustainedActuatorSaturation:
		return "Sustained Actuator Saturation";
	case Core::FlightControlDegradationReason::MagneticHeadingUnavailable:
		return "Magnetic Heading Unavailable";
	case Core::FlightControlDegradationReason::PressureAltitudeUnavailable:
		return "Pressure Altitude Unavailable";
	}
	throw std::logic_error("Unknown flight-control degradation reason.");
}

const char* disconnect_reason(Core::FlightControlDisconnectReason value)
{
	switch (value)
	{
	case Core::FlightControlDisconnectReason::None: return "None";
	case Core::FlightControlDisconnectReason::PilotCommand:
		return "Pilot Command";
	case Core::FlightControlDisconnectReason::SafetyCondition:
		return "Safety Condition";
	case Core::FlightControlDisconnectReason::InvalidInput:
		return "Invalid Input";
	}
	throw std::logic_error("Unknown flight-control disconnect reason.");
}

const char* automatic_reason(Core::AutomaticFlightControlReason value)
{
	switch (value)
	{
	case Core::AutomaticFlightControlReason::None: return "None";
	case Core::AutomaticFlightControlReason::Commanded: return "Commanded";
	case Core::AutomaticFlightControlReason::BelowMinimumIndicatedAirspeed:
		return "Below Minimum Indicated Airspeed";
	case Core::AutomaticFlightControlReason::WeightOnWheels:
		return "Weight On Wheels";
	case Core::AutomaticFlightControlReason::RollLimit: return "Roll Limit";
	case Core::AutomaticFlightControlReason::PitchLimit: return "Pitch Limit";
	case Core::AutomaticFlightControlReason::MachLimit: return "Mach Limit";
	case Core::AutomaticFlightControlReason::MagneticHeadingUnavailable:
		return "Magnetic Heading Unavailable";
	case Core::AutomaticFlightControlReason::PressureAltitudeUnavailable:
		return "Pressure Altitude Unavailable";
	}
	throw std::logic_error("Unknown automatic-flight-control reason.");
}

#define ACTUAL_CHANNELS(X) \
	X(double, ias, "flight_actual_indicated_airspeed_mps", "Actual IAS", "m/s", "Conditioned FCC input observation.", value.indicated_airspeed_mps) \
	X(double, normal_acceleration, "flight_actual_normal_acceleration_g", "Actual Nz", "g", "Raw normal acceleration observed at the FCC boundary.", value.normal_acceleration_g) \
	X(double, angle_of_attack, "flight_actual_angle_of_attack_deg", "Actual AOA", "deg", "Angle of attack observed by the FCC at this scheduled tick.", Common::deg(value.angle_of_attack_rad)) \
	X(bool, pressure_altitude_available, "flight_actual_pressure_altitude_available", "Pressure Altitude Available", "", "Barometric pressure-altitude observation is available to the FCC.", value.pressure_altitude_available) \
	X(double, pressure_altitude, "flight_actual_pressure_altitude_ft", "Actual Pressure Altitude", "ft", "Conditioned barometric pressure altitude observed by the FCC.", value.pressure_altitude_ft) \
	X(double, vertical_speed, "flight_actual_vertical_speed_ft_s", "Actual Vertical Speed", "ft/s", "Aircraft-domain FCC input observation at this scheduled tick.", value.vertical_speed_ft_s) \
	X(bool, magnetic_heading_available, "flight_actual_magnetic_heading_available", "Magnetic Heading Available", "", "External magnetic-heading observation is available to the FCC.", value.magnetic_heading_available) \
	X(double, magnetic_heading, "flight_actual_magnetic_heading_deg", "Actual Heading", "deg", "Magnetic heading observed by the FCC at this scheduled tick.", value.magnetic_heading_deg) \
	X(double, pitch, "flight_actual_pitch_attitude_rad", "Actual Pitch", "rad", "FCC input observation at this scheduled tick.", value.pitch_rad) \
	X(double, roll, "flight_actual_roll_attitude_rad", "Actual Roll", "rad", "FCC input observation at this scheduled tick.", value.roll_rad) \
	X(double, roll_rate, "flight_actual_roll_rate_rad_s", "Actual Roll Rate", "rad/s", "FCC input observation at this scheduled tick.", value.roll_rate_rad_s) \
	X(double, pitch_rate, "flight_actual_pitch_rate_rad_s", "Actual Pitch Rate", "rad/s", "FCC input observation at this scheduled tick.", value.pitch_rate_rad_s) \
	X(double, yaw_rate, "flight_actual_yaw_rate_rad_s", "Actual Yaw Rate", "rad/s", "FCC input observation at this scheduled tick.", value.yaw_rate_rad_s)

#define FLIGHT_CONTROL_CHANNELS(X) \
	X(std::int64_t, tick, "flight_control_tick", "FCC Tick", "tick", "Monotonic 64 Hz FCC execution tick.", static_cast<std::int64_t>(value.flight_control_tick)) \
	X(std::int64_t, pilot_shaping_updates, "flight_control_pilot_shaping_update_count", "Pilot Shaping Updates", "update", "Number of completed 32 Hz pilot-input shaping updates.", static_cast<std::int64_t>(value.pilot_shaping_update_count)) \
	X(std::int64_t, pilot_shaping_last_tick, "flight_control_pilot_shaping_last_update_tick", "Pilot Shaping Last Tick", "tick", "Last 64 Hz tick that completed the 32 Hz lateral/directional shaping update.", static_cast<std::int64_t>(value.pilot_shaping_last_update_tick)) \
	X(std::int64_t, pilot_shaping_age, "flight_control_pilot_shaping_age_ticks", "Pilot Shaping Age", "tick", "64 Hz ticks since the last pilot-input shaping update.", static_cast<std::int64_t>(value.pilot_shaping_age_ticks)) \
	X(std::int64_t, gain_schedule_updates, "flight_control_gain_schedule_update_count", "Gain Schedule Updates", "update", "Number of completed 4 Hz slow gain-schedule updates.", static_cast<std::int64_t>(value.gain_schedule_update_count)) \
	X(std::int64_t, gain_schedule_last_tick, "flight_control_gain_schedule_last_update_tick", "Gain Schedule Last Tick", "tick", "Last 64 Hz tick that completed the 4 Hz slow gain update.", static_cast<std::int64_t>(value.gain_schedule_last_update_tick)) \
	X(std::int64_t, gain_schedule_age, "flight_control_gain_schedule_age_ticks", "Gain Schedule Age", "tick", "64 Hz ticks since the last slow gain-schedule update.", static_cast<std::int64_t>(value.gain_schedule_age_ticks)) \
	X(double, conditioned_pitch, "flight_control_conditioned_pitch_input_normalized", "Conditioned Pitch Input", "normalized", "Current 64 Hz longitudinal pilot shaping output.", value.conditioned_pitch_input_normalized) \
	X(double, conditioned_roll, "flight_control_conditioned_roll_input_normalized", "Conditioned Roll Input", "normalized", "Held 32 Hz lateral pilot shaping output.", value.conditioned_roll_input_normalized) \
	X(double, conditioned_yaw, "flight_control_conditioned_yaw_input_normalized", "Conditioned Yaw Input", "normalized", "Held 32 Hz directional pilot shaping output.", value.conditioned_yaw_input_normalized) \
	X(double, filtered_normal_acceleration, "flight_control_filtered_normal_acceleration_g", "Filtered Nz", "g", "Normal acceleration after FCC input-signal filtering and used by the control law.", value.filtered_normal_acceleration_g) \
	X(double, active_command_gain, "flight_control_active_command_gain", "Active Command Gain", "ratio", "Held 4 Hz command gain.", value.active_command_gain) \
	X(double, active_damping_gain, "flight_control_active_damping_gain", "Active Damping Gain", "ratio", "Held 4 Hz damping gain.", value.active_damping_gain) \
	X(double, active_limiter_gain, "flight_control_active_limiter_gain", "Active Limiter Gain", "ratio", "Held 4 Hz limiter gain.", value.active_limiter_gain) \
	X(bool, cat3_selected, "flight_control_cat3_selected", "CAT III Selected", "", "Selected stores-dependent control-law category.", value.cat3_selected) \
	X(double, stores_transition, "flight_control_stores_transition_0_1", "Stores Transition", "0..1", "Current continuous CAT I to CAT III gain-schedule transition.", value.stores_transition_0_1) \
	X(bool, developer_direct_law, "flight_control_developer_direct_law_active", "Developer Direct Law", "", "Developer-only direct control-law state.", value.developer_direct_control_law_active) \
	X(bool, angle_of_attack_limit, "flight_control_angle_of_attack_limit_active", "AOA Limit Active", "", "Normal-law angle-of-attack protection is modifying the command.", value.angle_of_attack_limit_active) \
	X(std::string, longitudinal_command_mode, "flight_control_longitudinal_command_mode", "Longitudinal Command Mode", "", "The single active longitudinal objective.", std::string(::longitudinal_command_mode(value.longitudinal_command_mode))) \
	X(bool, longitudinal_transition, "flight_control_longitudinal_mode_transition_active", "Longitudinal Mode Transition", "", "The longitudinal state owner initialized a bumpless mode transfer this tick.", value.longitudinal_mode_transition_active) \
	X(double, requested_normal_acceleration, "flight_control_requested_normal_acceleration_reference_g", "Requested Nz", "g", "Resolved Nz command before load-factor and alpha protection.", value.requested_normal_acceleration_reference_g) \
	X(double, effective_normal_acceleration, "flight_control_effective_normal_acceleration_reference_g", "Effective Nz", "g", "Normal-acceleration command after load-factor and alpha protection.", value.effective_normal_acceleration_reference_g) \
	X(double, normal_acceleration_decrement, "flight_control_normal_acceleration_command_decrement_g", "AOA Nz Decrement", "g", "Nz removed by the alpha command limiter.", value.normal_acceleration_command_decrement_g) \
	X(double, requested_pitch_rate, "flight_control_requested_pitch_rate_command_rad_s", "Requested Pitch Rate", "rad/s", "Resolved pitch-rate command before protection.", value.requested_pitch_rate_command_rad_s) \
	X(double, effective_pitch_rate, "flight_control_effective_pitch_rate_command_rad_s", "Effective Pitch Rate", "rad/s", "Pitch-rate command after rate and alpha protection.", value.effective_pitch_rate_command_rad_s) \
	X(double, angle_of_attack_blend, "flight_control_angle_of_attack_blend_0_1", "AOA Command Blend", "0..1", "Project-defined smooth command blend between reference-derived alpha endpoints.", value.angle_of_attack_blend_0_1) \
	X(double, angle_of_attack_maximum_nz, "flight_control_angle_of_attack_maximum_normal_acceleration_g", "AOA Max Nz", "g", "Positive normal-acceleration authority available at the current angle of attack.", value.angle_of_attack_maximum_normal_acceleration_g) \
	X(double, normal_acceleration_error, "flight_control_normal_acceleration_error_g", "Nz Error", "g", "Protected Nz command minus filtered measured Nz.", value.normal_acceleration_error_g) \
	X(double, pitch_rate_error, "flight_control_pitch_rate_error_rad_s", "Pitch Rate Error", "rad/s", "Protected pitch-rate command minus measured q.", value.pitch_rate_error_rad_s) \
	X(double, pitch_rate_washout, "flight_control_pitch_rate_washout_feedback_effort", "q Washout Feedback", "effort", "Washed-out q damping contribution in normal Nz mode.", value.pitch_rate_washout_feedback_effort) \
	X(double, alpha_stability_feedback, "flight_control_angle_of_attack_stability_feedback_effort", "Alpha Stability Feedback", "effort", "Artificial static-stability alpha feedback contribution.", value.angle_of_attack_stability_feedback_effort) \
	X(double, longitudinal_integral, "flight_control_longitudinal_integral_effort", "Longitudinal Integral", "effort", "Active longitudinal controller integral contribution.", value.longitudinal_integral_effort) \
	X(double, unsaturated_pitch_effort, "flight_control_unsaturated_pitch_effort", "Unsaturated Pitch Effort", "effort", "Pitch effort before electronic limiting.", value.unsaturated_pitch_effort) \
	X(double, limited_pitch_effort, "flight_control_limited_pitch_effort", "Limited Pitch Effort", "effort", "Pitch effort sent to the surface mixer.", value.limited_pitch_effort) \
	X(bool, load_factor_limit, "flight_control_load_factor_limit_active", "Load Factor Limit Active", "", "Normal-law load-factor protection is modifying the command.", value.load_factor_limit_active) \
	X(bool, body_rate_limit, "flight_control_body_rate_limit_active", "Body Rate Limit Active", "", "A body-rate limit is modifying the command.", value.body_rate_limit_active) \
	X(bool, anti_windup, "flight_control_anti_windup_active", "Anti-Windup Active", "", "An integral controller is inhibited by exhausted electronic or physical authority.", value.anti_windup_active) \
	X(bool, output_transition, "flight_control_output_selection_transition_active", "Output Selection Transition", "", "Electronic command selection is performing a bumpless transfer.", value.output_selection_transition_active) \
	X(bool, actuator_feedback_valid, "flight_control_actuator_feedback_valid", "Actuator Feedback Valid", "", "Electronic actuator feedback routing passed numeric validation.", value.actuator_feedback_valid) \
	X(bool, electronic_command_saturated, "flight_control_electronic_command_saturated", "Electronic Command Saturated", "", "The selected electronic surface demand exceeded its configured physical command limit before the actuator.", value.electronic_command_saturated) \
	X(bool, actuator_saturated, "flight_control_actuator_saturated", "Physical Actuator Saturated", "", "At least one 256 Hz physical actuator reported a travel or rate limit.", value.actuator_saturated) \
	X(bool, control_authority_limited, "flight_control_control_authority_limited", "Control Authority Limited", "", "Sustained alpha protection has exhausted longitudinal electronic or stabilator authority.", value.control_authority_limited) \
	X(bool, actuator_tracking_consistent, "flight_control_actuator_tracking_consistent", "Actuator Tracking Consistent", "", "All actuator feedback is within the configured electronic tracking tolerance.", value.actuator_tracking_consistent) \
	X(double, maximum_actuator_tracking_error, "flight_control_maximum_actuator_tracking_error_rad", "Maximum Actuator Tracking Error", "rad", "Largest commanded-to-feedback surface position error.", value.maximum_actuator_tracking_error_rad) \
	X(bool, g_override_available, "flight_control_developer_g_limiter_override_available", "Developer G-Limiter Override Available", "", "Developer-only feature availability.", value.developer_g_limiter_override_available) \
	X(bool, g_override_active, "flight_control_developer_g_limiter_override_active", "Developer G-Limiter Override Active", "", "Developer-only feature state.", value.developer_g_limiter_override_active) \
	X(std::string, selected_longitudinal_source, "flight_control_selected_longitudinal_source", "Selected Longitudinal Source", "", "Reference source selected before guidance coordination.", std::string(reference_source(value.selected_longitudinal_source))) \
	X(std::string, selected_lateral_source, "flight_control_selected_lateral_source", "Selected Lateral Source", "", "Reference source selected before guidance coordination.", std::string(reference_source(value.selected_lateral_source))) \
	X(std::string, selected_directional_source, "flight_control_selected_directional_source", "Selected Directional Source", "", "Reference source selected before guidance coordination.", std::string(reference_source(value.selected_directional_source))) \
	X(std::string, selected_vertical_type, "flight_control_selected_vertical_reference_type", "Selected Vertical Reference Type", "", "Selected automatic vertical reference payload type.", std::string(vertical_reference_type(value.selected_vertical_reference_type))) \
	X(double, selected_normal_acceleration, "flight_control_selected_normal_acceleration_reference_g", "Selected Normal Acceleration Reference", "g", "Pre-coordination longitudinal reference.", value.selected_normal_acceleration_reference_g) \
	X(double, selected_pitch_rate, "flight_control_selected_pitch_rate_command_rad_s", "Selected Pitch Rate Command", "rad/s", "Pre-coordination longitudinal command.", value.selected_pitch_rate_command_rad_s) \
	X(double, selected_pitch_attitude, "flight_control_selected_pitch_attitude_reference_rad", "Selected Pitch Attitude Reference", "rad", "Pre-coordination automatic reference.", value.selected_pitch_attitude_reference_rad) \
	X(double, selected_vertical_speed, "flight_control_selected_vertical_speed_reference_ft_s", "Selected Vertical Speed Reference", "ft/s", "Pre-coordination automatic reference.", value.selected_vertical_speed_reference_ft_s) \
	X(double, selected_roll_rate, "flight_control_selected_roll_rate_reference_rad_s", "Selected Roll Rate Reference", "rad/s", "Pre-coordination manual reference.", value.selected_roll_rate_reference_rad_s) \
	X(double, selected_bank_angle, "flight_control_selected_bank_angle_reference_rad", "Selected Bank Angle Reference", "rad", "Pre-coordination automatic reference.", value.selected_bank_angle_reference_rad) \
	X(double, selected_sideslip, "flight_control_selected_sideslip_reference_rad", "Selected Sideslip Reference", "rad", "Pre-coordination directional reference.", value.selected_sideslip_reference_rad) \
	X(double, selected_yaw_rate, "flight_control_selected_yaw_rate_feedforward_rad_s", "Selected Yaw Rate Feedforward", "rad/s", "Pre-coordination directional reference.", value.selected_yaw_rate_feedforward_rad_s) \
	X(double, normal_acceleration_reference, "flight_control_normal_acceleration_reference_g", "Normal Acceleration Reference", "g", "Coordinated control-law reference.", value.normal_acceleration_reference_g) \
	X(double, pitch_rate_reference, "flight_control_pitch_rate_command_rad_s", "Pitch Rate Command", "rad/s", "Coordinated active pitch-rate command.", value.pitch_rate_command_rad_s) \
	X(double, roll_rate_reference, "flight_control_roll_rate_reference_rad_s", "Roll Rate Reference", "rad/s", "Coordinated control-law reference.", value.roll_rate_reference_rad_s) \
	X(double, sideslip_reference, "flight_control_sideslip_reference_rad", "Sideslip Reference", "rad", "Coordinated control-law reference.", value.sideslip_reference_rad) \
	X(double, yaw_rate_reference, "flight_control_yaw_rate_feedforward_rad_s", "Yaw Rate Feedforward", "rad/s", "Coordinated control-law reference.", value.yaw_rate_feedforward_rad_s) \
	X(double, stabilator_demand, "flight_control_symmetric_stabilator_demand_rad", "Symmetric Stabilator Demand", "rad", "Physical FCC demand sent to flight-control actuation.", value.symmetric_stabilator_demand_rad) \
	X(double, flaperon_demand, "flight_control_differential_flaperon_demand_rad", "Differential Flaperon Demand", "rad", "Physical FCC demand sent to flight-control actuation.", value.differential_flaperon_demand_rad) \
	X(double, rudder_demand, "flight_control_rudder_demand_rad", "Rudder Demand", "rad", "Physical FCC demand sent to flight-control actuation.", value.rudder_demand_rad) \
	X(std::string, constraint_reason, "flight_control_constraint_reason", "Flight-Control Constraint", "", "Current guidance coordination constraint.", std::string(::constraint_reason(value.constraint_reason))) \
	X(bool, vertical_constrained, "flight_control_vertical_constrained", "Vertical Constrained", "", "Vertical guidance was constrained this tick.", value.vertical_constrained) \
	X(bool, lateral_constrained, "flight_control_lateral_constrained", "Lateral Constrained", "", "Lateral guidance was constrained this tick.", value.lateral_constrained)

#define AUTOMATIC_CHANNELS(X) \
	X(bool, master, "afcs_master_engaged", "AP Master", "", "Automatic flight control master state.", value.master_engaged) \
	X(bool, bypass, "afcs_bypass_active", "AP Bypass", "", "Momentary AP bypass state.", value.bypass_active) \
	X(bool, auto_throttle_available, "afcs_experimental_auto_throttle_available", "Experimental Auto-Throttle Available", "", "True only when the developer configuration explicitly enables this non-authentic assist.", value.experimental_auto_throttle_available) \
	X(bool, auto_throttle, "afcs_auto_throttle_engaged", "Experimental Auto-Throttle", "", "Non-authentic optional assist state.", value.auto_throttle_engaged) \
	X(std::string, vertical_mode, "afcs_vertical_mode", "AP Vertical Mode", "", "Active vertical mode.", std::string(::vertical_mode(value.vertical_mode))) \
	X(std::string, lateral_mode, "afcs_lateral_mode", "AP Lateral Mode", "", "Active lateral mode.", std::string(::lateral_mode(value.lateral_mode))) \
	X(double, pitch_reference, "afcs_pitch_attitude_reference_rad", "AP Pitch Reference", "rad", "Shaped AP vertical reference.", value.pitch_attitude_reference_rad) \
	X(double, vertical_speed_reference, "afcs_vertical_speed_reference_ft_s", "AP Vertical Speed Reference", "ft/s", "Shaped AP vertical reference.", value.vertical_speed_reference_ft_s) \
	X(double, bank_reference, "afcs_bank_angle_reference_rad", "AP Bank Reference", "rad", "Shaped AP lateral reference.", value.bank_angle_reference_rad) \
	X(double, throttle_command, "afcs_throttle_command_normalized", "Experimental A/T Command", "normalized", "Experimental optional assist output.", value.throttle_command_normalized) \
	X(double, target_altitude, "afcs_target_altitude_ft", "AP Target Altitude", "ft", "Selected or captured AP target.", value.target_altitude_ft) \
	X(double, target_heading, "afcs_target_heading_deg", "AP Heading Select", "deg", "Persistent whole-degree heading selected through the F-16A/B-reference HSI control model.", static_cast<double>(value.target_heading_deg)) \
	X(double, target_speed, "afcs_target_speed_mps", "Experimental A/T Target Speed", "m/s", "Experimental optional assist target.", value.target_speed_mps) \
	X(double, target_pitch, "afcs_target_pitch_rad", "AP Target Pitch", "rad", "Selected or captured AP target.", value.target_pitch_rad) \
	X(std::string, longitudinal_authority, "afcs_longitudinal_authority", "AP Longitudinal Authority", "", "Current longitudinal authority state.", std::string(authority(value.longitudinal_authority))) \
	X(std::string, lateral_authority, "afcs_lateral_authority", "AP Lateral Authority", "", "Current lateral authority state.", std::string(authority(value.lateral_authority))) \
	X(std::string, constraint_reason, "afcs_constraint_reason", "AP Constraint", "", "Constraint reported to the AP monitor.", std::string(::constraint_reason(value.constraint_reason))) \
	X(std::string, degradation_reason, "afcs_degradation_reason", "AP Degradation", "", "Current AP degradation reason.", std::string(::degradation_reason(value.degradation_reason))) \
	X(std::string, disconnect_reason, "afcs_disconnect_reason", "AP Disconnect", "", "Current AP disconnect reason.", std::string(::disconnect_reason(value.disconnect_reason))) \
	X(bool, vertical_degraded, "afcs_vertical_degraded", "AP Vertical Degraded", "", "Vertical tracking monitor state.", value.vertical_degraded) \
	X(bool, lateral_degraded, "afcs_lateral_degraded", "AP Lateral Degraded", "", "Lateral tracking monitor state.", value.lateral_degraded) \
	X(std::string, ap_engage_rejection, "afcs_ap_engage_rejection_reason", "AP Engage Rejection", "", "Most recent AP engage rejection.", std::string(automatic_reason(value.autopilot_engage_rejection_reason))) \
	X(std::string, ap_disengage, "afcs_ap_disengage_reason", "AP Disengage Reason", "", "Most recent AP disengage reason.", std::string(automatic_reason(value.autopilot_disengage_reason))) \
	X(std::string, at_engage_rejection, "afcs_at_engage_rejection_reason", "A/T Engage Rejection", "", "Most recent experimental A/T engage rejection.", std::string(automatic_reason(value.auto_throttle_engage_rejection_reason))) \
	X(std::string, at_disengage, "afcs_at_disengage_reason", "A/T Disengage Reason", "", "Most recent experimental A/T disengage reason.", std::string(automatic_reason(value.auto_throttle_disengage_reason)))

#define CHANNEL_MEMBER(type, id, name, label, unit, description, expression) \
	Core::DebugTelemetryChannel<type> id##_;
#define DECLARE_CHANNEL(type, id, name, label, unit, description, expression) \
	id##_ = declare<type>(setup, { name, label, unit, description });
#define PUBLISH_CHANNEL(type, id, name, label, unit, description, expression) \
	id##_.publish(time, expression);

class ActualChannels
{
public:
	void declare_channels(Core::Systems::SystemSetup& setup)
	{
		ACTUAL_CHANNELS(DECLARE_CHANNEL)
	}

	void publish(
		Core::DebugSimulationTime time,
		const Core::FlightControlObservation& value) const
	{
		ACTUAL_CHANNELS(PUBLISH_CHANNEL)
	}

private:
	ACTUAL_CHANNELS(CHANNEL_MEMBER)
};

class FlightControlChannels
{
public:
	void declare_channels(Core::Systems::SystemSetup& setup)
	{
		FLIGHT_CONTROL_CHANNELS(DECLARE_CHANNEL)
	}

	void publish(
		Core::DebugSimulationTime time,
		const Core::FlightControlComputerSnapshot& value) const
	{
		FLIGHT_CONTROL_CHANNELS(PUBLISH_CHANNEL)
	}

private:
	FLIGHT_CONTROL_CHANNELS(CHANNEL_MEMBER)
};

class AutomaticChannels
{
public:
	void declare_channels(Core::Systems::SystemSetup& setup)
	{
		AUTOMATIC_CHANNELS(DECLARE_CHANNEL)
	}

	void publish(
		Core::DebugSimulationTime time,
		const Core::AutomaticFlightControlSnapshot& value) const
	{
		AUTOMATIC_CHANNELS(PUBLISH_CHANNEL)
	}

private:
	AUTOMATIC_CHANNELS(CHANNEL_MEMBER)
};

#undef PUBLISH_CHANNEL
#undef DECLARE_CHANNEL
#undef CHANNEL_MEMBER
#undef AUTOMATIC_CHANNELS
#undef FLIGHT_CONTROL_CHANNELS
#undef ACTUAL_CHANNELS
}

namespace Core
{
namespace Systems
{
class FlightControlComputerDebugTelemetry::Implementation
{
public:
	void declare_channels(SystemSetup& setup)
	{
		actual_.declare_channels(setup);
		flight_control_.declare_channels(setup);
		automatic_.declare_channels(setup);
	}

	void publish_initial(
		const FlightControlComputerSnapshot& flight_control,
		const AutomaticFlightControlSnapshot& automatic) const
	{
		const DebugSimulationTime initial_time = {};
		flight_control_.publish(initial_time, flight_control);
		automatic_.publish(initial_time, automatic);
	}

	void publish_step(const FlightControlComputerDebugFrame& frame) const
	{
		actual_.publish(frame.time, frame.observation);
		flight_control_.publish(frame.time, frame.flight_control);
		automatic_.publish(frame.time, frame.automatic);
	}

private:
	ActualChannels actual_;
	FlightControlChannels flight_control_;
	AutomaticChannels automatic_;
};

FlightControlComputerDebugTelemetry::FlightControlComputerDebugTelemetry()
	: implementation_(std::make_unique<Implementation>())
{
}

FlightControlComputerDebugTelemetry::~FlightControlComputerDebugTelemetry() =
	default;

void FlightControlComputerDebugTelemetry::declare_channels(SystemSetup& setup)
{
	implementation_->declare_channels(setup);
}

void FlightControlComputerDebugTelemetry::publish_initial(
	const FlightControlComputerSnapshot& flight_control,
	const AutomaticFlightControlSnapshot& automatic) const
{
	implementation_->publish_initial(flight_control, automatic);
}

void FlightControlComputerDebugTelemetry::publish_step(
	const FlightControlComputerDebugFrame& frame) const
{
	implementation_->publish_step(frame);
}
}
}
