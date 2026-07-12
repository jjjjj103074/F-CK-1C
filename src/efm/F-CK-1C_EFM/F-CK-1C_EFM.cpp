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
	config.left_engine_position = Vec3(-3.793, -0.391, -0.716);
	config.right_engine_position = Vec3(-3.793, -0.391, 0.716);
	return config;
}

// Single owner for DCS-neutral EFM state and configuration.
Core::Fck1cEfm g_efm(make_fck1c_efm_config());
Core::ForceMomentFrame& force_moment_frame = g_efm.force_moment();
Core::Fck1cEfmSystems& efm_systems = g_efm.systems();

// Compatibility aliases remain until the simulation pipeline moves into Core.
Vec3& common_force = force_moment_frame.force;
Vec3& common_moment = force_moment_frame.moment;
Vec3& center_of_mass = force_moment_frame.center_of_mass;
Core::AircraftState& aircraft_state = g_efm.aircraft_state();
Vec3& wind = aircraft_state.wind;
Vec3& velocity_world = aircraft_state.velocity_world;
Vec3& velocity_body = aircraft_state.velocity_body;
Vec3& angular_velocity_world = aircraft_state.angular_velocity_world;
Vec3& angular_velocity_body = aircraft_state.angular_velocity_body;
Vec3& airspeed = aircraft_state.airspeed;

// EFMSTATE: Common/Units - shared constants; replace with Common/Units during utility extraction.
double const pi = 3.1415926535897932384626433832795;
double const rad_to_deg = 180.0 / pi;

// Engine RPM compatibility value; retained until the remaining engine readout bridge is cleaned up.
double idle_rpm = FM_DATA::idle_rpm / 100; // RPM % at idle throttle

const Systems::AerodynamicsSystemConfig& aerodynamics_config = g_efm.config().aerodynamics;
Systems::AerodynamicsSystemState& aerodynamics_system = efm_systems.aerodynamics;
const Vec3& left_engine_pos = g_efm.config().left_engine_position;
const Vec3& right_engine_pos = g_efm.config().right_engine_position;

// EFMSTATE: Systems/InputSystem - pilot primary control state.
Systems::PrimaryControlState& primary_control_state = efm_systems.primary_controls;
Core::ControlSurfaceState& control_surface_state = g_efm.control_surfaces();

// Compatibility aliases while call sites migrate to primary_control_state.
double& pitch_input = primary_control_state.pitch.input;
int&	pitch_discrete = primary_control_state.pitch.discrete;
bool&	pitch_analog = primary_control_state.pitch.analog;
double&	pitch_trim = primary_control_state.pitch.trim;
double& elevator_command = control_surface_state.elevator_command;

double& roll_input = primary_control_state.roll.input;
int&	roll_discrete = primary_control_state.roll.discrete;
bool&	roll_analog = primary_control_state.roll.analog;
double& roll_trim = primary_control_state.roll.trim;
double& aileron_command = control_surface_state.aileron_command;

double& yaw_input = primary_control_state.yaw.input;
int&	yaw_discrete = primary_control_state.yaw.discrete;
bool&	yaw_analog = primary_control_state.yaw.analog;
double&	yaw_trim = primary_control_state.yaw.trim;
double& rudder_command = control_surface_state.rudder_command;

// EFMSTATE: Systems/EngineSystem - engine switches, throttle, readout, thrust, AB, and nozzle state.
Systems::EngineSystemState& engine_system = efm_systems.engines;

// Compatibility aliases while call sites migrate to engine_system.
bool&	left_engine_switch = engine_system.left.switch_on;
double&	left_throttle_input = engine_system.left.throttle_input;
double&	left_throttle_output = engine_system.left.throttle_output;
double&	left_engine_power_readout = engine_system.left.power_readout;
double&	left_thrust_force = engine_system.left.thrust_force;

bool&	right_engine_switch = engine_system.right.switch_on;
double&	right_throttle_input = engine_system.right.throttle_input;
double&	right_throttle_output = engine_system.right.throttle_output;
double&	right_engine_power_readout = engine_system.right.power_readout;
double&	right_thrust_force = engine_system.right.thrust_force;

// EFMSTATE: Systems/InputSystem - throttle axis/keyboard arbitration state.
Systems::ThrottleInputState& throttle_input_state = efm_systems.throttle_inputs;

// Compatibility aliases while call sites migrate to throttle_input_state.
bool&	throttle_axis_inverted = throttle_input_state.axis_inverted; // true = axis forward -> larger throttle
double&	throttle_axis_cmd_left = throttle_input_state.left.axis_cmd;
double&	throttle_axis_cmd_right = throttle_input_state.right.axis_cmd;
double&	throttle_keyboard_cmd_left = throttle_input_state.left.keyboard_cmd;
double&	throttle_keyboard_cmd_right = throttle_input_state.right.keyboard_cmd;
bool&	throttle_use_axis_left = throttle_input_state.left.use_axis;
bool&	throttle_use_axis_right = throttle_input_state.right.use_axis;
double&	pilot_throttle_cmd_left = throttle_input_state.left.pilot_cmd;
double&	pilot_throttle_cmd_right = throttle_input_state.right.pilot_cmd;

