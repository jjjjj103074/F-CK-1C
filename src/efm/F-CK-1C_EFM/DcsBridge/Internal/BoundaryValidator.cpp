#include "BoundaryValidator.h"

#include "EfmEventReporter.h"
#include "../../DcsIds/DrawArgs.h"

#include <cmath>
#include <initializer_list>

namespace
{
struct NumericField
{
	const char* name;
	double value;
};

struct NumericSample
{
	const char* callback;
	std::initializer_list<NumericField> fields;
};

constexpr double kMinimumDamageIntegrity = 0.0;
constexpr double kMaximumDamageIntegrity = 1.0;

bool validate_numeric_sample(
	const NumericSample& sample,
	DcsBridge::Internal::EfmEventReporter& reporter)
{
	for (const NumericField& field : sample.fields)
	{
		if (!std::isfinite(field.value))
		{
			reporter.log_invalid_numeric(sample.callback, field.name, field.value);
			return false;
		}
	}
	return true;
}
}

namespace DcsBridge
{
namespace Internal
{
bool is_valid_frame_dt(double dt_s) noexcept
{
	return std::isfinite(dt_s) && dt_s > 0.0;
}

bool validate_atmosphere_input(
	const Core::AtmosphereInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_atmosphere", {
		{ "altitude_asl_m", input.altitude_asl_m },
		{ "temperature_k", input.temperature_k },
		{ "speed_of_sound_mps", input.speed_of_sound_mps },
		{ "density_kg_m3", input.density_kg_m3 },
		{ "pressure_pa", input.pressure_pa },
		{ "wind_world_x_mps", input.wind_world_mps.x },
		{ "wind_world_y_mps", input.wind_world_mps.y },
		{ "wind_world_z_mps", input.wind_world_mps.z }
	} }, reporter);
}

bool validate_surface_input(
	const Core::SurfaceInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_surface", {
		{ "surface_height_m", input.surface_height_m },
		{ "surface_height_with_objects_m", input.surface_height_with_objects_m },
		{ "normal_world_x", input.normal_world_unit.x },
		{ "normal_world_y", input.normal_world_unit.y },
		{ "normal_world_z", input.normal_world_unit.z }
	} }, reporter);
}

bool validate_mass_input(
	const Core::MassStateInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_current_mass_state", {
		{ "mass_kg", input.mass_kg },
		{ "center_of_mass_body_x_m", input.center_of_mass_body_m.x },
		{ "center_of_mass_body_y_m", input.center_of_mass_body_m.y },
		{ "center_of_mass_body_z_m", input.center_of_mass_body_m.z },
		{ "moment_of_inertia_body_x_kg_m2", input.moment_of_inertia_body_kg_m2.x },
		{ "moment_of_inertia_body_y_kg_m2", input.moment_of_inertia_body_kg_m2.y },
		{ "moment_of_inertia_body_z_kg_m2", input.moment_of_inertia_body_kg_m2.z }
	} }, reporter);
}

bool validate_world_kinematics_input(
	const Core::WorldKinematicsInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_current_state", {
		{ "acceleration_world_x_mps2", input.acceleration_world_mps2.x },
		{ "acceleration_world_y_mps2", input.acceleration_world_mps2.y },
		{ "acceleration_world_z_mps2", input.acceleration_world_mps2.z },
		{ "velocity_world_x_mps", input.velocity_world_mps.x },
		{ "velocity_world_y_mps", input.velocity_world_mps.y },
		{ "velocity_world_z_mps", input.velocity_world_mps.z },
		{ "position_world_x_m", input.position_world_m.x },
		{ "position_world_y_m", input.position_world_m.y },
		{ "position_world_z_m", input.position_world_m.z },
		{ "angular_acceleration_world_x_rad_s2", input.angular_acceleration_world_rad_s2.x },
		{ "angular_acceleration_world_y_rad_s2", input.angular_acceleration_world_rad_s2.y },
		{ "angular_acceleration_world_z_rad_s2", input.angular_acceleration_world_rad_s2.z },
		{ "angular_velocity_world_x_rad_s", input.angular_velocity_world_rad_s.x },
		{ "angular_velocity_world_y_rad_s", input.angular_velocity_world_rad_s.y },
		{ "angular_velocity_world_z_rad_s", input.angular_velocity_world_rad_s.z },
		{ "quaternion_x", input.orientation.x },
		{ "quaternion_y", input.orientation.y },
		{ "quaternion_z", input.orientation.z },
		{ "quaternion_w", input.orientation.w }
	} }, reporter);
}

