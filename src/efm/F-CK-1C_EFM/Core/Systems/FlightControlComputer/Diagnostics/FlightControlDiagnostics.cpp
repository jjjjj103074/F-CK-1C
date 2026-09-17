#include "FlightControlDiagnostics.h"
#include "FlightControlSnapshotProjection.h"

#include <stdexcept>

namespace
{
constexpr double kNoseDownEffortTolerance = 1.0e-5;

Core::FlightControlVerticalReferenceType to_snapshot_reference_type(
	Core::Systems::VerticalGuidanceReferenceType type)
{
	switch (type)
	{
	case Core::Systems::VerticalGuidanceReferenceType::None:
		return Core::FlightControlVerticalReferenceType::None;
	case Core::Systems::VerticalGuidanceReferenceType::PitchAttitude:
		return Core::FlightControlVerticalReferenceType::PitchAttitude;
	case Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed:
		return Core::FlightControlVerticalReferenceType::VerticalSpeed;
	}
	throw std::logic_error("Unknown vertical guidance reference type.");
}

Core::FlightControlLongitudinalCommandMode to_snapshot_command_mode(
	Core::Systems::LongitudinalCommandMode mode)
{
	switch (mode)
	{
	case Core::Systems::LongitudinalCommandMode::NormalAcceleration:
		return Core::FlightControlLongitudinalCommandMode::NormalAcceleration;
	case Core::Systems::LongitudinalCommandMode::PitchRate:
		return Core::FlightControlLongitudinalCommandMode::PitchRate;
	}
	throw std::logic_error("Unknown longitudinal command mode.");
}
}

