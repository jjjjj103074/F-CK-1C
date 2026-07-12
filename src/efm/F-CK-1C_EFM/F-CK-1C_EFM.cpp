// F-CK-1C EFM implementation.
#include "stdafx.h"
#include "F-CK-1C_EFM.h"
#include "Utility.h"
#include "Core/AircraftState.h"
#include "Core/Fck1cEfm.h"
#include "Core/ForceMoment.h"
#include "DcsBridge/AutopilotBridge.h"
#include "DcsBridge/ConfigReader.h"
#include "DcsBridge/CockpitParams.h"
#include "DcsBridge/DrawArgs.h"
#include "DcsBridge/ModulePaths.h"
#include "DcsBridge/ParamExport.h"
#include "DcsBridge/SimulationEvents.h"
#include "DcsIds/Commands.h"
#include "Diagnostics/DebugLogger.h"
#include "Diagnostics/DebugWatch.h"
#include "Diagnostics/RuntimeDiagnostics.h"
#include "Diagnostics/SuspensionDiagnostics.h"
#include "Systems/AirframeDeviceSystem.h"
#include "Systems/AerodynamicsSystem.h"
#include "Systems/DamageModel.h"
#include "Systems/EngineSystem.h"
#include "Systems/FBWController.h"
#include "Systems/FuelSystem.h"
#include "Systems/InputSystem.h"
#include "Systems/LandingGearSystem.h"
#include "Systems/StartupSystem.h"
#include "Systems/SuspensionSystem.h"
#include <Math.h>
#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <direct.h>
#include "include/Cockpit/CockpitAPI_Declare.h" // Lua/cockpit parameter bridge.
#include "include/FM/API_Declare.h"
#include "FM_data.h"

// EFM version metadata.
static const char* FCK1C_EFM_VERSION = "v0.1.3-april-fools";
static const char* FCK1C_EFM_VERSION_DATE = "2026-04-01";
// Version history:
// v0.1.0     - Initial module load and base structure.
// v0.1.1     - EFM integration and diagnostic mode switch.
// v0.1.2-dev - Version metadata and iteration tracking.
// v0.1.3-april-fools - April Fools build with experimental ground-contact tuning.
//
// EFMREF classification tags:
// - DCS_CONTRACT: exported ED callback/API surface. Keep name/signature stable.
// - DCS_BRIDGE: adapts DCS/Lua/config/draw-arg data into our internal state.
// - CUSTOM_SYSTEM: F-CK-1C simulation behavior that can be refactored behind tests.
// - COMMON_UTIL: reusable helper, candidate for Common/.
// - DIAGNOSTICS: logging/probe/debug output, should become optional and isolated.
// - LEGACY_CANDIDATE: unused or duplicated logic, review before keeping.
//
// EFMSTATE owner tags mark where current globals should eventually move.
// They are intentionally coarse groups so the next refactor can move state
// by ownership instead of moving one variable at a time.

static Core::AutopilotCommand read_autopilot_command();
static Core::MaxPowerCommand read_max_power_command();
static void observe_first_frame(const Core::Fck1cEfm& efm);
static void observe_engine_shutdown(const Core::Fck1cEfm& efm);
static void observe_thrust(const Core::Fck1cEfm& efm, const Core::MaxPowerCommand& command);
static void observe_ground_diagnostics(const Core::Fck1cEfm& efm, double dt);

namespace FM
{
static const size_t kFckPathMax = DcsBridge::kModulePathMax;
static DcsBridge::ModulePaths g_module_paths = { ".", "FM\\config.lua", false };

// EFMREF: DCS_BRIDGE - Provides the active FM/config.lua path to config readers.
static const char* active_fm_config_path()
{
	return DcsBridge::active_fm_config_path(g_module_paths);
}

// EFMREF: DCS_BRIDGE - Builds module-relative paths for diagnostics and config.
static void build_mod_path(char* out, size_t out_size, const char* relative)
{
	DcsBridge::build_mod_path(g_module_paths, out, out_size, relative);
}

// EFMREF: DIAGNOSTICS - Finds Saved Games log directory for debug output.
static bool resolve_saved_games_logs_dir(char* out, size_t out_size)
{
	return Common::resolve_saved_games_logs_dir(out, out_size);
}

class DcsEfmRuntime final : public Core::Fck1cEfmRuntime
{
public:
	Core::AutopilotCommand read_autopilot() override
	{
		return read_autopilot_command();
	}

	Core::MaxPowerCommand read_max_power() override
	{
		return read_max_power_command();
	}

	void on_first_frame(const Core::Fck1cEfm& efm) override
	{
		observe_first_frame(efm);
	}

	void on_engine_shutdown(const Core::Fck1cEfm& efm) override
	{
		observe_engine_shutdown(efm);
	}

	void on_thrust_updated(const Core::Fck1cEfm& efm, const Core::MaxPowerCommand& command) override
	{
		observe_thrust(efm, command);
	}

