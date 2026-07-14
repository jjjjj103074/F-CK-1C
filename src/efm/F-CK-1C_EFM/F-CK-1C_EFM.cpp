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
	const Core::ForceMomentFrame frame = efm().force_moment_output();
	x = frame.force.x;
	y = frame.force.y;
	z = frame.force.z;
	pos_x = frame.center_of_mass.x;
	pos_y = frame.center_of_mass.y;
	pos_z = frame.center_of_mass.z;
}

void ed_fm_add_local_moment(double& x, double& y, double& z)
{
	const Core::ForceMomentFrame frame = efm().force_moment_output();
	x = frame.moment.x;
	y = frame.moment.y;
	z = frame.moment.z;
}

void ed_fm_simulate(double dt)
{
	efm().simulate(dt);
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
	(void)p;
	efm().set_atmosphere({ h, t, a, ro, Common::Vec3(wind_vx, wind_vy, wind_vz) });
	runtime().export_temperature(t);
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
	(void)normal_x;
	(void)normal_y;
	(void)normal_z;
	efm().set_surface({ h, h_obj, surface_type });
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
	(void)moment_of_inertia_x;
	(void)moment_of_inertia_y;
	(void)moment_of_inertia_z;
	efm().set_mass_state({
		mass,
		Common::Vec3(center_of_mass_x, center_of_mass_y, center_of_mass_z)
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
	(void)ax;
	(void)ay;
	(void)az;
	(void)px;
	(void)py;
	(void)omegadotx;
	(void)omegadoty;
	(void)omegadotz;
	(void)quaternion_x;
	(void)quaternion_y;
	(void)quaternion_z;
	(void)quaternion_w;
	efm().set_world_kinematics({
		Common::Vec3(vx, vy, vz),
		Common::Vec3(omegax, omegay, omegaz),
		pz
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
	(void)ax;
	(void)az;
	(void)wind_vx;
	(void)wind_vy;
	(void)wind_vz;
	(void)omegadotx;
	(void)omegadoty;
	(void)omegadotz;
	efm().set_body_kinematics({
		Common::Vec3(vx, vy, vz),
		Common::Vec3(omegax, omegay, omegaz),
		yaw,
		pitch,
		roll,
		common_angle_of_attack,
		common_angle_of_slide,
		ay
	});
}

void ed_fm_set_command(int command, float value)
{
	const DcsBridge::DcsCommandMapping mapping = DcsBridge::map_command(command, value);
	if (mapping.mapped)
	{
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
	efm().set_internal_fuel(fuel);
}

double ed_fm_get_internal_fuel()
{
	return efm().internal_fuel();
}

// NOLINTNEXTLINE(readability-function-size): Signature is fixed by the DCS EFM ABI.
void ed_fm_set_external_fuel(int station, double fuel, double x, double y, double z)
{
	efm().set_external_fuel({ station, fuel, Common::Vec3(x, y, z) });
}

double ed_fm_get_external_fuel()
{
	return efm().external_fuel();
}

void ed_fm_set_draw_args(EdDrawArgument* drawargs, size_t size)
{
	DcsBridge::set_draw_args(drawargs, size, DcsBridge::make_draw_arg_state(efm().snapshot()));
}

void ed_fm_configure(const char* cfg_path)
{
	runtime().configure(cfg_path, efm().snapshot());
}

double ed_fm_get_param(unsigned index)
{
	return DcsBridge::get_param(index, DcsBridge::make_param_export_state(efm().snapshot()));
}

void ed_fm_refueling_add_fuel(double fuel)
{
	efm().add_refueling_fuel(fuel);
}

void ed_fm_unlimited_fuel(bool value)
{
	efm().set_infinite_fuel(value);
}

void ed_fm_set_easy_flight(bool value)
{
	efm().set_easy_flight(value);
}

void ed_fm_set_immortal(bool value)
{
	efm().set_invincible(value);
}

void ed_fm_on_damage(int element, double integrity)
{
	const DcsBridge::DcsDamageMapping mapping = DcsBridge::map_damage(element, integrity);
	if (mapping.mapped)
	{
		efm().apply_damage(mapping.event);
	}
	runtime().log_damage(efm().snapshot(), element, integrity);
}

void ed_fm_suspension_feedback(int index, const ed_fm_suspension_info* info)
{
	runtime().update_suspension_feedback(efm(), index, info);
}

void ed_fm_repair()
{
	efm().repair();
}

bool ed_fm_pop_simulation_event(ed_fm_simulation_event& out)
{
	return runtime().pop_simulation_event(
		efm().snapshot(),
		efm().max_dry_thrust_at(kCarrierLaunchReferenceMach),
		out);
}

bool ed_fm_push_simulation_event(const ed_fm_simulation_event& in)
{
	return runtime().push_simulation_event(in);
}

void ed_fm_cold_start()
{
	runtime().reset_startup_suspension_probe();
	efm().cold_start();
	runtime().reset_carrier_launch();
}

void ed_fm_hot_start()
{
	runtime().reset_startup_suspension_probe();
	efm().hot_ground_start();
	runtime().reset_carrier_launch();
}

void ed_fm_hot_start_in_air()
{
	runtime().reset_startup_suspension_probe();
	efm().hot_air_start();
	runtime().reset_carrier_launch();
}

void ed_fm_release()
{
	efm().release();
}

double ed_fm_get_shake_amplitude()
{
	return efm().shake_amplitude();
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
	(void)x;
	(void)y;
	(void)z;
	(void)pos_x;
	(void)pos_y;
	(void)pos_z;
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
	(void)x;
	(void)y;
	(void)z;
	(void)pos_x;
	(void)pos_y;
	(void)pos_z;
	return false;
}

bool ed_fm_add_local_moment_component(double& x, double& y, double& z)
{
	(void)x;
	(void)y;
	(void)z;
	return false;
}

bool ed_fm_add_global_moment_component(double& x, double& y, double& z)
{
	(void)x;
	(void)y;
	(void)z;
	return false;
}

bool ed_fm_enable_debug_info()
{
	return false;
}

size_t ed_fm_debug_watch(int level, char* buffer, size_t maxlen)
{
	return runtime().debug_watch(efm().snapshot(), level, { buffer, maxlen });
}
