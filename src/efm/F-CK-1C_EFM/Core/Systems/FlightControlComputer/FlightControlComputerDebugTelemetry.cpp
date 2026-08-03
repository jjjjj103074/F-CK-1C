#include "FlightControlComputerDebugTelemetry.h"

#include "../SystemPipeline.h"

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
	return {
		definition.name,
		definition.label,
		Core::DebugTelemetryValueType::Double,
		definition.unit,
		definition.description
	};
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

const char* vertical_mode(Core::AutomaticFlightControlVerticalMode value)
{
	switch (value)
	{
	case Core::AutomaticFlightControlVerticalMode::Off: return "Off";
	case Core::AutomaticFlightControlVerticalMode::PitchHold:
		return "Pitch Hold";
	case Core::AutomaticFlightControlVerticalMode::VerticalSpeedHold:
		return "Vertical Speed Hold";
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
	case Core::AutomaticFlightControlLateralMode::HeadingHold:
		return "Heading Hold";
	case Core::AutomaticFlightControlLateralMode::HeadingSelect:
		return "Heading Select";
	case Core::AutomaticFlightControlLateralMode::NavigationTrack:
		return "Navigation Track";
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
	}
	throw std::logic_error("Unknown automatic-flight-control reason.");
}

}

namespace Core
{
namespace Systems
{
void FlightControlComputerDebugTelemetry::declare_channels(SystemSetup& setup)
{
	declare_actual_channels(setup);
	declare_reference_channels(setup);
	declare_afcs_channels(setup);
}

void FlightControlComputerDebugTelemetry::declare_actual_channels(
	SystemSetup& setup)
{
	actual_ias_ = declare<double>(setup, { "flight_actual_indicated_airspeed_mps", "Actual IAS", "m/s", "Conditioned FCC input observation." });
	actual_vertical_speed_ = declare<double>(setup, { "flight_actual_vertical_speed_mps", "Actual Vertical Speed", "m/s", "FCC input observation at this scheduled tick." });
	actual_heading_ = declare<double>(setup, { "flight_actual_heading_rad", "Actual Heading", "rad", "FCC input observation at this scheduled tick." });
	actual_pitch_ = declare<double>(setup, { "flight_actual_pitch_attitude_rad", "Actual Pitch", "rad", "FCC input observation at this scheduled tick." });
	actual_roll_ = declare<double>(setup, { "flight_actual_roll_attitude_rad", "Actual Roll", "rad", "FCC input observation at this scheduled tick." });
	actual_roll_rate_ = declare<double>(setup, { "flight_actual_roll_rate_rad_s", "Actual Roll Rate", "rad/s", "FCC input observation at this scheduled tick." });
	actual_pitch_rate_ = declare<double>(setup, { "flight_actual_pitch_rate_rad_s", "Actual Pitch Rate", "rad/s", "FCC input observation at this scheduled tick." });
	actual_yaw_rate_ = declare<double>(setup, { "flight_actual_yaw_rate_rad_s", "Actual Yaw Rate", "rad/s", "FCC input observation at this scheduled tick." });
}

void FlightControlComputerDebugTelemetry::declare_reference_channels(
	SystemSetup& setup)
{
	g_override_available_ = declare<bool>(setup, { "flight_control_developer_g_limiter_override_available", "Developer G-Limiter Override Available", "", "Developer-only feature availability." });
	g_override_active_ = declare<bool>(setup, { "flight_control_developer_g_limiter_override_active", "Developer G-Limiter Override Active", "", "Developer-only feature state." });
	selected_longitudinal_source_ = declare<std::string>(setup, { "flight_control_selected_longitudinal_source", "Selected Longitudinal Source", "", "Reference source selected before guidance coordination." });
	selected_lateral_source_ = declare<std::string>(setup, { "flight_control_selected_lateral_source", "Selected Lateral Source", "", "Reference source selected before guidance coordination." });
	selected_directional_source_ = declare<std::string>(setup, { "flight_control_selected_directional_source", "Selected Directional Source", "", "Reference source selected before guidance coordination." });
	selected_vertical_type_ = declare<std::string>(setup, { "flight_control_selected_vertical_reference_type", "Selected Vertical Reference Type", "", "Selected automatic vertical reference payload type." });
	selected_normal_acceleration_ = declare<double>(setup, { "flight_control_selected_normal_acceleration_reference_g", "Selected Normal Acceleration Reference", "g", "Pre-coordination longitudinal reference." });
	selected_pitch_rate_ = declare<double>(setup, { "flight_control_selected_pitch_rate_feedforward_rad_s", "Selected Pitch Rate Feedforward", "rad/s", "Pre-coordination longitudinal reference." });
	selected_pitch_attitude_ = declare<double>(setup, { "flight_control_selected_pitch_attitude_reference_rad", "Selected Pitch Attitude Reference", "rad", "Pre-coordination automatic reference." });
	selected_vertical_speed_ = declare<double>(setup, { "flight_control_selected_vertical_speed_reference_mps", "Selected Vertical Speed Reference", "m/s", "Pre-coordination automatic reference." });
	selected_roll_rate_ = declare<double>(setup, { "flight_control_selected_roll_rate_reference_rad_s", "Selected Roll Rate Reference", "rad/s", "Pre-coordination manual reference." });
	selected_bank_angle_ = declare<double>(setup, { "flight_control_selected_bank_angle_reference_rad", "Selected Bank Angle Reference", "rad", "Pre-coordination automatic reference." });
	selected_sideslip_ = declare<double>(setup, { "flight_control_selected_sideslip_reference_rad", "Selected Sideslip Reference", "rad", "Pre-coordination directional reference." });
	selected_yaw_rate_ = declare<double>(setup, { "flight_control_selected_yaw_rate_feedforward_rad_s", "Selected Yaw Rate Feedforward", "rad/s", "Pre-coordination directional reference." });
	normal_acceleration_reference_ = declare<double>(setup, { "flight_control_normal_acceleration_reference_g", "Normal Acceleration Reference", "g", "Coordinated control-law reference." });
	pitch_rate_reference_ = declare<double>(setup, { "flight_control_pitch_rate_feedforward_rad_s", "Pitch Rate Feedforward", "rad/s", "Coordinated control-law reference." });
	roll_rate_reference_ = declare<double>(setup, { "flight_control_roll_rate_reference_rad_s", "Roll Rate Reference", "rad/s", "Coordinated control-law reference." });
	sideslip_reference_ = declare<double>(setup, { "flight_control_sideslip_reference_rad", "Sideslip Reference", "rad", "Coordinated control-law reference." });
	yaw_rate_reference_ = declare<double>(setup, { "flight_control_yaw_rate_feedforward_rad_s", "Yaw Rate Feedforward", "rad/s", "Coordinated control-law reference." });
	elevator_command_ = declare<double>(setup, { "flight_control_elevator_command_normalized", "Elevator Demand", "normalized", "FCC demand sent to flight-control actuation." });
	aileron_command_ = declare<double>(setup, { "flight_control_aileron_command_normalized", "Aileron Demand", "normalized", "FCC demand sent to flight-control actuation." });
	rudder_command_ = declare<double>(setup, { "flight_control_rudder_command_normalized", "Rudder Demand", "normalized", "FCC demand sent to flight-control actuation." });
	constraint_reason_ = declare<std::string>(setup, { "flight_control_constraint_reason", "Flight-Control Constraint", "", "Current guidance coordination constraint." });
	vertical_constrained_ = declare<bool>(setup, { "flight_control_vertical_constrained", "Vertical Constrained", "", "Vertical guidance was constrained this tick." });
	lateral_constrained_ = declare<bool>(setup, { "flight_control_lateral_constrained", "Lateral Constrained", "", "Lateral guidance was constrained this tick." });
}

void FlightControlComputerDebugTelemetry::declare_afcs_channels(
	SystemSetup& setup)
{
	ap_master_ = declare<bool>(setup, { "afcs_master_engaged", "AP Master", "", "Automatic flight control master state." });
	ap_bypass_ = declare<bool>(setup, { "afcs_bypass_active", "AP Bypass", "", "Momentary AP bypass state." });
	auto_throttle_ = declare<bool>(setup, { "afcs_auto_throttle_engaged", "Experimental Auto-Throttle", "", "Non-authentic optional assist state." });
	vertical_mode_ = declare<std::string>(setup, { "afcs_vertical_mode", "AP Vertical Mode", "", "Active vertical mode." });
	lateral_mode_ = declare<std::string>(setup, { "afcs_lateral_mode", "AP Lateral Mode", "", "Active lateral mode." });
	pitch_reference_ = declare<double>(setup, { "afcs_pitch_attitude_reference_rad", "AP Pitch Reference", "rad", "Shaped AP vertical reference." });
	vertical_speed_reference_ = declare<double>(setup, { "afcs_vertical_speed_reference_mps", "AP Vertical Speed Reference", "m/s", "Shaped AP vertical reference." });
	bank_reference_ = declare<double>(setup, { "afcs_bank_angle_reference_rad", "AP Bank Reference", "rad", "Shaped AP lateral reference." });
	throttle_command_ = declare<double>(setup, { "afcs_throttle_command_normalized", "Experimental A/T Command", "normalized", "Experimental optional assist output." });
	target_altitude_ = declare<double>(setup, { "afcs_target_altitude_m", "AP Target Altitude", "m", "Selected or captured AP target." });
	target_heading_ = declare<double>(setup, { "afcs_target_heading_rad", "AP Target Heading", "rad", "Selected or captured AP target." });
	target_speed_ = declare<double>(setup, { "afcs_target_speed_mps", "Experimental A/T Target Speed", "m/s", "Experimental optional assist target." });
	target_pitch_ = declare<double>(setup, { "afcs_target_pitch_rad", "AP Target Pitch", "rad", "Selected or captured AP target." });
	target_vertical_speed_ = declare<double>(setup, { "afcs_target_vertical_speed_mps", "AP Target Vertical Speed", "m/s", "Selected or captured AP target." });
	longitudinal_authority_ = declare<std::string>(setup, { "afcs_longitudinal_authority", "AP Longitudinal Authority", "", "Current longitudinal authority state." });
	lateral_authority_ = declare<std::string>(setup, { "afcs_lateral_authority", "AP Lateral Authority", "", "Current lateral authority state." });
	ap_constraint_reason_ = declare<std::string>(setup, { "afcs_constraint_reason", "AP Constraint", "", "Constraint reported to the AP monitor." });
	degradation_reason_ = declare<std::string>(setup, { "afcs_degradation_reason", "AP Degradation", "", "Current AP degradation reason." });
	disconnect_reason_ = declare<std::string>(setup, { "afcs_disconnect_reason", "AP Disconnect", "", "Current AP disconnect reason." });
	vertical_degraded_ = declare<bool>(setup, { "afcs_vertical_degraded", "AP Vertical Degraded", "", "Vertical tracking monitor state." });
	lateral_degraded_ = declare<bool>(setup, { "afcs_lateral_degraded", "AP Lateral Degraded", "", "Lateral tracking monitor state." });
	ap_engage_rejection_ = declare<std::string>(setup, { "afcs_ap_engage_rejection_reason", "AP Engage Rejection", "", "Most recent AP engage rejection." });
	ap_disengage_ = declare<std::string>(setup, { "afcs_ap_disengage_reason", "AP Disengage Reason", "", "Most recent AP disengage reason." });
	at_engage_rejection_ = declare<std::string>(setup, { "afcs_at_engage_rejection_reason", "A/T Engage Rejection", "", "Most recent experimental A/T engage rejection." });
	at_disengage_ = declare<std::string>(setup, { "afcs_at_disengage_reason", "A/T Disengage Reason", "", "Most recent experimental A/T disengage reason." });
}

void FlightControlComputerDebugTelemetry::publish_initial(
	const FlightControlComputerSnapshot& flight_control,
	const AutomaticFlightControlSnapshot& automatic) const
{
	const DebugSimulationTime initial_time = {};
	publish_selected(initial_time, flight_control);
	publish_control(initial_time, flight_control);
	publish_afcs_state(initial_time, automatic);
	publish_afcs_reasons(initial_time, automatic);
}

void FlightControlComputerDebugTelemetry::publish_step(
	const FlightControlComputerDebugFrame& frame) const
{
	publish_actual(frame.time, frame.observation);
	publish_selected(frame.time, frame.flight_control);
	publish_control(frame.time, frame.flight_control);
	publish_afcs_state(frame.time, frame.automatic);
	publish_afcs_reasons(frame.time, frame.automatic);
}

void FlightControlComputerDebugTelemetry::publish_actual(
	DebugSimulationTime time,
	const FlightControlObservation& value) const
{
	actual_ias_.publish(time, value.indicated_airspeed_mps);
	actual_vertical_speed_.publish(time, value.vertical_speed_mps);
	actual_heading_.publish(time, value.heading_rad);
	actual_pitch_.publish(time, value.pitch_rad);
	actual_roll_.publish(time, value.roll_rad);
	actual_roll_rate_.publish(time, value.roll_rate_rad_s);
	actual_pitch_rate_.publish(time, value.pitch_rate_rad_s);
	actual_yaw_rate_.publish(time, value.yaw_rate_rad_s);
}

void FlightControlComputerDebugTelemetry::publish_selected(
	DebugSimulationTime time,
	const FlightControlComputerSnapshot& value) const
{
	g_override_available_.publish(
		time, value.developer_g_limiter_override_available);
	g_override_active_.publish(time, value.developer_g_limiter_override_active);
	selected_longitudinal_source_.publish(
		time, std::string(reference_source(value.selected_longitudinal_source)));
	selected_lateral_source_.publish(
		time, std::string(reference_source(value.selected_lateral_source)));
	selected_directional_source_.publish(
		time, std::string(reference_source(value.selected_directional_source)));
	selected_vertical_type_.publish(
		time, std::string(vertical_reference_type(
			value.selected_vertical_reference_type)));
	selected_normal_acceleration_.publish(
		time, value.selected_normal_acceleration_reference_g);
	selected_pitch_rate_.publish(
		time, value.selected_pitch_rate_feedforward_rad_s);
	selected_pitch_attitude_.publish(
		time, value.selected_pitch_attitude_reference_rad);
	selected_vertical_speed_.publish(
		time, value.selected_vertical_speed_reference_mps);
	selected_roll_rate_.publish(time, value.selected_roll_rate_reference_rad_s);
	selected_bank_angle_.publish(time, value.selected_bank_angle_reference_rad);
	selected_sideslip_.publish(time, value.selected_sideslip_reference_rad);
	selected_yaw_rate_.publish(
		time, value.selected_yaw_rate_feedforward_rad_s);
}

void FlightControlComputerDebugTelemetry::publish_control(
	DebugSimulationTime time,
	const FlightControlComputerSnapshot& value) const
{
	normal_acceleration_reference_.publish(
		time, value.normal_acceleration_reference_g);
	pitch_rate_reference_.publish(time, value.pitch_rate_feedforward_rad_s);
	roll_rate_reference_.publish(time, value.roll_rate_reference_rad_s);
	sideslip_reference_.publish(time, value.sideslip_reference_rad);
	yaw_rate_reference_.publish(time, value.yaw_rate_feedforward_rad_s);
	elevator_command_.publish(time, value.elevator_command_normalized);
	aileron_command_.publish(time, value.aileron_command_normalized);
	rudder_command_.publish(time, value.rudder_command_normalized);
	constraint_reason_.publish(
		time, std::string(constraint_reason(value.constraint_reason)));
	vertical_constrained_.publish(time, value.vertical_constrained);
	lateral_constrained_.publish(time, value.lateral_constrained);
}

void FlightControlComputerDebugTelemetry::publish_afcs_state(
	DebugSimulationTime time,
	const AutomaticFlightControlSnapshot& value) const
{
	ap_master_.publish(time, value.master_engaged);
	ap_bypass_.publish(time, value.bypass_active);
	auto_throttle_.publish(time, value.auto_throttle_engaged);
	vertical_mode_.publish(time, std::string(::vertical_mode(value.vertical_mode)));
	lateral_mode_.publish(time, std::string(::lateral_mode(value.lateral_mode)));
	pitch_reference_.publish(time, value.pitch_attitude_reference_rad);
	vertical_speed_reference_.publish(time, value.vertical_speed_reference_mps);
	bank_reference_.publish(time, value.bank_angle_reference_rad);
	throttle_command_.publish(time, value.throttle_command_normalized);
	target_altitude_.publish(time, value.target_altitude_m);
	target_heading_.publish(time, value.target_heading_rad);
	target_speed_.publish(time, value.target_speed_mps);
	target_pitch_.publish(time, value.target_pitch_rad);
	target_vertical_speed_.publish(time, value.target_vertical_speed_mps);
	longitudinal_authority_.publish(
		time, std::string(authority(value.longitudinal_authority)));
	lateral_authority_.publish(
		time, std::string(authority(value.lateral_authority)));
}

void FlightControlComputerDebugTelemetry::publish_afcs_reasons(
	DebugSimulationTime time,
	const AutomaticFlightControlSnapshot& value) const
{
	ap_constraint_reason_.publish(
		time, std::string(constraint_reason(value.constraint_reason)));
	degradation_reason_.publish(
		time, std::string(::degradation_reason(value.degradation_reason)));
	disconnect_reason_.publish(
		time, std::string(::disconnect_reason(value.disconnect_reason)));
	vertical_degraded_.publish(time, value.vertical_degraded);
	lateral_degraded_.publish(time, value.lateral_degraded);
	ap_engage_rejection_.publish(
		time, std::string(automatic_reason(
			value.autopilot_engage_rejection_reason)));
	ap_disengage_.publish(
		time, std::string(automatic_reason(value.autopilot_disengage_reason)));
	at_engage_rejection_.publish(
		time, std::string(automatic_reason(
			value.auto_throttle_engage_rejection_reason)));
	at_disengage_.publish(
		time, std::string(automatic_reason(
			value.auto_throttle_disengage_reason)));
}
}
}