	void on_ground_diagnostics(const Core::Fck1cEfm& efm, double dt) override
	{
		observe_ground_diagnostics(efm, dt);
	}
};

static Core::Fck1cEfmConfig make_fck1c_efm_config()
{
	Core::Fck1cEfmConfig config;
	config.aerodynamics.wing_area = FM_DATA::wing_area;
	config.aerodynamics.wingspan = FM_DATA::wingspan;
	config.aerodynamics.length = FM_DATA::length;
	config.aerodynamics.height = FM_DATA::height;
	config.aerodynamics.mach_max = FM_DATA::mach_max;
	config.aerodynamics.cy_zero = FM_DATA::Cy0;
	config.aerodynamics.cz_beta = FM_DATA::Czbe;
	config.aerodynamics.cx_gear = FM_DATA::cx_gear;
	config.aerodynamics.cx_airbrake = FM_DATA::cx_brk;
	config.aerodynamics.cx_flap = FM_DATA::cx_flap;
	config.aerodynamics.cx_lift_k = FM_DATA::cx_lift_k;
	config.aerodynamics.cx_alpha_k = FM_DATA::cx_alpha_k;
	config.aerodynamics.cx_elevator_k = FM_DATA::cx_elevator_k;
	config.aerodynamics.cy_flap = FM_DATA::cy_flap;
	config.aerodynamics.airbrake_pitch_comp_k = FM_DATA::airbrake_pitch_comp_k;
	config.aerodynamics.mach_table = FM_DATA::mach_table;
	config.aerodynamics.cx_zero_table = FM_DATA::cx0;
	config.aerodynamics.cy_alpha_table = FM_DATA::Cya;
	config.aerodynamics.roll_rate_max_table = FM_DATA::OmxMax;
	config.aerodynamics.alpha_max_table = FM_DATA::Aldop;
	config.aerodynamics.cy_max_table = FM_DATA::CyMax;
	config.aerodynamics.table_size = FM_DATA::kAeroTableSize;
	config.engine.fuel_consumption = FM_DATA::fuel_consumption;
	config.engine.start_time = FM_DATA::engine_start_time;
	config.engine.spool_up_tau = FM_DATA::engine_spool_up_tau;
	config.engine.spool_down_tau = FM_DATA::engine_spool_down_tau;
	config.engine.mach_table = FM_DATA::engine_mach_table;
	config.engine.max_thrust_table = FM_DATA::max_thrust;
	config.engine.mach_table_size = FM_DATA::kEngineTableSize;
	config.engine.throttle_input_table = FM_DATA::throttle_input_table;
	config.engine.power_table = FM_DATA::engine_power_table;
	config.engine.throttle_table_size = sizeof(FM_DATA::throttle_input_table) / sizeof(float);
	config.left_engine_position = Vec3(-3.793, -0.391, -0.716);
	config.right_engine_position = Vec3(-3.793, -0.391, 0.716);
	return config;
}

// Single owner for DCS-neutral EFM state and configuration.
DcsEfmRuntime dcs_runtime;
Core::Fck1cEfm g_efm(make_fck1c_efm_config(), dcs_runtime);

// Lift and drag devices
enum FlapMode
{
	FLAP_MODE_UP = 0,
	FLAP_MODE_AUTO = 1,
	FLAP_MODE_DOWN = 2,
};

DcsBridge::CarrierLaunchState carrier_launch = {};
Diagnostics::SuspensionDiagnosticsState suspension_diagnostics;
Diagnostics::SuspensionDiagnosticsConfig suspension_diagnostics_config;
bool suspension_diagnostics_config_loaded = false;

// DLL-Lua interface
// EFMSTATE: DcsInterface/CockpitBridge - cockpit parameter API entrypoint.
EDPARAM interface;
}

using namespace FM;

static void dbg_susp(const char* msg);
static void susp_probe_log(const char* msg);
static void suspension_debug_log(const char* msg);

// EFMSTATE: DcsInterface/ModelBridge - model node-name mapping for suspension probes.
static const char* kSuspOriginalWheelNodes[3] = {
	"WHEEL_F",
	"WHEEL_L",
	"WHEEL_R"
};

static const char* kSuspModelViewerWheelNodes[3] = {
	"WHEEL_F",
	"WHEEL_L",
	"WHEEL_R"
};

// EFMREF: DCS_BRIDGE - Reads temporary FM/config.lua flags for suspension experiments.
static bool config_flag_is_true(const char* flag_name)
{
	return DcsBridge::config_flag_is_true(active_fm_config_path(), flag_name);
}

// EFMREF: DCS_BRIDGE - Reads temporary numeric FM/config.lua values for experiments.
static double config_number_or_default(const char* key_name, double default_value)
{
	return DcsBridge::config_number_or_default(active_fm_config_path(), key_name, default_value);
}

// EFMREF: DCS_BRIDGE - Reads temporary string FM/config.lua values for experiments.
static void config_string_or_default(const char* key_name, const char* default_value, char* out, size_t out_size)
{
	DcsBridge::config_string_or_default(active_fm_config_path(), key_name, default_value, out, out_size);
}

// EFMREF: DCS_BRIDGE - Suspension experiment config adapter; candidate for config cleanup.
static double active_susp_radius_add()
{
	if (!config_flag_is_true("SUSP_GEOMETRY_TEST"))
	{
		return 0.0;
	}

	return config_number_or_default("SUSP_GEOMETRY_TEST_RADIUS_ADD", 0.30);
}

// EFMREF: DCS_BRIDGE - Suspension experiment config adapter; candidate for config cleanup.
static double active_susp_wheel_y_offset()
{
	if (!config_flag_is_true("SUSP_GEOMETRY_TEST"))
	{
		return 0.0;
	}

	return config_number_or_default("SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET", -0.50);
}

// EFMREF: DCS_BRIDGE - Selects model node names used by DCS collision/suspension setup.
static void active_susp_node_names(const char*& nose, const char*& left, const char*& right)
{
	const bool use_modelviewer_nodes = config_flag_is_true("SUSP_USE_MODELVIEWER_WHEEL_NODES");
	const char** nodes = use_modelviewer_nodes ? kSuspModelViewerWheelNodes : kSuspOriginalWheelNodes;
	nose = nodes[0];
	left = nodes[1];
	right = nodes[2];
}

// EFMREF: DCS_BRIDGE - Converts FM/config.lua and suspension geometry into diagnostics-only config.
static void refresh_suspension_diagnostics_config()
{
	const Systems::SuspensionSystemConfig& suspension_config = g_efm.config().suspension;
	Diagnostics::SuspensionDiagnosticsConfig config;
	config.use_modelviewer_nodes = config_flag_is_true("SUSP_USE_MODELVIEWER_WHEEL_NODES");
	config.geometry_test = config_flag_is_true("SUSP_GEOMETRY_TEST");
	config.radius_add = active_susp_radius_add();
	config.wheel_y_offset = active_susp_wheel_y_offset();
	config_string_or_default(
		"SUSP_TEST_MARK",
		"SUSP_TEST_MARK_NOT_FOUND",
		config.test_mark,
		sizeof(config.test_mark));
	active_susp_node_names(
		config.wheel_nodes[0],
		config.wheel_nodes[1],
		config.wheel_nodes[2]);
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		config.final_wheel_radius[index] = suspension_config.fallback_wheel_radius[index] + config.radius_add;
		config.final_wheel_pos[index] = Systems::active_susp_wheel_pos(
			suspension_config,
			index,
			config.wheel_y_offset);
	}
	config.active_collision_shell = suspension_config.active_collision_shell_name;
	config.suspension_mode = suspension_config.suspension_mode_name;
	config.fallback_enabled = suspension_config.enable_fallback_ground_forces;
	config.build_date = __DATE__;
	config.build_time = __TIME__;
	suspension_diagnostics_config = config;
	suspension_diagnostics_config_loaded = true;
}

// EFMREF: CUSTOM_SYSTEM - Landing-gear/suspension state query; candidate for Systems/SuspensionSystem.
static inline bool has_suspension_feedback()
{
	return Systems::has_suspension_feedback(g_efm.systems().suspension);
}