namespace Core
{
namespace Systems
{
FlightControlDiagnostics::FlightControlDiagnostics(
	const ::Systems::FlightControlDevelopmentConfig& development,
	const ::Systems::FlightControlDiagnosticsConfig& config)
	: config_(config)
{
	snapshot_.status.available = true;
	snapshot_.developer_g_limiter_override_available =
		development.g_limiter_override_available;
	snapshot_.developer_direct_control_law_active =
		development.direct_control_law;
}

const FlightControlComputerSnapshot& FlightControlDiagnostics::update(
	const FlightControlDiagnosticsInput& input)
{
	++snapshot_.status.revision;
	update_subrates(input);
	update_status(input);
	update_selected_reference(input.command.selected);
	update_coordinated_reference(input);
	return snapshot_;
}

void FlightControlDiagnostics::update_subrates(
	const FlightControlDiagnosticsInput& input)
{
	snapshot_.flight_control_tick = input.flight_control_tick;
	snapshot_.pilot_shaping_update_count =
		input.pilot_shaping_update_count;
	snapshot_.pilot_shaping_last_update_tick =
		input.pilot_shaping_last_update_tick;
	snapshot_.pilot_shaping_age_ticks = input.flight_control_tick -
		input.pilot_shaping_last_update_tick;
	snapshot_.gain_schedule_update_count =
		input.gain_schedule_update_count;
	snapshot_.gain_schedule_last_update_tick =
		input.gain_schedule_last_update_tick;
	snapshot_.gain_schedule_age_ticks = input.flight_control_tick -
		input.gain_schedule_last_update_tick;
	snapshot_.conditioned_pitch_input_normalized =
		input.signals.pilot_pitch_normalized;
	snapshot_.conditioned_roll_input_normalized =
		input.signals.pilot_roll_normalized;
	snapshot_.conditioned_yaw_input_normalized =
		input.signals.pilot_yaw_normalized;
	snapshot_.filtered_normal_acceleration_g =
		input.signals.observation.normal_acceleration_g;
	snapshot_.active_command_gain = input.configuration.gains.command_gain;
	snapshot_.active_damping_gain = input.configuration.gains.damping_gain;
	snapshot_.active_limiter_gain = input.configuration.gains.limiter_gain;
}

void FlightControlDiagnostics::update_status(
	const FlightControlDiagnosticsInput& input)
{
	snapshot_.cat3_selected = input.cat3_selected;
	snapshot_.stores_transition_0_1 =
		input.configuration.stores_transition_0_1;
	snapshot_.developer_g_limiter_override_active =
		input.developer_g_limiter_override_active;
	update_longitudinal_status(input);
	snapshot_.load_factor_limit_active =
		input.laws.status.load_factor_limit_active;
	snapshot_.body_rate_limit_active =
		input.laws.status.body_rate_limit_active;
	snapshot_.anti_windup_active = input.laws.status.anti_windup_active;
	update_output_status(input);
	update_control_authority(input);
}

void FlightControlDiagnostics::update_longitudinal_status(
	const FlightControlDiagnosticsInput& input)
{
	snapshot_.angle_of_attack_limit_active =
		input.laws.status.angle_of_attack_limit_active;
	snapshot_.longitudinal_command_mode = to_snapshot_command_mode(
		input.laws.diagnostics.longitudinal_command_mode);
	snapshot_.longitudinal_mode_transition_active =
		input.laws.status.longitudinal_mode_transition_active;
	snapshot_.requested_normal_acceleration_reference_g =
		input.laws.diagnostics.requested_normal_acceleration_reference_g;
	snapshot_.effective_normal_acceleration_reference_g =
		input.laws.diagnostics.effective_normal_acceleration_reference_g;
	snapshot_.normal_acceleration_command_decrement_g =
		input.laws.diagnostics.normal_acceleration_command_decrement_g;
	snapshot_.requested_pitch_rate_command_rad_s =
		input.laws.diagnostics.requested_pitch_rate_command_rad_s;
	snapshot_.effective_pitch_rate_command_rad_s =
		input.laws.diagnostics.effective_pitch_rate_command_rad_s;
	snapshot_.angle_of_attack_blend_0_1 =
		input.laws.diagnostics.angle_of_attack_blend_0_1;
	snapshot_.angle_of_attack_maximum_normal_acceleration_g =
		input.laws.diagnostics.angle_of_attack_maximum_normal_acceleration_g;
	snapshot_.normal_acceleration_error_g =
		input.laws.diagnostics.normal_acceleration_error_g;
	snapshot_.pitch_rate_error_rad_s =
		input.laws.diagnostics.pitch_rate_error_rad_s;
	snapshot_.pitch_rate_washout_feedback_effort =
		input.laws.diagnostics.pitch_rate_washout_feedback_effort;
	snapshot_.angle_of_attack_stability_feedback_effort =
		input.laws.diagnostics.angle_of_attack_stability_feedback_effort;
	snapshot_.longitudinal_integral_effort =
		input.laws.diagnostics.longitudinal_integral_effort;
	snapshot_.unsaturated_pitch_effort =
		input.laws.diagnostics.unsaturated_pitch_effort;
	snapshot_.limited_pitch_effort =
		input.laws.diagnostics.limited_pitch_effort;
}

void FlightControlDiagnostics::update_output_status(
	const FlightControlDiagnosticsInput& input)
{
	snapshot_.output_selection_transition_active =
		input.output_status.selection_transition_active;
	snapshot_.actuator_feedback_valid =
		input.output_status.actuator_feedback_valid;
	snapshot_.electronic_command_saturated =
		input.output_status.electronic_command_saturated;
	snapshot_.actuator_saturated = input.output_status.actuator_saturated;
	snapshot_.actuator_tracking_consistent =
		input.output_status.actuator_tracking_consistent;
	snapshot_.maximum_actuator_tracking_error_rad =
		input.output_status.maximum_tracking_error_rad;
}

void FlightControlDiagnostics::update_control_authority(
	const FlightControlDiagnosticsInput& input)
{
	const double alpha_rad = input.signals.observation.angle_of_attack_rad;
	if (!angle_of_attack_initialized_)
	{
		previous_angle_of_attack_rad_ = alpha_rad;
		angle_of_attack_initialized_ = true;
		snapshot_.control_authority_limited = false;
		return;
	}
	const double alpha_rate_rad_s =
		(alpha_rad - previous_angle_of_attack_rad_) / input.signals.dt_s;
	previous_angle_of_attack_rad_ = alpha_rad;
	const bool nose_down_demand =
		input.laws.diagnostics.limited_pitch_effort <
			-kNoseDownEffortTolerance;
	const bool alpha_not_recovering = alpha_rate_rad_s >=
		-config_.minimum_alpha_recovery_rate_rad_s;
	const bool limited = input.laws.status.angle_of_attack_limit_active &&
		input.output_status.protection_authority_exhausted &&
		nose_down_demand && alpha_not_recovering;
	authority_limit_time_s_ = limited
		? authority_limit_time_s_ + input.signals.dt_s : 0.0;
	snapshot_.control_authority_limited = authority_limit_time_s_ >=
		config_.control_authority_persistence_s;
}

void FlightControlDiagnostics::update_selected_reference(
	const SelectedFlightReference& selected)
{
	snapshot_.selected_normal_acceleration_reference_g = 0.0;
	snapshot_.selected_pitch_rate_command_rad_s = 0.0;
	snapshot_.selected_pitch_attitude_reference_rad = 0.0;
	snapshot_.selected_vertical_speed_reference_ft_s = 0.0;
	snapshot_.selected_roll_rate_reference_rad_s = 0.0;
	snapshot_.selected_bank_angle_reference_rad = 0.0;
	update_longitudinal(selected);
	update_lateral(selected);
	snapshot_.selected_directional_source =
		FlightControlReferenceSource::Manual;
	snapshot_.selected_sideslip_reference_rad =
		selected.directional.sideslip_reference_rad;
	snapshot_.selected_yaw_rate_feedforward_rad_s =
		selected.directional.yaw_rate_feedforward_rad_s;
}

void FlightControlDiagnostics::update_longitudinal(
	const SelectedFlightReference& selected)
{
	const auto* manual =
		std::get_if<LongitudinalManeuverReference>(&selected.longitudinal);
	if (manual != nullptr)
	{
		snapshot_.selected_longitudinal_source =
			FlightControlReferenceSource::Manual;
		snapshot_.selected_vertical_reference_type =
			FlightControlVerticalReferenceType::None;
		if (const auto* normal = std::get_if<NormalAccelerationCommand>(
			&manual->command))
		{
			snapshot_.selected_normal_acceleration_reference_g =
				normal->target_g;
		}
		else
		{
			snapshot_.selected_pitch_rate_command_rad_s =
				std::get<PitchRateCommand>(manual->command).target_rad_s;
		}
		return;
	}
	const auto& automatic =
		std::get<AutomaticLongitudinalFlightReference>(selected.longitudinal);
	snapshot_.selected_longitudinal_source =
		FlightControlReferenceSource::Automatic;
	snapshot_.selected_vertical_reference_type =
		to_snapshot_reference_type(automatic.type);
	snapshot_.selected_pitch_attitude_reference_rad =
		automatic.pitch_attitude_reference_rad;
	snapshot_.selected_vertical_speed_reference_ft_s =
		automatic.vertical_speed_reference_ft_s;
}

void FlightControlDiagnostics::update_lateral(
	const SelectedFlightReference& selected)
{
	const auto* manual =
		std::get_if<ManualLateralFlightReference>(&selected.lateral);
	if (manual != nullptr)
	{
		snapshot_.selected_lateral_source =
			FlightControlReferenceSource::Manual;
		snapshot_.selected_roll_rate_reference_rad_s =
			manual->roll_rate_reference_rad_s;
		return;
	}
	snapshot_.selected_lateral_source =
		FlightControlReferenceSource::Automatic;
	snapshot_.selected_bank_angle_reference_rad =
		std::get<AutomaticLateralFlightReference>(selected.lateral)
			.bank_angle_reference_rad;
}

void FlightControlDiagnostics::update_coordinated_reference(
	const FlightControlDiagnosticsInput& input)
{
	const auto& reference = input.command.coordinated.reference;
	snapshot_.normal_acceleration_reference_g = 0.0;
	snapshot_.pitch_rate_command_rad_s = 0.0;
	if (const auto* normal = std::get_if<NormalAccelerationCommand>(
		&reference.longitudinal.command))
	{
		snapshot_.normal_acceleration_reference_g = normal->target_g;
	}
	else
	{
		snapshot_.pitch_rate_command_rad_s =
			std::get<PitchRateCommand>(reference.longitudinal.command)
				.target_rad_s;
	}
	snapshot_.roll_rate_reference_rad_s =
		reference.lateral_directional.roll_rate_reference_rad_s;
	snapshot_.sideslip_reference_rad =
		reference.lateral_directional.sideslip_reference_rad;
	snapshot_.yaw_rate_feedforward_rad_s =
		reference.lateral_directional.yaw_rate_feedforward_rad_s;
	snapshot_.symmetric_stabilator_demand_rad =
		input.actuator_command.symmetric_stabilator_demand_rad;
	snapshot_.differential_flaperon_demand_rad =
		input.actuator_command.differential_flaperon_demand_rad;
	snapshot_.rudder_demand_rad = input.actuator_command.rudder_demand_rad;
	snapshot_.constraint_reason = SnapshotProjection::constraint(
		input.command.coordinated.constraint.reason);
	snapshot_.vertical_constrained =
		input.command.coordinated.constraint.vertical_constrained;
	snapshot_.lateral_constrained =
		input.command.coordinated.constraint.lateral_constrained;
}

const FlightControlComputerSnapshot& FlightControlDiagnostics::snapshot() const
{
	return snapshot_;
}
}
}