// EFMSTATE: Systems/EngineSystem - final engine throttle command and afterburner model state.
double&	engine_throttle_cmd_left = engine_system.throttle_cmd_left; // Final command after pilot/FBW mixing
double&	engine_throttle_cmd_right = engine_system.throttle_cmd_right;
double&	afterburner_detent = engine_system.afterburner.detent;       // Throttle position where AB starts
double&	afterburner_thrust_factor = engine_system.afterburner.thrust_factor; // AB max thrust = dry thrust * factor
																		   // TFE1042-70: 2x46.7 kN AB / 2x27.0 kN MIL ~= 1.73
double&	afterburner_fuel_factor = engine_system.afterburner.fuel_factor;   // Fuel burn multiplier at full AB
double&	afterburner_core_rpm = engine_system.afterburner.core_rpm;      // Core RPM readout while in AB
double&	afterburner_core_drop_time = engine_system.afterburner.core_drop_time; // Seconds for core RPM transition between mil and AB
double&	left_afterburner_ratio = engine_system.left.afterburner_ratio;    // 0..1  (lagged, actual AB ratio driving thrust & nozzle)
double&	right_afterburner_ratio = engine_system.right.afterburner_ratio;   // 0..1
// Afterburner spool-lag state.
// lit flag becomes true only when the engine throttle_output has reached near-military power.
// Once lit, the ratio ramps in over ab_spool_in_tau; on extinguish it ramps out over ab_spool_out_tau.
bool&	left_afterburner_lit = engine_system.left.afterburner_lit;
bool&	right_afterburner_lit = engine_system.right.afterburner_lit;
double&	ab_spool_in_tau = engine_system.afterburner.spool_in_tau;   // ~2 s ramp from ignition to full AB
double&	ab_spool_out_tau = engine_system.afterburner.spool_out_tau;   // ~0.6 s to extinguish flame
double&	ab_light_throttle_output_min = engine_system.afterburner.light_throttle_output_min;  // throttle_output must reach this before AB can light

// Lift and drag devices
enum FlapMode
{
	FLAP_MODE_UP = 0,
	FLAP_MODE_AUTO = 1,
	FLAP_MODE_DOWN = 2,
};

// EFMSTATE: Systems/AirframeDeviceSystem - high-lift, speedbrake, and gear command/position state.
Systems::AirframeDeviceState& airframe_device_state = efm_systems.airframe_devices;

// Compatibility aliases while call sites migrate to airframe_device_state.
bool&	airbrake_switch = airframe_device_state.airbrake_switch;
double&	airbrake_pos = airframe_device_state.airbrake_pos;
double&	flaps_pos = airframe_device_state.flaps_pos;
int&	flap_mode = airframe_device_state.flap_mode;
double&	slats_pos = airframe_device_state.slats_pos;

// Landing gear
// EFMSTATE: Systems/LandingGearSystem - brake, wheel animation, and carrier launch state.
bool&	gear_switch = airframe_device_state.gear_switch;
double&	gear_pos = airframe_device_state.gear_pos;
Systems::WheelState& wheel_state = efm_systems.wheels;
DcsBridge::CarrierLaunchState carrier_launch = {};
double&	current_mass = aircraft_state.current_mass;

// EFMSTATE: Systems/FuelSystem - fuel quantities and pending DCS mass delta.
Systems::FuelSystem& fuel_system = efm_systems.fuel;

// EFMSTATE: Core/AircraftState - atmosphere, terrain, speed, aero angles, and derived flight state.
double& atmosphere_density = aircraft_state.atmosphere_density; // Atmosphere/air density (Pascals)
double&	altitude_ASL = aircraft_state.altitude_asl; // Altitude above sea level
double&	altitude_AGL = aircraft_state.altitude_agl; // Altitude above ground/surface level
double&	surface_height_raw = aircraft_state.surface_height_raw;
double&	surface_height_with_objects = aircraft_state.surface_height_with_objects;
unsigned& surface_type_raw = aircraft_state.surface_type_raw;
double&	position_world_z = aircraft_state.position_world_z; // World position Z for debug
double&	V_scalar = aircraft_state.speed_scalar; // Velocity scalar
double& speed_of_sound = aircraft_state.speed_of_sound; // Speed of sound (m/s)
double&	mach = aircraft_state.mach; // Air speed as a multiple of the speed of sound
double&	engine_alt_effect = aircraft_state.engine_alt_effect; // Multiplier of maximum thrust based on altitude

double& aoa = aircraft_state.aoa; // Angle of attack in radians
double& alpha = aircraft_state.alpha; // Angle of attack in degrees

double& aos = aircraft_state.aos; // Angle of slide in radians
double& beta = aircraft_state.beta; // Angle of slide in degrees

double& g = aircraft_state.g; // G force

double&	atmosphere_temperature = aircraft_state.atmosphere_temperature; // Current temperature in Kelvin

// EFMSTATE: Systems/SuspensionSystem - native/fallback suspension feedback and WOW state.
const Systems::SuspensionSystemConfig& suspension_config = g_efm.config().suspension;
Systems::SuspensionSystemState& suspension_system = efm_systems.suspension;