// EFMREF: CUSTOM_SYSTEM - Weight-on-wheels state query; candidate for Systems/SuspensionSystem.
static inline bool any_wow()
{
	return Systems::any_wow(g_efm.systems().suspension);
}

// EFMREF: DIAGNOSTICS - Thin adapter used by DCS startup callbacks.
static void reset_startup_susp_probe_state()
{
	Diagnostics::reset_startup_suspension_probe(suspension_diagnostics);
}

// EFMREF: DCS_BRIDGE - Maps suspension feedback into draw-arg space.
static inline double suspension_visual_arg(int idx)
{
	const Core::Fck1cEfmSystems& systems = g_efm.systems();
	return Systems::suspension_visual_arg(
		systems.suspension,
		idx,
		systems.airframe_devices.gear_pos);
}

// EFMREF: DCS_BRIDGE - Builds a read-only diagnostics snapshot from current EFM state.
static Diagnostics::SuspensionDiagnosticsSnapshot make_suspension_diagnostics_snapshot(
	const Core::Fck1cEfm& efm)
{
	const Core::AircraftState& aircraft = efm.aircraft_state();
	const Core::Fck1cEfmSystems& systems = efm.systems();
	const Core::ControlSurfaceState& controls = efm.control_surfaces();
	Diagnostics::SuspensionDiagnosticsSnapshot snapshot;
	snapshot.simulation_time = systems.startup.simulation_time;
	snapshot.altitude_agl = aircraft.altitude_agl;
	snapshot.surface_height = aircraft.surface_height_raw;
	snapshot.surface_height_with_objects = aircraft.surface_height_with_objects;
	snapshot.surface_type = aircraft.surface_type_raw;
	snapshot.vertical_velocity = aircraft.velocity_world.y;
	snapshot.pitch_deg = Common::deg(aircraft.pitch);
	snapshot.roll_deg = Common::deg(aircraft.roll);
	snapshot.current_mass = aircraft.current_mass;
	snapshot.gear_pos = systems.airframe_devices.gear_pos;
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		snapshot.feedback_valid[index] = systems.suspension.feedback_valid[index];
		snapshot.wow[index] = systems.suspension.wow[index];
		snapshot.compression[index] = systems.suspension.compression[index];
		snapshot.force_vec[index] = systems.suspension.force_vec[index];
		snapshot.force_magnitude[index] = systems.suspension.force_mag[index];
		snapshot.fallback_wow[index] = systems.suspension.fallback_wow[index];
		snapshot.fallback_compression[index] = systems.suspension.fallback_compression[index];
	}
	snapshot.fallback_ground_force = systems.suspension.fallback_ground_force;
	snapshot.left_throttle_output = systems.engines.left.throttle_output;
	snapshot.right_throttle_output = systems.engines.right.throttle_output;
	snapshot.left_thrust = systems.engines.left.thrust_force;
	snapshot.right_thrust = systems.engines.right.thrust_force;
	snapshot.velocity_world = aircraft.velocity_world;
	snapshot.velocity_body = aircraft.velocity_body;
	snapshot.angular_velocity_world = aircraft.angular_velocity_world;
	snapshot.angular_velocity_body = aircraft.angular_velocity_body;
	snapshot.brake = systems.wheels.brake;
	snapshot.brake_left = systems.wheels.brake_left;
	snapshot.brake_right = systems.wheels.brake_right;
	snapshot.yaw_input = systems.primary_controls.yaw.input;
	snapshot.rudder_command = controls.rudder_command;
	snapshot.nose_wheel_command = Systems::compute_nose_wheel_steering(
		systems.wheels,
		systems.airframe_devices.gear_pos,
		aircraft.speed_scalar,
		systems.primary_controls.yaw.input);
	snapshot.nose_wheel_draw_arg = systems.wheels.nose_steering;
	snapshot.nose_turn_enabled = systems.wheels.nose_turn_enabled;
	return snapshot;
}

static Diagnostics::ThrustDiagnosticsSnapshot make_thrust_diagnostics_snapshot(
	const Core::Fck1cEfm& efm,
	double maxpower_ready,
	double maxpower_value)
{
	const Core::Fck1cEfmConfig& config = efm.config();
	const Core::Fck1cEfmSystems& systems = efm.systems();
	const Core::ForceMomentFrame& frame = efm.force_moment();
	Diagnostics::ThrustDiagnosticsSnapshot snapshot;
	snapshot.left_thrust = systems.engines.left.thrust_force;
	snapshot.right_thrust = systems.engines.right.thrust_force;
	const Vec3 left_force(snapshot.left_thrust, 0.0, 0.0);
	const Vec3 right_force(snapshot.right_thrust, 0.0, 0.0);
	const Vec3 left_moment = cross(config.left_engine_position, left_force);
	const Vec3 right_moment = cross(config.right_engine_position, right_force);
	snapshot.net_moment = Vec3(
		left_moment.x + right_moment.x + frame.moment.x,
		left_moment.y + right_moment.y + frame.moment.y,
		left_moment.z + right_moment.z + frame.moment.z);
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		snapshot.suspension_force[index] = systems.suspension.force_mag[index];
	}
	snapshot.maxpower_ready = maxpower_ready;
	snapshot.maxpower_value = maxpower_value;
	snapshot.left_engine_switch = systems.engines.left.switch_on;
	snapshot.right_engine_switch = systems.engines.right.switch_on;
	snapshot.left_throttle_input = systems.engines.left.throttle_input;
	snapshot.right_throttle_input = systems.engines.right.throttle_input;
	snapshot.left_throttle_output = systems.engines.left.throttle_output;
	snapshot.right_throttle_output = systems.engines.right.throttle_output;
	snapshot.left_power_readout = systems.engines.left.power_readout;
	snapshot.right_power_readout = systems.engines.right.power_readout;
	snapshot.left_wing_integrity = systems.damage.left_wing_integrity;
	snapshot.right_wing_integrity = systems.damage.right_wing_integrity;
	snapshot.left_engine_integrity = systems.damage.left_engine_integrity;
	snapshot.right_engine_integrity = systems.damage.right_engine_integrity;
	snapshot.internal_fuel = systems.fuel.internal_fuel;
	return snapshot;
}


// Cockpit/Lua parameter handles.
// EFMSTATE: DcsInterface/CockpitBridge - cockpit parameter handles.
DcsBridge::CockpitParamHandles cockpit_params = DcsBridge::make_cockpit_param_handles(interface);

// Autopilot parameter handles read from Lua autopilot_system.lua.
// EFMSTATE: DcsInterface/CockpitBridge - Lua autopilot command input handles.
DcsBridge::AutopilotParamHandles ap_params = DcsBridge::make_autopilot_param_handles(interface);

