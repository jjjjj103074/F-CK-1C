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
		{ "altitude", input.altitude_asl },
		{ "temperature", input.temperature },
		{ "speed_of_sound", input.speed_of_sound },
		{ "density", input.density },
		{ "pressure", input.pressure },
		{ "wind_x", input.wind.x },
		{ "wind_y", input.wind.y },
		{ "wind_z", input.wind.z }
	} }, reporter);
}

bool validate_surface_input(
	const Core::SurfaceInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_surface", {
		{ "height", input.surface_height },
		{ "object_height", input.surface_height_with_objects },
		{ "normal_x", input.normal.x },
		{ "normal_y", input.normal.y },
		{ "normal_z", input.normal.z }
	} }, reporter);
}

bool validate_mass_input(
	const Core::MassStateInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_current_mass_state", {
		{ "mass", input.mass },
		{ "center_of_mass_x", input.center_of_mass.x },
		{ "center_of_mass_y", input.center_of_mass.y },
		{ "center_of_mass_z", input.center_of_mass.z },
		{ "moment_of_inertia_x", input.moment_of_inertia.x },
		{ "moment_of_inertia_y", input.moment_of_inertia.y },
		{ "moment_of_inertia_z", input.moment_of_inertia.z }
	} }, reporter);
}

bool validate_world_kinematics_input(
	const Core::WorldKinematicsInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_current_state", {
		{ "acceleration_x", input.acceleration.x },
		{ "acceleration_y", input.acceleration.y },
		{ "acceleration_z", input.acceleration.z },
		{ "velocity_x", input.velocity.x },
		{ "velocity_y", input.velocity.y },
		{ "velocity_z", input.velocity.z },
		{ "position_x", input.position.x },
		{ "position_y", input.position.y },
		{ "position_z", input.position.z },
		{ "angular_acceleration_x", input.angular_acceleration.x },
		{ "angular_acceleration_y", input.angular_acceleration.y },
		{ "angular_acceleration_z", input.angular_acceleration.z },
		{ "angular_velocity_x", input.angular_velocity.x },
		{ "angular_velocity_y", input.angular_velocity.y },
		{ "angular_velocity_z", input.angular_velocity.z },
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
		{ "acceleration_x", input.acceleration.x },
		{ "acceleration_y", input.acceleration.y },
		{ "acceleration_z", input.acceleration.z },
		{ "velocity_x", input.velocity.x },
		{ "velocity_y", input.velocity.y },
		{ "velocity_z", input.velocity.z },
		{ "wind_x", input.wind_velocity.x },
		{ "wind_y", input.wind_velocity.y },
		{ "wind_z", input.wind_velocity.z },
		{ "angular_acceleration_x", input.angular_acceleration.x },
		{ "angular_acceleration_y", input.angular_acceleration.y },
		{ "angular_acceleration_z", input.angular_acceleration.z },
		{ "angular_velocity_x", input.angular_velocity.x },
		{ "angular_velocity_y", input.angular_velocity.y },
		{ "angular_velocity_z", input.angular_velocity.z },
		{ "yaw", input.heading },
		{ "pitch", input.pitch },
		{ "roll", input.roll },
		{ "angle_of_attack", input.angle_of_attack },
		{ "angle_of_slide", input.angle_of_slide }
	} }, reporter);
}

bool validate_internal_fuel_input(double fuel, EfmEventReporter& reporter)
{
	return validate_numeric_sample(
		{ "ed_fm_set_internal_fuel", { { "fuel", fuel } } },
		reporter);
}

bool validate_external_fuel_input(
	const Core::ExternalFuelInput& input,
	EfmEventReporter& reporter)
{
	return validate_numeric_sample({ "ed_fm_set_external_fuel", {
		{ "fuel", input.fuel },
		{ "position_x", input.position.x },
		{ "position_y", input.position.y },
		{ "position_z", input.position.z }
	} }, reporter);
}

bool validate_refueling_fuel_input(double fuel, EfmEventReporter& reporter)
{
	return validate_numeric_sample(
		{ "ed_fm_refueling_add_fuel", { { "fuel", fuel } } },
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
