// DCS EFM ABI callbacks. Keep this file limited to contract adaptation.
#include "../stdafx.h"
#include "../F-CK-1C_EFM.h"
#include "../Data/AircraftConfig.h"
#include "Internal/BoundaryValidator.h"
#include "Internal/BridgeContext.h"
#include "Internal/DcsCommandRouter.h"
#include "Internal/DcsDamageMapper.h"
#include "Internal/DrawArgs.h"
#include "../include/FM/API_Declare.h"

namespace
{
DcsBridge::Internal::BridgeContext& bridge(const char* initial_config_path = nullptr)
{
	static const int module_address_anchor = 0;
	// Process lifetime prevents destructor I/O and thread joins under loader lock.
	static auto* owner = new DcsBridge::Internal::BridgeContextOwner({
		ed_get_cockpit_param_api,
		Data::fck1c_aircraft_config(),
		&module_address_anchor
	});
	return owner->get(initial_config_path);
}

void ensure_module_initialized()
{
	(void)bridge();
}

void start_efm(Core::StartMode mode)
{
	ensure_module_initialized();
	Core::FrameOutput output;
	{
		const std::lock_guard<std::mutex> lock(bridge().execution_mutex());
		bridge().input_collector().reset();
		bridge().param_exporter().reset();
		bridge().carrier_bridge().reset();
		output = bridge().core().start(mode);
		bridge().output_store().publish(output);
		bridge().param_exporter().observe(output);
		bridge().state_csv_writer().publish_start(output);
	}
	bridge().event_reporter().log_start(mode, output.simulation_time_s);
}

Core::SuspensionFeedbackInput make_suspension_feedback(
	int index,
	const ed_fm_suspension_info& info)
{
	return {
		index,
		{ info.acting_force[0], info.acting_force[1], info.acting_force[2] },
		{ info.acting_force_point[0], info.acting_force_point[1],
			info.acting_force_point[2] },
		info.integrity_factor,
		info.struct_compression,
		info.wheel_speed_X
	};
}

}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
void ed_fm_add_local_force(
	double& x,
	double& y,
	double& z,
	double& pos_x,
	double& pos_y,
	double& pos_z)
{
	ensure_module_initialized();
	const std::optional<Core::FrameOutput> output = bridge().output_store().read();
	if (!output)
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;
		pos_x = 0.0;
		pos_y = 0.0;
		pos_z = 0.0;
		bridge().event_reporter().log_unavailable_output({ "ed_fm_add_local_force" });
		return;
	}
	const Core::ForceMomentOutput& frame = output->force_moment;
	x = frame.force.x;
	y = frame.force.y;
	z = frame.force.z;
	pos_x = frame.center_of_mass.x;
	pos_y = frame.center_of_mass.y;
	pos_z = frame.center_of_mass.z;
}

void ed_fm_add_local_moment(double& x, double& y, double& z)
{
	ensure_module_initialized();
	const std::optional<Core::FrameOutput> output = bridge().output_store().read();
	if (!output)
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;
		bridge().event_reporter().log_unavailable_output({ "ed_fm_add_local_moment" });
		return;
	}
	const Core::ForceMomentOutput& frame = output->force_moment;
	x = frame.moment.x;
	y = frame.moment.y;
	z = frame.moment.z;
}