// AP state cached per frame.
// EFMSTATE: Systems/AutopilotBridge - cached Lua autopilot command state.
DcsBridge::AutopilotState ap_state = {};

// EFMREF: DCS_BRIDGE - Reads cockpit Lua AP parameters into cached EFM state.
static void update_autopilot_from_lua()
{
	DcsBridge::update_autopilot_from_lua(interface, ap_params, ap_state);
}

static Core::AutopilotCommand read_autopilot_command()
{
	update_autopilot_from_lua();
	Core::AutopilotCommand command;
	command.master = ap_state.master;
	command.bypass = ap_state.bypass;
	command.auto_throttle_engaged = ap_state.at_engaged;
	command.pitch_command = ap_state.pitch_cmd;
	command.roll_command = ap_state.roll_cmd;
	command.throttle_command = ap_state.throttle_cmd;
	return command;
}

static Core::MaxPowerCommand read_max_power_command()
{
	const DcsBridge::MaxPowerSwitchState state = DcsBridge::read_max_power_switch(interface, cockpit_params);
	Core::MaxPowerCommand command;
	command.ready = state.ready;
	command.value = state.value;
	return command;
}

static void observe_first_frame(const Core::Fck1cEfm& efm)
{
	(void)efm;
	refresh_suspension_diagnostics_config();
	Diagnostics::log_ground_configuration_once(
		suspension_diagnostics,
		suspension_diagnostics_config,
		[](const char* message) { dbg_susp(message); },
		[](const char* message) { susp_probe_log(message); });
}

static void observe_engine_shutdown(const Core::Fck1cEfm& efm)
{
	const Core::AircraftState& aircraft = efm.aircraft_state();
	const Core::Fck1cEfmSystems& systems = efm.systems();
	char shutdown_dbg[256];
	Diagnostics::format_engine_shutdown(
		shutdown_dbg,
		sizeof(shutdown_dbg),
		systems.fuel.internal_fuel,
		aircraft.altitude_asl,
		systems.engines.left.switch_on,
		systems.engines.right.switch_on);
	dbg_susp(shutdown_dbg);
}

static void observe_thrust(const Core::Fck1cEfm& efm, const Core::MaxPowerCommand& command)
{
	char dbgline[768];
	Diagnostics::format_thrust_diagnostics(
		dbgline,
		sizeof(dbgline),
		make_thrust_diagnostics_snapshot(efm, command.ready, command.value));
	dbg_susp(dbgline);
}

static void observe_ground_diagnostics(const Core::Fck1cEfm& efm, double dt)
{
	if (!suspension_diagnostics_config_loaded)
	{
		refresh_suspension_diagnostics_config();
	}
	const Diagnostics::SuspensionDiagnosticsSnapshot snapshot = make_suspension_diagnostics_snapshot(efm);
	Diagnostics::log_startup_suspension_probe(
		suspension_diagnostics,
		suspension_diagnostics_config,
		snapshot,
		dt,
		[](const char* message) { susp_probe_log(message); });
	Diagnostics::update_periodic_suspension_probe(
		suspension_diagnostics,
		suspension_diagnostics_config,
		snapshot,
		dt,
		[](const char* message) { susp_probe_log(message); });
	Diagnostics::update_periodic_ground_log(
		suspension_diagnostics,
		suspension_diagnostics_config,
		snapshot,
		[](const char* message) { dbg_susp(message); });
}

// EFMREF: DCS_CONTRACT - DCS force callback; keep exported name/signature stable.
void ed_fm_add_local_force(double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z)
{
	const Core::ForceMomentFrame& frame = g_efm.force_moment();
	x = frame.force.x;
	y = frame.force.y;
	z = frame.force.z;
	pos_x = frame.center_of_mass.x;
	pos_y = frame.center_of_mass.y;
	pos_z = frame.center_of_mass.z;
}

// EFMREF: DCS_CONTRACT - DCS moment callback; keep exported name/signature stable.
void ed_fm_add_local_moment(double& x, double& y, double& z)
{
	const Core::ForceMomentFrame& frame = g_efm.force_moment();
	x = frame.moment.x;
	y = frame.moment.y;
	z = frame.moment.z;
}

/*
// Unused, doesn't seem to work.
void ed_fm_add_global_force(double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z)
{

}
		fallback_ground_force = 0.0;

void ed_fm_add_global_moment(double & x,double &y,double &z)
{

}
*/

// The most important part of this whole thing.
// dt is apparently fixed to 0.006 seconds.
// EFMREF: DCS_CONTRACT - Main DCS simulation callback; future thin wrapper around Core/Fck1cEfm.
void ed_fm_simulate(double dt)
{
	g_efm.simulate(dt);
}

// Atmosphere data
// EFMREF: DCS_CONTRACT - DCS atmosphere/state input callback; keep signature stable.
void ed_fm_set_atmosphere(double h, //altitude above sea level
							double t, // current atmosphere temperature in Kelvin
							double a, // speed of sound
							double ro, // atmosphere density
							double p, // atmosphere pressure
							double wind_vx, double wind_vy, double wind_vz // components of velocity vector, including turbulence in world coordinate system
						)

{
	Core::set_atmosphere(g_efm.aircraft_state(), h, t, a, ro, wind_vx, wind_vy, wind_vz);

	// Export atmosphere temperature for cockpit/debug consumers.
	DcsBridge::export_temperature_param(interface, cockpit_params, t + 273);
}

// EFMREF: DCS_CONTRACT - DCS surface/terrain input callback; keep signature stable.
void ed_fm_set_surface(double h, // distance between sea level and the surface/ground
	double h_obj, // h but with objects
	unsigned surface_type, // type of surface under the aircraft?
	double normal_x, double normal_y, double normal_z // components of normal vector to surface
)
{
	Core::set_surface(g_efm.aircraft_state(), h, h_obj, surface_type);
}

// Called before simulation to set up your environment for the next step
// EFMREF: DCS_CONTRACT - DCS mass/COM input callback; keep signature stable.
void ed_fm_set_current_mass_state (double mass,
									double center_of_mass_x, double center_of_mass_y, double center_of_mass_z,
									double moment_of_inertia_x, double moment_of_inertia_y, double moment_of_inertia_z
									)
{
	Core::set_current_mass(g_efm.aircraft_state(), mass);
	Common::Vec3& center_of_mass = g_efm.force_moment().center_of_mass;
	center_of_mass.x = center_of_mass_x;
	center_of_mass.y = center_of_mass_y;
	center_of_mass.z = center_of_mass_z;
}

