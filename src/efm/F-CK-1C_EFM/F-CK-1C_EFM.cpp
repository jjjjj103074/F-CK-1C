// DCS EFM ABI callbacks. Keep this file limited to contract adaptation.
#include "stdafx.h"
#include "F-CK-1C_EFM.h"
#include "DcsBridge/DcsCommandRouter.h"
#include "DcsBridge/DcsDamageMapper.h"
#include "DcsBridge/DcsModule.h"
#include "DcsBridge/DcsRuntime.h"
#include "DcsBridge/DcsSnapshots.h"
#include "DcsBridge/DrawArgs.h"
#include "DcsBridge/ParamExport.h"
#include "include/FM/API_Declare.h"

namespace
{
constexpr double kCarrierLaunchReferenceMach = 0.1;

Core::Fck1cEfm& efm()
{
	return DcsBridge::efm();
}

DcsBridge::DcsRuntime& runtime()
{
	return DcsBridge::runtime();
}

DcsBridge::Internal::FrameInputCollector& input_collector()
{
	return DcsBridge::input_collector();
}

DcsBridge::Internal::OutputStore& output_store()
{
	return DcsBridge::output_store();
}

std::mutex& execution_mutex()
{
	return DcsBridge::execution_mutex();
}

void start_efm(Core::StartMode mode)
{
	{
		const std::lock_guard<std::mutex> lock(execution_mutex());
		input_collector().reset();
		output_store().publish(efm().start(mode));
	}
	runtime().reset_carrier_launch();
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
	const std::optional<Core::FrameOutput> output = output_store().read();
	if (!output)
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;
		pos_x = 0.0;
		pos_y = 0.0;
		pos_z = 0.0;
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
	const std::optional<Core::FrameOutput> output = output_store().read();
	if (!output)
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;
		return;
	}
	const Core::ForceMomentOutput& frame = output->force_moment;
	x = frame.moment.x;
	y = frame.moment.y;
	z = frame.moment.z;
}

void ed_fm_simulate(double dt)
{
	if (!Core::is_valid_frame_dt(dt))
	{
		return;
	}
	const Core::AutopilotCommand autopilot = runtime().read_autopilot();
	const Core::MaxPowerCommand max_power = runtime().read_max_power();
	Core::FrameOutput output;
	{
		const std::lock_guard<std::mutex> lock(execution_mutex());
		if (!output_store().read())
		{
			return;
		}
		input_collector().publish_autopilot(autopilot);
		input_collector().publish_max_power(max_power);
		output = efm().step(input_collector().snapshot(dt));
		output_store().publish(output);
	}
	runtime().export_temperature(output.flight.atmosphere_temperature_k);
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
	const Core::AtmosphereInput input = {
		h,
		t,
		a,
		ro,
		p,
		Common::Vec3(wind_vx, wind_vy, wind_vz)
	};
	input_collector().publish_atmosphere(input);
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
	const Core::SurfaceInput input = {
		h,
		h_obj,
		surface_type,
		Common::Vec3(normal_x, normal_y, normal_z)
	};
	input_collector().publish_surface(input);
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
	const Core::MassStateInput input = {
		mass,
		Common::Vec3(center_of_mass_x, center_of_mass_y, center_of_mass_z),
		Common::Vec3(moment_of_inertia_x, moment_of_inertia_y, moment_of_inertia_z)
	};
	input_collector().publish_mass(input);
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
	const Core::WorldKinematicsInput input = {
		Common::Vec3(ax, ay, az),
		Common::Vec3(vx, vy, vz),
		Common::Vec3(px, py, pz),
		Common::Vec3(omegadotx, omegadoty, omegadotz),
		Common::Vec3(omegax, omegay, omegaz),
		{ quaternion_x, quaternion_y, quaternion_z, quaternion_w }
	};
	input_collector().publish_world_kinematics(input);
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
	input_collector().publish_body_kinematics(input);
}

void ed_fm_set_command(int command, float value)
{
	const DcsBridge::DcsCommandMapping mapping = DcsBridge::map_command(command, value);
	if (mapping.mapped)
	{
		const std::lock_guard<std::mutex> lock(execution_mutex());
		if (output_store().is_released())
		{
			return;
		}
		efm().handle_command(mapping.command);
	}
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
	delta_mass = 0.0;
	delta_mass_pos_x = 0.0;
	delta_mass_pos_y = 0.0;
	delta_mass_pos_z = 0.0;
	delta_mass_moment_of_inertia_x = 0.0;
	delta_mass_moment_of_inertia_y = 0.0;
	delta_mass_moment_of_inertia_z = 0.0;
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (!output_store().read())
	{
		return false;
	}
	const Core::MassDeltaResult result = efm().take_mass_delta();
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
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (output_store().is_released())
	{
		return;
	}
	efm().set_internal_fuel(fuel);
}