// Compatibility aliases while call sites migrate to suspension_system.
bool&	on_ground = suspension_system.on_ground; // Is the aircraft currently on the ground?
double&	fallback_ground_force = suspension_system.fallback_ground_force;

// Pitch
// EFMSTATE: Core/AircraftState - aircraft attitude and body-rate state.
double&	pitch = aircraft_state.pitch; // Pitch angle in radians
double&	pitch_rate = aircraft_state.pitch_rate;

// Roll
double&	roll = aircraft_state.roll; // Roll/bank angle in radians
double&	roll_rate = aircraft_state.roll_rate;

// Yaw/heading
double&	heading = aircraft_state.heading;
double&	yaw_rate = aircraft_state.yaw_rate;

// Damage stuff
// EFMSTATE: Systems/DamageModel - DCS damage element integrity and subsystem multipliers.
Systems::DamageModel& damage_model = efm_systems.damage;

// EFMREF: CUSTOM_SYSTEM - Damage model reset; candidate for Systems/DamageModel.
static void reset_damage_state()
{
	Systems::reset_damage_model(damage_model);
}

// Optional parameters set in the options menu.
// EFMSTATE: DcsInterface/GameOptions - gameplay option flags set by DCS callbacks.
Core::GameplayState& gameplay_state = g_efm.gameplay();
bool& invincible = gameplay_state.invincible; // No damage received if true
bool& infinite_fuel = gameplay_state.infinite_fuel; // No fuel drained if true
bool& easy_flight = gameplay_state.easy_flight; // Easier and more stable flight characteristics if true

// Cockpit/head shaking intensity.
// EFMSTATE: DcsInterface/CockpitOutput - cockpit shake value returned to DCS.
double& shake_amplitude = gameplay_state.shake_amplitude;

// EFMSTATE: Systems/StartupSystem - simulation clock, startup mode, and first-frame state.
Systems::StartupSystemState& startup_system = efm_systems.startup;
Diagnostics::SuspensionDiagnosticsState suspension_diagnostics;
Diagnostics::SuspensionDiagnosticsConfig suspension_diagnostics_config;
bool suspension_diagnostics_config_loaded = false;
double& left_nozzle_aperture = engine_system.left.nozzle_aperture;
double& right_nozzle_aperture = engine_system.right.nozzle_aperture;

// EFMSTATE: Systems/FBWController - FBW CAT tables, tuning config, and runtime state.
const Systems::FBWControllerConfig& fbw_config = g_efm.config().fbw;
Systems::FBWControllerState& fbw_controller = efm_systems.fbw;

// DLL-Lua interface
// EFMSTATE: DcsInterface/CockpitBridge - cockpit parameter API entrypoint.
EDPARAM interface;
}

using namespace FM;

// EFMREF: forward declarations mirror tagged definitions below.
void add_local_force(const Vec3 & Force, const Vec3 & Force_pos);
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
	return Systems::has_suspension_feedback(suspension_system);
}

// EFMREF: CUSTOM_SYSTEM - Weight-on-wheels state query; candidate for Systems/SuspensionSystem.
static inline bool any_wow()
{
	return Systems::any_wow(suspension_system);
}

// EFMREF: DIAGNOSTICS - Thin adapter used by DCS startup callbacks.
static void reset_startup_susp_probe_state()
{
	Diagnostics::reset_startup_suspension_probe(suspension_diagnostics);
}

// EFMREF: DCS_BRIDGE - Maps suspension feedback into draw-arg space.
static inline double suspension_visual_arg(int idx)
{
	return Systems::suspension_visual_arg(suspension_system, idx, gear_pos);
}

// EFMREF: CUSTOM_SYSTEM - Optional fallback ground-force model; candidate for Systems/SuspensionSystem.
static inline double apply_fallback_ground_forces()
{
	const Systems::SuspensionFallbackInput input = {
		altitude_AGL,
		pitch,
		roll,
		velocity_world.y,
		velocity_body.x,
		gear_pos,
		current_mass,
		left_throttle_input,
		right_throttle_input,
		left_thrust_force,
		right_thrust_force,
		wheel_state.brake_left,
		wheel_state.brake_right
	};

	return Systems::apply_fallback_ground_forces(
		suspension_system,
		suspension_config,
		input,
		[](const Common::Vec3& force, const Common::Vec3& pos)
		{
			add_local_force(force, pos);
		});
}

// EFMREF: CUSTOM_SYSTEM - Nose-wheel steering law; candidate for Systems/LandingGearSystem.
static inline double compute_nose_wheel_steering()
{
	return Systems::compute_nose_wheel_steering(wheel_state, gear_pos, V_scalar, yaw_input);
}