void ed_fm_simulate(double dt)
{
	ensure_module_initialized();
	if (!Core::is_valid_frame_dt(dt))
	{
		bridge().event_reporter().log_invalid_frame_dt(dt);
		return;
	}
	if (!bridge().output_store().read())
	{
		bridge().event_reporter().log_unavailable_output({ "ed_fm_simulate" });
		return;
	}
	const DcsBridge::Internal::CockpitStepInput cockpit =
		bridge().cockpit_bridge().read_step_input();
	bridge().event_reporter().log_cockpit_parameter_events(cockpit.events);
	Core::FrameOutput output;
	bool output_available = false;
	{
		const std::lock_guard<std::mutex> lock(bridge().execution_mutex());
		output_available = bridge().output_store().read().has_value();
		if (output_available)
		{
			bridge().input_collector().publish_autopilot(cockpit.autopilot);
			bridge().input_collector().publish_max_power(cockpit.max_power);
			output = bridge().core().step(bridge().input_collector().snapshot(dt));
			bridge().output_store().publish(output);
			bridge().param_exporter().observe(output);
			bridge().state_csv_writer().publish_step(output);
		}
	}
	if (!output_available)
	{
		bridge().event_reporter().log_unavailable_output({ "ed_fm_simulate" });
		return;
	}
	bridge().event_reporter().log_cockpit_parameter_events(
		bridge().cockpit_bridge().export_temperature(
			output.flight.atmosphere_temperature_k));
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
void ed_fm_set_atmosphere(
	double h,
	double t,
	double a,
	double ro,
	double p,
	double wind_vx,
	double wind_vy,
	double wind_vz)
{
	ensure_module_initialized();
	const Core::AtmosphereInput input = {
		h,
		t,
		a,
		ro,
		p,
		Common::Vec3(wind_vx, wind_vy, wind_vz)
	};
	(void)bridge().perform_flight_action(
		{ "ed_fm_set_atmosphere" },
		[&input]()
		{
			if (!DcsBridge::Internal::validate_atmosphere_input(
				input, bridge().event_reporter())) return;
			bridge().input_collector().publish_atmosphere(input);
		});
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
void ed_fm_set_surface(
	double h,
	double h_obj,
	unsigned surface_type,
	double normal_x,
	double normal_y,
	double normal_z)
{
	ensure_module_initialized();
	const Core::SurfaceInput input = {
		h,
		h_obj,
		surface_type,
		Common::Vec3(normal_x, normal_y, normal_z)
	};
	(void)bridge().perform_flight_action(
		{ "ed_fm_set_surface" },
		[&input]()
		{
			if (!DcsBridge::Internal::validate_surface_input(
				input, bridge().event_reporter())) return;
			bridge().input_collector().publish_surface(input);
		});
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
void ed_fm_set_current_mass_state(
	double mass,
	double center_of_mass_x,
	double center_of_mass_y,
	double center_of_mass_z,
	double moment_of_inertia_x,
	double moment_of_inertia_y,
	double moment_of_inertia_z)
{
	ensure_module_initialized();
	const Core::MassStateInput input = {
		mass,
		Common::Vec3(center_of_mass_x, center_of_mass_y, center_of_mass_z),
		Common::Vec3(moment_of_inertia_x, moment_of_inertia_y, moment_of_inertia_z)
	};
	(void)bridge().perform_flight_action(
		{ "ed_fm_set_current_mass_state" },
		[&input]()
		{
			if (!DcsBridge::Internal::validate_mass_input(
				input, bridge().event_reporter())) return;
			bridge().input_collector().publish_mass(input);
		});
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
void ed_fm_set_current_state(
	double ax,
	double ay,
	double az,
	double vx,
	double vy,
	double vz,
	double px,
	double py,
	double pz,
	double omegadotx,
	double omegadoty,
	double omegadotz,
	double omegax,
	double omegay,
	double omegaz,
	double quaternion_x,
	double quaternion_y,
	double quaternion_z,
	double quaternion_w)
{
	ensure_module_initialized();
	const Core::WorldKinematicsInput input = {
		Common::Vec3(ax, ay, az),
		Common::Vec3(vx, vy, vz),
		Common::Vec3(px, py, pz),
		Common::Vec3(omegadotx, omegadoty, omegadotz),
		Common::Vec3(omegax, omegay, omegaz),
		{ quaternion_x, quaternion_y, quaternion_z, quaternion_w }
	};
	(void)bridge().perform_flight_action(
		{ "ed_fm_set_current_state" },
		[&input]()
		{
			if (!DcsBridge::Internal::validate_world_kinematics_input(
				input, bridge().event_reporter())) return;
			bridge().input_collector().publish_world_kinematics(input);
		});
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
void ed_fm_set_current_state_body_axis(
	double ax,
	double ay,
	double az,
	double vx,
	double vy,
	double vz,
	double wind_vx,
	double wind_vy,
	double wind_vz,
	double omegadotx,
	double omegadoty,
	double omegadotz,
	double omegax,
	double omegay,
	double omegaz,
	double yaw,
	double pitch,
	double roll,
	double common_angle_of_attack,
	double common_angle_of_slide)
{
	ensure_module_initialized();
	const Core::BodyKinematicsInput input = {
		Common::Vec3(ax, ay, az),
		Common::Vec3(vx, vy, vz),
		Common::Vec3(wind_vx, wind_vy, wind_vz),
		Common::Vec3(omegadotx, omegadoty, omegadotz),
		Common::Vec3(omegax, omegay, omegaz),
		yaw,
		pitch,
		roll,
		common_angle_of_attack,
		common_angle_of_slide
	};
	(void)bridge().perform_flight_action(
		{ "ed_fm_set_current_state_body_axis" },
		[&input]()
		{
			if (!DcsBridge::Internal::validate_body_kinematics_input(
				input, bridge().event_reporter())) return;
			bridge().input_collector().publish_body_kinematics(input);
		});
}

void ed_fm_set_command(int command, float value)
{
	ensure_module_initialized();
	(void)bridge().perform_core_action(
		{ "ed_fm_set_command", "command", command },
		[command, value](Core::Fck1cEfm& core)
		{
			const DcsBridge::DcsCommandMapping mapping =
				DcsBridge::map_command(command, value);
			if (!DcsBridge::Internal::validate_command_mapping(
				{ command, value, mapping }, bridge().event_reporter())) return;
			core.handle_command(mapping.command);
		});
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
bool ed_fm_change_mass(
	double& delta_mass,
	double& delta_mass_pos_x,
	double& delta_mass_pos_y,
	double& delta_mass_pos_z,
	double& delta_mass_moment_of_inertia_x,
	double& delta_mass_moment_of_inertia_y,
	double& delta_mass_moment_of_inertia_z)
{
	ensure_module_initialized();
	delta_mass = delta_mass_pos_x = delta_mass_pos_y = delta_mass_pos_z = 0.0;
	delta_mass_moment_of_inertia_x = delta_mass_moment_of_inertia_y = 0.0;
	delta_mass_moment_of_inertia_z = 0.0;
	const Core::MassDeltaResult result = bridge().take_flight_mass_delta();
	if (!result.available)
	{
		return false;
	}
	delta_mass = result.delta.mass;
	delta_mass_pos_x = result.delta.position.x;
	delta_mass_pos_y = result.delta.position.y;
	delta_mass_pos_z = result.delta.position.z;
	delta_mass_moment_of_inertia_x = result.delta.moment_of_inertia.x;
	delta_mass_moment_of_inertia_y = result.delta.moment_of_inertia.y;
	delta_mass_moment_of_inertia_z = result.delta.moment_of_inertia.z;
	return true;
}

void ed_fm_set_internal_fuel(double fuel)
{
	ensure_module_initialized();
	if (!DcsBridge::Internal::validate_internal_fuel_input(
		fuel, bridge().event_reporter())) return;
	bridge().perform_core_preparation(
		[fuel](Core::Fck1cEfm& core) { core.set_internal_fuel(fuel); });
}

double ed_fm_get_internal_fuel()
{
	ensure_module_initialized();
	return bridge().query_core_preparation(
		[](const Core::Fck1cEfm& core) { return core.internal_fuel(); });
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
void ed_fm_set_external_fuel(int station, double fuel, double x, double y, double z)
{
	ensure_module_initialized();
	const Core::ExternalFuelInput command = {
		station,
		fuel,
		Common::Vec3(x, y, z)
	};
	if (!DcsBridge::Internal::validate_external_fuel_input(
		command, bridge().event_reporter())) return;
	bridge().perform_core_preparation(
		[&command](Core::Fck1cEfm& core) { core.set_external_fuel(command); });
}

double ed_fm_get_external_fuel()
{
	ensure_module_initialized();
	return bridge().query_core_preparation(
		[](const Core::Fck1cEfm& core) { return core.external_fuel(); });
}

void ed_fm_set_draw_args(EdDrawArgument* drawargs, size_t size)
{
	ensure_module_initialized();
	const std::optional<Core::FrameOutput> output = bridge().output_store().read();
	if (!output)
	{
		bridge().event_reporter().log_unavailable_output({ "ed_fm_set_draw_args" });
		return;
	}
	if (!DcsBridge::Internal::validate_draw_args_buffer(
		drawargs, size, bridge().event_reporter())) return;
	DcsBridge::set_draw_args(drawargs, size, DcsBridge::make_draw_arg_state(*output));
}

void ed_fm_configure(const char* cfg_path)
{
	bridge(cfg_path).event_reporter().log_configure(cfg_path);
}

double ed_fm_get_param(unsigned index)
{
	ensure_module_initialized();
	const std::optional<Core::FrameOutput> output = bridge().output_store().read();
	if (!output)
	{
		bridge().event_reporter().log_unavailable_output(
			{ "ed_fm_get_param", "index", index });
		return 0.0;
	}
	return bridge().param_exporter().read(index, *output);
}

void ed_fm_refueling_add_fuel(double fuel)
{
	ensure_module_initialized();
	(void)bridge().perform_core_action(
		{ "ed_fm_refueling_add_fuel" },
		[fuel](Core::Fck1cEfm& core)
		{
			if (!DcsBridge::Internal::validate_refueling_fuel_input(
				fuel, bridge().event_reporter())) return;
			core.add_refueling_fuel(fuel);
		});
}

void ed_fm_unlimited_fuel(bool value)
{
	ensure_module_initialized();
	bridge().perform_core_preparation(
		[value](Core::Fck1cEfm& core) { core.set_infinite_fuel(value); });
}

void ed_fm_set_easy_flight(bool value)
{
	ensure_module_initialized();
	bridge().perform_core_preparation(
		[value](Core::Fck1cEfm& core) { core.set_easy_flight(value); });
}

void ed_fm_set_immortal(bool value)
{
	ensure_module_initialized();
	bridge().perform_core_preparation(
		[value](Core::Fck1cEfm& core) { core.set_invincible(value); });
}

void ed_fm_on_damage(int element, double integrity)
{
	ensure_module_initialized();
	Core::DamageApplyResult result;
	bool applied = false;
	const bool completed = bridge().perform_core_action(
		{ "ed_fm_on_damage", "element", element },
		[element, integrity, &result, &applied](Core::Fck1cEfm& core)
		{
			if (!DcsBridge::Internal::validate_damage_input(
				integrity, bridge().event_reporter())) return;
			const DcsBridge::DcsDamageMapping mapping =
				DcsBridge::map_damage(element, integrity);
			if (!mapping.mapped)
			{
				bridge().event_reporter().log_invalid_index(
					"ed_fm_on_damage", element);
				return;
			}
			result = core.apply_damage(mapping.event);
			applied = true;
		});
	if (completed && applied)
	{
		bridge().event_reporter().log_damage(result, element, integrity);
	}
}

void ed_fm_suspension_feedback(int index, const ed_fm_suspension_info* info)
{
	ensure_module_initialized();
	(void)bridge().perform_flight_action(
		{ "ed_fm_suspension_feedback", "index", index },
		[index, info]()
		{
			if (!DcsBridge::Internal::validate_suspension_feedback(
				index, info, bridge().event_reporter())) return;
			const Core::SuspensionFeedbackInput feedback =
				make_suspension_feedback(index, *info);
			if (!bridge().input_collector().publish_suspension(feedback))
			{
				bridge().event_reporter().log_suspension_feedback_error(
					index, false);
			}
		});
}

void ed_fm_repair()
{
	ensure_module_initialized();
	(void)bridge().perform_core_action(
		{ "ed_fm_repair" },
		[](Core::Fck1cEfm& core) { core.repair(); });
}

bool ed_fm_pop_simulation_event(ed_fm_simulation_event& out)
{
	ensure_module_initialized();
	out = {};
	bool popped = false;
	(void)bridge().perform_flight_action(
		{ "ed_fm_pop_simulation_event" },
		[&out, &popped]()
		{
			const std::optional<Core::FrameOutput> output =
				bridge().output_store().read();
			if (!output) return;
			popped = bridge().carrier_bridge().pop_event(
				{ output->engines[0].throttle_output }, out);
		});
	return popped;
}

bool ed_fm_push_simulation_event(const ed_fm_simulation_event& in)
{
	ensure_module_initialized();
	bool accepted = false;
	(void)bridge().perform_flight_action(
		{ "ed_fm_push_simulation_event" },
		[&in, &accepted]()
		{
			if (!DcsBridge::Internal::validate_simulation_event_input(
				in, bridge().event_reporter())) return;
			accepted = bridge().carrier_bridge().push_event(in);
		});
	return accepted;
}

void ed_fm_cold_start()
{
	start_efm(Core::StartMode::ColdGround);
}

void ed_fm_hot_start()
{
	start_efm(Core::StartMode::HotGround);
}

void ed_fm_hot_start_in_air()
{
	start_efm(Core::StartMode::HotAir);
}

void ed_fm_release()
{
	ensure_module_initialized();
	std::optional<double> final_simulation_time;
	bool released = false;
	{
		const std::lock_guard<std::mutex> lock(bridge().execution_mutex());
		released = bridge().output_store().is_released();
		if (!released)
		{
			const std::optional<Core::FrameOutput> output =
				bridge().output_store().read();
			if (output)
			{
				final_simulation_time = output->simulation_time_s;
			}
			bridge().core().release();
			bridge().output_store().mark_released();
			bridge().event_reporter().log_release(final_simulation_time);
		}
	}
	if (released)
	{
		bridge().event_reporter().log_callback_lifecycle_error(
			{ "ed_fm_release" },
			"released");
		return;
	}
}

double ed_fm_get_shake_amplitude()
{
	ensure_module_initialized();
	const std::optional<Core::FrameOutput> output = bridge().output_store().read();
	if (!output)
	{
		bridge().event_reporter().log_unavailable_output(
			{ "ed_fm_get_shake_amplitude" });
		return 0.0;
	}
	return output->shake_amplitude;
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
bool ed_fm_add_local_force_component(
	double& x,
	double& y,
	double& z,
	double& pos_x,
	double& pos_y,
	double& pos_z)
{
	ensure_module_initialized();
	x = 0.0;
	y = 0.0;
	z = 0.0;
	pos_x = 0.0;
	pos_y = 0.0;
	pos_z = 0.0;
	return false;
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
bool ed_fm_add_global_force_component(
	double& x,
	double& y,
	double& z,
	double& pos_x,
	double& pos_y,
	double& pos_z)
{
	ensure_module_initialized();
	x = 0.0;
	y = 0.0;
	z = 0.0;
	pos_x = 0.0;
	pos_y = 0.0;
	pos_z = 0.0;
	return false;
}

bool ed_fm_add_local_moment_component(double& x, double& y, double& z)
{
	ensure_module_initialized();
	x = 0.0;
	y = 0.0;
	z = 0.0;
	return false;
}

bool ed_fm_add_global_moment_component(double& x, double& y, double& z)
{
	ensure_module_initialized();
	x = 0.0;
	y = 0.0;
	z = 0.0;
	return false;
}

bool ed_fm_enable_debug_info()
{
	ensure_module_initialized();
	return false;
}

size_t ed_fm_debug_watch(int level, char* buffer, size_t maxlen)
{
	ensure_module_initialized();
	(void)level;
	if (buffer != nullptr && maxlen > 0)
	{
		buffer[0] = '\0';
	}
	return 0;
}