bool validate_body_kinematics_input(
	const Core::BodyKinematicsInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_current_state_body_axis", {
		{ "acceleration_body_x_mps2", input.acceleration_body_mps2.x },
		{ "acceleration_body_y_mps2", input.acceleration_body_mps2.y },
		{ "acceleration_body_z_mps2", input.acceleration_body_mps2.z },
		{ "velocity_body_x_mps", input.velocity_body_mps.x },
		{ "velocity_body_y_mps", input.velocity_body_mps.y },
		{ "velocity_body_z_mps", input.velocity_body_mps.z },
		{ "wind_body_x_mps", input.wind_velocity_body_mps.x },
		{ "wind_body_y_mps", input.wind_velocity_body_mps.y },
		{ "wind_body_z_mps", input.wind_velocity_body_mps.z },
		{ "roll_acceleration_rad_s2", input.angular.roll_acceleration_rad_s2 },
		{ "pitch_acceleration_rad_s2", input.angular.pitch_acceleration_rad_s2 },
		{ "yaw_acceleration_rad_s2", input.angular.yaw_acceleration_rad_s2 },
		{ "roll_rate_rad_s", input.angular.roll_rate_rad_s },
		{ "pitch_rate_rad_s", input.angular.pitch_rate_rad_s },
		{ "yaw_rate_rad_s", input.angular.yaw_rate_rad_s },
		{ "world_yaw_rad", input.world_yaw_rad },
		{ "pitch_rad", input.pitch_rad },
		{ "roll_rad", input.roll_rad },
		{ "angle_of_attack_rad", input.angle_of_attack_rad },
		{ "angle_of_slide_rad", input.angle_of_slide_rad }
	} }, reporter);
}

bool validate_internal_fuel_input(double fuel_kg, EfmEventReporter& reporter)
{
	return validate_numeric_sample(
		{ "ed_fm_set_internal_fuel", { { "fuel_kg", fuel_kg } } },
		reporter);
}

bool validate_external_fuel_input(
	const Core::ExternalFuelInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_external_fuel", {
		{ "fuel_kg", input.fuel_kg },
		{ "position_body_x_m", input.position_body_m.x },
		{ "position_body_y_m", input.position_body_m.y },
		{ "position_body_z_m", input.position_body_m.z }
	} }, reporter);
}

bool validate_refueling_fuel_input(double fuel_kg, EfmEventReporter& reporter)
{
	return validate_numeric_sample(
		{ "ed_fm_refueling_add_fuel", { { "fuel_kg", fuel_kg } } },
		reporter);
}

bool validate_damage_input(double integrity, EfmEventReporter& reporter)
{
	if (!validate_numeric_sample(
		{ "ed_fm_on_damage", { { "integrity", integrity } } },
		reporter))
	{
		return false;
	}
	if (integrity < kMinimumDamageIntegrity ||
		integrity > kMaximumDamageIntegrity)
	{
		reporter.log_damage_integrity_out_of_range(integrity);
		return false;
	}
	return true;
}

bool validate_command_mapping(
	const CommandCallbackSample& sample,
	EfmEventReporter& reporter)
{
	switch (sample.mapping.status)
	{
	case DcsCommandMappingStatus::Mapped:
		return true;
	case DcsCommandMappingStatus::IgnoredRelease:
	case DcsCommandMappingStatus::IgnoredCommand:
		return false;
	case DcsCommandMappingStatus::UnknownCommand:
		reporter.log_unknown_command(sample.command_id, sample.value);
		return false;
	case DcsCommandMappingStatus::InvalidValue:
		reporter.log_invalid_numeric("ed_fm_set_command", "value", sample.value);
		return false;
	case DcsCommandMappingStatus::InvalidBindingTable:
		const char* reason = sample.mapping.table_validation.error ==
			CommandBindingError::DuplicateId ? "duplicate_id" : "invalid_rule";
		reporter.log_command_binding_error(
			reason, sample.mapping.table_validation.command_id);
		return false;
	}
	return false;
}

bool validate_simulation_event_input(
	const ed_fm_simulation_event& event,
	EfmEventReporter& reporter)
{
	if (event.event_type != ED_FM_EVENT_CARRIER_CATAPULT)
	{
		return true;
	}
	return validate_numeric_sample({ "ed_fm_push_simulation_event", {
		{ "event_params[0]", event.event_params[0] }
	} }, reporter);
}

bool validate_suspension_feedback(
	int index,
	const ed_fm_suspension_info* info,
	EfmEventReporter& reporter)
{
	if (info == nullptr || index < 0 ||
		static_cast<std::size_t>(index) >= Core::kFrameSuspensionWheelCount)
	{
		reporter.log_suspension_feedback_error(index, info == nullptr);
		return false;
	}
	return validate_numeric_sample({ "ed_fm_suspension_feedback", {
		{ "acting_force_x", info->acting_force[0] },
		{ "acting_force_y", info->acting_force[1] },
		{ "acting_force_z", info->acting_force[2] },
		{ "acting_force_point_x", info->acting_force_point[0] },
		{ "acting_force_point_y", info->acting_force_point[1] },
		{ "acting_force_point_z", info->acting_force_point[2] },
		{ "integrity_factor", info->integrity_factor },
		{ "struct_compression", info->struct_compression },
		{ "wheel_speed_x", info->wheel_speed_X }
	} }, reporter);
}

std::size_t required_draw_arg_count()
{
	return static_cast<std::size_t>(DcsIds::DrawArgs::AirbrakeTertiary) + 1;
}

bool validate_draw_args_buffer(
	const EdDrawArgument* draw_args,
	std::size_t size,
	EfmEventReporter& reporter)
{
	const std::size_t required = required_draw_arg_count();
	if (draw_args != nullptr && size >= required)
	{
		return true;
	}
	reporter.log_draw_args_buffer_error(draw_args == nullptr, size, required);
	return false;
}
}
}