// EFMREF: DCS_BRIDGE - Builds a read-only diagnostics snapshot from current EFM state.
static Diagnostics::SuspensionDiagnosticsSnapshot make_suspension_diagnostics_snapshot()
{
	Diagnostics::SuspensionDiagnosticsSnapshot snapshot;
	snapshot.simulation_time = startup_system.simulation_time;
	snapshot.altitude_agl = altitude_AGL;
	snapshot.surface_height = surface_height_raw;
	snapshot.surface_height_with_objects = surface_height_with_objects;
	snapshot.surface_type = surface_type_raw;
	snapshot.vertical_velocity = velocity_world.y;
	snapshot.pitch_deg = Common::deg(pitch);
	snapshot.roll_deg = Common::deg(roll);
	snapshot.current_mass = current_mass;
	snapshot.gear_pos = gear_pos;
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		snapshot.feedback_valid[index] = suspension_system.feedback_valid[index];
		snapshot.wow[index] = suspension_system.wow[index];
		snapshot.compression[index] = suspension_system.compression[index];
		snapshot.force_vec[index] = suspension_system.force_vec[index];
		snapshot.force_magnitude[index] = suspension_system.force_mag[index];
		snapshot.fallback_wow[index] = suspension_system.fallback_wow[index];
		snapshot.fallback_compression[index] = suspension_system.fallback_compression[index];
	}
	snapshot.fallback_ground_force = suspension_system.fallback_ground_force;
	snapshot.left_throttle_output = engine_system.left.throttle_output;
	snapshot.right_throttle_output = engine_system.right.throttle_output;
	snapshot.left_thrust = engine_system.left.thrust_force;
	snapshot.right_thrust = engine_system.right.thrust_force;
	snapshot.velocity_world = aircraft_state.velocity_world;
	snapshot.velocity_body = aircraft_state.velocity_body;
	snapshot.angular_velocity_world = aircraft_state.angular_velocity_world;
	snapshot.angular_velocity_body = aircraft_state.angular_velocity_body;
	snapshot.brake = wheel_state.brake;
	snapshot.brake_left = wheel_state.brake_left;
	snapshot.brake_right = wheel_state.brake_right;
	snapshot.yaw_input = yaw_input;
	snapshot.rudder_command = rudder_command;
	snapshot.nose_wheel_command = compute_nose_wheel_steering();
	snapshot.nose_wheel_draw_arg = wheel_state.nose_steering;
	snapshot.nose_turn_enabled = wheel_state.nose_turn_enabled;
	return snapshot;
}

static Diagnostics::ThrustDiagnosticsSnapshot make_thrust_diagnostics_snapshot(
	double maxpower_ready,
	double maxpower_value)
{
	Diagnostics::ThrustDiagnosticsSnapshot snapshot;
	snapshot.left_thrust = left_thrust_force;
	snapshot.right_thrust = right_thrust_force;
	const Vec3 left_force(left_thrust_force, 0.0, 0.0);
	const Vec3 right_force(right_thrust_force, 0.0, 0.0);
	const Vec3 left_moment = cross(left_engine_pos, left_force);
	const Vec3 right_moment = cross(right_engine_pos, right_force);
	snapshot.net_moment = Vec3(
		left_moment.x + right_moment.x + common_moment.x,
		left_moment.y + right_moment.y + common_moment.y,
		left_moment.z + right_moment.z + common_moment.z);
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		snapshot.suspension_force[index] = suspension_system.force_mag[index];
	}
	snapshot.maxpower_ready = maxpower_ready;
	snapshot.maxpower_value = maxpower_value;
	snapshot.left_engine_switch = left_engine_switch;
	snapshot.right_engine_switch = right_engine_switch;
	snapshot.left_throttle_input = left_throttle_input;
	snapshot.right_throttle_input = right_throttle_input;
	snapshot.left_throttle_output = left_throttle_output;
	snapshot.right_throttle_output = right_throttle_output;
	snapshot.left_power_readout = left_engine_power_readout;
	snapshot.right_power_readout = right_engine_power_readout;
	snapshot.left_wing_integrity = damage_model.left_wing_integrity;
	snapshot.right_wing_integrity = damage_model.right_wing_integrity;
	snapshot.left_engine_integrity = damage_model.left_engine_integrity;
	snapshot.right_engine_integrity = damage_model.right_engine_integrity;
	snapshot.internal_fuel = fuel_system.internal_fuel;
	return snapshot;
}

// EFMREF: CUSTOM_SYSTEM - Pilot/FBW throttle blending; candidate for Systems/InputSystem or FBWController.
static inline double compose_engine_throttle_cmd(double pilot_cmd, double fbw_cmd)
{
	return Systems::compose_engine_throttle_cmd(
		pilot_cmd,
		fbw_cmd,
		fbw_controller.throttle_override,
		fbw_controller.throttle_blend);
}

// EFMREF: CUSTOM_SYSTEM - Applies throttle arbitration to engine commands.
static inline void update_engine_throttle_inputs_from_interface()
{
	Systems::update_pilot_throttle_cmds(throttle_input_state);
	Systems::apply_engine_throttle_commands(
		engine_system,
		compose_engine_throttle_cmd(pilot_throttle_cmd_left, fbw_controller.throttle_cmd_left),
		compose_engine_throttle_cmd(pilot_throttle_cmd_right, fbw_controller.throttle_cmd_right));
}

// EFMREF: CUSTOM_SYSTEM - Pilot axis/discrete input update; candidate for Systems/InputSystem.
static void update_primary_control_inputs()
{
	Systems::update_primary_control_inputs(primary_control_state);
}