double ed_fm_get_internal_fuel()
{
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (!output_store().read())
	{
		return 0.0;
	}
	return efm().internal_fuel();
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
void ed_fm_set_external_fuel(int station, double fuel, double x, double y, double z)
{
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (output_store().is_released())
	{
		return;
	}
	efm().set_external_fuel({ station, fuel, Common::Vec3(x, y, z) });
}

double ed_fm_get_external_fuel()
{
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (!output_store().read())
	{
		return 0.0;
	}
	return efm().external_fuel();
}

void ed_fm_set_draw_args(EdDrawArgument* drawargs, size_t size)
{
	const std::optional<Core::FrameOutput> output = output_store().read();
	if (output)
	{
		DcsBridge::set_draw_args(drawargs, size, DcsBridge::make_draw_arg_state(*output));
	}
}

void ed_fm_configure(const char* cfg_path)
{
	runtime().configure(cfg_path);
}

double ed_fm_get_param(unsigned index)
{
	const std::optional<Core::FrameOutput> output = output_store().read();
	if (!output)
	{
		return 0.0;
	}
	return DcsBridge::get_param(index, DcsBridge::make_param_export_state(*output));
}

void ed_fm_refueling_add_fuel(double fuel)
{
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (output_store().is_released())
	{
		return;
	}
	efm().add_refueling_fuel(fuel);
}

void ed_fm_unlimited_fuel(bool value)
{
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (output_store().is_released())
	{
		return;
	}
	efm().set_infinite_fuel(value);
}

void ed_fm_set_easy_flight(bool value)
{
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (output_store().is_released())
	{
		return;
	}
	efm().set_easy_flight(value);
}

void ed_fm_set_immortal(bool value)
{
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (output_store().is_released())
	{
		return;
	}
	efm().set_invincible(value);
}

void ed_fm_on_damage(int element, double integrity)
{
	const DcsBridge::DcsDamageMapping mapping = DcsBridge::map_damage(element, integrity);
	Core::Fck1cEfmSnapshot snapshot;
	{
		const std::lock_guard<std::mutex> lock(execution_mutex());
		if (output_store().is_released())
		{
			return;
		}
		if (mapping.mapped)
		{
			efm().apply_damage(mapping.event);
		}
		snapshot = efm().snapshot();
	}
	runtime().log_damage(snapshot, element, integrity);
}

void ed_fm_suspension_feedback(int index, const ed_fm_suspension_info* info)
{
	runtime().publish_suspension_feedback(input_collector(), index, info);
}

void ed_fm_repair()
{
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (output_store().is_released())
	{
		return;
	}
	efm().repair();
}

bool ed_fm_pop_simulation_event(ed_fm_simulation_event& out)
{
	Core::Fck1cEfmSnapshot snapshot;
	double max_dry_thrust = 0.0;
	{
		const std::lock_guard<std::mutex> lock(execution_mutex());
		if (output_store().is_released())
		{
			out = {};
			return false;
		}
		snapshot = efm().snapshot();
		max_dry_thrust = efm().max_dry_thrust_at(kCarrierLaunchReferenceMach);
	}
	return runtime().pop_simulation_event(snapshot, max_dry_thrust, out);
}

bool ed_fm_push_simulation_event(const ed_fm_simulation_event& in)
{
	return runtime().push_simulation_event(in);
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
	const std::lock_guard<std::mutex> lock(execution_mutex());
	if (output_store().is_released())
	{
		return;
	}
	efm().release();
	output_store().mark_released();
}

double ed_fm_get_shake_amplitude()
{
	const std::optional<Core::FrameOutput> output = output_store().read();
	return output ? output->shake_amplitude : 0.0;
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
	x = 0.0;
	y = 0.0;
	z = 0.0;
	return false;
}

bool ed_fm_add_global_moment_component(double& x, double& y, double& z)
{
	x = 0.0;
	y = 0.0;
	z = 0.0;
	return false;
}

bool ed_fm_enable_debug_info()
{
	return false;
}

size_t ed_fm_debug_watch(int level, char* buffer, size_t maxlen)
{
	(void)level;
	if (buffer != nullptr && maxlen > 0)
	{
		buffer[0] = '\0';
	}
	return 0;
}