// Called before simulation to set up your environment for the next step
// EFMREF: DCS_CONTRACT - DCS world-axis state input callback; keep signature stable.
void ed_fm_set_current_state (double ax, double ay, double az,//linear acceleration component in world coordinate system
							double vx, double vy, double vz,//linear velocity component in world coordinate system
							double px, double py, double pz,//center of the body position in world coordinate system
							double omegadotx, double omegadoty, double omegadotz,//angular accelearation components in world coordinate system
							double omegax, double omegay, double omegaz, //angular velocity components in world coordinate system
							double quaternion_x, double quaternion_y, double quaternion_z, double quaternion_w //orientation quaternion components in world coordinate system
							)
{
	Core::set_world_kinematics(g_efm.aircraft_state(), vx, vy, vz, omegax, omegay, omegaz, pz);
}


// Called before simulation to set up your environment for the next step
// EFMREF: DCS_CONTRACT - DCS body-axis state input callback; keep signature stable.
void ed_fm_set_current_state_body_axis(double ax, double ay, double az,//linear acceleration components in body coordinate system
	double vx, double vy, double vz,//linear velocity components in body coordinate system
	double wind_vx, double wind_vy, double wind_vz,//wind linear velocity components in body coordinate system
	double omegadotx, double omegadoty, double omegadotz,//angular accelearation components in body coordinate system
	double omegax, double omegay, double omegaz,//angular velocity components in body coordinate system
	double yaw,  //radians
	double pitch,//radians
	double roll, //radians
	double common_angle_of_attack, //AoA radians
	double common_angle_of_slide   //AoS radians
	)
{
	// Positive aos is yaw left, negative is right.
	// Positive aos means more wind on the right wing, negative on the left wing.

	Core::set_body_kinematics(
		g_efm.aircraft_state(),
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

// Input handling
// EFMREF: DCS_CONTRACT - DCS input command callback; command IDs are a semi-fixed bridge.
void ed_fm_set_command (int command, float value)
{
	using namespace DcsIds::Commands;
	Core::Fck1cEfmSystems& systems = g_efm.systems();
	Systems::PrimaryControlState& primary_control_state = systems.primary_controls;
	Systems::FBWControllerState& fbw_controller = systems.fbw;
	Systems::EngineSystemState& engine_system = systems.engines;
	Systems::ThrottleInputState& throttle_input_state = systems.throttle_inputs;
	Systems::AirframeDeviceState& airframe_device_state = systems.airframe_devices;
	Systems::WheelState& wheel_state = systems.wheels;
	switch (command)
	{

	// Flight controls

	// Pitch

	case JoystickPitch: //iCommandPlanePitch
		Systems::set_pitch_axis_input(primary_control_state, value);
		break;

	case PitchUp:
		Systems::set_pitch_discrete_input(primary_control_state, 1);
		break;
	case PitchUpStop:
		Systems::set_pitch_discrete_input(primary_control_state, 0);
		break;

	case PitchDown:
		Systems::set_pitch_discrete_input(primary_control_state, -1);
		break;
	case PitchDownStop:
		Systems::set_pitch_discrete_input(primary_control_state, 0);
		break;

	case TrimUp:
		Systems::adjust_pitch_trim(primary_control_state, 0.0015);
		break;
	case TrimDown:
		Systems::adjust_pitch_trim(primary_control_state, -0.0015);
		break;

	// Roll

	case JoystickRoll: //iCommandPlaneRoll
		Systems::set_roll_axis_input(primary_control_state, value);
		break;

	case RollLeft:
		Systems::set_roll_discrete_input(primary_control_state, -1);
		break;
	case RollLeftStop:
		Systems::set_roll_discrete_input(primary_control_state, 0);
		break;

	case RollRight:
		Systems::set_roll_discrete_input(primary_control_state, 1);
		break;
	case RollRightStop:
		Systems::set_roll_discrete_input(primary_control_state, 0);
		break;

	case TrimLeft:
		Systems::adjust_roll_trim(primary_control_state, -0.001);
		break;
	case TrimRight:
		Systems::adjust_roll_trim(primary_control_state, 0.001);
		break;

	// Yaw

	case PedalYaw: //Yaw
		Systems::set_yaw_axis_input(primary_control_state, value);
		break;

	case RudderLeft:
		Systems::set_yaw_discrete_input(primary_control_state, 1);
		break;
	case RudderLeftStop:
		Systems::set_yaw_discrete_input(primary_control_state, 0);
		break;

	case RudderRight:
		Systems::set_yaw_discrete_input(primary_control_state, -1);
		break;
	case RudderRightStop:
		Systems::set_yaw_discrete_input(primary_control_state, 0);
		break;

	case RudderTrimLeft:
		Systems::adjust_yaw_trim(primary_control_state, 0.001);
		break;
	case RudderTrimRight:
		Systems::adjust_yaw_trim(primary_control_state, -0.001);
		break;

	case ResetTrim:
		Systems::reset_primary_trims(primary_control_state);
		break;

	case FBWCatToggle:
		if (value > 0.5f)
		{
			fbw_controller.mode_target = (fbw_controller.mode_target == Systems::FBW_CAT1) ? Systems::FBW_CAT3 : Systems::FBW_CAT1;
		}
		break;
	case FBWCat1:
		if (value > 0.5f)
		{
			fbw_controller.mode_target = Systems::FBW_CAT1;
		}
		break;
	case FBWCat3:
		if (value > 0.5f)
		{
			fbw_controller.mode_target = Systems::FBW_CAT3;
		}
		break;
	case FBWGLimiterOverride:
		fbw_controller.g_limiter_override = (value > 0.5f);
		break;
	case FBWGLimiterOverrideToggle:
		if (value > 0.5f)
		{
			fbw_controller.g_limiter_override = !fbw_controller.g_limiter_override;
		}
		break;

	//	Engine and throttle commands

	case EnginesOn: // Both engines
		Systems::set_both_engine_switches(engine_system, true);
		break;
	case LeftEngineOn:
		Systems::set_left_engine_switch(engine_system, true);
		break;
	case RightEngineOn:
		Systems::set_right_engine_switch(engine_system, true);
		break;

	case EnginesOff: // Both engines
		Systems::set_both_engine_switches(engine_system, false);
		break;
	case LeftEngineOff:
		Systems::set_left_engine_switch(engine_system, false);
		break;
	case RightEngineOff:
		Systems::set_right_engine_switch(engine_system, false);
		break;

	case ThrottleAxis://iCommandPlaneThrustCommon
		Systems::set_common_throttle_axis(throttle_input_state, value);
		break;
	case ThrottleAxisLeft:
		Systems::set_left_throttle_axis(throttle_input_state, value);
		break;
	case ThrottleAxisRight:
		Systems::set_right_throttle_axis(throttle_input_state, value);
		break;

	case ThrottleIncrease: // Both engines
		Systems::step_common_keyboard_throttle(throttle_input_state, 0.0075);
		break;
	case ThrottleLeftUp:
		Systems::step_left_keyboard_throttle(throttle_input_state, 0.0075);
		break;
	case ThrottleRightUp:
		Systems::step_right_keyboard_throttle(throttle_input_state, 0.0075);
		break;

	case ThrottleDecrease: // Both engines
		Systems::step_common_keyboard_throttle(throttle_input_state, -0.0075);
		break;
	case ThrottleLeftDown:
		Systems::step_left_keyboard_throttle(throttle_input_state, -0.0075);
		break;
	case ThrottleRightDown:
		Systems::step_right_keyboard_throttle(throttle_input_state, -0.0075);
		break;
	case ThrottleStop:
		// Release of keyboard throttle commands should stop the stepping action,
		// not snap the commanded throttle back to idle.
		break;

	// Other commands

	case AirBrakes: //toggle
		Systems::toggle_airbrake(airframe_device_state);
		break;
	case AirBrakesOff:
		Systems::set_airbrake(airframe_device_state, false);
		break;
	case AirBrakesOn:
		Systems::set_airbrake(airframe_device_state, true);
		break;
	case AirBrakesAuto:
		break;
	case AirBrakesUp:
		Systems::set_airbrake(airframe_device_state, false);
		break;
	case AirBrakesDown:
		Systems::set_airbrake(airframe_device_state, true);
		break;

	case FlapsToggle: //toggle
		Systems::toggle_flap_mode(airframe_device_state, FLAP_MODE_UP, FLAP_MODE_DOWN);
		break;
	case FlapsDown:
		Systems::set_flap_mode(airframe_device_state, FLAP_MODE_DOWN);
		break;
	case FlapsUp:
		Systems::set_flap_mode(airframe_device_state, FLAP_MODE_UP);
		break;
	case FlapsAuto:
		Systems::set_flap_mode(airframe_device_state, FLAP_MODE_AUTO);
		break;
	case FlapsUpCmd:
		Systems::set_flap_mode(airframe_device_state, FLAP_MODE_UP);
		break;
	case FlapsDownCmd:
		Systems::set_flap_mode(airframe_device_state, FLAP_MODE_DOWN);
		break;

	case GearToggle:
		Systems::toggle_gear(airframe_device_state);
		break;
	case GearDown:
		Systems::set_gear(airframe_device_state, true);
		break;
	case GearUp:
		Systems::set_gear(airframe_device_state, false);
		break;
	case GearAuto:
		break;
	case GearHandleUp:
		Systems::set_gear(airframe_device_state, false);
		break;
	case GearHandleDown:
		Systems::set_gear(airframe_device_state, true);
		break;
	case NoseTurnToggle:
		Systems::toggle_nose_turn_enabled(wheel_state, value > 0.5f);
		break;
	case NoseTurnUp:
		Systems::set_nose_turn_enabled(wheel_state, false);
		break;
	case NoseTurnAuto:
		Systems::set_nose_turn_enabled(wheel_state, false);
		break;
	case NoseTurnDown:
		Systems::set_nose_turn_enabled(wheel_state, true);
		break;
	case WheelBrakeAxis:
		Systems::set_brake_axis(wheel_state, Systems::normalize_brake_axis(value));
		break;
	case WheelBrakeAxisLeft:
		Systems::set_left_brake(wheel_state, Systems::normalize_brake_axis(value));
		break;
	case WheelBrakeAxisRight:
		Systems::set_right_brake(wheel_state, Systems::normalize_brake_axis(value));
		break;

	case WheelBrakeOn:
		Systems::set_brake_axis(wheel_state, 1.0);
		break;
	case WheelBrakeOff:
		Systems::set_brake_axis(wheel_state, 0.0);
		break;
	case WheelBrakeLeftOn:
		Systems::set_left_brake(wheel_state, 1.0);
		break;
	case WheelBrakeLeftOff:
		Systems::set_left_brake(wheel_state, 0.0);
		break;
	case WheelBrakeRightOn:
		Systems::set_right_brake(wheel_state, 1.0);
		break;
	case WheelBrakeRightOff:
		Systems::set_right_brake(wheel_state, 0.0);
		break;

	}

}

// EFMREF: DCS_CONTRACT - DCS mass-delta callback; fuel/mass internals can move behind it.
bool ed_fm_change_mass  (double & delta_mass,
						double & delta_mass_pos_x,
						double & delta_mass_pos_y,
						double & delta_mass_pos_z,
						double & delta_mass_moment_of_inertia_x,
						double & delta_mass_moment_of_inertia_y,
						double & delta_mass_moment_of_inertia_z
						)
{
	return Systems::change_mass(
		g_efm.systems().fuel,
		delta_mass,
		delta_mass_pos_x,
		delta_mass_pos_y,
		delta_mass_pos_z,
		delta_mass_moment_of_inertia_x,
		delta_mass_moment_of_inertia_y,
		delta_mass_moment_of_inertia_z);
}

// Set internal fuel volume , init function, called on object creation and for refueling
// EFMREF: DCS_CONTRACT - DCS internal fuel setter callback.
void   ed_fm_set_internal_fuel(double fuel)
{
	Systems::set_internal_fuel(g_efm.systems().fuel, fuel);
}

// Get internal fuel volume
// EFMREF: DCS_CONTRACT - DCS internal fuel getter callback.
double ed_fm_get_internal_fuel()
{
	return Systems::get_internal_fuel(g_efm.systems().fuel);
}

// Set external fuel volume for each payload station, called for weapon init and on reload.
// EFMREF: DCS_CONTRACT - DCS external fuel station callback.
void  ed_fm_set_external_fuel (int	 station,
								double fuel,
								double x, double y, double z)
{
	Systems::set_external_fuel(g_efm.systems().fuel, station, fuel, x, y, z);
}

// Get external fuel volume
// EFMREF: DCS_CONTRACT - DCS external fuel total callback.
double ed_fm_get_external_fuel ()
{
	return Systems::get_external_fuel(g_efm.systems().fuel);
}

// Drive model draw arguments for moving parts, lights, and visual effects.
// EFMREF: DCS_CONTRACT - DCS draw-arg callback; IDs should move to a generated/central map.
void ed_fm_set_draw_args (EdDrawArgument * drawargs,size_t size)
{
	const Core::Fck1cEfmSystems& systems = g_efm.systems();
	const Core::ControlSurfaceState& controls = g_efm.control_surfaces();
	const DcsBridge::DrawArgState state = {
		systems.airframe_devices.gear_pos,
		systems.wheels.nose_steering,
		controls.elevator_command,
		systems.airframe_devices.flaps_pos,
		controls.aileron_command,
		controls.rudder_command,
		systems.airframe_devices.airbrake_pos,
		systems.engines.left.afterburner_ratio,
		systems.engines.right.afterburner_ratio,
		systems.engines.right.nozzle_aperture,
		systems.engines.left.nozzle_aperture,
		systems.airframe_devices.slats_pos,
		{ systems.wheels.spin[0], systems.wheels.spin[1], systems.wheels.spin[2] }
	};

	DcsBridge::set_draw_args(drawargs, size, state);
}

// EFMREF: DCS_CONTRACT - DCS configure callback; currently seeds FM/config.lua path.
void ed_fm_configure(const char * cfg_path)
{
	DcsBridge::configure_module_paths(g_module_paths, cfg_path);
	refresh_suspension_diagnostics_config();
}

// Interface with default parameters like gear and engines
// EFMREF: DCS_CONTRACT - DCS parameter callback; index IDs are a semi-fixed bridge.
double ed_fm_get_param(unsigned index)
{
	const Core::AircraftState& aircraft = g_efm.aircraft_state();
	const Core::Fck1cEfmSystems& systems = g_efm.systems();
	const DcsBridge::ParamExportState state = {
		has_suspension_feedback(),
		any_wow(),
		systems.airframe_devices.gear_pos,
		systems.wheels.nose_steering,
		{ systems.wheels.spin[0], systems.wheels.spin[1], systems.wheels.spin[2] },
		systems.wheels.brake_left,
		systems.wheels.brake_right,
		systems.primary_controls.pitch.input,
		systems.primary_controls.roll.input,
		systems.primary_controls.yaw.input,
		systems.engines.left.switch_on,
		systems.engines.right.switch_on,
		systems.engines.left.throttle_input,
		systems.engines.right.throttle_input,
		systems.engines.left.throttle_output,
		systems.engines.right.throttle_output,
		systems.engines.left.power_readout,
		systems.engines.right.power_readout,
		systems.engines.left.thrust_force,
		systems.engines.right.thrust_force,
		aircraft.atmosphere_temperature,
		systems.fuel.internal_fuel,
		systems.fuel.total_fuel
	};

	return DcsBridge::get_param(index, state);
}

// EFMREF: DCS_CONTRACT - DCS refueling callback; currently unused.
void ed_fm_refueling_add_fuel(double fuel)
{
	// External refuel callback is currently unused.
}

// Infinite fuel setting.
// EFMREF: DCS_CONTRACT - DCS gameplay option callback.
void ed_fm_unlimited_fuel(bool value)
{
	g_efm.gameplay().infinite_fuel = value;
}

// Easy/game flight mode setting.
// EFMREF: DCS_CONTRACT - DCS gameplay option callback.
void ed_fm_set_easy_flight(bool value)
{
	g_efm.gameplay().easy_flight = value;
}

// Invincibility setting.
// EFMREF: DCS_CONTRACT - DCS gameplay option callback.
void ed_fm_set_immortal(bool value)
{
	g_efm.gameplay().invincible = value;
}

// Apply damage to aircraft subsystems.
// EFMREF: DCS_CONTRACT - DCS damage callback; maps DCS element IDs into DamageModel state.
void ed_fm_on_damage(int Element, double element_integrity_factor)
{
	Core::Fck1cEfmSystems& systems = g_efm.systems();
	const bool invincible = g_efm.gameplay().invincible;
	Systems::apply_damage(systems.damage, Element, element_integrity_factor, invincible);

	char buf[160];
	Diagnostics::format_damage_event(
		buf,
		sizeof(buf),
		Element,
		element_integrity_factor,
		invincible);
	dbg_susp(buf);

	suspension_debug_log(buf);
}

// EFMREF: DIAGNOSTICS - Module-local debug log writer; isolate behind Diagnostics.
static void dbg_susp(const char* msg)
{
	char debug_dir[kFckPathMax];
	build_mod_path(debug_dir, sizeof(debug_dir), "debug");
	Diagnostics::write_module_debug_log(debug_dir, msg);
}

// EFMREF: DIAGNOSTICS - Suspension probe log writer; isolate behind Diagnostics.
static void susp_probe_log(const char* msg)
{
	char log_dir[1024];
	if (!resolve_saved_games_logs_dir(log_dir, sizeof(log_dir)))
	{
		build_mod_path(log_dir, sizeof(log_dir), "debug");
	}
	Diagnostics::write_suspension_probe_log(
		log_dir,
		g_efm.systems().startup.simulation_time,
		msg);
}

// EFMREF: DIAGNOSTICS - Routes legacy suspension messages through the resolved probe-log path.
static void suspension_debug_log(const char* msg)
{
	susp_probe_log(msg);
}

// EFMREF: DCS_CONTRACT - DCS suspension feedback callback; bridges native gear physics into state.
void ed_fm_suspension_feedback(int idx, const ed_fm_suspension_info* info)
{
	if (info == nullptr)
	{
		susp_probe_log("suspension_feedback: invalid idx or null info");
		dbg_susp("suspension_feedback: invalid idx or null info");
		return;
	}

	const double fx = info->acting_force[0];
	const double fy = info->acting_force[1];
	const double fz = info->acting_force[2];
	Systems::SuspensionSystemState& suspension = g_efm.systems().suspension;
	if (!Systems::update_suspension_feedback(
		suspension,
		idx,
		info->struct_compression,
		fx,
		fy,
		fz))
	{
		susp_probe_log("suspension_feedback: invalid idx or null info");
		dbg_susp("suspension_feedback: invalid idx or null info");
		return;
	}

	char buf[512];
	Diagnostics::format_suspension_feedback(
		buf,
		sizeof(buf),
		idx,
		suspension.compression[idx],
		fx,
		fy,
		fz,
		suspension.force_mag[idx],
		suspension.wow[idx]);
	dbg_susp(buf);

	Diagnostics::format_suspension_animation(
		buf,
		sizeof(buf),
		idx,
		suspension.compression[idx],
		static_cast<float>(suspension_visual_arg(idx)),
		info->wheel_speed_X);
	suspension_debug_log(buf);
}

// Reset damage state after repair.
// EFMREF: DCS_CONTRACT - DCS repair callback; currently forwards to DamageModel reset.
void ed_fm_repair()
{
	Systems::reset_damage_model(g_efm.systems().damage);
}

// EFMREF: DCS_CONTRACT - DCS outbound event callback; currently handles carrier launch.
bool ed_fm_pop_simulation_event(ed_fm_simulation_event& out)
{
	return DcsBridge::pop_carrier_launch_event(
		carrier_launch,
		out,
		g_efm.systems().engines.left.throttle_output,
		FM_DATA::max_thrust[1] * 0.5 * 2);
}

// DCS calls this when an in-game simulation event occurs.
// EFMREF: DCS_CONTRACT - DCS inbound event callback; currently handles carrier launch state.
bool ed_fm_push_simulation_event(const ed_fm_simulation_event& in)
{
	return DcsBridge::push_carrier_launch_event(carrier_launch, in);
}


// Cold start on the ground.
// EFMREF: DCS_CONTRACT - DCS cold-start callback; should delegate to a StartupState/System reset.
void ed_fm_cold_start()
{
	reset_startup_susp_probe_state();
	Core::AircraftState& aircraft = g_efm.aircraft_state();
	Core::Fck1cEfmSystems& systems = g_efm.systems();
	Systems::configure_cold_ground_start(
		systems.startup,
		systems.damage,
		systems.suspension,
		systems.fbw,
		systems.wheels,
		systems.airframe_devices,
		systems.throttle_inputs,
		systems.engines,
		aircraft.roll,
		aircraft.pitch,
		aircraft.alpha,
		aircraft.g);
	DcsBridge::reset_carrier_launch_state(carrier_launch);
}

// Hot start on the ground.
// EFMREF: DCS_CONTRACT - DCS hot-start callback; should delegate to a StartupState/System reset.
void ed_fm_hot_start()
{
	reset_startup_susp_probe_state();
	Core::AircraftState& aircraft = g_efm.aircraft_state();
	Core::Fck1cEfmSystems& systems = g_efm.systems();
	Systems::configure_hot_ground_start(
		systems.startup,
		systems.damage,
		systems.suspension,
		systems.fbw,
		systems.wheels,
		systems.airframe_devices,
		systems.throttle_inputs,
		systems.engines,
		FLAP_MODE_DOWN,
		aircraft.roll,
		aircraft.pitch,
		aircraft.alpha,
		aircraft.g);
	DcsBridge::reset_carrier_launch_state(carrier_launch);
}

// Hot start in the air.
// EFMREF: DCS_CONTRACT - DCS air-start callback; should delegate to a StartupState/System reset.
void ed_fm_hot_start_in_air()
{
	reset_startup_susp_probe_state();
	Core::AircraftState& aircraft = g_efm.aircraft_state();
	Core::Fck1cEfmSystems& systems = g_efm.systems();
	Systems::configure_hot_air_start(
		systems.startup,
		systems.damage,
		systems.suspension,
		systems.fbw,
		systems.wheels,
		systems.airframe_devices,
		systems.throttle_inputs,
		systems.engines,
		aircraft.roll,
		aircraft.pitch,
		aircraft.alpha,
		aircraft.g);
	DcsBridge::reset_carrier_launch_state(carrier_launch);
}

// Mission exit cleanup.
// EFMREF: DCS_CONTRACT - DCS release callback; should delegate to Core cleanup/reset.
void ed_fm_release()
{
	Core::AircraftState& aircraft = g_efm.aircraft_state();
	Core::ControlSurfaceState& controls = g_efm.control_surfaces();
	Core::Fck1cEfmSystems& systems = g_efm.systems();
	Systems::configure_release(
		systems.startup,
		systems.suspension,
		systems.fbw,
		systems.primary_controls,
		systems.wheels,
		systems.throttle_inputs,
		systems.engines,
		aircraft.roll,
		aircraft.pitch,
		aircraft.alpha,
		aircraft.g,
		controls.elevator_command,
		controls.aileron_command,
		controls.rudder_command);
	DcsBridge::reset_autopilot_state(ap_state);
	ed_fm_repair();
}

// Cockpit view shaking.
// EFMREF: DCS_CONTRACT - DCS cockpit shake callback.
double ed_fm_get_shake_amplitude()
{
	return g_efm.gameplay().shake_amplitude;
}

// Optional force/moment component callbacks are not used by this FM.
// EFMREF: DCS_CONTRACT - Optional DCS component callback; intentionally unused.
bool ed_fm_add_local_force_component( double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z )
{
	return false;
}

// EFMREF: DCS_CONTRACT - Optional DCS component callback; intentionally unused.
bool ed_fm_add_global_force_component( double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z )
{
	return false;
}

// EFMREF: DCS_CONTRACT - Optional DCS component callback; intentionally unused.
bool ed_fm_add_local_moment_component( double & x,double &y,double &z )
{
	return false;
}

// EFMREF: DCS_CONTRACT - Optional DCS component callback; intentionally unused.
bool ed_fm_add_global_moment_component( double & x,double &y,double &z )
{
	return false;
}

// DCS debug-vector overlay is disabled for normal builds.
// EFMREF: DCS_CONTRACT - DCS debug-overlay enable callback.
bool ed_fm_enable_debug_info()
{
	return false;
}

// EFMREF: DCS_CONTRACT - DCS debug-watch callback; output formatting should move to Diagnostics.
size_t ed_fm_debug_watch(int level, char* buffer, size_t maxlen)
{
	const Core::AircraftState& aircraft = g_efm.aircraft_state();
	const Core::Fck1cEfmSystems& systems = g_efm.systems();
	Diagnostics::DebugWatchSnapshot snapshot;
	snapshot.version = FCK1C_EFM_VERSION;
	snapshot.version_date = FCK1C_EFM_VERSION_DATE;
	snapshot.altitude_asl = aircraft.altitude_asl;
	snapshot.altitude_agl = aircraft.altitude_agl;
	snapshot.position_world_z = aircraft.position_world_z;
	snapshot.gear_pos = systems.airframe_devices.gear_pos;
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		snapshot.wow[index] = systems.suspension.wow[index];
	}
	snapshot.wow_any = Systems::any_wow(systems.suspension);
	snapshot.wow_valid = Systems::has_suspension_feedback(systems.suspension);
	snapshot.on_ground = systems.suspension.on_ground;
	snapshot.fallback_ground_force = systems.suspension.fallback_ground_force;
	snapshot.fbw = &systems.fbw;
	return Diagnostics::format_debug_watch(level, snapshot, buffer, maxlen);
}
