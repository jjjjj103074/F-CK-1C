// DCS EFM ABI callbacks. Keep this file limited to contract adaptation.
#include "stdafx.h"
#include "F-CK-1C_EFM.h"
#include "Core/AircraftState.h"
#include "DcsBridge/DcsCommandRouter.h"
#include "DcsBridge/DcsModule.h"
#include "DcsBridge/DcsRuntime.h"
#include "DcsBridge/DcsSnapshots.h"
#include "DcsBridge/DrawArgs.h"
#include "DcsBridge/MassDelta.h"
#include "DcsBridge/ParamExport.h"
#include "Systems/DamageModel.h"
#include "Systems/FuelSystem.h"
#include "include/FM/API_Declare.h"

namespace
{
Core::Fck1cEfm& efm()
{
	return DcsBridge::efm();
}

DcsBridge::DcsRuntime& runtime()
{
	return DcsBridge::runtime();
}
}

void ed_fm_add_local_force(
	double& x,
	double& y,
	double& z,
	double& pos_x,
	double& pos_y,
	double& pos_z)
{
	const Core::ForceMomentFrame& frame = efm().force_moment();
	x = frame.force.x;
	y = frame.force.y;
	z = frame.force.z;
	pos_x = frame.center_of_mass.x;
	pos_y = frame.center_of_mass.y;
	pos_z = frame.center_of_mass.z;
}

void ed_fm_add_local_moment(double& x, double& y, double& z)
{
	const Core::ForceMomentFrame& frame = efm().force_moment();
	x = frame.moment.x;
	y = frame.moment.y;
	z = frame.moment.z;
}

void ed_fm_simulate(double dt)
{
	efm().simulate(dt);
}

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
	Core::set_atmosphere(efm().aircraft_state(), h, t, a, ro, wind_vx, wind_vy, wind_vz);
	runtime().export_temperature(t);
}

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
	Core::set_surface(efm().aircraft_state(), h, h_obj, surface_type);
}

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
	Core::set_current_mass(efm().aircraft_state(), mass);
	Common::Vec3& center_of_mass = efm().force_moment().center_of_mass;
	center_of_mass.x = center_of_mass_x;
	center_of_mass.y = center_of_mass_y;
	center_of_mass.z = center_of_mass_z;
}

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
	Core::set_world_kinematics(efm().aircraft_state(), vx, vy, vz, omegax, omegay, omegaz, pz);
}

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
	Core::set_body_kinematics(
		efm().aircraft_state(),
		vx,
		vy,
		vz,
		omegax,
		omegay,
		omegaz,
		yaw,
		pitch,
		roll,
		common_angle_of_attack,
		common_angle_of_slide,
		ay);
}

void ed_fm_set_command(int command, float value)
{
	DcsBridge::route_command(efm().systems(), command, value);
}

bool ed_fm_change_mass(
	double& delta_mass,
	double& delta_mass_pos_x,
	double& delta_mass_pos_y,
	double& delta_mass_pos_z,
	double& delta_mass_moment_of_inertia_x,
	double& delta_mass_moment_of_inertia_y,
	double& delta_mass_moment_of_inertia_z)
{
	const DcsBridge::MassDeltaResult result =
		DcsBridge::take_mass_delta(efm().systems().fuel);
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
	Systems::set_internal_fuel(efm().systems().fuel, fuel);
}

double ed_fm_get_internal_fuel()
{
	return Systems::get_internal_fuel(efm().systems().fuel);
}

void ed_fm_set_external_fuel(int station, double fuel, double x, double y, double z)
{
	Systems::set_external_fuel(efm().systems().fuel, station, fuel, x, y, z);
}

double ed_fm_get_external_fuel()
{
	return Systems::get_external_fuel(efm().systems().fuel);
}

void ed_fm_set_draw_args(EdDrawArgument* drawargs, size_t size)
{
	DcsBridge::set_draw_args(drawargs, size, DcsBridge::make_draw_arg_state(efm()));
}

void ed_fm_configure(const char* cfg_path)
{
	runtime().configure(cfg_path, efm());
}

double ed_fm_get_param(unsigned index)
{
	return DcsBridge::get_param(index, DcsBridge::make_param_export_state(efm()));
}

void ed_fm_refueling_add_fuel(double fuel)
{
	(void)fuel;
}

void ed_fm_unlimited_fuel(bool value)
{
	efm().gameplay().infinite_fuel = value;
}

void ed_fm_set_easy_flight(bool value)
{
	efm().gameplay().easy_flight = value;
}

void ed_fm_set_immortal(bool value)
{
	efm().gameplay().invincible = value;
}

void ed_fm_on_damage(int element, double integrity)
{
	Systems::apply_damage(
		efm().systems().damage,
		element,
		integrity,
		efm().gameplay().invincible);
	runtime().log_damage(efm(), element, integrity);
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
	return runtime().pop_simulation_event(efm(), out);
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
	return efm().gameplay().shake_amplitude;
}

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
	return runtime().debug_watch(efm(), level, { buffer, maxlen });
}