// EFMREF: DCS_BRIDGE - Adapts FM globals into Systems/FBWController.
static void update_fbw_controller_from_fm_state(double dt, double qbar, double alpha_limit_deg)
{
	Systems::FBWControllerInput input;
	input.dt = dt;
	input.qbar = qbar;
	input.alpha_limit_deg = alpha_limit_deg;
	input.roll = roll;
	input.pitch = pitch;
	input.roll_rate = roll_rate;
	input.pitch_rate = pitch_rate;
	input.yaw_rate = yaw_rate;
	input.alpha = alpha;
	input.beta = beta;
	input.speed_scalar = V_scalar;
	input.mach = mach;
	input.g = g;
	input.roll_input = roll_input;
	input.roll_trim = roll_trim;
	input.pitch_input = pitch_input;
	input.pitch_trim = pitch_trim;
	input.yaw_input = yaw_input;
	input.yaw_trim = yaw_trim;
	input.gear_pos = gear_pos;
	input.wow = has_suspension_feedback() && any_wow();
	input.elevator_command = elevator_command;
	input.aileron_command = aileron_command;
	input.rudder_command = rudder_command;

	const Systems::FBWControllerOutput output = Systems::update_fbw_controller(fbw_controller, fbw_config, input);
	elevator_command = output.elevator_command;
	aileron_command = output.aileron_command;
	rudder_command = output.rudder_command;
}

// EFMREF: DCS_BRIDGE - Adapts current FM/system state into the aerodynamic model input.
static Systems::AerodynamicsFrameInput make_aerodynamics_frame_input()
{
	Systems::AerodynamicsFrameInput input;
	input.center_of_mass = center_of_mass;
	input.mach = mach;
	input.aoa = aoa;
	input.alpha_deg = alpha;
	input.aos = aos;
	input.roll = roll;
	input.pitch_rate = pitch_rate;
	input.roll_rate = roll_rate;
	input.yaw_rate = yaw_rate;
	input.elevator_command = elevator_command;
	input.aileron_command = aileron_command;
	input.rudder_command = rudder_command;
	input.airbrake_pos = airbrake_pos;
	input.flaps_pos = flaps_pos;
	input.gear_pos = gear_pos;
	input.left_wing_integrity = damage_model.left_wing_integrity;
	input.right_wing_integrity = damage_model.right_wing_integrity;
	input.tail_integrity = damage_model.tail_integrity;
	input.easy_flight = easy_flight;
	return input;
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

// EFMREF: CUSTOM_SYSTEM - Internal force accumulator helper; candidate for Core/ForceMoment.
// Add force
void add_local_force(const Vec3 & Force, const Vec3 & Force_pos)
{
	Core::add_local_force(common_force, common_moment, center_of_mass, Force, Force_pos);
}

// EFMREF: CUSTOM_SYSTEM - Internal moment accumulator helper; candidate for Core/ForceMoment.
// Add moment
void add_local_moment(const Vec3& Moment)
{
	Core::add_local_moment(common_moment, Moment);
}

// EFMREF: DCS_CONTRACT - DCS force callback; keep exported name/signature stable.
void ed_fm_add_local_force(double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z)
{
	x = common_force.x;
	y = common_force.y;
	z = common_force.z;
	pos_x = center_of_mass.x;
	pos_y = center_of_mass.y;
	pos_z = center_of_mass.z;
}

// EFMREF: DCS_CONTRACT - DCS moment callback; keep exported name/signature stable.
void ed_fm_add_local_moment(double& x, double& y, double& z)
{
	x = common_moment.x;
	y = common_moment.y;
	z = common_moment.z;
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

// Fuel consumption
// EFMREF: CUSTOM_SYSTEM - Fuel burn model; candidate for Systems/FuelSystem.
void simulate_fuel_consumption(double dt)
{
	Systems::simulate_fuel_consumption(
		fuel_system,
		dt,
		FM_DATA::fuel_consumption,
		left_throttle_output,
		right_throttle_output,
		left_afterburner_ratio,
		right_afterburner_ratio,
		afterburner_fuel_factor);
}

// The most important part of this whole thing.
// dt is apparently fixed to 0.006 seconds.
// EFMREF: DCS_CONTRACT - Main DCS simulation callback; future thin wrapper around Core/Fck1cEfm.
void ed_fm_simulate(double dt)
{
	Systems::advance_simulation_time(startup_system, dt);

	Core::reset_force_moment(common_force, common_moment);

	// Update the force positions to be relative to the center of mass.
	// Somewhat unrealistic, but if this isn't done it usually leads to really weird flight behaviour.
	if (!startup_system.first_frame_completed)
	{
		Systems::initialize_aerodynamic_force_positions(aerodynamics_system, aerodynamics_config, center_of_mass);
		refresh_suspension_diagnostics_config();
		Diagnostics::log_ground_configuration_once(
			suspension_diagnostics,
			suspension_diagnostics_config,
			[](const char* message) { dbg_susp(message); },
			[](const char* message) { susp_probe_log(message); });
	}

	Systems::update_airframe_device_positions(airframe_device_state, V_scalar, FLAP_MODE_DOWN, FLAP_MODE_AUTO);
	Systems::update_nose_wheel_steering(wheel_state, compute_nose_wheel_steering());

#pragma region AERODYNAMICS
	Core::update_airspeed(aircraft_state);

	const double ground_speed = Core::ground_speed(aircraft_state);
	Systems::update_wheel_spin(wheel_state, ground_speed, dt, gear_pos, altitude_AGL, suspension_config.fallback_wheel_radius, pi);

	// Many coefficients are not static, they change with mach.
	// Here, we use a linear interpolation (lerp for short) function for these coefficients.
	// See the definition of the lerp function in ED_FM_Utility.h for more info on how it works.

	Systems::update_aerodynamic_conditions(
		aerodynamics_system,
		aerodynamics_config,
		center_of_mass,
		atmosphere_density,
		V_scalar,
		mach,
		alpha,
		beta,
		slats_pos);
	const double AlphaMax_ = aerodynamics_system.alpha_max_deg;
	const double q = aerodynamics_system.dynamic_pressure;

	// Update pilot inputs first, then run FBW once per frame.
	update_primary_control_inputs();

	// Autopilot: read commands from Lua and blend with pilot inputs.
	// AP sits above the FBW: it replaces stick commands, and the FBW
	// processes them identically to pilot inputs (rate limiting, gain
	// scheduling, safety features all remain active).
	update_autopilot_from_lua();
	if (ap_state.master && !ap_state.bypass)
	{
		// AP overrides pitch/roll inputs; pilot trim remains additive.
		pitch_input = ap_state.pitch_cmd;
		roll_input  = ap_state.roll_cmd;
	}
	// A/T: use existing FBW throttle blend infrastructure
	if (ap_state.at_engaged)
	{
		fbw_controller.throttle_cmd_left = ap_state.throttle_cmd;
		fbw_controller.throttle_cmd_right = ap_state.throttle_cmd;
		fbw_controller.throttle_blend = 1.0;
		fbw_controller.throttle_override = false;
	}
	else
	{
		fbw_controller.throttle_blend = 0.0;
	}

	update_fbw_controller_from_fm_state(dt, q, AlphaMax_);

	const Systems::AerodynamicsFrameInput aerodynamics_input = make_aerodynamics_frame_input();
	Systems::apply_primary_aerodynamics(
		aerodynamics_system,
		aerodynamics_config,
		aerodynamics_input,
		[](const Vec3& force, const Vec3& force_pos)
		{
			add_local_force(force, force_pos);
		});
	#pragma endregion

	// ENGINE(S) AND THRUST //
#pragma region THRUST

	double max_dry_thrust = lerp(FM_DATA::engine_mach_table, FM_DATA::max_thrust, sizeof(FM_DATA::engine_mach_table) / sizeof(double), mach);

	// FBW/autothrottle interface point:
	// Keep pilot throttle path intact, then optionally blend/override with FBW command.
	update_engine_throttle_inputs_from_interface();

	Systems::clamp_engine_throttle_inputs(engine_system);

	Systems::update_dry_engine_channels(
		engine_system,
		dt,
		FM_DATA::engine_start_time,
		FM_DATA::throttle_input_table,
		FM_DATA::engine_power_table,
		sizeof(FM_DATA::throttle_input_table) / sizeof(float),
		FM_DATA::engine_spool_up_tau,
		FM_DATA::engine_spool_down_tau);

	// AB stage logic uses an ignition gate and spool lag.
	Systems::update_afterburners(engine_system, dt);

	// Nozzle schedule is driven by throttle_output, so it follows actual engine state rather than raw command.
	Systems::update_nozzle_apertures(engine_system, dt);

	Systems::update_engine_thrust_outputs(
		engine_system,
		max_dry_thrust,
		engine_alt_effect,
		damage_model.left_engine_integrity,
		damage_model.right_engine_integrity);
	Systems::apply_engine_readout_integrity(
		engine_system,
		damage_model.left_engine_integrity,
		damage_model.right_engine_integrity);

	// Engine shutdown
	if (Systems::should_shutdown_engines(fuel_system.internal_fuel, altitude_ASL))
	{
		char shutdown_dbg[256];
		Diagnostics::format_engine_shutdown(
			shutdown_dbg,
			sizeof(shutdown_dbg),
			fuel_system.internal_fuel,
			altitude_ASL,
			left_engine_switch,
			right_engine_switch);
		dbg_susp(shutdown_dbg);

		Systems::shutdown_engines(engine_system, dt);
	};

	// Apply the thrust cut switch only after the Lua side reports the param is initialised.
	// This prevents a missing/uninitialised cockpit device from silently killing all thrust.
	const DcsBridge::MaxPowerSwitchState maxpower = DcsBridge::read_max_power_switch(interface, cockpit_params);
	double maxpower_ready = maxpower.ready;
	double maxpower_val = maxpower.value;
	Systems::apply_thrust_cut(engine_system, maxpower_ready > 0.5 && maxpower_val < 0.5);

	// Apply thrust forces at engine positions
	add_local_force(Vec3(left_thrust_force, 0, 0), left_engine_pos);
	add_local_force(Vec3(right_thrust_force, 0, 0), right_engine_pos);

	// Structured diagnostics: thrust, net moment, suspension force, and engine state.
	{
		char dbgline[768];
		Diagnostics::format_thrust_diagnostics(
			dbgline,
			sizeof(dbgline),
			make_thrust_diagnostics_snapshot(maxpower_ready, maxpower_val));
		dbg_susp(dbgline);
	}

	if (infinite_fuel == false)
	{
		simulate_fuel_consumption(dt);
	};

#pragma endregion

	// MISC //
#pragma region MISC
	Systems::apply_aerodynamic_limiters(
		aerodynamics_system,
		aerodynamics_config,
		aerodynamics_input,
		[](const Vec3& force, const Vec3& force_pos)
		{
			add_local_force(force, force_pos);
		},
		[](const Vec3& moment)
		{
			add_local_moment(moment);
		});

	fallback_ground_force = 0.0;
	fallback_ground_force = apply_fallback_ground_forces();

	if (!suspension_diagnostics_config_loaded)
	{
		refresh_suspension_diagnostics_config();
	}
	const Diagnostics::SuspensionDiagnosticsSnapshot diagnostics_snapshot =
		make_suspension_diagnostics_snapshot();
	Diagnostics::log_startup_suspension_probe(
		suspension_diagnostics,
		suspension_diagnostics_config,
		diagnostics_snapshot,
		dt,
		[](const char* message) { susp_probe_log(message); });
	Diagnostics::update_periodic_suspension_probe(
		suspension_diagnostics,
		suspension_diagnostics_config,
		diagnostics_snapshot,
		dt,
		[](const char* message) { susp_probe_log(message); });
	Diagnostics::update_periodic_ground_log(
		suspension_diagnostics,
		suspension_diagnostics_config,
		diagnostics_snapshot,
		[](const char* message) { dbg_susp(message); });

	Systems::update_on_ground(suspension_system, gear_pos);

	shake_amplitude = Systems::update_aerodynamic_shake(
		aerodynamics_system,
		aerodynamics_config,
		aerodynamics_input,
		on_ground,
		g);

#pragma endregion

	Systems::mark_first_frame_completed(startup_system);
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
	Core::set_atmosphere(aircraft_state, h, t, a, ro, wind_vx, wind_vy, wind_vz);

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
	Core::set_surface(aircraft_state, h, h_obj, surface_type);
}

// Called before simulation to set up your environment for the next step
// EFMREF: DCS_CONTRACT - DCS mass/COM input callback; keep signature stable.
void ed_fm_set_current_mass_state (double mass,
									double center_of_mass_x, double center_of_mass_y, double center_of_mass_z,
									double moment_of_inertia_x, double moment_of_inertia_y, double moment_of_inertia_z
									)
{
	Core::set_current_mass(aircraft_state, mass);
	center_of_mass.x  = center_of_mass_x;
	center_of_mass.y  = center_of_mass_y;
	center_of_mass.z  = center_of_mass_z;
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
	Core::set_world_kinematics(aircraft_state, vx, vy, vz, omegax, omegay, omegaz, pz);
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
		aircraft_state,
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
		fuel_system,
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
	Systems::set_internal_fuel(fuel_system, fuel);
}

// Get internal fuel volume
// EFMREF: DCS_CONTRACT - DCS internal fuel getter callback.
double ed_fm_get_internal_fuel()
{
	return Systems::get_internal_fuel(fuel_system);
}

// Set external fuel volume for each payload station, called for weapon init and on reload.
// EFMREF: DCS_CONTRACT - DCS external fuel station callback.
void  ed_fm_set_external_fuel (int	 station,
								double fuel,
								double x, double y, double z)
{
	Systems::set_external_fuel(fuel_system, station, fuel, x, y, z);
}

// Get external fuel volume
// EFMREF: DCS_CONTRACT - DCS external fuel total callback.
double ed_fm_get_external_fuel ()
{
	return Systems::get_external_fuel(fuel_system);
}

// Drive model draw arguments for moving parts, lights, and visual effects.
// EFMREF: DCS_CONTRACT - DCS draw-arg callback; IDs should move to a generated/central map.
void ed_fm_set_draw_args (EdDrawArgument * drawargs,size_t size)
{
	const DcsBridge::DrawArgState state = {
		gear_pos,
		wheel_state.nose_steering,
		elevator_command,
		flaps_pos,
		aileron_command,
		rudder_command,
		airbrake_pos,
		left_afterburner_ratio,
		right_afterburner_ratio,
		right_nozzle_aperture,
		left_nozzle_aperture,
		slats_pos,
		{ wheel_state.spin[0], wheel_state.spin[1], wheel_state.spin[2] }
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
	const DcsBridge::ParamExportState state = {
		has_suspension_feedback(),
		any_wow(),
		gear_pos,
		wheel_state.nose_steering,
		{ wheel_state.spin[0], wheel_state.spin[1], wheel_state.spin[2] },
		wheel_state.brake_left,
		wheel_state.brake_right,
		pitch_input,
		roll_input,
		yaw_input,
		left_engine_switch,
		right_engine_switch,
		left_throttle_input,
		right_throttle_input,
		left_throttle_output,
		right_throttle_output,
		left_engine_power_readout,
		right_engine_power_readout,
		left_thrust_force,
		right_thrust_force,
		atmosphere_temperature,
		fuel_system.internal_fuel,
		fuel_system.total_fuel
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
	infinite_fuel = value;
}

// Easy/game flight mode setting.
// EFMREF: DCS_CONTRACT - DCS gameplay option callback.
void ed_fm_set_easy_flight(bool value)
{
	easy_flight = value;
}

// Invincibility setting.
// EFMREF: DCS_CONTRACT - DCS gameplay option callback.
void ed_fm_set_immortal(bool value)
{
	invincible = value;
}

// Apply damage to aircraft subsystems.
// EFMREF: DCS_CONTRACT - DCS damage callback; maps DCS element IDs into DamageModel state.
void ed_fm_on_damage(int Element, double element_integrity_factor)
{
	Systems::apply_damage(damage_model, Element, element_integrity_factor, invincible);

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
	Diagnostics::write_suspension_probe_log(log_dir, startup_system.simulation_time, msg);
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
	if (!Systems::update_suspension_feedback(
		suspension_system,
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
		suspension_system.compression[idx],
		fx,
		fy,
		fz,
		suspension_system.force_mag[idx],
		suspension_system.wow[idx]);
	dbg_susp(buf);

	Diagnostics::format_suspension_animation(
		buf,
		sizeof(buf),
		idx,
		suspension_system.compression[idx],
		static_cast<float>(suspension_visual_arg(idx)),
		info->wheel_speed_X);
	suspension_debug_log(buf);
}

// Reset damage state after repair.
// EFMREF: DCS_CONTRACT - DCS repair callback; currently forwards to DamageModel reset.
void ed_fm_repair()
{
	reset_damage_state();
}

// EFMREF: DCS_CONTRACT - DCS outbound event callback; currently handles carrier launch.
bool ed_fm_pop_simulation_event(ed_fm_simulation_event& out)
{
	return DcsBridge::pop_carrier_launch_event(
		carrier_launch,
		out,
		left_throttle_output,
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
	Systems::configure_cold_ground_start(
		startup_system,
		damage_model,
		suspension_system,
		fbw_controller,
		wheel_state,
		airframe_device_state,
		throttle_input_state,
		engine_system,
		roll,
		pitch,
		alpha,
		g);
	DcsBridge::reset_carrier_launch_state(carrier_launch);
}

// Hot start on the ground.
// EFMREF: DCS_CONTRACT - DCS hot-start callback; should delegate to a StartupState/System reset.
void ed_fm_hot_start()
{
	reset_startup_susp_probe_state();
	Systems::configure_hot_ground_start(
		startup_system,
		damage_model,
		suspension_system,
		fbw_controller,
		wheel_state,
		airframe_device_state,
		throttle_input_state,
		engine_system,
		FLAP_MODE_DOWN,
		roll,
		pitch,
		alpha,
		g);
	DcsBridge::reset_carrier_launch_state(carrier_launch);
}

// Hot start in the air.
// EFMREF: DCS_CONTRACT - DCS air-start callback; should delegate to a StartupState/System reset.
void ed_fm_hot_start_in_air()
{
	reset_startup_susp_probe_state();
	Systems::configure_hot_air_start(
		startup_system,
		damage_model,
		suspension_system,
		fbw_controller,
		wheel_state,
		airframe_device_state,
		throttle_input_state,
		engine_system,
		roll,
		pitch,
		alpha,
		g);
	DcsBridge::reset_carrier_launch_state(carrier_launch);
}

// Mission exit cleanup.
// EFMREF: DCS_CONTRACT - DCS release callback; should delegate to Core cleanup/reset.
void ed_fm_release()
{
	Systems::configure_release(
		startup_system,
		suspension_system,
		fbw_controller,
		primary_control_state,
		wheel_state,
		throttle_input_state,
		engine_system,
		roll,
		pitch,
		alpha,
		g,
		elevator_command,
		aileron_command,
		rudder_command);
	DcsBridge::reset_autopilot_state(ap_state);
	ed_fm_repair();
}

// Cockpit view shaking.
// EFMREF: DCS_CONTRACT - DCS cockpit shake callback.
double ed_fm_get_shake_amplitude()
{
	return shake_amplitude;
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
	Diagnostics::DebugWatchSnapshot snapshot;
	snapshot.version = FCK1C_EFM_VERSION;
	snapshot.version_date = FCK1C_EFM_VERSION_DATE;
	snapshot.altitude_asl = altitude_ASL;
	snapshot.altitude_agl = altitude_AGL;
	snapshot.position_world_z = position_world_z;
	snapshot.gear_pos = gear_pos;
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		snapshot.wow[index] = suspension_system.wow[index];
	}
	snapshot.wow_any = Systems::any_wow(suspension_system);
	snapshot.wow_valid = Systems::has_suspension_feedback(suspension_system);
	snapshot.on_ground = suspension_system.on_ground;
	snapshot.fallback_ground_force = suspension_system.fallback_ground_force;
	snapshot.fbw = &fbw_controller;
	return Diagnostics::format_debug_watch(level, snapshot, buffer, maxlen);
}
