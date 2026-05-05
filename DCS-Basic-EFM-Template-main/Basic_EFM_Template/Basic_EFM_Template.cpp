// Basic_EFM_Template.cpp : Defines the exported functions for the DLL application.
// This is essentially the main file.
#include "stdafx.h"
#include "Basic_EFM_Template.h"
#include "Utility.h"
#include <Math.h>
#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <direct.h>
#include "Inputs.h"
#include "include/Cockpit/CockpitAPI_Declare.h" // Provides param handle interfacing for use in lua
#include "include/FM/API_Declare.h"
#include "FM_data.h"

// F-CK-1C EFM version metadata
static const char* FCK1C_EFM_VERSION = "v0.1.3-april-fools";
static const char* FCK1C_EFM_VERSION_DATE = "2026-04-01";
// Version iteration (summary):
// v0.1.0     - Initial module load and base structure.
// v0.1.1     - EFM integration and diagnostic mode switch.
// v0.1.2-dev - Version metadata and iteration tracking.
// v0.1.3-april-fools - April Fools build with experimental ground-contact tuning.

namespace FM 
{
Vec3	common_force;
Vec3	common_moment;
Vec3    center_of_mass;
Vec3	wind;
Vec3	velocity_world;
Vec3	velocity_body;
Vec3	angular_velocity_world;
Vec3	angular_velocity_body;
Vec3	airspeed;

double	const	pi = 3.1415926535897932384626433832795;
double	const	rad_to_deg = 180.0 / pi;

// Defining aircraft stats here so we don't have to keep calling the FM_DATA namespace.
double S = FM_DATA::wing_area; // Wing area
double wingspan = FM_DATA::wingspan; // Wing span
double length = FM_DATA::length; // Overall length
double height = FM_DATA::height; // Overall height, not counting landing gear
double idle_rpm = FM_DATA::idle_rpm / 100; // RPM % at idle throttle

// Initializing force positions
// The positions are relative to the object's 3d model origin point.
Vec3 left_wing_pos(center_of_mass.x - 0.7, center_of_mass.y + 0.5, -wingspan / 2);
Vec3 right_wing_pos(center_of_mass.x - 0.7, center_of_mass.y + 0.5, wingspan / 2);
Vec3 tail_pos(center_of_mass.x - 0.5, center_of_mass.y, 0);

Vec3 elevator_pos(-length / 2, center_of_mass.y, 0);
Vec3 left_aileron_pos(center_of_mass.x, center_of_mass.y, -wingspan * 0.5);
Vec3 right_aileron_pos(center_of_mass.x, center_of_mass.y, wingspan * 0.5);
Vec3 rudder_pos(-length / 2, height / 2, 0);

// Greater Y and Z offsets create moments from the thrust force.
Vec3 left_engine_pos(-3.793, -0.391, -0.716); // Position (forward/back, up/down, left/right) of the first engine, usually left.
Vec3 right_engine_pos(-3.793, -0.391, 0.716); // Position of the second engine, usually right.

// Pitch variables
double  pitch_input = 0;
int		pitch_discrete = 0;
bool	pitch_analog = true;
double	pitch_trim = 0;
double	elevator_command = 0;

// Roll variables
double  roll_input = 0;
int		roll_discrete = 0;
bool	roll_analog = true;
double  roll_trim = 0;
double	aileron_command = 0;

// Yaw variables
double  yaw_input = 0;
int	yaw_discrete = 0;
bool	yaw_analog = true;
double	yaw_trim = 0;
double	rudder_command = 0;

// Left engine (# 1) variables
bool	left_engine_switch = false;
double  left_throttle_input = 0;
double	left_throttle_output = 0;
double	left_engine_power_readout = 0;
double	left_thrust_force = 0;

// Left engine (# 2) variables
bool	right_engine_switch = false;
double  right_throttle_input = 0;
double	right_throttle_output = 0;
double	right_engine_power_readout = 0;
double	right_thrust_force = 0;
bool	throttle_axis_inverted = true; // true = axis forward -> larger throttle
double	throttle_axis_cmd_left = 0.0;
double	throttle_axis_cmd_right = 0.0;
double	throttle_keyboard_cmd_left = 0.0;
double	throttle_keyboard_cmd_right = 0.0;
bool	throttle_use_axis_left = false;
bool	throttle_use_axis_right = false;
double	pilot_throttle_cmd_left = 0.0;
double	pilot_throttle_cmd_right = 0.0;
double	fbw_throttle_cmd_left = 0.0;   // Hook for future FBW/autothrottle
double	fbw_throttle_cmd_right = 0.0;  // Hook for future FBW/autothrottle
double	fbw_throttle_blend = 0.0;      // 0 = pilot only, 1 = FBW only
bool	fbw_throttle_override = false;  // true = force FBW throttle command
double	engine_throttle_cmd_left = 0.0; // Final command after pilot/FBW mixing
double	engine_throttle_cmd_right = 0.0;
double	afterburner_detent = 0.70;       // Throttle position where AB starts
double	afterburner_thrust_factor = 1.73; // AB max thrust = dry thrust * factor
											 // TFE1042-70: 2×46.7kN AB / 2×27.0kN MIL ≈ 1.73
double	afterburner_fuel_factor = 2.2;   // Fuel burn multiplier at full AB
double	afterburner_core_rpm = 0.94;      // Core RPM readout while in AB
double	afterburner_core_drop_time = 0.80; // Seconds for core RPM transition between mil and AB
double	left_afterburner_ratio = 0.0;    // 0..1  (lagged, actual AB ratio driving thrust & nozzle)
double	right_afterburner_ratio = 0.0;   // 0..1
// Afterburner spool-lag state.
// lit flag becomes true only when the engine throttle_output has reached near-military power.
// Once lit, the ratio ramps in over ab_spool_in_tau; on extinguish it ramps out over ab_spool_out_tau.
bool	left_afterburner_lit           = false;
bool	right_afterburner_lit          = false;
double	ab_spool_in_tau               = 2.0;   // ~2 s ramp from ignition to full AB
double	ab_spool_out_tau              = 0.6;   // ~0.6 s to extinguish flame
double	ab_light_throttle_output_min  = 0.88;  // throttle_output must reach this before AB can light
bool	fbw_g_limiter_override = false;

// Lift and drag devices
bool	airbrake_switch = false;
double	airbrake_pos = 0;
double	flaps_pos = 0;
enum FlapMode
{
	FLAP_MODE_UP = 0,
	FLAP_MODE_AUTO = 1,
	FLAP_MODE_DOWN = 2,
};
int		flap_mode = FLAP_MODE_UP;
double	slats_pos = 0;
bool	nose_turn_enabled = true;

// Landing gear
bool	gear_switch = false;
double	gear_pos = 0;
double	wheel_brake = 0; 
double	wheel_brake_left = 0;
double	wheel_brake_right = 0;
double	wheel_spin[3] = { 0.0, 0.0, 0.0 };
double	nose_wheel_steering = 0.0;
int	carrier_pos = 0;
double	current_mass = 9000.0;

double  internal_fuel = 0; // Amount of fuel in the aircraft (Kg)
double	external_fuel = 0; // Amount of fuel in external stations (Kg)
double	total_fuel = internal_fuel + external_fuel; // Total fuel amount (Kg)
double  fuel_consumption_since_last_time = 0;

double  atmosphere_density = 101000.0; // Atmosphere/air density (Pascals)
double	altitude_ASL = 0; // Altitude above sea level
double	altitude_AGL = 0; // Altitude above ground/surface level
double	surface_height_raw = 0;
double	surface_height_with_objects = 0;
unsigned surface_type_raw = 0;
double	position_world_z = 0; // World position Z for debug
double	V_scalar = 0; // Velocity scalar
double  speed_of_sound = 320; // Speed of sound (m/s)
double	mach = 0; // Air speed as a multiple of the speed of sound
double	engine_alt_effect = 1; // Multiplier of maximum thrust based on altitude

double  aoa = 0; // Angle of attack in radians
double  alpha = 0; // Angle of attack in degrees

double  aos = 0; // Angle of slide in radians
double  beta = 0; // Angle of slide in degrees

double  g = 0; // G force

double	atmosphere_temperature = 273; // Current temperature in Kelvin

bool	on_ground = false; // Is the aircraft currently on the ground?
double	suspension_compression[3] = { 0.0, 0.0, 0.0 };
Vec3	suspension_force_vec[3] = { Vec3(), Vec3(), Vec3() };
double	suspension_force_mag[3] = { 0.0, 0.0, 0.0 };
bool	suspension_wow[3] = { false, false, false };
bool	suspension_feedback_valid[3] = { false, false, false };
double	fallback_suspension_compression[3] = { 0.0, 0.0, 0.0 };
double	fallback_suspension_force_mag[3] = { 0.0, 0.0, 0.0 };
bool	fallback_suspension_wow[3] = { false, false, false };
double	fallback_ground_force = 0.0;

// Pitch
double	pitch = 0; // Pitch angle in radians
double	pitch_rate = 0;

// Roll
double	roll = 0; // Roll/bank angle in radians
double	roll_rate = 0;

// Yaw/heading
double	heading = 0;
double	yaw_rate = 0;

// Damage stuff
double element_integrity[111] = {};
double left_wing_integrity = 1.0;
double right_wing_integrity = 1.0;
double tail_integrity = 1.0;
double left_engine_integrity = 1.0;
double right_engine_integrity = 1.0;
double total_damage = 1 - (left_wing_integrity + right_wing_integrity + tail_integrity + 
						left_engine_integrity + right_engine_integrity) / 5;

static void reset_damage_state()
{
	for (int i = 0; i < 111; ++i)
	{
		element_integrity[i] = 1.0;
	}
	left_wing_integrity = 1.0;
	right_wing_integrity = 1.0;
	tail_integrity = 1.0;
	left_engine_integrity = 1.0;
	right_engine_integrity = 1.0;
	total_damage = 0.0;
}

// Optional parameters set in the options menu
bool invincible = false; // No damage received if true
bool infinite_fuel = false; // No fuel drained if true
bool easy_flight = false; // Easier and more stable flight characteristics if true

// Cockpit/head shaking intensity
double shake_amplitude = 0; 

// Basic timer
double fm_clock = 0; 

// Has the simulation passed frame 1?
bool sim_inititalised = false;
double left_nozzle_aperture = 0.80;
double right_nozzle_aperture = 0.80;

// 臨時用：專門記錄動畫與物理同步的獨立 Log
static void suspension_debug_log(const char* msg)
{
	// 檔案會獨立生成在你的 DCS Logs 資料夾下
	const char* path = "C:\\Users\\User\\Saved Games\\DCS\\Logs\\susp_debug.log";
	FILE* f = fopen(path, "a");
	if (!f) return;

	fprintf(f, "%s\n", msg);
	fclose(f);
}

static double remap_range(double value, double in_min, double in_max, double out_min, double out_max)
{
	if (in_max <= in_min)
	{
		return out_max;
	}

	const double normalized = limit((value - in_min) / (in_max - in_min), 0.0, 1.0);
	return out_min + (out_max - out_min) * normalized;
}

static double estimate_nozzle_aperture_target(double throttle_input, double engine_power_readout, double afterburner_ratio, bool engine_on)
{
	if (!engine_on)
	{
		return 0.80;
	}

	const double limited_power = limit(engine_power_readout, 0.0, 1.0);
	const double limited_throttle = limit(throttle_input, 0.0, 1.0);
	const double limited_ab = limit(afterburner_ratio, 0.0, 1.0);

	// Engine start / idle: bring the animation toward about 0.4 instead of staying fully open.
	if (limited_power < 0.50)
	{
		return remap_range(limited_power, 0.0, 0.50, 0.80, 0.40);
	}

	// Dry-power schedule in the model's actual animation space:
	// idle ~0.40, then progressively closes to 0.00 at max military power.
	if (limited_ab <= 0.0)
	{
		const double dry_ratio = limit(limited_throttle / afterburner_detent, 0.0, 1.0);

		if (dry_ratio <= 0.15)
		{
			return remap_range(dry_ratio, 0.0, 0.15, 0.40, 0.30);
		}
		if (dry_ratio <= 0.45)
		{
			return remap_range(dry_ratio, 0.15, 0.45, 0.30, 0.18);
		}
		if (dry_ratio <= 0.75)
		{
			return remap_range(dry_ratio, 0.45, 0.75, 0.18, 0.08);
		}

		return remap_range(dry_ratio, 0.75, 1.0, 0.08, 0.00);
	}

	// Afterburner opens back up from military power, but starts gently.
	if (limited_ab <= 0.25)
	{
		return remap_range(limited_ab, 0.0, 0.25, 0.00, 0.18);
	}
	if (limited_ab <= 0.60)
	{
		return remap_range(limited_ab, 0.25, 0.60, 0.18, 0.55);
	}

	return remap_range(limited_ab, 0.60, 1.0, 0.55, 1.0);
}

static double update_nozzle_aperture(double current, double target, double engine_power_readout, double afterburner_ratio, bool engine_on, double dt)
{
	double aperture_rate = 0.50;

	// About one second from engine-start spool toward idle geometry.
	if (!engine_on || engine_power_readout < 0.50)
	{
		aperture_rate = 0.40;
	}
	// Slow the opening surge when entering AB so it follows the engine more naturally.
	else if (afterburner_ratio > 0.0 && target > current)
	{
		aperture_rate = 0.45;
	}
	// Let closure toward military power stay deliberate rather than snappy.
	else if (target < current)
	{
		aperture_rate = 0.35;
	}

	return actuator(current, target, -aperture_rate * dt, aperture_rate * dt);
}

enum FBWCatMode
{
	FBW_CAT1 = 0,
	FBW_CAT3 = 1
};

enum FBWControlState
{
	FBW_STATE_RATE = 0,
	FBW_STATE_HOLD = 1,
	FBW_STATE_DEGRADE = 2
};

enum FBWHoldExitReason
{
	FBW_HOLD_EXIT_NONE = 0,
	FBW_HOLD_EXIT_STICK = 1,
	FBW_HOLD_EXIT_AOA = 2,
	FBW_HOLD_EXIT_QBAR = 3,
	FBW_HOLD_EXIT_ACTUATOR_SAT = 4,
	FBW_HOLD_EXIT_HOLD_CMD = 5
};

struct FBWCatParams
{
	double deadband;
	double hold_engage_time;
	double hold_phi_kp;
	double hold_theta_kp;
	double hold_p_cmd_max;
	double hold_q_cmd_max;
	double alpha_hold_degrade_deg;
	double qbar_min_hold;
	double sat_time;
	double hold_cmd_ratio_limit;
	double hold_decay_tau;
	double command_shape_tau;
	double command_shape_rate;
	double stick_expo;
	double p_cmd_max;
	double q_cmd_max;
	double r_cmd_max;
	double aoa_soft_deg;
	double aoa_hard_deg;
	double g_soft;
	double g_hard;
	double p_rate_limit;
	double q_rate_limit;
	double r_rate_limit;
	double yaw_damper_beta;
	double yaw_damper_r;
};

struct FBWGainSchedulePoint
{
	double qbar;
	double cmd_gain;
	double hold_gain;
	double damping_gain;
	double limiter_gain;
};

struct FBWGainScheduleValues
{
	double cmd_gain;
	double hold_gain;
	double damping_gain;
	double limiter_gain;
};

FBWCatParams fbw_cat1 = {
	0.03, 0.18, 2.8, 2.2, rad(55.0), rad(42.0), 15.5, 2500.0, 0.35, 0.85, 0.65, 0.05, 9.5, 0.10,
	rad(190.0), rad(145.0), rad(80.0), 15.0, 21.0, 6.4, 8.8, rad(220.0), rad(170.0), rad(95.0), 0.90, 0.60
};

FBWCatParams fbw_cat3 = {
	0.05, 0.26, 2.0, 1.6, rad(40.0), rad(30.0), 13.5, 4000.0, 0.22, 0.70, 0.40, 0.10, 5.5, 0.20,
	rad(140.0), rad(110.0), rad(60.0), 12.5, 17.5, 5.8, 7.6, rad(170.0), rad(130.0), rad(75.0), 1.10, 0.80
};

FBWGainSchedulePoint fbw_gain_schedule[] = {
	{ 1500.0, 1.15, 1.20, 1.15, 0.82 },
	{ 5000.0, 1.05, 1.05, 1.00, 0.95 },
	{ 15000.0, 0.90, 0.85, 0.90, 1.00 },
	{ 35000.0, 0.75, 0.65, 0.80, 0.90 }
};

double	fbw_mode_switch_tau = 0.45;
double	fbw_signal_filter_tau = 0.06;
double	fbw_qbar_filter_tau = 0.18;

double	fbw_kp_p = 0.55;
double	fbw_ki_p = 0.35;
double	fbw_kp_q = 0.88;
double	fbw_ki_q = 0.48;
double	fbw_kp_r = 0.65;
double	fbw_ki_r = 0.25;
double	fbw_aw_gain = 1.20;
double	fbw_int_limit = 1.20;
double	fbw_outer_aw_gain = 1.10;
double	fbw_outer_int_limit = rad(75.0);
double	fbw_alpha_trim_tau = 1.20;
double	fbw_nz_trim_tau = 1.60;
double	fbw_nz_filter_tau = 0.26;
double	fbw_pitch_ref_tau = 0.14;
double	fbw_pitch_ref_rate_deg_s = 90.0;
double	fbw_nz_limit_gain_floor = 0.58;
double	fbw_nz_limit_buffer_bias = 0.15;
double	fbw_region_low_kts = 220.0;
double	fbw_region_high_kts = 380.0;
double	fbw_region_approach_kts = 240.0;
double	fbw_region_min_kts = 110.0;
double	fbw_region_alpha1_deg = 12.0;
double	fbw_region_alpha2_deg = 18.0;
double	fbw_alpha_cmd_per_stick_deg = 13.5;
double	fbw_q_cmd_land_max_deg = 50.0;

double	fbw_ail_limit_deg = 22.0;
double	fbw_ele_limit_deg = 25.0;
double	fbw_rud_limit_deg = 30.0;
double	fbw_ail_rate_deg_s = 110.0;
double	fbw_ele_rate_deg_s = 120.0;
double	fbw_rud_rate_deg_s = 80.0;
double	fbw_ail_lag_tau = 0.05;
double	fbw_ele_lag_tau = 0.04;
double	fbw_rud_lag_tau = 0.07;

bool	fbw_enabled = true;
FBWCatMode fbw_mode_target = FBW_CAT1;
double	fbw_mode_blend = 0.0;
FBWControlState fbw_state = FBW_STATE_RATE;
bool	fbw_hold_active = false;
FBWHoldExitReason fbw_hold_exit_reason = FBW_HOLD_EXIT_NONE;
int		fbw_hold_enter_reason = 0;
double	fbw_hold_timer = 0.0;
double	fbw_hold_gain_scale = 1.0;
double	fbw_phi_ref = 0.0;
double	fbw_theta_ref = 0.0;

double	fbw_int_p = 0.0;
double	fbw_int_q = 0.0;
double	fbw_int_r = 0.0;

double	fbw_ail_rate_state_deg = 0.0;
double	fbw_ail_lag_state_deg = 0.0;
double	fbw_ele_rate_state_deg = 0.0;
double	fbw_ele_lag_state_deg = 0.0;
double	fbw_rud_rate_state_deg = 0.0;
double	fbw_rud_lag_state_deg = 0.0;

bool	fbw_aoa_limit_active = false;
bool	fbw_rate_limit_active = false;
bool	fbw_actuator_sat = false;
bool	fbw_anti_windup_active = false;
double	fbw_actuator_sat_timer = 0.0;

double	fbw_stick_roll_raw = 0.0;
double	fbw_stick_pitch_raw = 0.0;
double	fbw_stick_yaw_raw = 0.0;
double	fbw_stick_roll_shaped = 0.0;
double	fbw_stick_pitch_shaped = 0.0;
double	fbw_stick_yaw_shaped = 0.0;

double	fbw_p_cmd = 0.0;
double	fbw_q_cmd = 0.0;
double	fbw_r_cmd = 0.0;
double	fbw_p_cmd_rate = 0.0;
double	fbw_q_cmd_rate = 0.0;
double	fbw_r_cmd_rate = 0.0;
double	fbw_p_cmd_hold = 0.0;
double	fbw_q_cmd_hold = 0.0;
double	fbw_r_cmd_damper = 0.0;

double	fbw_p_err = 0.0;
double	fbw_q_err = 0.0;
double	fbw_r_err = 0.0;
double	fbw_phi_err = 0.0;
double	fbw_theta_err = 0.0;

double	fbw_phi_raw = 0.0;
double	fbw_theta_raw = 0.0;
double	fbw_p_raw = 0.0;
double	fbw_q_raw = 0.0;
double	fbw_r_raw = 0.0;
double	fbw_alpha_raw = 0.0;
double	fbw_beta_raw = 0.0;
double	fbw_qbar_raw = 0.0;
double	fbw_ias_raw = 0.0;
double	fbw_mach_raw = 0.0;

double	fbw_phi_f = 0.0;
double	fbw_theta_f = 0.0;
double	fbw_p_f = 0.0;
double	fbw_q_f = 0.0;
double	fbw_r_f = 0.0;
double	fbw_alpha_f = 0.0;
double	fbw_beta_f = 0.0;
double	fbw_qbar_f = 0.0;
double	fbw_ias_f = 0.0;
double	fbw_mach_f = 0.0;
double	fbw_nz_raw = 1.0;
double	fbw_nz_f = 1.0;
double	fbw_alpha_trim_deg = 0.0;
double	fbw_nz_trim_g = 1.0;
double	fbw_alpha_outer_int = 0.0;
double	fbw_nz_outer_int = 0.0;
double	fbw_w_alpha = 1.0;
double	fbw_w_nz = 0.0;
double	fbw_w_q = 0.0;
double	fbw_alpha_cmd_deg = 0.0;
double	fbw_alpha_cmd_lim_deg = 0.0;
double	fbw_nz_cmd = 1.0;
double	fbw_nz_cmd_lim = 1.0;
double	fbw_q_cmd_direct = 0.0;
double	fbw_q_ref_alpha = 0.0;
double	fbw_q_ref_nz = 0.0;
double	fbw_q_ref_q = 0.0;
double	fbw_q_ref_blended = 0.0;
double	fbw_q_ref_filtered = 0.0;
bool	fbw_g_limit_active = false;

double	fbw_ail_cmd_pre = 0.0;
double	fbw_ail_cmd_sat = 0.0;
double	fbw_ail_cmd_rate = 0.0;
double	fbw_ail_cmd_lag = 0.0;
double	fbw_ele_cmd_pre = 0.0;
double	fbw_ele_cmd_sat = 0.0;
double	fbw_ele_cmd_rate = 0.0;
double	fbw_ele_cmd_lag = 0.0;
double	fbw_rud_cmd_pre = 0.0;
double	fbw_rud_cmd_sat = 0.0;
double	fbw_rud_cmd_rate = 0.0;
double	fbw_rud_cmd_lag = 0.0;

// DLL-Lua interface
EDPARAM interface; 
}

using namespace FM;

void add_local_force(const Vec3 & Force, const Vec3 & Force_pos);
static void dbg_susp(const char* msg);
static void susp_probe_log(const char* msg);
static inline double suspension_total_force_mag();
static void append_tag(char* tags, size_t tags_size, const char* tag);

static const Vec3 kFallbackGearPoints[3] = {
	Vec3(4.12, -1.912, 0.0),
	Vec3(-1.185, -1.913, -0.7905),
	Vec3(-1.185, -1.913, 0.7905)
};

static const double kFallbackWheelRadius[3] = { 0.2286, 0.3048, 0.3048 };
static const double kFallbackSpring[3] = { 1000000.0, 3200000.0, 3200000.0 };
static const double kFallbackDamping[3] = { 12000.0, 20000.0, 20000.0 };
static const double kFallbackContactBand[3] = { 0.015, 0.055, 0.055 };
static const Vec3 kFallbackBellyPoint(0.0, -1.05, 0.0);
static const bool kEnableFallbackGroundForces = false;
static const bool kSuspProbeMode = true;
static const double kSuspProbeInterval = 0.25;
static const bool kStartupSuspProbeMode = true;
static const double kStartupSuspProbeDuration = 5.0;
static const double kStartupSuspForceEpsilon = 1e-3;
static const double kSuspensionVisualTravel[3] = { 0.2, 0.184, 0.184 };
static const char* kActiveCollisionShellName = "F-CK-1C-box.edm";
static const char* kActiveGearShellNodes = "F-CK-1C-F_W/F-CK-1C-LBW/F-CK-1C-RBW";
static const char* kSuspensionModeName = "native_collision_shell_name";
static int kFallbackLogDecimation = 0;
static bool kGroundConfigLogged = false;
static double kSuspProbeTimer = 0.0;
static unsigned long long kStartupSuspFrameIndex = 0;
static double kStartupSuspProbeElapsed = 0.0;
static bool kStartupSuspSeenForce = false;
static bool kStartupSuspLoggedFirstForce = false;
static bool kStartupSuspLoggedZeroAfterForce = false;
static bool kStartupSuspSeenAnyWow = false;
static bool kStartupSuspLoggedAllWowZero = false;
static bool kStartupSuspSeenAnyCompression = false;
static bool kStartupSuspLoggedCompAllZero = false;

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

static bool config_flag_is_true(const char* flag_name)
{
	const char* cfg_path = "C:\\Users\\Ragdoll\\Saved Games\\DCS\\Mods\\aircraft\\F-CK-1C\\FM\\config.lua";
	FILE* f = fopen(cfg_path, "rb");
	if (!f)
	{
		return false;
	}

	char content[8192];
	const size_t read = fread(content, 1, sizeof(content) - 1, f);
	fclose(f);
	content[read] = '\0';

	const char* p = strstr(content, flag_name);
	if (!p)
	{
		return false;
	}

	const char* line_end = strchr(p, '\n');
	const char* true_pos = strstr(p, "true");
	return true_pos && (!line_end || true_pos < line_end);
}

static double config_number_or_default(const char* key_name, double default_value)
{
	const char* cfg_path = "C:\\Users\\Ragdoll\\Saved Games\\DCS\\Mods\\aircraft\\F-CK-1C\\FM\\config.lua";
	FILE* f = fopen(cfg_path, "rb");
	if (!f)
	{
		return default_value;
	}

	char content[8192];
	const size_t read = fread(content, 1, sizeof(content) - 1, f);
	fclose(f);
	content[read] = '\0';

	const char* p = strstr(content, key_name);
	if (!p)
	{
		return default_value;
	}

	const char* line_end = strchr(p, '\n');
	const char* equals = strchr(p, '=');
	if (!equals || (line_end && equals > line_end))
	{
		return default_value;
	}

	char* end = nullptr;
	const double value = strtod(equals + 1, &end);
	return (end != equals + 1) ? value : default_value;
}

static void config_string_or_default(const char* key_name, const char* default_value, char* out, size_t out_size)
{
	if (!out || out_size == 0)
	{
		return;
	}

	snprintf(out, out_size, "%s", default_value ? default_value : "");

	const char* cfg_path = "C:\\Users\\Ragdoll\\Saved Games\\DCS\\Mods\\aircraft\\F-CK-1C\\FM\\config.lua";
	FILE* f = fopen(cfg_path, "rb");
	if (!f)
	{
		return;
	}

	char content[8192];
	const size_t read = fread(content, 1, sizeof(content) - 1, f);
	fclose(f);
	content[read] = '\0';

	const char* p = strstr(content, key_name);
	if (!p)
	{
		return;
	}

	const char* line_end = strchr(p, '\n');
	const char* first_quote = strchr(p, '"');
	if (!first_quote || (line_end && first_quote > line_end))
	{
		return;
	}

	const char* second_quote = strchr(first_quote + 1, '"');
	if (!second_quote || (line_end && second_quote > line_end))
	{
		return;
	}

	const size_t len = (size_t)(second_quote - first_quote - 1);
	const size_t copy_len = (len < out_size - 1) ? len : out_size - 1;
	memcpy(out, first_quote + 1, copy_len);
	out[copy_len] = '\0';
}

static double active_susp_radius_add()
{
	if (!config_flag_is_true("SUSP_GEOMETRY_TEST"))
	{
		return 0.0;
	}

	return config_number_or_default("SUSP_GEOMETRY_TEST_RADIUS_ADD", 0.30);
}

static double active_susp_wheel_y_offset()
{
	if (!config_flag_is_true("SUSP_GEOMETRY_TEST"))
	{
		return 0.0;
	}

	return config_number_or_default("SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET", -0.50);
}

static Vec3 active_susp_wheel_pos(int idx)
{
	if (idx < 0 || idx >= 3)
	{
		return Vec3();
	}

	Vec3 pos = kFallbackGearPoints[idx];
	pos.y += active_susp_wheel_y_offset();
	return pos;
}

static void active_susp_node_names(const char*& nose, const char*& left, const char*& right)
{
	const bool use_modelviewer_nodes = config_flag_is_true("SUSP_USE_MODELVIEWER_WHEEL_NODES");
	const char** nodes = use_modelviewer_nodes ? kSuspModelViewerWheelNodes : kSuspOriginalWheelNodes;
	nose = nodes[0];
	left = nodes[1];
	right = nodes[2];
}

static void log_startup_susp_probe(double dt)
{
	if (!kStartupSuspProbeMode || kStartupSuspProbeElapsed > kStartupSuspProbeDuration)
	{
		return;
	}

	++kStartupSuspFrameIndex;

	const double total_force = suspension_total_force_mag();
	const bool any_force = total_force > kStartupSuspForceEpsilon;
	const bool any_wow_now = suspension_wow[0] || suspension_wow[1] || suspension_wow[2];
	const bool all_wow_zero = !any_wow_now;
	const bool any_comp_now =
		(fabs(suspension_compression[0]) > 1e-5) ||
		(fabs(suspension_compression[1]) > 1e-5) ||
		(fabs(suspension_compression[2]) > 1e-5);
	const bool comp_all_zero = !any_comp_now;

	char tags[256] = "";
	if (any_force)
	{
		kStartupSuspSeenForce = true;
		if (!kStartupSuspLoggedFirstForce)
		{
			append_tag(tags, sizeof(tags), "FIRST_FORCE_FRAME");
			kStartupSuspLoggedFirstForce = true;
		}
	}
	else if (kStartupSuspSeenForce && !kStartupSuspLoggedZeroAfterForce)
	{
		append_tag(tags, sizeof(tags), "FIRST_ZERO_FORCE_AFTER_FORCE");
		kStartupSuspLoggedZeroAfterForce = true;
	}

	if (any_wow_now)
	{
		kStartupSuspSeenAnyWow = true;
	}
	else if (kStartupSuspSeenAnyWow && all_wow_zero && !kStartupSuspLoggedAllWowZero)
	{
		append_tag(tags, sizeof(tags), "FIRST_ALL_WOW_ZERO");
		kStartupSuspLoggedAllWowZero = true;
	}

	if (any_comp_now)
	{
		kStartupSuspSeenAnyCompression = true;
	}
	else if (kStartupSuspSeenAnyCompression && comp_all_zero && !kStartupSuspLoggedCompAllZero)
	{
		append_tag(tags, sizeof(tags), "FIRST_COMP_ALL_ZERO");
		kStartupSuspLoggedCompAllZero = true;
	}

	if (tags[0] == '\0')
	{
		snprintf(tags, sizeof(tags), "-");
	}

	char buf[2048];
	snprintf(
		buf, sizeof(buf),
		"STARTUP_SUSP sim_time=%.4f startup_time=%.4f frame=%llu dt=%.5f AGL=%.3f h=%.3f h_obj=%.3f "
		"vy=%.3f pitch=%.3f roll=%.3f mass=%.1f gear_down=%d "
		"wow=%d/%d/%d comp=%.6f/%.6f/%.6f "
		"force_vec=(%.1f,%.1f,%.1f)/(%.1f,%.1f,%.1f)/(%.1f,%.1f,%.1f) "
		"force_mag=%.1f/%.1f/%.1f SUSP_total=%.1f "
		"throttle_output=%.3f/%.3f thrust=%.1f/%.1f "
		"vel_world=(%.3f,%.3f,%.3f) vel_body=(%.3f,%.3f,%.3f) "
		"omega_world=(%.5f,%.5f,%.5f) omega_body=(%.5f,%.5f,%.5f) tags=%s",
		fm_clock,
		kStartupSuspProbeElapsed,
		kStartupSuspFrameIndex,
		dt,
		altitude_AGL,
		surface_height_raw,
		surface_height_with_objects,
		velocity_world.y,
		pitch * rad_to_deg,
		roll * rad_to_deg,
		current_mass,
		gear_pos > 0.5 ? 1 : 0,
		suspension_wow[0] ? 1 : 0,
		suspension_wow[1] ? 1 : 0,
		suspension_wow[2] ? 1 : 0,
		suspension_compression[0],
		suspension_compression[1],
		suspension_compression[2],
		suspension_force_vec[0].x,
		suspension_force_vec[0].y,
		suspension_force_vec[0].z,
		suspension_force_vec[1].x,
		suspension_force_vec[1].y,
		suspension_force_vec[1].z,
		suspension_force_vec[2].x,
		suspension_force_vec[2].y,
		suspension_force_vec[2].z,
		suspension_force_mag[0],
		suspension_force_mag[1],
		suspension_force_mag[2],
		total_force,
		left_throttle_output,
		right_throttle_output,
		left_thrust_force,
		right_thrust_force,
		velocity_world.x,
		velocity_world.y,
		velocity_world.z,
		velocity_body.x,
		velocity_body.y,
		velocity_body.z,
		angular_velocity_world.x,
		angular_velocity_world.y,
		angular_velocity_world.z,
		angular_velocity_body.x,
		angular_velocity_body.y,
		angular_velocity_body.z,
		tags
	);
	susp_probe_log(buf);

	if (dt > 0.0)
	{
		kStartupSuspProbeElapsed += dt;
	}
}

static inline bool has_suspension_feedback()
{
	return suspension_feedback_valid[0] || suspension_feedback_valid[1] || suspension_feedback_valid[2];
}

static inline bool any_wow()
{
	return suspension_wow[0] || suspension_wow[1] || suspension_wow[2];
}

static inline double suspension_total_force_mag()
{
	return suspension_force_mag[0] + suspension_force_mag[1] + suspension_force_mag[2];
}

static void append_tag(char* tags, size_t tags_size, const char* tag)
{
	if (!tags || !tag || tags_size == 0)
	{
		return;
	}

	const size_t used = strlen(tags);
	if (used >= tags_size - 1)
	{
		return;
	}

	snprintf(tags + used, tags_size - used, "%s%s", used > 0 ? "|" : "", tag);
}

static void reset_startup_susp_probe_state()
{
	kStartupSuspFrameIndex = 0;
	kStartupSuspProbeElapsed = 0.0;
	kStartupSuspSeenForce = false;
	kStartupSuspLoggedFirstForce = false;
	kStartupSuspLoggedZeroAfterForce = false;
	kStartupSuspSeenAnyWow = false;
	kStartupSuspLoggedAllWowZero = false;
	kStartupSuspSeenAnyCompression = false;
	kStartupSuspLoggedCompAllZero = false;
}

static inline bool any_fallback_wow()
{
	return fallback_suspension_wow[0] || fallback_suspension_wow[1] || fallback_suspension_wow[2];
}

static inline double suspension_visual_arg(int idx)
{
	if (idx < 0 || idx >= 3 || !suspension_feedback_valid[idx])
	{
		return 0.0;
	}

	return limit(suspension_compression[idx], 0.0, 1.0) * limit(gear_pos, 0.0, 1.0);
}

static inline double fallback_world_vertical_offset(const Vec3& point)
{
	const double cp = cos(pitch);
	const double sp = sin(pitch);
	const double cr = cos(roll);
	const double sr = sin(roll);

	const double y_after_pitch = point.x * sp + point.y * cp;
	return (y_after_pitch * cr) - (point.z * sr);
}

static inline double apply_fallback_ground_forces()
{
	for (int i = 0; i < 3; ++i)
	{
		fallback_suspension_compression[i] = 0.0;
		fallback_suspension_force_mag[i] = 0.0;
		fallback_suspension_wow[i] = false;
	}

	if (!kEnableFallbackGroundForces)
	{
		return 0.0;
	}

	double total_force = 0.0;
	double total_main_normal = 0.0;
	double left_main_normal = 0.0;
	double right_main_normal = 0.0;
	const double sink_rate = limit(-velocity_world.y, 0.0, 80.0);
	const double gear_support = limit((gear_pos - 0.2) / 0.8, 0.0, 1.0);
	bool gear_contact = false;

	if (gear_support > 0.0)
	{
		for (int i = 0; i < 3; ++i)
		{
			const double wheel_bottom_agl =
				altitude_AGL +
				fallback_world_vertical_offset(kFallbackGearPoints[i]) -
				kFallbackWheelRadius[i];
			const double compression = kFallbackContactBand[i] - wheel_bottom_agl;

			if (compression > 0.0)
			{
				double force_mag =
					(compression * kFallbackSpring[i] * gear_support) +
					(sink_rate * kFallbackDamping[i] * gear_support);

				if (i == 0)
				{
					force_mag = limit(force_mag, 0.0, current_mass * 9.81 * 0.45);
				}
				else
				{
					force_mag = limit(force_mag, 0.0, current_mass * 9.81 * 1.15);
				}

				add_local_force(Vec3(0.0, force_mag, 0.0), kFallbackGearPoints[i]);

				fallback_suspension_compression[i] = compression;
				fallback_suspension_force_mag[i] = force_mag;
				fallback_suspension_wow[i] = true;

				total_force += force_mag;
				if (i == 1)
				{
					left_main_normal = force_mag;
					total_main_normal += force_mag;
				}
				else if (i == 2)
				{
					right_main_normal = force_mag;
					total_main_normal += force_mag;
				}
				gear_contact = true;
			}
		}
	}

	if (total_main_normal > 1.0)
	{
		const double forward_speed = velocity_body.x;
		const double speed_abs = fabs(forward_speed);
		const double speed_sign = (forward_speed >= 0.0) ? 1.0 : -1.0;
		const double avg_throttle = 0.5 * (left_throttle_input + right_throttle_input);
		const double brake_norm =
			(left_main_normal * limit(wheel_brake_left, 0.0, 1.0)) +
			(right_main_normal * limit(wheel_brake_right, 0.0, 1.0));

		double longitudinal_resist =
			(total_main_normal * 0.035) +
			(brake_norm * 0.85);

		if (speed_abs < 1.5 && avg_throttle < 0.05)
		{
			longitudinal_resist += limit(left_thrust_force + right_thrust_force, 0.0, total_main_normal * 0.12);
		}

		if (speed_abs > 0.05)
		{
			add_local_force(Vec3(-speed_sign * longitudinal_resist, 0.0, 0.0), Vec3(-0.9, -1.6, 0.0));
		}
		else if (avg_throttle < 0.05 || brake_norm > 1.0)
		{
			add_local_force(Vec3(-longitudinal_resist, 0.0, 0.0), Vec3(-0.9, -1.6, 0.0));
		}
	}

	if (!gear_contact)
	{
		const double belly_bottom_agl = altitude_AGL + fallback_world_vertical_offset(kFallbackBellyPoint);
		const double belly_compression = 0.03 - belly_bottom_agl;

		if (belly_compression > 0.0)
		{
			double belly_force =
				(belly_compression * 260000.0) +
				(sink_rate * 40000.0);

			belly_force = limit(belly_force, 0.0, current_mass * 9.81 * 2.0);
			add_local_force(Vec3(0.0, belly_force, 0.0), kFallbackBellyPoint);
			total_force += belly_force;
		}
	}

	return total_force;
}

static void reset_suspension_feedback_state()
{
	for (int i = 0; i < 3; ++i)
	{
		suspension_compression[i] = 0.0;
		suspension_force_vec[i] = Vec3();
		suspension_force_mag[i] = 0.0;
		suspension_wow[i] = false;
		suspension_feedback_valid[i] = false;
		fallback_suspension_compression[i] = 0.0;
		fallback_suspension_force_mag[i] = 0.0;
		fallback_suspension_wow[i] = false;
	}
	fallback_ground_force = 0.0;
}

static void reset_wheel_brakes()
{
	wheel_brake = 0.0;
	wheel_brake_left = 0.0;
	wheel_brake_right = 0.0;
}

static void reset_wheel_spin()
{
	for (int i = 0; i < 3; ++i)
	{
		wheel_spin[i] = 0.0;
	}
	nose_wheel_steering = 0.0;
}

static inline double compute_nose_wheel_steering()
{
	if (!nose_turn_enabled || gear_pos <= 0.5 || V_scalar >= 70.0)
	{
		return 0.0;
	}

	return -limit(yaw_input, -1.0, 1.0) * 0.75;
}

static inline double normalize_throttle_axis(double raw_value)
{
	double normalized = limit((raw_value + 1.0) * 0.5, 0.0, 1.0);
	if (throttle_axis_inverted)
	{
		normalized = 1.0 - normalized;
	}
	return limit(normalized, 0.0, 1.0);
}

static inline double normalize_brake_axis(double raw_value)
{
	if (raw_value < 0.0)
	{
		return limit((raw_value + 1.0) * 0.5, 0.0, 1.0);
	}
	return limit(raw_value, 0.0, 1.0);
}

static inline double resolve_pilot_throttle_cmd(double axis_cmd, double keyboard_cmd, bool use_axis)
{
	return limit(use_axis ? axis_cmd : keyboard_cmd, 0.0, 1.0);
}

static inline double resolve_keyboard_throttle_base(double axis_cmd, double keyboard_cmd, bool use_axis)
{
	return limit(use_axis ? axis_cmd : keyboard_cmd, 0.0, 1.0);
}

static inline double compose_engine_throttle_cmd(double pilot_cmd, double fbw_cmd)
{
	const double pilot = limit(pilot_cmd, 0.0, 1.0);
	const double fbw = limit(fbw_cmd, 0.0, 1.0);

	if (fbw_throttle_override)
	{
		return fbw;
	}

	const double blend = limit(fbw_throttle_blend, 0.0, 1.0);
	return limit((1.0 - blend) * pilot + blend * fbw, 0.0, 1.0);
}

static inline void update_engine_throttle_inputs_from_interface()
{
	pilot_throttle_cmd_left = resolve_pilot_throttle_cmd(throttle_axis_cmd_left, throttle_keyboard_cmd_left, throttle_use_axis_left);
	pilot_throttle_cmd_right = resolve_pilot_throttle_cmd(throttle_axis_cmd_right, throttle_keyboard_cmd_right, throttle_use_axis_right);

	engine_throttle_cmd_left = compose_engine_throttle_cmd(pilot_throttle_cmd_left, fbw_throttle_cmd_left);
	engine_throttle_cmd_right = compose_engine_throttle_cmd(pilot_throttle_cmd_right, fbw_throttle_cmd_right);

	left_throttle_input = engine_throttle_cmd_left;
	right_throttle_input = engine_throttle_cmd_right;
}

static inline double fbw_blend_value(double a, double b, double t)
{
	return a + (b - a) * limit(t, 0.0, 1.0);
}

static inline double fbw_first_order(double current, double target, double tau, double dt)
{
	if (tau <= 1e-6)
	{
		return target;
	}
	const double k = limit(dt / (tau + dt), 0.0, 1.0);
	return current + (target - current) * k;
}

static inline double fbw_wrap_pi(double angle)
{
	while (angle > pi)
	{
		angle -= 2.0 * pi;
	}
	while (angle < -pi)
	{
		angle += 2.0 * pi;
	}
	return angle;
}

static inline double fbw_soft_limit_symmetric(double command, double limit_value)
{
	if (limit_value <= 1e-6)
	{
		return 0.0;
	}
	return limit_value * tanh(command / limit_value);
}

static inline double fbw_soft_limit_asymmetric(double command, double pos_limit, double neg_limit)
{
	if (command >= 0.0)
	{
		return fbw_soft_limit_symmetric(command, pos_limit);
	}
	return -fbw_soft_limit_symmetric(-command, neg_limit);
}

static inline double fbw_smoothstep01(double t)
{
	t = limit(t, 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

static inline double fbw_soft_clip_positive(double value, double soft_limit, double hard_limit)
{
	if (value <= soft_limit)
	{
		return value;
	}
	if (value >= hard_limit)
	{
		return hard_limit;
	}
	const double t = fbw_smoothstep01((value - soft_limit) / (hard_limit - soft_limit));
	return soft_limit + (hard_limit - soft_limit) * t;
}

static inline double fbw_soft_clip_negative(double value, double soft_limit, double hard_limit)
{
	if (value >= soft_limit)
	{
		return value;
	}
	if (value <= hard_limit)
	{
		return hard_limit;
	}
	const double t = fbw_smoothstep01((soft_limit - value) / (soft_limit - hard_limit));
	return soft_limit - (soft_limit - hard_limit) * t;
}

static inline FBWCatParams fbw_blend_cat_params(const FBWCatParams& cat1, const FBWCatParams& cat3, double t)
{
	FBWCatParams out;
	out.deadband = fbw_blend_value(cat1.deadband, cat3.deadband, t);
	out.hold_engage_time = fbw_blend_value(cat1.hold_engage_time, cat3.hold_engage_time, t);
	out.hold_phi_kp = fbw_blend_value(cat1.hold_phi_kp, cat3.hold_phi_kp, t);
	out.hold_theta_kp = fbw_blend_value(cat1.hold_theta_kp, cat3.hold_theta_kp, t);
	out.hold_p_cmd_max = fbw_blend_value(cat1.hold_p_cmd_max, cat3.hold_p_cmd_max, t);
	out.hold_q_cmd_max = fbw_blend_value(cat1.hold_q_cmd_max, cat3.hold_q_cmd_max, t);
	out.alpha_hold_degrade_deg = fbw_blend_value(cat1.alpha_hold_degrade_deg, cat3.alpha_hold_degrade_deg, t);
	out.qbar_min_hold = fbw_blend_value(cat1.qbar_min_hold, cat3.qbar_min_hold, t);
	out.sat_time = fbw_blend_value(cat1.sat_time, cat3.sat_time, t);
	out.hold_cmd_ratio_limit = fbw_blend_value(cat1.hold_cmd_ratio_limit, cat3.hold_cmd_ratio_limit, t);
	out.hold_decay_tau = fbw_blend_value(cat1.hold_decay_tau, cat3.hold_decay_tau, t);
	out.command_shape_tau = fbw_blend_value(cat1.command_shape_tau, cat3.command_shape_tau, t);
	out.command_shape_rate = fbw_blend_value(cat1.command_shape_rate, cat3.command_shape_rate, t);
	out.stick_expo = fbw_blend_value(cat1.stick_expo, cat3.stick_expo, t);
	out.p_cmd_max = fbw_blend_value(cat1.p_cmd_max, cat3.p_cmd_max, t);
	out.q_cmd_max = fbw_blend_value(cat1.q_cmd_max, cat3.q_cmd_max, t);
	out.r_cmd_max = fbw_blend_value(cat1.r_cmd_max, cat3.r_cmd_max, t);
	out.aoa_soft_deg = fbw_blend_value(cat1.aoa_soft_deg, cat3.aoa_soft_deg, t);
	out.aoa_hard_deg = fbw_blend_value(cat1.aoa_hard_deg, cat3.aoa_hard_deg, t);
	out.g_soft = fbw_blend_value(cat1.g_soft, cat3.g_soft, t);
	out.g_hard = fbw_blend_value(cat1.g_hard, cat3.g_hard, t);
	out.p_rate_limit = fbw_blend_value(cat1.p_rate_limit, cat3.p_rate_limit, t);
	out.q_rate_limit = fbw_blend_value(cat1.q_rate_limit, cat3.q_rate_limit, t);
	out.r_rate_limit = fbw_blend_value(cat1.r_rate_limit, cat3.r_rate_limit, t);
	out.yaw_damper_beta = fbw_blend_value(cat1.yaw_damper_beta, cat3.yaw_damper_beta, t);
	out.yaw_damper_r = fbw_blend_value(cat1.yaw_damper_r, cat3.yaw_damper_r, t);
	return out;
}

static inline FBWGainScheduleValues fbw_eval_gain_schedule(double qbar_value)
{
	const unsigned count = sizeof(fbw_gain_schedule) / sizeof(fbw_gain_schedule[0]);
	FBWGainScheduleValues out;
	out.cmd_gain = fbw_gain_schedule[0].cmd_gain;
	out.hold_gain = fbw_gain_schedule[0].hold_gain;
	out.damping_gain = fbw_gain_schedule[0].damping_gain;
	out.limiter_gain = fbw_gain_schedule[0].limiter_gain;

	if (qbar_value <= fbw_gain_schedule[0].qbar)
	{
		return out;
	}

	for (unsigned i = 1; i < count; ++i)
	{
		if (qbar_value <= fbw_gain_schedule[i].qbar)
		{
			const double q0 = fbw_gain_schedule[i - 1].qbar;
			const double q1 = fbw_gain_schedule[i].qbar;
			const double t = limit((qbar_value - q0) / (q1 - q0), 0.0, 1.0);
			out.cmd_gain = fbw_blend_value(fbw_gain_schedule[i - 1].cmd_gain, fbw_gain_schedule[i].cmd_gain, t);
			out.hold_gain = fbw_blend_value(fbw_gain_schedule[i - 1].hold_gain, fbw_gain_schedule[i].hold_gain, t);
			out.damping_gain = fbw_blend_value(fbw_gain_schedule[i - 1].damping_gain, fbw_gain_schedule[i].damping_gain, t);
			out.limiter_gain = fbw_blend_value(fbw_gain_schedule[i - 1].limiter_gain, fbw_gain_schedule[i].limiter_gain, t);
			return out;
		}
	}

	out.cmd_gain = fbw_gain_schedule[count - 1].cmd_gain;
	out.hold_gain = fbw_gain_schedule[count - 1].hold_gain;
	out.damping_gain = fbw_gain_schedule[count - 1].damping_gain;
	out.limiter_gain = fbw_gain_schedule[count - 1].limiter_gain;
	return out;
}

static inline const char* fbw_mode_name()
{
	return (fbw_mode_blend >= 0.5) ? "CAT3" : "CAT1";
}

static inline const char* fbw_state_name()
{
	switch (fbw_state)
	{
	case FBW_STATE_HOLD:
		return "HOLD";
	case FBW_STATE_DEGRADE:
		return "DEGRADE";
	default:
		return "RATE";
	}
}

static inline const char* fbw_exit_reason_name()
{
	switch (fbw_hold_exit_reason)
	{
	case FBW_HOLD_EXIT_STICK:
		return "STICK";
	case FBW_HOLD_EXIT_AOA:
		return "AOA";
	case FBW_HOLD_EXIT_QBAR:
		return "QBAR";
	case FBW_HOLD_EXIT_ACTUATOR_SAT:
		return "SAT";
	case FBW_HOLD_EXIT_HOLD_CMD:
		return "HCMD";
	default:
		return "NONE";
	}
}

static void reset_fbw_state()
{
	fbw_mode_target = FBW_CAT1;
	fbw_mode_blend = 0.0;
	fbw_state = FBW_STATE_RATE;
	fbw_hold_active = false;
	fbw_hold_exit_reason = FBW_HOLD_EXIT_NONE;
	fbw_hold_enter_reason = 0;
	fbw_hold_timer = 0.0;
	fbw_hold_gain_scale = 1.0;
	fbw_phi_ref = roll;
	fbw_theta_ref = pitch;

	fbw_int_p = 0.0;
	fbw_int_q = 0.0;
	fbw_int_r = 0.0;

	fbw_ail_rate_state_deg = 0.0;
	fbw_ail_lag_state_deg = 0.0;
	fbw_ele_rate_state_deg = 0.0;
	fbw_ele_lag_state_deg = 0.0;
	fbw_rud_rate_state_deg = 0.0;
	fbw_rud_lag_state_deg = 0.0;

	fbw_stick_roll_shaped = 0.0;
	fbw_stick_pitch_shaped = 0.0;
	fbw_stick_yaw_shaped = 0.0;

	fbw_p_cmd = 0.0;
	fbw_q_cmd = 0.0;
	fbw_r_cmd = 0.0;
	fbw_p_cmd_rate = 0.0;
	fbw_q_cmd_rate = 0.0;
	fbw_r_cmd_rate = 0.0;
	fbw_p_cmd_hold = 0.0;
	fbw_q_cmd_hold = 0.0;
	fbw_r_cmd_damper = 0.0;
	fbw_p_err = 0.0;
	fbw_q_err = 0.0;
	fbw_r_err = 0.0;
	fbw_phi_err = 0.0;
	fbw_theta_err = 0.0;
	fbw_nz_raw = g;
	fbw_nz_f = g;
	fbw_alpha_trim_deg = alpha;
	fbw_nz_trim_g = g;
	fbw_alpha_outer_int = 0.0;
	fbw_nz_outer_int = 0.0;
	fbw_w_alpha = 1.0;
	fbw_w_nz = 0.0;
	fbw_w_q = 0.0;
	fbw_alpha_cmd_deg = alpha;
	fbw_alpha_cmd_lim_deg = alpha;
	fbw_nz_cmd = g;
	fbw_nz_cmd_lim = g;
	fbw_q_cmd_direct = 0.0;
	fbw_q_ref_alpha = 0.0;
	fbw_q_ref_nz = 0.0;
	fbw_q_ref_q = 0.0;
	fbw_q_ref_blended = 0.0;
	fbw_q_ref_filtered = 0.0;

	fbw_aoa_limit_active = false;
	fbw_rate_limit_active = false;
	fbw_actuator_sat = false;
	fbw_anti_windup_active = false;
	fbw_g_limit_active = false;
	fbw_actuator_sat_timer = 0.0;
}

static void update_primary_control_inputs()
{
	// Pitch
	if (pitch_analog == true)
	{
		pitch_input = limit(pitch_input, -1, 1);
	}
	else
	{
		if (pitch_discrete > 0.1)
		{
			pitch_input += 0.0035;
			if (pitch_input > 1.0)
				pitch_input = 1.0;
		}
		if (pitch_discrete == 0 && pitch_input > 0.5)
		{
			if (pitch_input > 0.7)
				pitch_input *= 0.98;
		}
		if (pitch_discrete < -0.1)
		{
			pitch_input -= 0.0035;
			if (pitch_input < -1.0)
				pitch_input = -1.0;
		}
		if (pitch_discrete == 0 && pitch_input < -0.5)
		{
			if (pitch_input < -0.5)
				pitch_input *= 0.98;
		}
	}
	pitch_trim = limit(pitch_trim, -0.3, 0.3);

	// Roll
	if (roll_analog == true)
	{
		roll_input = limit(roll_input, -1, 1);
	}
	else
	{
		if (roll_discrete > 0.1)
		{
			roll_input += 0.004;
			if (roll_input > 1.0)
				roll_input = 1.0;
		}
		if (roll_discrete < -0.1)
		{
			roll_input -= 0.004;
			if (roll_input < -1.0)
				roll_input = -1.0;
		}
		if (roll_discrete == 0)
			roll_input *= 0.9;
	}
	roll_trim = limit(roll_trim, -0.3, 0.3);

	// Yaw
	if (yaw_analog == true)
	{
		yaw_input = limit(yaw_input, -1, 1);
	}
	else
	{
		if (yaw_discrete > 0.1)
		{
			yaw_input += 0.0035;
			if (yaw_input > 1.0)
				yaw_input = 1.0;
		}
		if (yaw_discrete < -0.1)
		{
			yaw_input -= 0.0035;
			if (yaw_input < -1.0)
				yaw_input = -1.0;
		}
		if (yaw_discrete == 0)
			yaw_input *= 0.9;
	}
	yaw_trim = limit(yaw_trim, -0.2, 0.2);
}

static inline double fbw_apply_axis_actuator(
	double cmd_norm,
	double limit_deg,
	double rate_deg_s,
	double lag_tau,
	double dt,
	double& rate_state_deg,
	double& lag_state_deg,
	double& dbg_pre_deg,
	double& dbg_sat_deg,
	double& dbg_rate_deg,
	double& dbg_lag_deg,
	bool& axis_saturated)
{
	dbg_pre_deg = cmd_norm * limit_deg;
	dbg_sat_deg = limit(dbg_pre_deg, -limit_deg, limit_deg);

	const double max_step = rate_deg_s * dt;
	rate_state_deg = rate_state_deg + limit(dbg_sat_deg - rate_state_deg, -max_step, max_step);
	rate_state_deg = limit(rate_state_deg, -limit_deg, limit_deg);

	const double lag_alpha = limit(dt / (lag_tau + dt), 0.0, 1.0);
	lag_state_deg = lag_state_deg + (rate_state_deg - lag_state_deg) * lag_alpha;
	lag_state_deg = limit(lag_state_deg, -limit_deg, limit_deg);

	dbg_rate_deg = rate_state_deg;
	dbg_lag_deg = lag_state_deg;

	axis_saturated =
		(fabs(dbg_pre_deg - dbg_sat_deg) > 1e-3) ||
		(fabs(dbg_sat_deg - rate_state_deg) > 1e-3) ||
		(fabs(lag_state_deg) > (limit_deg - 1e-3));

	return limit(lag_state_deg / limit_deg, -1.0, 1.0);
}

static void update_fbw_controller(double dt, double qbar, double alpha_limit_deg)
{
	// RCAH state machine:
	// RATE    : non-zero stick -> rate command (p/q/r tracking)
	// HOLD    : stick stays in deadband for T_hold_engage -> lock phi/theta and hold attitude
	// DEGRADE : if near-limit/saturation/low-qbar, hold_gain_scale decays to 0 and falls back to damping
	// CAT switching is parameter-blended (fbw_mode_blend) to avoid control-law jumps.
	if (fbw_enabled == false)
	{
		elevator_command = limit(actuator(elevator_command, pitch_input + pitch_trim, -0.0125, 0.0125), -1, 1);
		aileron_command = limit(actuator(aileron_command, roll_input + roll_trim, -0.02, 0.02), -1, 1);
		rudder_command = limit(actuator(rudder_command, yaw_input + yaw_trim, -0.012, 0.012), -1, 1);
		return;
	}

	const double mode_target = (fbw_mode_target == FBW_CAT3) ? 1.0 : 0.0;
	fbw_mode_blend = fbw_first_order(fbw_mode_blend, mode_target, fbw_mode_switch_tau, dt);

	const FBWCatParams cat = fbw_blend_cat_params(fbw_cat1, fbw_cat3, fbw_mode_blend);

	fbw_phi_raw = roll;
	fbw_theta_raw = pitch;
	fbw_p_raw = roll_rate;
	fbw_q_raw = pitch_rate;
	fbw_r_raw = yaw_rate;
	fbw_alpha_raw = alpha;
	fbw_beta_raw = beta;
	fbw_qbar_raw = qbar;
	fbw_ias_raw = V_scalar * 1.943844;
	fbw_mach_raw = mach;

	fbw_phi_f = fbw_first_order(fbw_phi_f, fbw_phi_raw, fbw_signal_filter_tau, dt);
	fbw_theta_f = fbw_first_order(fbw_theta_f, fbw_theta_raw, fbw_signal_filter_tau, dt);
	fbw_p_f = fbw_first_order(fbw_p_f, fbw_p_raw, fbw_signal_filter_tau, dt);
	fbw_q_f = fbw_first_order(fbw_q_f, fbw_q_raw, fbw_signal_filter_tau, dt);
	fbw_r_f = fbw_first_order(fbw_r_f, fbw_r_raw, fbw_signal_filter_tau, dt);
	fbw_alpha_f = fbw_first_order(fbw_alpha_f, fbw_alpha_raw, fbw_signal_filter_tau, dt);
	fbw_beta_f = fbw_first_order(fbw_beta_f, fbw_beta_raw, fbw_signal_filter_tau, dt);
	fbw_qbar_f = fbw_first_order(fbw_qbar_f, fbw_qbar_raw, fbw_qbar_filter_tau, dt);
	fbw_ias_f = fbw_first_order(fbw_ias_f, fbw_ias_raw, fbw_signal_filter_tau, dt);
	fbw_mach_f = fbw_first_order(fbw_mach_f, fbw_mach_raw, fbw_signal_filter_tau, dt);

	fbw_stick_roll_raw = limit(roll_input + roll_trim, -1.0, 1.0);
	fbw_stick_pitch_raw = limit(pitch_input + pitch_trim, -1.0, 1.0);
	fbw_stick_yaw_raw = limit(yaw_input + yaw_trim, -1.0, 1.0);

	const double roll_target = (1.0 - cat.stick_expo) * fbw_stick_roll_raw + cat.stick_expo * fbw_stick_roll_raw * fbw_stick_roll_raw * fbw_stick_roll_raw;
	const double pitch_target = (1.0 - cat.stick_expo) * fbw_stick_pitch_raw + cat.stick_expo * fbw_stick_pitch_raw * fbw_stick_pitch_raw * fbw_stick_pitch_raw;
	const double yaw_target = (1.0 - cat.stick_expo) * fbw_stick_yaw_raw + cat.stick_expo * fbw_stick_yaw_raw * fbw_stick_yaw_raw * fbw_stick_yaw_raw;

	const double roll_prev = fbw_stick_roll_shaped;
	const double pitch_prev = fbw_stick_pitch_shaped;
	const double yaw_prev = fbw_stick_yaw_shaped;
	fbw_stick_roll_shaped = fbw_first_order(fbw_stick_roll_shaped, roll_target, cat.command_shape_tau, dt);
	fbw_stick_pitch_shaped = fbw_first_order(fbw_stick_pitch_shaped, pitch_target, cat.command_shape_tau, dt);
	fbw_stick_yaw_shaped = fbw_first_order(fbw_stick_yaw_shaped, yaw_target, cat.command_shape_tau, dt);
	const double shape_step = cat.command_shape_rate * dt;
	fbw_stick_roll_shaped = limit(fbw_stick_roll_shaped, roll_prev - shape_step, roll_prev + shape_step);
	fbw_stick_pitch_shaped = limit(fbw_stick_pitch_shaped, pitch_prev - shape_step, pitch_prev + shape_step);
	fbw_stick_yaw_shaped = limit(fbw_stick_yaw_shaped, yaw_prev - shape_step, yaw_prev + shape_step);

	const bool stick_in_deadband = (fabs(fbw_stick_roll_raw) <= cat.deadband) && (fabs(fbw_stick_pitch_raw) <= cat.deadband);
	const bool wow = has_suspension_feedback() && any_wow();
	if (wow)
	{
		fbw_state = FBW_STATE_RATE;
		fbw_hold_active = false;
		fbw_hold_timer = 0.0;
		fbw_hold_gain_scale = 1.0;
		fbw_hold_exit_reason = FBW_HOLD_EXIT_STICK;
		fbw_hold_enter_reason = 0;
	}
	else if (stick_in_deadband == false)
	{
		fbw_state = FBW_STATE_RATE;
		fbw_hold_active = false;
		fbw_hold_timer = 0.0;
		fbw_hold_gain_scale = 1.0;
		fbw_hold_exit_reason = FBW_HOLD_EXIT_STICK;
		fbw_hold_enter_reason = 0;
	}
	else
	{
		fbw_hold_timer += dt;
		if (fbw_state == FBW_STATE_RATE && fbw_hold_timer >= cat.hold_engage_time)
		{
			fbw_state = FBW_STATE_HOLD;
			fbw_hold_active = true;
			fbw_phi_ref = fbw_phi_f;
			fbw_theta_ref = fbw_theta_f;
			fbw_hold_gain_scale = 1.0;
			fbw_hold_exit_reason = FBW_HOLD_EXIT_NONE;
			fbw_hold_enter_reason = 1;
		}
	}

	const FBWGainScheduleValues gs = fbw_eval_gain_schedule(fbw_qbar_f);
	fbw_nz_raw = g;
	fbw_nz_f = fbw_first_order(fbw_nz_f, fbw_nz_raw, fbw_nz_filter_tau, dt);

	fbw_p_cmd_rate = fbw_stick_roll_shaped * cat.p_cmd_max * gs.cmd_gain;
	fbw_r_cmd_rate = fbw_stick_yaw_shaped * cat.r_cmd_max * gs.cmd_gain;

	const bool pitch_stick_in_deadband = fabs(fbw_stick_pitch_raw) <= cat.deadband;
	if (wow || pitch_stick_in_deadband)
	{
		fbw_alpha_trim_deg = fbw_first_order(fbw_alpha_trim_deg, fbw_alpha_f, fbw_alpha_trim_tau, dt);
		fbw_nz_trim_g = fbw_first_order(fbw_nz_trim_g, fbw_nz_f, fbw_nz_trim_tau, dt);
	}

	const double gear_weight = (gear_pos > 0.5) ? 1.0 : 0.0;
	const double approach_t = limit((fbw_region_approach_kts - fbw_ias_f) / (fbw_region_approach_kts - fbw_region_min_kts), 0.0, 1.0);
	const double high_speed_t = limit((fbw_ias_f - fbw_region_low_kts) / (fbw_region_high_kts - fbw_region_low_kts), 0.0, 1.0);
	const double alpha_region_t = limit((fabs(fbw_alpha_f) - fbw_region_alpha1_deg) / (fbw_region_alpha2_deg - fbw_region_alpha1_deg), 0.0, 1.0);

	fbw_w_q = gear_weight * fbw_blend_value(0.35, 1.0, approach_t);
	fbw_w_nz = high_speed_t * (1.0 - fbw_w_q);
	fbw_w_alpha = 1.0 - fbw_w_nz - fbw_w_q;
	fbw_w_alpha = max(fbw_w_alpha, alpha_region_t);
	const double pitch_weight_sum = max(fbw_w_alpha + fbw_w_nz + fbw_w_q, 1e-6);
	fbw_w_alpha /= pitch_weight_sum;
	fbw_w_nz /= pitch_weight_sum;
	fbw_w_q /= pitch_weight_sum;

	fbw_phi_err = 0.0;
	fbw_theta_err = 0.0;
	fbw_p_cmd_hold = 0.0;
	fbw_q_cmd_hold = 0.0;
	bool hold_cmd_overlimit = false;
	if ((fbw_state == FBW_STATE_HOLD || fbw_state == FBW_STATE_DEGRADE) && stick_in_deadband)
	{
		fbw_phi_err = fbw_wrap_pi(fbw_phi_ref - fbw_phi_f);
		fbw_theta_err = fbw_theta_ref - fbw_theta_f;
		const double p_hold_raw = fbw_phi_err * cat.hold_phi_kp * gs.hold_gain;
		const double q_hold_raw = fbw_theta_err * cat.hold_theta_kp * gs.hold_gain;
		const double p_hold_lim = cat.hold_p_cmd_max * gs.limiter_gain;
		const double q_hold_lim = cat.hold_q_cmd_max * gs.limiter_gain;
		fbw_p_cmd_hold = limit(p_hold_raw, -p_hold_lim, p_hold_lim);
		fbw_q_cmd_hold = limit(q_hold_raw, -q_hold_lim, q_hold_lim);
		hold_cmd_overlimit =
			(fabs(p_hold_raw) > (p_hold_lim * cat.hold_cmd_ratio_limit)) ||
			(fabs(q_hold_raw) > (q_hold_lim * cat.hold_cmd_ratio_limit));
	}

	const double alpha_abs = fabs(fbw_alpha_f);
	const double alpha_soft = limit(cat.aoa_soft_deg, 0.1, alpha_limit_deg);
	const double alpha_hard = limit(cat.aoa_hard_deg, alpha_soft + 0.1, alpha_limit_deg + 5.0);
	const double alpha_cmd_range = fbw_blend_value(fbw_alpha_cmd_per_stick_deg, fbw_alpha_cmd_per_stick_deg * 0.65, high_speed_t);
	fbw_alpha_cmd_deg = fbw_alpha_trim_deg + fbw_stick_pitch_shaped * alpha_cmd_range;
	fbw_alpha_cmd_lim_deg = fbw_soft_limit_symmetric(fbw_alpha_cmd_deg, alpha_soft);

	const double nz_pos_limit = fbw_g_limiter_override ? (cat.g_hard + 2.0) : cat.g_hard;
	const double nz_neg_limit = 2.5;
	const double nz_pos_soft = min(cat.g_soft, nz_pos_limit - max(0.25, fbw_nz_limit_buffer_bias));
	const double nz_neg_hard = -nz_neg_limit;
	const double nz_neg_soft = -max(1.0, nz_neg_limit * 0.65);
	if (fbw_stick_pitch_shaped >= 0.0)
	{
		fbw_nz_cmd = 1.0 + fbw_stick_pitch_shaped * (nz_pos_limit - 1.0);
	}
	else
	{
		fbw_nz_cmd = 1.0 + fbw_stick_pitch_shaped * (1.0 + nz_neg_limit);
	}
	fbw_nz_cmd_lim = fbw_nz_cmd;
	if (fbw_nz_cmd_lim > nz_pos_soft)
	{
		fbw_nz_cmd_lim = fbw_soft_clip_positive(fbw_nz_cmd_lim, nz_pos_soft, nz_pos_limit);
	}
	if (fbw_nz_cmd_lim < nz_neg_soft)
	{
		fbw_nz_cmd_lim = fbw_soft_clip_negative(fbw_nz_cmd_lim, nz_neg_soft, nz_neg_hard);
	}

	const double q_outer_limit = cat.q_rate_limit * gs.limiter_gain;
	const double kp_alpha_outer = fbw_blend_value(2.4, 1.7, fbw_mode_blend) * gs.cmd_gain;
	const double ki_alpha_outer = fbw_blend_value(1.05, 0.60, fbw_mode_blend) * gs.hold_gain;
	double nz_limit_gain_scale = 1.0;
	if (fbw_stick_pitch_shaped > 0.0)
	{
		const double t = limit((fbw_nz_f - nz_pos_soft) / max(nz_pos_limit - nz_pos_soft, 0.1), 0.0, 1.0);
		const double shaped_t = fbw_smoothstep01(t);
		nz_limit_gain_scale = fbw_blend_value(1.0, fbw_nz_limit_gain_floor, shaped_t);
	}
	const double kp_nz_outer = fbw_blend_value(0.34, 0.24, fbw_mode_blend) * gs.cmd_gain * nz_limit_gain_scale;
	const double ki_nz_outer = fbw_blend_value(0.11, 0.06, fbw_mode_blend) * gs.hold_gain * nz_limit_gain_scale;
	const double alpha_err_rad = rad(fbw_alpha_cmd_lim_deg - fbw_alpha_f);
	const double nz_err = fbw_nz_cmd_lim - fbw_nz_f;
	const double q_ref_alpha_raw = kp_alpha_outer * alpha_err_rad + fbw_alpha_outer_int;
	const double q_ref_nz_raw = kp_nz_outer * nz_err + fbw_nz_outer_int;
	fbw_q_ref_alpha = limit(q_ref_alpha_raw, -q_outer_limit, q_outer_limit);
	fbw_q_ref_nz = limit(q_ref_nz_raw, -q_outer_limit, q_outer_limit);
	fbw_alpha_outer_int += (ki_alpha_outer * alpha_err_rad + fbw_outer_aw_gain * (fbw_q_ref_alpha - q_ref_alpha_raw)) * dt;
	fbw_nz_outer_int += (ki_nz_outer * nz_err + fbw_outer_aw_gain * (fbw_q_ref_nz - q_ref_nz_raw)) * dt;
	fbw_alpha_outer_int = limit(fbw_alpha_outer_int, -fbw_outer_int_limit, fbw_outer_int_limit);
	fbw_nz_outer_int = limit(fbw_nz_outer_int, -fbw_outer_int_limit, fbw_outer_int_limit);
	if (fbw_w_alpha < 0.05)
	{
		fbw_alpha_outer_int = fbw_first_order(fbw_alpha_outer_int, 0.0, 0.35, dt);
	}
	if (fbw_w_nz < 0.05)
	{
		fbw_nz_outer_int = fbw_first_order(fbw_nz_outer_int, 0.0, 0.35, dt);
	}

	fbw_q_cmd_direct = fbw_stick_pitch_shaped * rad(fbw_q_cmd_land_max_deg) * gs.cmd_gain;
	fbw_q_ref_q = fbw_q_cmd_direct;
	fbw_q_ref_blended = fbw_w_alpha * fbw_q_ref_alpha + fbw_w_nz * fbw_q_ref_nz + fbw_w_q * fbw_q_ref_q;
	const double q_ref_prev = fbw_q_ref_filtered;
	fbw_q_ref_filtered = fbw_first_order(fbw_q_ref_filtered, fbw_q_ref_blended, fbw_pitch_ref_tau, dt);
	const double q_ref_step = rad(fbw_pitch_ref_rate_deg_s) * dt;
	fbw_q_ref_filtered = limit(fbw_q_ref_filtered, q_ref_prev - q_ref_step, q_ref_prev + q_ref_step);
	fbw_q_cmd_rate = limit(fbw_q_ref_filtered, -q_outer_limit, q_outer_limit);
	fbw_aoa_limit_active = fabs(fbw_alpha_cmd_deg - fbw_alpha_cmd_lim_deg) > 0.05;
	fbw_g_limit_active = fabs(fbw_nz_cmd - fbw_nz_cmd_lim) > 0.02;

	const bool degrade_aoa = (alpha_abs > cat.alpha_hold_degrade_deg) || (alpha_abs > (alpha_limit_deg * 0.95));
	const bool degrade_qbar = fbw_qbar_f < cat.qbar_min_hold;
	const bool degrade_hold = hold_cmd_overlimit;

	if (fbw_state == FBW_STATE_HOLD && (degrade_aoa || degrade_qbar || degrade_hold))
	{
		fbw_state = FBW_STATE_DEGRADE;
		fbw_hold_active = false;
		fbw_hold_exit_reason = degrade_aoa ? FBW_HOLD_EXIT_AOA : (degrade_qbar ? FBW_HOLD_EXIT_QBAR : FBW_HOLD_EXIT_HOLD_CMD);
	}

	if (fbw_state == FBW_STATE_DEGRADE)
	{
		fbw_hold_gain_scale = fbw_first_order(fbw_hold_gain_scale, 0.0, cat.hold_decay_tau, dt);
		if (fbw_hold_gain_scale < 1e-3)
		{
			fbw_hold_gain_scale = 0.0;
		}
	}
	else
	{
		fbw_hold_gain_scale = 1.0;
	}

	const bool hold_path_active = stick_in_deadband && (fbw_state == FBW_STATE_HOLD || fbw_state == FBW_STATE_DEGRADE);
	if (hold_path_active)
	{
		fbw_p_cmd = fbw_p_cmd_hold * fbw_hold_gain_scale;
		fbw_q_cmd = fbw_q_cmd_hold * fbw_hold_gain_scale;
	}
	else
	{
		fbw_p_cmd = fbw_p_cmd_rate;
		fbw_q_cmd = fbw_q_cmd_rate;
	}

	const double beta_rad = fbw_beta_f / rad_to_deg;
	fbw_r_cmd_damper = -(cat.yaw_damper_beta * beta_rad + cat.yaw_damper_r * fbw_r_f) * gs.damping_gain;
	fbw_r_cmd = fbw_r_cmd_rate + fbw_r_cmd_damper;

	const double p_lim = cat.p_rate_limit * gs.limiter_gain;
	const double q_lim = cat.q_rate_limit * gs.limiter_gain;
	const double r_lim = cat.r_rate_limit * gs.limiter_gain;
	const double p_cmd_before_limit = fbw_p_cmd;
	const double q_cmd_before_limit = fbw_q_cmd;
	const double r_cmd_before_limit = fbw_r_cmd;
	fbw_p_cmd = limit(fbw_p_cmd, -p_lim, p_lim);
	fbw_q_cmd = limit(fbw_q_cmd, -q_lim, q_lim);
	fbw_r_cmd = limit(fbw_r_cmd, -r_lim, r_lim);
	fbw_rate_limit_active =
		(fabs(p_cmd_before_limit - fbw_p_cmd) > 1e-5) ||
		(fabs(q_cmd_before_limit - fbw_q_cmd) > 1e-5) ||
		(fabs(r_cmd_before_limit - fbw_r_cmd) > 1e-5);

	fbw_p_err = fbw_p_cmd - fbw_p_f;
	fbw_q_err = fbw_q_cmd - fbw_q_f;
	fbw_r_err = fbw_r_cmd - fbw_r_f;

	const double kp_p = fbw_kp_p * gs.damping_gain;
	const double kp_q = fbw_kp_q * gs.damping_gain;
	const double kp_r = fbw_kp_r * gs.damping_gain;
	const double ki_p = fbw_ki_p * gs.damping_gain;
	const double ki_q = fbw_ki_q * gs.damping_gain;
	const double ki_r = fbw_ki_r * gs.damping_gain;

	const double ail_pre_norm = kp_p * fbw_p_err + ki_p * fbw_int_p;
	const double ele_pre_norm = kp_q * fbw_q_err + ki_q * fbw_int_q;
	const double rud_pre_norm = kp_r * fbw_r_err + ki_r * fbw_int_r;
	const double ail_sat_norm = limit(ail_pre_norm, -1.0, 1.0);
	const double ele_sat_norm = limit(ele_pre_norm, -1.0, 1.0);
	const double rud_sat_norm = limit(rud_pre_norm, -1.0, 1.0);

	fbw_int_p += (fbw_p_err + fbw_aw_gain * (ail_sat_norm - ail_pre_norm)) * dt;
	fbw_int_q += (fbw_q_err + fbw_aw_gain * (ele_sat_norm - ele_pre_norm)) * dt;
	fbw_int_r += (fbw_r_err + fbw_aw_gain * (rud_sat_norm - rud_pre_norm)) * dt;
	fbw_int_p = limit(fbw_int_p, -fbw_int_limit, fbw_int_limit);
	fbw_int_q = limit(fbw_int_q, -fbw_int_limit, fbw_int_limit);
	fbw_int_r = limit(fbw_int_r, -fbw_int_limit, fbw_int_limit);

	fbw_anti_windup_active =
		(fabs(ail_pre_norm - ail_sat_norm) > 1e-4) ||
		(fabs(ele_pre_norm - ele_sat_norm) > 1e-4) ||
		(fabs(rud_pre_norm - rud_sat_norm) > 1e-4);

	bool axis_sat_ail = false;
	bool axis_sat_ele = false;
	bool axis_sat_rud = false;
	aileron_command = fbw_apply_axis_actuator(
		ail_sat_norm, fbw_ail_limit_deg, fbw_ail_rate_deg_s, fbw_ail_lag_tau, dt,
		fbw_ail_rate_state_deg, fbw_ail_lag_state_deg,
		fbw_ail_cmd_pre, fbw_ail_cmd_sat, fbw_ail_cmd_rate, fbw_ail_cmd_lag, axis_sat_ail);
	elevator_command = fbw_apply_axis_actuator(
		ele_sat_norm, fbw_ele_limit_deg, fbw_ele_rate_deg_s, fbw_ele_lag_tau, dt,
		fbw_ele_rate_state_deg, fbw_ele_lag_state_deg,
		fbw_ele_cmd_pre, fbw_ele_cmd_sat, fbw_ele_cmd_rate, fbw_ele_cmd_lag, axis_sat_ele);
	rudder_command = fbw_apply_axis_actuator(
		rud_sat_norm, fbw_rud_limit_deg, fbw_rud_rate_deg_s, fbw_rud_lag_tau, dt,
		fbw_rud_rate_state_deg, fbw_rud_lag_state_deg,
		fbw_rud_cmd_pre, fbw_rud_cmd_sat, fbw_rud_cmd_rate, fbw_rud_cmd_lag, axis_sat_rud);

	fbw_actuator_sat = axis_sat_ail || axis_sat_ele || axis_sat_rud;
	if (fbw_actuator_sat)
	{
		fbw_actuator_sat_timer += dt;
	}
	else
	{
		fbw_actuator_sat_timer = limit(fbw_actuator_sat_timer - dt, 0.0, 10.0);
	}

	if (fbw_state == FBW_STATE_HOLD && fbw_actuator_sat_timer > cat.sat_time)
	{
		fbw_state = FBW_STATE_DEGRADE;
		fbw_hold_active = false;
		fbw_hold_exit_reason = FBW_HOLD_EXIT_ACTUATOR_SAT;
	}
}

// An example of how to interface with the Lua environment.
// Conventionally, parameter names are in all-caps.
void* fm_export_temperature = interface.getParamHandle("FM_TEMPERATURE_C");

// Test hook: Maxpower switch (set from cockpit Lua). When OFF (0) the EFM will zero engine thrust.
void* fm_param_maxpower = interface.getParamHandle("FM_MAXPOWER_SWITCH");
void* fm_param_maxpower_ready = interface.getParamHandle("FM_MAXPOWER_READY");

// Autopilot param handles (read from Lua autopilot_system.lua)
void* ap_param_master    = interface.getParamHandle("AP_MASTER_ENGAGED");
void* ap_param_pitch_cmd = interface.getParamHandle("AP_PITCH_CMD");
void* ap_param_roll_cmd  = interface.getParamHandle("AP_ROLL_CMD");
void* ap_param_thr_cmd   = interface.getParamHandle("AP_THROTTLE_CMD");
void* ap_param_bypass    = interface.getParamHandle("AP_BYPASS_ACTIVE");
void* ap_param_at_eng    = interface.getParamHandle("AP_AT_ENGAGED");

// AP state cached per frame
double ap_pitch_cmd_cached  = 0.0;
double ap_roll_cmd_cached   = 0.0;
double ap_throttle_cached   = 0.0;
bool   ap_master_cached     = false;
bool   ap_bypass_cached     = false;
bool   ap_at_engaged_cached = false;

static void update_autopilot_from_lua()
{
	ap_master_cached     = interface.getParamNumber(ap_param_master) > 0.5;
	ap_bypass_cached     = interface.getParamNumber(ap_param_bypass) > 0.5;
	ap_at_engaged_cached = interface.getParamNumber(ap_param_at_eng) > 0.5;

	if (ap_master_cached && !ap_bypass_cached)
	{
		ap_pitch_cmd_cached = limit(interface.getParamNumber(ap_param_pitch_cmd), -1.0, 1.0);
		ap_roll_cmd_cached  = limit(interface.getParamNumber(ap_param_roll_cmd), -1.0, 1.0);
	}
	else
	{
		ap_pitch_cmd_cached = 0.0;
		ap_roll_cmd_cached  = 0.0;
	}

	if (ap_at_engaged_cached)
	{
		ap_throttle_cached = limit(interface.getParamNumber(ap_param_thr_cmd), 0.0, 1.0);
	}
	else
	{
		ap_throttle_cached = 0.0;
	}
}

// Add force
void add_local_force(const Vec3 & Force, const Vec3 & Force_pos)
{
	common_force.x += Force.x;
	common_force.y += Force.y;
	common_force.z += Force.z;

	Vec3 delta_pos(Force_pos.x - center_of_mass.x,
				   Force_pos.y - center_of_mass.y,
				   Force_pos.z - center_of_mass.z);

	Vec3 delta_moment = cross(delta_pos, Force);

	common_moment.x += delta_moment.x;
	common_moment.y += delta_moment.y;
	common_moment.z += delta_moment.z;
}

// Add moment
void add_local_moment(const Vec3& Moment)
{
	common_moment.x += Moment.x;
	common_moment.y += Moment.y;
	common_moment.z += Moment.z;
}

void ed_fm_add_local_force(double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z)
{
	x = common_force.x;
	y = common_force.y;
	z = common_force.z;
	pos_x = center_of_mass.x;
	pos_y = center_of_mass.y;
	pos_z = center_of_mass.z;
}

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
void simulate_fuel_consumption(double dt)
{
	const double ab_avg = 0.5 * (left_afterburner_ratio + right_afterburner_ratio);
	const double ab_fuel_mult = 1.0 + ab_avg * (afterburner_fuel_factor - 1.0);

	// Fuel drain at full throttle in Kg/s. 
	fuel_consumption_since_last_time = FM_DATA::fuel_consumption * ((left_throttle_output + right_throttle_output + 1) / 3) * ab_fuel_mult * dt;

	if (external_fuel >= 0) // Drain external fuel first
	{
		if (fuel_consumption_since_last_time > external_fuel)
			fuel_consumption_since_last_time = external_fuel;
		external_fuel -= fuel_consumption_since_last_time;
	}
	else // Drain internal fuel
	{
		if (fuel_consumption_since_last_time > internal_fuel)
			fuel_consumption_since_last_time = internal_fuel;
		internal_fuel -= fuel_consumption_since_last_time;
	};
}

// The most important part of this whole thing.
// dt is apparently fixed to 0.006 seconds.
void ed_fm_simulate(double dt)
{
	fm_clock += dt;

	common_force = Vec3();
	common_moment = Vec3();

	// Update the force positions to be relative to the center of mass.
	// Somewhat unrealistic, but if this isn't done it usually leads to really weird flight behaviour.
	if (sim_inititalised == false)
	{
	left_wing_pos.x = center_of_mass.x - 0.7;
	left_wing_pos.y = center_of_mass.y + 0.5;

	right_wing_pos.x = center_of_mass.x - 0.7;
	right_wing_pos.y = center_of_mass.y + 0.5;

	tail_pos.x = center_of_mass.x - 0.5;
	tail_pos.y = center_of_mass.y;

	elevator_pos.y = center_of_mass.y;

	left_aileron_pos.x = center_of_mass.x;
	left_aileron_pos.y = center_of_mass.y;

	right_aileron_pos.x = center_of_mass.x;
	right_aileron_pos.y = center_of_mass.y;
	if (!kGroundConfigLogged)
	{
		const bool use_modelviewer_nodes = config_flag_is_true("SUSP_USE_MODELVIEWER_WHEEL_NODES");
		const bool geometry_test = config_flag_is_true("SUSP_GEOMETRY_TEST");
		const double radius_add = active_susp_radius_add();
		const double wheel_y_offset = active_susp_wheel_y_offset();
		const Vec3 final_pos_nose = active_susp_wheel_pos(0);
		const Vec3 final_pos_left = active_susp_wheel_pos(1);
		const Vec3 final_pos_right = active_susp_wheel_pos(2);
		char susp_test_mark[128];
		config_string_or_default("SUSP_TEST_MARK", "SUSP_TEST_MARK_NOT_FOUND", susp_test_mark, sizeof(susp_test_mark));
		const char* node_nose = nullptr;
		const char* node_left = nullptr;
		const char* node_right = nullptr;
		active_susp_node_names(node_nose, node_left, node_right);

		char cfg_buf[1536];
		snprintf(
			cfg_buf, sizeof(cfg_buf),
			"GROUND_CFG SUSP_TEST_MARK=%s efm_session=%s_%s SUSP_USE_MODELVIEWER_WHEEL_NODES=%d SUSP_GEOMETRY_TEST=%d "
			"radius_add=%.2f wheel_y_offset=%.2f gear_nodes=%s/%s/%s "
			"final_wheel_radius=%.4f/%.4f/%.4f "
			"final_wheel_pos=(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f) "
			"active_collision_shell=%s suspension_mode=%s fallback=%d",
			susp_test_mark,
			__DATE__,
			__TIME__,
			use_modelviewer_nodes ? 1 : 0,
			geometry_test ? 1 : 0,
			radius_add,
			wheel_y_offset,
			node_nose,
			node_left,
			node_right,
			kFallbackWheelRadius[0] + radius_add,
			kFallbackWheelRadius[1] + radius_add,
			kFallbackWheelRadius[2] + radius_add,
			final_pos_nose.x,
			final_pos_nose.y,
			final_pos_nose.z,
			final_pos_left.x,
			final_pos_left.y,
			final_pos_left.z,
			final_pos_right.x,
			final_pos_right.y,
			final_pos_right.z,
			kActiveCollisionShellName,
			kSuspensionModeName,
			kEnableFallbackGroundForces ? 1 : 0
		);
		dbg_susp(cfg_buf);
		susp_probe_log(cfg_buf);
		kGroundConfigLogged = true;
	}
	}

	// Actuator animation function for the moving parts
	gear_pos = limit(actuator(gear_pos, gear_switch, -0.001, 0.001), 0, 1); // Landing gear (all 3)
	airbrake_pos = limit(actuator(airbrake_pos, airbrake_switch, -0.003, 0.004), 0, 1); // Air brakes

	double flap_target = 0.0;
	if (flap_mode == FLAP_MODE_DOWN)
	{
		flap_target = 1.0;
	}
	else if (flap_mode == FLAP_MODE_AUTO)
	{
		const double ias_kts = V_scalar * 1.943844;
		if (gear_pos > 0.5 || ias_kts <= 240.0)
		{
			flap_target = 1.0;
		}
		else if (ias_kts >= 450.0)
		{
			flap_target = 0.0;
		}
		else
		{
			flap_target = 1.0 - ((ias_kts - 240.0) / (450.0 - 240.0));
		}
	}
	flaps_pos = limit(actuator(flaps_pos, flap_target, -0.002, 0.002), 0, 1); // Flaps
	slats_pos = limit(actuator(slats_pos, flap_target, -0.003, 0.003), 0, 1); // Slats track the flap command on the F-CK-1C
	nose_wheel_steering = limit(actuator(nose_wheel_steering, compute_nose_wheel_steering(), -0.06, 0.06), -1, 1);

#pragma region AERODYNAMICS
	airspeed.x = velocity_world.x - wind.x;
	airspeed.y = velocity_world.y - wind.y;
	airspeed.z = velocity_world.z - wind.z;

	V_scalar = sqrt(airspeed.x * airspeed.x + airspeed.y * airspeed.y + airspeed.z * airspeed.z);

	const double ground_speed = sqrt(velocity_world.x * velocity_world.x + velocity_world.z * velocity_world.z);
	const double spin_enable = ((gear_pos > 0.2) && (altitude_AGL < 2.5)) ? 1.0 : 0.0;
	for (int i = 0; i < 3; ++i)
	{
		const double wheel_circumference = 2.0 * pi * kFallbackWheelRadius[i];
		if (wheel_circumference > 1e-6)
		{
			wheel_spin[i] = fmod(wheel_spin[i] + (ground_speed / wheel_circumference) * dt * spin_enable, 1.0);
			if (wheel_spin[i] < 0.0)
			{
				wheel_spin[i] += 1.0;
			}
		}
	}

	mach = V_scalar / speed_of_sound;

	// Many coefficients are not static, they change with mach.
	// Here, we use a linear interpolation (lerp for short) function for these coefficients.
	// See the definition of the lerp function in ED_FM_Utility.h for more info on how it works.

	double CyAlpha_ = lerp(FM_DATA::mach_table, FM_DATA::Cya, sizeof(FM_DATA::mach_table) / sizeof(double), mach); // Lift
	double Cx0_ = lerp(FM_DATA::mach_table, FM_DATA::cx0, sizeof(FM_DATA::mach_table) / sizeof(double), mach); // Drag
	double CyMax_ = lerp(FM_DATA::mach_table, FM_DATA::CyMax, sizeof(FM_DATA::mach_table) / sizeof(double), mach); // Max lift
	double AlphaMax_ = lerp(FM_DATA::mach_table, FM_DATA::Aldop, sizeof(FM_DATA::mach_table) / sizeof(double), mach); // Max alpha
	double OmxMax_ = lerp(FM_DATA::mach_table, FM_DATA::OmxMax, sizeof(FM_DATA::mach_table) / sizeof(double), mach); // Max roll rate

	CyMax_ += (FM_DATA::cy_flap * 0.4 * slats_pos);	// Slats increase max lift coefficient.

	// Lift coefficient
	double Cy = CyAlpha_ * alpha;
	if (Cy > CyMax_)
		Cy = CyMax_;
	if (Cy < -CyMax_)
		Cy = -CyMax_;

	// Tail lift coefficient, defined as a guess
	double Cy_tail = (0.5 * CyAlpha_ + FM_DATA::Czbe) * beta;
	if (Cy_tail > CyMax_)
		Cy_tail = CyMax_;
	if (Cy_tail < -CyMax_)
		Cy_tail = -CyMax_;

	// Dynamic pressure (0.5 * rho * v^2)
	double q = 0.5 * atmosphere_density * V_scalar * V_scalar;

	// Update pilot inputs first, then run FBW once per frame.
	update_primary_control_inputs();

	// Autopilot: read commands from Lua and blend with pilot inputs.
	// AP sits above the FBW: it replaces stick commands, and the FBW
	// processes them identically to pilot inputs (rate limiting, gain
	// scheduling, safety features all remain active).
	update_autopilot_from_lua();
	if (ap_master_cached && !ap_bypass_cached)
	{
		// AP overrides pitch/roll inputs; pilot trim remains additive.
		pitch_input = ap_pitch_cmd_cached;
		roll_input  = ap_roll_cmd_cached;
	}
	// A/T: use existing FBW throttle blend infrastructure
	if (ap_at_engaged_cached)
	{
		fbw_throttle_cmd_left  = ap_throttle_cached;
		fbw_throttle_cmd_right = ap_throttle_cached;
		fbw_throttle_blend     = 1.0;
		fbw_throttle_override  = false;
	}
	else
	{
		fbw_throttle_blend     = 0.0;
	}

	update_fbw_controller(dt, q, AlphaMax_);

	// Lift/normal force, acts upwards
	double Lift = Cy + FM_DATA::Cy0 + (FM_DATA::cy_flap * flaps_pos);

	// Add lift/AoA-dependent drag so hard pulls trade energy for turn performance.
	const double induced_drag = (FM_DATA::cx_lift_k * Lift * Lift) +
		(FM_DATA::cx_alpha_k * aoa * aoa) +
		(FM_DATA::cx_elevator_k * fabs(elevator_command));

	// Drag force, acts backwards
	double Drag = Cx0_ + (FM_DATA::cx_brk * airbrake_pos) + (FM_DATA::cx_flap * flaps_pos) + (FM_DATA::cx_gear * gear_pos) + induced_drag;

	// Cheap, unrealistic, but effective aoa limiter
	if ((fabs(alpha) / AlphaMax_) >= 0.75)
	{
		left_wing_pos.x = center_of_mass.x - 0.7 - ( limit(pow((fabs(alpha) / (AlphaMax_ * 1.1)), 3) / 2000.0, 0, length / 3) + limit(-aos * 10, 0, 1) );
		right_wing_pos.x = center_of_mass.x - 0.7 - ( limit(pow((fabs(alpha) / (AlphaMax_ * 1.1)), 3) / 2000.0, 0, length / 3) + limit(aos * 10, 0, 1) );
	}
	else
	{
		left_wing_pos.x = center_of_mass.x - 0.7;
		right_wing_pos.x = center_of_mass.x - 0.7;
	};

	// Left wing forces
	Vec3 left_wing_forces(-Drag * (sin(-aos / 2) + 1) * q * (S / 2) * left_wing_integrity, Lift * (sin(-aos / 2) / 2 + 1) * q * (S / 2) * left_wing_integrity, 0);
	add_local_force(left_wing_forces, left_wing_pos);

	// Right wing forces
	Vec3 right_wing_forces(-Drag * (sin(aos / 2) + 1) * q * (S / 2) * right_wing_integrity, Lift * (sin(aos / 2) / 2 + 1) * q * (S / 2) * right_wing_integrity, 0);
	add_local_force(right_wing_forces, right_wing_pos);

	// Tail forces
	Vec3 tail_force(pow(-Cy_tail, 3) * sin(aoa) * (S / 2) * q * tail_integrity, 0, -Cy_tail * cos(aoa) * q * (S / 2) * tail_integrity);
	add_local_force(tail_force, tail_pos);
#pragma endregion

	// PITCH //
#pragma region PITCH

	// Elevator deflection plus default angle
	double elevator_deflection = (-(rescale(elevator_command + 0.15, rad(-25), rad(35))) * 18) * cos(aoa / 2);

	double pitch_stability = (aoa + sin(aoa / 2) / 2) + (pitch_rate * 2);

	add_local_force(Vec3(0, ((elevator_deflection * limit(1 - sqrt((mach + FM_DATA::mach_max * 0.4) / 3), 0.001, 1)) + (pitch_stability * (mach / 2 + 1))) * q, 0), elevator_pos);

#pragma endregion

	// ROLL //
#pragma region ROLL

	// Aileron deflection
	double aileron_deflection = rescale(aileron_command, rad(-30), rad(30)) * 4;

	double roll_stabilty = -roll_rate * (((fabs(aoa + 0.5) * fabs(aos + 0.5)) + 1) * (5 / wingspan)) +
		(sin(roll) / 2 * fabs(aoa / 2)); // Stability and correcting some of the rolling moment whhen in a turn.

	add_local_force(Vec3(0, (aileron_deflection + roll_stabilty) * q, 0), left_aileron_pos);
	add_local_force(Vec3(0, -(aileron_deflection + roll_stabilty) * q, 0), right_aileron_pos);

#pragma endregion

	// YAW //
#pragma region YAW

	// Rudder deflection
	double rudder_deflection = rescale(rudder_command, rad(-30), rad(30)) * 1.5;

	double yaw_stability = -((aos * 2) + yaw_rate);

	add_local_force(Vec3(0, 0, (rudder_deflection + yaw_stability) * q), rudder_pos);

#pragma endregion

	// ENGINE(S) AND THRUST //
#pragma region THRUST

	double max_dry_thrust = lerp(FM_DATA::engine_mach_table, FM_DATA::max_thrust, sizeof(FM_DATA::engine_mach_table) / sizeof(double), mach);
	double max_ab_thrust = max_dry_thrust * afterburner_thrust_factor;

	// FBW/autothrottle interface point:
	// Keep pilot throttle path intact, then optionally blend/override with FBW command.
	update_engine_throttle_inputs_from_interface();

	left_throttle_input = limit(left_throttle_input, 0, 1);
	right_throttle_input = limit(right_throttle_input, 0, 1);

	// Left engine
	if (left_engine_switch == false)
	{
		left_throttle_output = actuator(left_throttle_output, 0, -0.01, 0.01);
		left_engine_power_readout = actuator(left_engine_power_readout, 0.0, -dt / (FM_DATA::engine_start_time / 2), dt / (FM_DATA::engine_start_time / 2));
		left_throttle_input = limit(left_throttle_input, 0, 0);
	};

	if (left_engine_switch == true && left_engine_power_readout < 0.5)
	{
		left_engine_power_readout = actuator(left_engine_power_readout, 0.5, -dt / (FM_DATA::engine_start_time / 2), dt / (FM_DATA::engine_start_time / 2));
		left_throttle_input = limit(left_throttle_input, 0, 0.1);
	};

	if (left_engine_switch == true && left_engine_power_readout >= 0.5)
	{
		const double left_mil_cmd = limit(left_throttle_input / afterburner_detent, 0.0, 1.0);
		// Spool lag: TFE1042-70 does not respond instantaneously to throttle changes.
		// Apply a first-order lag so throttle_output tracks the target with realistic delay.
		const double left_throttle_target = limit(lerp(FM_DATA::throttle_input_table, FM_DATA::engine_power_table, sizeof(FM_DATA::throttle_input_table) / sizeof(float), left_mil_cmd), 0.1, 1.0);
		const double l_spool_tau = (left_throttle_target > left_throttle_output)
			? FM_DATA::engine_spool_up_tau
			: FM_DATA::engine_spool_down_tau;
		left_throttle_output = fbw_first_order(left_throttle_output, left_throttle_target, l_spool_tau, dt);
		left_throttle_output = limit(left_throttle_output, 0.1, 1.0);
		double left_target_core = 0.5 + 0.5 * left_mil_cmd;
		if (left_throttle_input <= afterburner_detent)
		{
			// Before AB: core reaches 100% at detent.
			left_target_core = limit(left_target_core, 0.0, 1.0);
		}
		else
		{
			left_target_core = afterburner_core_rpm;
		}
		// Smooth transition both directions (mil->AB and AB->mil).
		const double core_step = dt * ((1.0 - afterburner_core_rpm) / afterburner_core_drop_time);
		left_engine_power_readout = actuator(left_engine_power_readout, left_target_core, -core_step, core_step);
	};

	// Right engine
	if (right_engine_switch == false)
	{
		right_throttle_output = actuator(right_throttle_output, 0, -0.01, 0.01);
		right_engine_power_readout = actuator(right_engine_power_readout, 0.0, -dt / (FM_DATA::engine_start_time / 2), dt / (FM_DATA::engine_start_time / 2));
		right_throttle_input = limit(right_throttle_input, 0, 0);
	};

	if (right_engine_switch == true && right_engine_power_readout < 0.5)
	{
		right_engine_power_readout = actuator(right_engine_power_readout, 0.5, -dt / (FM_DATA::engine_start_time / 2), dt / (FM_DATA::engine_start_time / 2));
		right_throttle_input = limit(right_throttle_input, 0, 0.1);
	};

	if (right_engine_switch == true && right_engine_power_readout >= 0.5)
	{
		const double right_mil_cmd = limit(right_throttle_input / afterburner_detent, 0.0, 1.0);
		// Spool lag: same first-order lag as left engine.
		const double right_throttle_target = limit(lerp(FM_DATA::throttle_input_table, FM_DATA::engine_power_table, sizeof(FM_DATA::throttle_input_table) / sizeof(float), right_mil_cmd), 0.1, 1.0);
		const double r_spool_tau = (right_throttle_target > right_throttle_output)
			? FM_DATA::engine_spool_up_tau
			: FM_DATA::engine_spool_down_tau;
		right_throttle_output = fbw_first_order(right_throttle_output, right_throttle_target, r_spool_tau, dt);
		right_throttle_output = limit(right_throttle_output, 0.1, 1.0);
		double right_target_core = 0.5 + 0.5 * right_mil_cmd;
		if (right_throttle_input <= afterburner_detent)
		{
			// Before AB: core reaches 100% at detent.
			right_target_core = limit(right_target_core, 0.0, 1.0);
		}
		else
		{
			right_target_core = afterburner_core_rpm;
		}
		// Smooth transition both directions (mil->AB and AB->mil).
		const double core_step = dt * ((1.0 - afterburner_core_rpm) / afterburner_core_drop_time);
		right_engine_power_readout = actuator(right_engine_power_readout, right_target_core, -core_step, core_step);
	};

	// AB stage logic with ignition gate and spool lag.
	// Light condition : pilot pushes past detent AND engine throttle_output >= ab_light_throttle_output_min
	//                   (i.e., engine is already near military thrust).  Prevents instant AB at low power.
	// Extinguish      : pilot pulls below detent OR engine is shut down; ratio ramps to 0.
	// Nozzle follows the lagged afterburner_ratio, so opening/closing tracks actual AB state.
	{
		// --- Left AB ---
		const double left_ab_demand = limit((left_throttle_input - afterburner_detent) / (1.0 - afterburner_detent), 0.0, 1.0);
		if (left_engine_switch && left_throttle_input > afterburner_detent
			&& left_throttle_output >= ab_light_throttle_output_min)
		{
			left_afterburner_lit = true;
		}
		else if (!left_engine_switch || left_throttle_input <= afterburner_detent)
		{
			left_afterburner_lit = false;
		}
		const double left_ab_target  = left_afterburner_lit ? left_ab_demand : 0.0;
		const double left_ab_tau     = (left_ab_target > left_afterburner_ratio) ? ab_spool_in_tau : ab_spool_out_tau;
		left_afterburner_ratio  = limit(fbw_first_order(left_afterburner_ratio,  left_ab_target,  left_ab_tau,  dt), 0.0, 1.0);

		// --- Right AB ---
		const double right_ab_demand = limit((right_throttle_input - afterburner_detent) / (1.0 - afterburner_detent), 0.0, 1.0);
		if (right_engine_switch && right_throttle_input > afterburner_detent
			&& right_throttle_output >= ab_light_throttle_output_min)
		{
			right_afterburner_lit = true;
		}
		else if (!right_engine_switch || right_throttle_input <= afterburner_detent)
		{
			right_afterburner_lit = false;
		}
		const double right_ab_target = right_afterburner_lit ? right_ab_demand : 0.0;
		const double right_ab_tau    = (right_ab_target > right_afterburner_ratio) ? ab_spool_in_tau : ab_spool_out_tau;
		right_afterburner_ratio = limit(fbw_first_order(right_afterburner_ratio, right_ab_target, right_ab_tau, dt), 0.0, 1.0);
	}

	// Nozzle: dry-power schedule driven by throttle_output (follows actual engine state, not raw command).
	// Scale throttle_output back into the [0, afterburner_detent] range so the schedule's dry_ratio
	// matches the original calibration: throttle_output=1.0 at military maps to dry_ratio=1.0.
	const double left_nozzle_throttle  = left_throttle_output  * afterburner_detent;
	const double right_nozzle_throttle = right_throttle_output * afterburner_detent;
	const double left_nozzle_target  = estimate_nozzle_aperture_target(left_nozzle_throttle,  left_engine_power_readout,  left_afterburner_ratio,  left_engine_switch);
	const double right_nozzle_target = estimate_nozzle_aperture_target(right_nozzle_throttle, right_engine_power_readout, right_afterburner_ratio, right_engine_switch);
	left_nozzle_aperture = update_nozzle_aperture(left_nozzle_aperture, left_nozzle_target, left_engine_power_readout, left_afterburner_ratio, left_engine_switch, dt);
	right_nozzle_aperture = update_nozzle_aperture(right_nozzle_aperture, right_nozzle_target, right_engine_power_readout, right_afterburner_ratio, right_engine_switch, dt);

	double left_dry_force = left_throttle_output * max_dry_thrust * engine_alt_effect * left_engine_integrity * 0.5;
	double right_dry_force = right_throttle_output * max_dry_thrust * engine_alt_effect * right_engine_integrity * 0.5;
	double left_ab_extra = left_afterburner_ratio * (max_ab_thrust - max_dry_thrust) * engine_alt_effect * left_engine_integrity * 0.5;
	double right_ab_extra = right_afterburner_ratio * (max_ab_thrust - max_dry_thrust) * engine_alt_effect * right_engine_integrity * 0.5;

	left_thrust_force = left_dry_force + left_ab_extra;
	right_thrust_force = right_dry_force + right_ab_extra;

	left_engine_power_readout *= left_engine_integrity;
	right_engine_power_readout *= right_engine_integrity;

	// Engine shutdown
	if (internal_fuel <= 0 || altitude_ASL > 20000)
	{
		char shutdown_dbg[256];
		snprintf(
			shutdown_dbg, sizeof(shutdown_dbg),
			"ENG_SHUTDOWN fuel=%.3f asl=%.3f left_sw=%d right_sw=%d",
			internal_fuel,
			altitude_ASL,
			left_engine_switch ? 1 : 0,
			right_engine_switch ? 1 : 0
		);
		dbg_susp(shutdown_dbg);

		left_thrust_force = 0;
		right_thrust_force = 0;
		left_afterburner_ratio  = 0.0;
		right_afterburner_ratio = 0.0;
		left_afterburner_lit    = false;
		right_afterburner_lit   = false;
		left_engine_switch  = false;
		right_engine_switch = false;
		left_engine_power_readout = actuator(left_engine_power_readout, 0.0, -dt / 10, dt / 10);
		right_engine_power_readout = actuator(right_engine_power_readout, 0.0, -dt / 10, dt / 10);
	};

	//add_local_force(thrust, thrust_pos);

	// Apply Maxpower test switch only after the Lua side reports the param is initialised.
	// This prevents a missing/uninitialised cockpit device from silently killing all thrust.
	double maxpower_ready = 0.0;
	double maxpower_val = 1.0;
	if (fm_param_maxpower_ready != nullptr)
	{
		maxpower_ready = interface.getParamNumber(fm_param_maxpower_ready);
	}
	if (maxpower_ready > 0.5 && fm_param_maxpower != nullptr)
	{
		maxpower_val = interface.getParamNumber(fm_param_maxpower) > 0.5 ? 1.0 : 0.0;
	}
	if (maxpower_ready > 0.5 && maxpower_val < 0.5)
	{
		left_thrust_force = 0.0;
		right_thrust_force = 0.0;
	}

	// Apply thrust forces at engine positions
	add_local_force(Vec3(left_thrust_force, 0, 0), left_engine_pos);
	add_local_force(Vec3(right_thrust_force, 0, 0), right_engine_pos);

	// Structured debug output: left/right thrust, net moment from engines, suspension forces
	{
		char dbgline[768];
		Vec3 lforce(left_thrust_force, 0.0, 0.0);
		Vec3 rforce(right_thrust_force, 0.0, 0.0);
		Vec3 lm = cross(left_engine_pos, lforce);
		Vec3 rm = cross(right_engine_pos, rforce);
		Vec3 netm(lm.x + rm.x + common_moment.x, lm.y + rm.y + common_moment.y, lm.z + rm.z + common_moment.z);
		snprintf(dbgline, sizeof(dbgline), "THRUST L=%.3f R=%.3f NETM=(%.3f,%.3f,%.3f) SUSP=(%.3f,%.3f,%.3f) MAXPWR ready=%.1f sw=%.1f ENGSW=(%d,%d) THRIN=(%.3f,%.3f) THROUT=(%.3f,%.3f) CORE=(%.3f,%.3f) INTEG wing=(%.3f,%.3f) eng=(%.3f,%.3f) FUEL=%.1f",
			left_thrust_force, right_thrust_force,
			netm.x, netm.y, netm.z,
			suspension_force_mag[0], suspension_force_mag[1], suspension_force_mag[2],
			maxpower_ready, maxpower_val,
			left_engine_switch ? 1 : 0, right_engine_switch ? 1 : 0,
			left_throttle_input, right_throttle_input,
			left_throttle_output, right_throttle_output,
			left_engine_power_readout, right_engine_power_readout,
			left_wing_integrity, right_wing_integrity,
			left_engine_integrity, right_engine_integrity,
			internal_fuel);
		dbg_susp(dbgline);
	}

	if (infinite_fuel == false)
	{
		simulate_fuel_consumption(dt);
	};

#pragma endregion

	// MISC //
#pragma region MISC
	// Artificial limiters and other forces and moments.
	// Not exactly realistic, but added for convenience.

	double roll_yaw_moment = -(roll_rate / 2) * (q + 1e5 * 0.5); // Subtle yaw moment to keep stable in sharp turns
	add_local_moment(Vec3(0, roll_yaw_moment, 0));

	double roll_rate_limiter = -roll_rate * limit(pow((limit(fabs(roll_rate) / (OmxMax_ + 0.1), 0.0001, 2)), 6) * (q + q + 1e5 * 0.3), -1e7, 1e7);
	add_local_moment(Vec3(roll_rate_limiter, 0, 0));

	double yaw_rate_limiter = -(yaw_rate + aos) * (q + 1e5 * 0.5);
	add_local_moment(Vec3(0, yaw_rate_limiter, 0));

	// Only apply extra drag limiter when actually above design mach.
	double speed_limiter = 0.0;
	if (mach > FM_DATA::mach_max)
	{
		double over_mach = (mach - FM_DATA::mach_max) / FM_DATA::mach_max;
		speed_limiter = limit(pow(over_mach * 3.0, 2.0) * (q * 0.35 + 25000.0), 0.0, 6e5);
	}
	add_local_force(-speed_limiter, center_of_mass);

	// Speedbrake pitch compensation (F-CK-1C FLCS auto-trims for speedbrake deployment).
	// When the dorsal speedbrake opens it creates a pitch-down disturbance (reduced tail
	// effectiveness from changed downwash + drag couple below the net aerodynamic centre).
	// The real FLCS applies a nose-up feed-forward through the elevators before the Nz/alpha
	// outer loop saturates and the hold mode degrades.  We model this as a direct pitching
	// moment proportional to airbrake_pos * q * S * c_bar (dimensionally correct Cm scaling).
	// Positive z-moment = nose UP in the DCS body-axis convention.
	{
		const double c_bar = FM_DATA::wing_area / FM_DATA::wingspan; // mean aerodynamic chord [m]
		const double airbrake_pitch_comp =
			FM_DATA::airbrake_pitch_comp_k * airbrake_pos * q * FM_DATA::wing_area * c_bar;
		add_local_moment(Vec3(0, 0, airbrake_pitch_comp));
	}

	// Note about speed:
	// In DCS, if a plane goes faster than around 3100 Km/h (860 m/s) ground speed, it explodes. Even with invincibility on.

	// Additional optional artificial stuff for easier and more stable flight.
	if (easy_flight == true)
	{
		// Attitude stability.
		add_local_moment(Vec3(-(roll_rate / 4) * (1 - sqrt(fabs(aileron_command))) * (1e5 + q * 0.5),
			-(yaw_rate + (sin(aos) / 2)) * (1 - sqrt(fabs(rudder_command))) * (1e5 + q * 0.5),
			-(pitch_rate + (sin(aoa) / 2)) * (1 - sqrt(fabs(elevator_command))) * (1e5 + q * 0.5)));

		// Additional side (yaw) force.
		add_local_force(Vec3(0, 0, -rudder_command * (1e5 + q * 0.1)), Vec3(center_of_mass.x - 0.2, center_of_mass.y, 0));
	};

	fallback_ground_force = 0.0;
	fallback_ground_force = apply_fallback_ground_forces();

	log_startup_susp_probe(dt);

	if (kSuspProbeMode)
	{
		kSuspProbeTimer += dt;
		if (kSuspProbeTimer >= kSuspProbeInterval)
		{
			kSuspProbeTimer = 0.0;

			const char* node_nose = nullptr;
			const char* node_left = nullptr;
			const char* node_right = nullptr;
			active_susp_node_names(node_nose, node_left, node_right);
			const bool geometry_test = config_flag_is_true("SUSP_GEOMETRY_TEST");
			const double radius_add = active_susp_radius_add();
			const double wheel_y_offset = active_susp_wheel_y_offset();
			char susp_test_mark[128];
			config_string_or_default("SUSP_TEST_MARK", "SUSP_TEST_MARK_NOT_FOUND", susp_test_mark, sizeof(susp_test_mark));

			char probe_buf[1536];
			snprintf(
				probe_buf, sizeof(probe_buf),
				"SUSP_PROBE mode=1 SUSP_TEST_MARK=%s geometry_test=%d radius_add=%.2f wheel_y_offset=%.2f AGL=%.3f gear_down=%d nodes=%s/%s/%s "
				"valid=%d/%d/%d WOW=%d/%d/%d compression=%.5f/%.5f/%.5f force=%.1f/%.1f/%.1f "
				"brake_moment=%.3f/%.3f/%.3f input_only_raw_brake=%.3f/%.3f/%.3f "
				"input_only_raw_yaw=%.3f rudder_command=%.3f NWS_command=%.3f NWS_drawarg=%.3f",
				susp_test_mark,
				geometry_test ? 1 : 0,
				radius_add,
				wheel_y_offset,
				altitude_AGL,
				gear_pos > 0.5 ? 1 : 0,
				node_nose,
				node_left,
				node_right,
				suspension_feedback_valid[0] ? 1 : 0,
				suspension_feedback_valid[1] ? 1 : 0,
				suspension_feedback_valid[2] ? 1 : 0,
				suspension_wow[0] ? 1 : 0,
				suspension_wow[1] ? 1 : 0,
				suspension_wow[2] ? 1 : 0,
				suspension_compression[0],
				suspension_compression[1],
				suspension_compression[2],
				suspension_force_mag[0],
				suspension_force_mag[1],
				suspension_force_mag[2],
				0.0,
				limit(wheel_brake_left, 0.0, 1.0),
				limit(wheel_brake_right, 0.0, 1.0),
				wheel_brake,
				wheel_brake_left,
				wheel_brake_right,
				yaw_input,
				rudder_command,
				compute_nose_wheel_steering(),
				nose_wheel_steering
			);
			susp_probe_log(probe_buf);
		}
	}

	if (++kFallbackLogDecimation >= 20)
	{
		char ground_buf[768];
		snprintf(
			ground_buf, sizeof(ground_buf),
			"ground agl=%.3f h=%.3f h_obj=%.3f surf=%u vy=%.3f gear=%.2f mass=%.1f native=(valid:%d wow:%d comp:%.4f/%.4f/%.4f force:%.1f/%.1f/%.1f) fallback=(enabled:%d wow:%d fg:%.1f comp:%.4f/%.4f/%.4f) pitch=%.2f roll=%.2f nws=%d yaw=%.2f nwsarg=%.2f brk=%.2f/%.2f",
			altitude_AGL,
			surface_height_raw,
			surface_height_with_objects,
			surface_type_raw,
			velocity_world.y,
			gear_pos,
			current_mass,
			has_suspension_feedback() ? 1 : 0,
			any_wow() ? 1 : 0,
			suspension_compression[0],
			suspension_compression[1],
			suspension_compression[2],
			suspension_force_mag[0],
			suspension_force_mag[1],
			suspension_force_mag[2],
			kEnableFallbackGroundForces ? 1 : 0,
			any_fallback_wow() ? 1 : 0,
			fallback_ground_force,
			fallback_suspension_compression[0],
			fallback_suspension_compression[1],
			fallback_suspension_compression[2],
			pitch * rad_to_deg,
			roll * rad_to_deg,
			nose_turn_enabled ? 1 : 0,
			yaw_input,
			nose_wheel_steering,
			wheel_brake_left,
			wheel_brake_right
		);
		dbg_susp(ground_buf);
		kFallbackLogDecimation = 0;
	}

	on_ground = ((gear_pos > 0.5) && has_suspension_feedback() && any_wow());

	// Cockpit shaking intensity
	shake_amplitude = 0; // Starts at zero every frame

	shake_amplitude += limit((FM_DATA::cx_brk + 1) * airbrake_pos * mach, 0, 2) / 6; // Air brakes

	if (on_ground == false)
	{
		if (fabs(alpha) > 10) // High angle of attack
			shake_amplitude += (fabs(alpha) - 10) / 100;

		if (fabs(beta) > 10) // High angle of slide
			shake_amplitude += (fabs(beta) - 10) / 100;

		if (fabs(g) > 5) // High g
			shake_amplitude += (fabs(g) - 5) / 100;

		if (mach > FM_DATA::mach_max * 0.8) // Approaching maximum speed
			shake_amplitude += (mach - (FM_DATA::mach_max * 0.8)) / 2;
	};

	// if (on_ground && gear_pos > 0.5 && fabs(nose_wheel_steering) > 0.01)
	// {
	// 	const double speed_ms = limit(sqrt(velocity_body.x * velocity_body.x + velocity_body.z * velocity_body.z), 0.0, 55.0);
	// 	const double steering_gain = limit(1.0 - (speed_ms / 70.0), 0.15, 1.0);
	// 	const double side_force = current_mass * 9.81 * 0.10 * steering_gain * nose_wheel_steering;
	// 	add_local_force(Vec3(0.0, 0.0, side_force), kFallbackGearPoints[0]);
	// }
#pragma endregion

	sim_inititalised = true; // The first step is complete
}

// Atmosphere data
void ed_fm_set_atmosphere(double h, //altitude above sea level
							double t, // current atmosphere temperature in Kelvin
							double a, // speed of sound
							double ro, // atmosphere density
							double p, // atmosphere pressure
							double wind_vx, double wind_vy, double wind_vz // components of velocity vector, including turbulence in world coordinate system
						)

{
	wind.x = wind_vx;
	wind.y = wind_vy;
	wind.z = wind_vz;

	atmosphere_density = ro;
	speed_of_sound     = a;

	altitude_ASL = h;

	engine_alt_effect = limit(pow(1 - (h / 30000), 0.3), 0.1, 1); 

	atmosphere_temperature = t;

	// FM interface example: exporting the current outside temperature in degrees Celsius.
	interface.setParamNumber(fm_export_temperature, t + 273);
}

void ed_fm_set_surface(double h, // distance between sea level and the surface/ground
	double h_obj, // h but with objects
	unsigned surface_type, // type of surface under the aircraft?
	double normal_x, double normal_y, double normal_z // components of normal vector to surface
)
{
	surface_height_raw = h;
	surface_height_with_objects = h_obj;
	surface_type_raw = surface_type;
	altitude_AGL = altitude_ASL - h;
}

// Called before simulation to set up your environment for the next step
void ed_fm_set_current_mass_state (double mass,
									double center_of_mass_x, double center_of_mass_y, double center_of_mass_z,
									double moment_of_inertia_x, double moment_of_inertia_y, double moment_of_inertia_z
									)
{
	current_mass = mass;
	center_of_mass.x  = center_of_mass_x;
	center_of_mass.y  = center_of_mass_y;
	center_of_mass.z  = center_of_mass_z;
}

// Called before simulation to set up your environment for the next step
void ed_fm_set_current_state (double ax, double ay, double az,//linear acceleration component in world coordinate system
							double vx, double vy, double vz,//linear velocity component in world coordinate system
							double px, double py, double pz,//center of the body position in world coordinate system
							double omegadotx, double omegadoty, double omegadotz,//angular accelearation components in world coordinate system
							double omegax, double omegay, double omegaz, //angular velocity components in world coordinate system 
							double quaternion_x, double quaternion_y, double quaternion_z, double quaternion_w //orientation quaternion components in world coordinate system
							)
{
	velocity_world.x = vx;
	velocity_world.y = vy;
	velocity_world.z = vz;
	angular_velocity_world.x = omegax;
	angular_velocity_world.y = omegay;
	angular_velocity_world.z = omegaz;
	position_world_z = pz;
}


// Called before simulation to set up your environment for the next step
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
	velocity_body.x = vx;
	velocity_body.y = vy;
	velocity_body.z = vz;
	angular_velocity_body.x = omegax;
	angular_velocity_body.y = omegay;
	angular_velocity_body.z = omegaz;

	aoa = common_angle_of_attack;
	alpha = common_angle_of_attack * rad_to_deg;

	aos = common_angle_of_slide;
	beta = common_angle_of_slide * rad_to_deg; 
	// Positive aos is yaw left, negative is right.
	// Positive aos means more wind on the right wing, negative on the left wing.

	g = (ay / 9.81) + 1; // 1 g is -9.81 m/s^2, Earth's gravity. 

	FM::pitch = pitch;
	FM::roll = roll;
	FM::heading = yaw;

	roll_rate = omegax;
	yaw_rate = omegay;
	pitch_rate = omegaz;
}

// Input handling
void ed_fm_set_command (int command, float value)
{
	// See Inputs.h
	switch (command)
	{

	// Flight controls

	// Pitch

	case JoystickPitch: //iCommandPlanePitch
		pitch_input = limit(value, -1, 1);
		pitch_analog = true;
		pitch_discrete = 0;
		break;

	case PitchUp:
		pitch_discrete = 1;
		pitch_analog = false;
		//pitch_acc = 1;
		break;
	case PitchUpStop:
		pitch_discrete = 0;
		pitch_analog = false;
		break;

	case PitchDown:
		pitch_discrete = -1;
		pitch_analog = false;
		break;
	case PitchDownStop:
		pitch_discrete = 0;
		pitch_analog = false;
		break;

	case trimUp:
		pitch_trim += 0.0015;
		break;
	case trimDown:
		pitch_trim -= 0.0015;
		break;

	// Roll

	case JoystickRoll: //iCommandPlaneRoll
		roll_input = limit(value, -1, 1);
		roll_analog = true;
		roll_discrete = 0;
		break;

	case RollLeft:
		roll_discrete = -1;
		roll_analog = false;
		break;
	case RollLeftStop:
		roll_discrete = 0;
		roll_analog = false;
		break;

	case RollRight:
		roll_discrete = 1;
		roll_analog = false;
		break;
	case RollRightStop:
		roll_discrete = 0;
		roll_analog = false;
		break;

	case trimLeft:
		roll_trim -= 0.001;
		break;
	case trimRight:
		roll_trim += 0.001;
		break;

	// Yaw

	case PedalYaw: //Yaw
		yaw_input = limit(-value, -1, 1);
		yaw_discrete = 0;
		yaw_analog = true;
		break;

	case rudderleft:
		yaw_discrete = 1;
		yaw_analog = false;
		break;
	case rudderleftstop:
		yaw_discrete = 0;
		yaw_analog = false;
		break;

	case rudderright:
		yaw_discrete = -1;
		yaw_analog = false;
		break;
	case rudderrightstop:
		yaw_discrete = 0;
		yaw_analog = false;
		break;

	case ruddertrimLeft:
		yaw_trim += 0.001;
		break;
	case ruddertrimRight:
		yaw_trim -= 0.001;
		break;

	case resetTrim:
		pitch_trim = 0;
		roll_trim = 0;
		yaw_trim = 0;
		break;

	case FBWCatToggle:
		if (value > 0.5f)
		{
			fbw_mode_target = (fbw_mode_target == FBW_CAT1) ? FBW_CAT3 : FBW_CAT1;
		}
		break;
	case FBWCat1:
		if (value > 0.5f)
		{
			fbw_mode_target = FBW_CAT1;
		}
		break;
	case FBWCat3:
		if (value > 0.5f)
		{
			fbw_mode_target = FBW_CAT3;
		}
		break;
	case FBWGLimiterOverride:
		fbw_g_limiter_override = (value > 0.5f);
		break;
	case FBWGLimiterOverrideToggle:
		if (value > 0.5f)
		{
			fbw_g_limiter_override = !fbw_g_limiter_override;
		}
		break;

	//	Engine and throttle commands

	case EnginesOn: // Both engines
		left_engine_switch = true;
		right_engine_switch = true;
		break;
	case LeftEngineOn:
		left_engine_switch = true;
		break;
	case RightEngineOn:
		right_engine_switch = true;
		break;

	case EnginesOff: // Both engines
		left_engine_switch = false;
		right_engine_switch = false;
		break;
	case LeftEngineOff:
		left_engine_switch = false;
		break;
	case RightEngineOff:
		right_engine_switch = false;
		break;

	case ThrottleAxis://iCommandPlaneThrustCommon
		{
			const double normalized = normalize_throttle_axis(value);
			if (fabs(normalized - throttle_axis_cmd_left) > 1e-4)
			{
				throttle_use_axis_left = true;
			}
			if (fabs(normalized - throttle_axis_cmd_right) > 1e-4)
			{
				throttle_use_axis_right = true;
			}
			throttle_axis_cmd_left = normalized;
			throttle_axis_cmd_right = normalized;
		}
		break;
	case ThrottleAxisLeft:
		{
			const double normalized = normalize_throttle_axis(value);
			if (fabs(normalized - throttle_axis_cmd_left) > 1e-4)
			{
				throttle_use_axis_left = true;
			}
			throttle_axis_cmd_left = normalized;
		}
		break;
	case ThrottleAxisRight:
		{
			const double normalized = normalize_throttle_axis(value);
			if (fabs(normalized - throttle_axis_cmd_right) > 1e-4)
			{
				throttle_use_axis_right = true;
			}
			throttle_axis_cmd_right = normalized;
		}
		break;

	case ThrottleIncrease: // Both engines
		throttle_keyboard_cmd_left = limit(resolve_keyboard_throttle_base(throttle_axis_cmd_left, throttle_keyboard_cmd_left, throttle_use_axis_left) + 0.0075, 0.0, 1.0);
		throttle_keyboard_cmd_right = limit(resolve_keyboard_throttle_base(throttle_axis_cmd_right, throttle_keyboard_cmd_right, throttle_use_axis_right) + 0.0075, 0.0, 1.0);
		throttle_use_axis_left = false;
		throttle_use_axis_right = false;
		break;
	case ThrottleLeftUp:
		throttle_keyboard_cmd_left = limit(resolve_keyboard_throttle_base(throttle_axis_cmd_left, throttle_keyboard_cmd_left, throttle_use_axis_left) + 0.0075, 0.0, 1.0);
		throttle_use_axis_left = false;
		break;
	case ThrottleRightUp:
		throttle_keyboard_cmd_right = limit(resolve_keyboard_throttle_base(throttle_axis_cmd_right, throttle_keyboard_cmd_right, throttle_use_axis_right) + 0.0075, 0.0, 1.0);
		throttle_use_axis_right = false;
		break;

	case ThrottleDecrease: // Both engines
		throttle_keyboard_cmd_left = limit(resolve_keyboard_throttle_base(throttle_axis_cmd_left, throttle_keyboard_cmd_left, throttle_use_axis_left) - 0.0075, 0.0, 1.0);
		throttle_keyboard_cmd_right = limit(resolve_keyboard_throttle_base(throttle_axis_cmd_right, throttle_keyboard_cmd_right, throttle_use_axis_right) - 0.0075, 0.0, 1.0);
		throttle_use_axis_left = false;
		throttle_use_axis_right = false;
		break;
	case ThrottleLeftDown:
		throttle_keyboard_cmd_left = limit(resolve_keyboard_throttle_base(throttle_axis_cmd_left, throttle_keyboard_cmd_left, throttle_use_axis_left) - 0.0075, 0.0, 1.0);
		throttle_use_axis_left = false;
		break;
	case ThrottleRightDown:
		throttle_keyboard_cmd_right = limit(resolve_keyboard_throttle_base(throttle_axis_cmd_right, throttle_keyboard_cmd_right, throttle_use_axis_right) - 0.0075, 0.0, 1.0);
		throttle_use_axis_right = false;
		break;
	case ThrottleStop:
		// Release of keyboard throttle commands should stop the stepping action,
		// not snap the commanded throttle back to idle.
		break;

	// Other commands

	case AirBrakes: //toggle
		if (airbrake_switch == false)
			airbrake_switch = true;
		else if (airbrake_switch == true)
			airbrake_switch = false;
		break;
	case AirBrakesOff:
		airbrake_switch = false;
		break;
	case AirBrakesOn:
		airbrake_switch = true;
		break;
	case AirBrakesAuto:
		break;
	case AirBrakesUp:
		airbrake_switch = false;
		break;
	case AirBrakesDown:
		airbrake_switch = true;
		break;

	case flapsToggle: //toggle
		if (flap_mode == FLAP_MODE_DOWN)
			flap_mode = FLAP_MODE_UP;
		else
			flap_mode = FLAP_MODE_DOWN;
		break;
	case flapsDown:
		flap_mode = FLAP_MODE_DOWN;
		break;
	case flapsUp:
		flap_mode = FLAP_MODE_UP;
		break;
	case FlapsAuto:
		flap_mode = FLAP_MODE_AUTO;
		break;
	case FlapsUpCmd:
		flap_mode = FLAP_MODE_UP;
		break;
	case FlapsDownCmd:
		flap_mode = FLAP_MODE_DOWN;
		break;

	case gearToggle:
		if (gear_switch == true)
			gear_switch = false;
		else if (gear_switch == false)
			gear_switch = true;
		break;
	case gearDown:
		gear_switch = true;
		break;
	case gearUp:
		gear_switch = false;
		break;
	case GearAuto:
		break;
	case GearHandleUp:
		gear_switch = false;
		break;
	case GearHandleDown:
		gear_switch = true;
		break;
	case NoseTurnToggle:
		if (value > 0.5f)
		{
			nose_turn_enabled = !nose_turn_enabled;
		}
		break;
	case NoseTurnUp:
		nose_turn_enabled = false;
		break;
	case NoseTurnAuto:
		nose_turn_enabled = false;
		break;
	case NoseTurnDown:
		nose_turn_enabled = true;
		break;
	case WheelBrakeAxis:
		wheel_brake = normalize_brake_axis(value);
		wheel_brake_left = wheel_brake;
		wheel_brake_right = wheel_brake;
		break;
	case WheelBrakeAxisLeft:
		wheel_brake_left = normalize_brake_axis(value);
		wheel_brake = (wheel_brake_left + wheel_brake_right) * 0.5;
		break;
	case WheelBrakeAxisRight:
		wheel_brake_right = normalize_brake_axis(value);
		wheel_brake = (wheel_brake_left + wheel_brake_right) * 0.5;
		break;

	case WheelBrakeOn:
		wheel_brake = 1;
		wheel_brake_left = 1;
		wheel_brake_right = 1;
		break;
	case WheelBrakeOff:
		wheel_brake = 0;
		wheel_brake_left = 0;
		wheel_brake_right = 0;
		break;
	case WheelBrakeLeftOn:
		wheel_brake_left = 1;
		wheel_brake = (wheel_brake_left + wheel_brake_right) * 0.5;
		break;
	case WheelBrakeLeftOff:
		wheel_brake_left = 0;
		wheel_brake = (wheel_brake_left + wheel_brake_right) * 0.5;
		break;
	case WheelBrakeRightOn:
		wheel_brake_right = 1;
		wheel_brake = (wheel_brake_left + wheel_brake_right) * 0.5;
		break;
	case WheelBrakeRightOff:
		wheel_brake_right = 0;
		wheel_brake = (wheel_brake_left + wheel_brake_right) * 0.5;
		break;

	}

}

/*
	Mass handling 

	will be called  after ed_fm_simulate :
	you should collect mass changes in ed_fm_simulate 

	double delta_mass = 0;
	double x = 0;
	double y = 0; 
	double z = 0;
	double piece_of_mass_MOI_x = 0;
	double piece_of_mass_MOI_y = 0; 
	double piece_of_mass_MOI_z = 0;
 
	//
	while (ed_fm_change_mass(delta_mass,x,y,z,piece_of_mass_MOI_x,piece_of_mass_MOI_y,piece_of_mass_MOI_z))
	{
	//internal DCS calculations for changing mass, center of gravity,  and moments of inertia
	}
*/

bool ed_fm_change_mass  (double & delta_mass,
						double & delta_mass_pos_x,
						double & delta_mass_pos_y,
						double & delta_mass_pos_z,
						double & delta_mass_moment_of_inertia_x,
						double & delta_mass_moment_of_inertia_y,
						double & delta_mass_moment_of_inertia_z
						)
{
	if (fuel_consumption_since_last_time > 0)
	{
		delta_mass		 = fuel_consumption_since_last_time;
		delta_mass_pos_x = -1.0;
		delta_mass_pos_y =  1.0;
		delta_mass_pos_z =  0;

		delta_mass_moment_of_inertia_x	= 0;
		delta_mass_moment_of_inertia_y	= 0;
		delta_mass_moment_of_inertia_z	= 0;

		fuel_consumption_since_last_time = 0; // set it 0 to avoid infinite loop, because it called in cycle 
		// better to use stack like structure for mass changing 
		return true;
	}
	else 
	{
		return false;
	}
}

// Set internal fuel volume , init function, called on object creation and for refueling
void   ed_fm_set_internal_fuel(double fuel)
{
	internal_fuel = fuel;
}

// Get internal fuel volume 
double ed_fm_get_internal_fuel()
{
	return internal_fuel;
}

// Set external fuel volume for each payload station, called for weapon init and on reload.
void  ed_fm_set_external_fuel (int	 station,
								double fuel,
								double x, double y, double z)
{
	// Not sure how to work with this.
}

// Get external fuel volume
double ed_fm_get_external_fuel ()
{
	return 0;
}

// This stuff controls "arguments", which are mostly moving parts, pylons, lights, etc on the aircraft's model.
void ed_fm_set_draw_args (EdDrawArgument * drawargs,size_t size)
{
	//See the model viewer on your aircraft model for arguments on the aircraft.

	// Landing gear
	drawargs[0].f = (float)limit(gear_pos, 0, 1); // Nose
	drawargs[3].f = (float)limit(gear_pos, 0, 1); // Right
	drawargs[5].f = (float)limit(gear_pos, 0, 1); // Left
	drawargs[2].f = (float)limit(nose_wheel_steering, -1, 1); // Nose wheel steering
	// drawargs[1].f = (float)suspension_visual_arg(0); // Nose shock absorber
	// drawargs[6].f = (float)suspension_visual_arg(1); // Left main shock absorber
	// drawargs[4].f = (float)suspension_visual_arg(2); // Right main shock absorber

	// Elevators/stabilators
	drawargs[15].f = (float)limit(elevator_command, -1, 1);
	drawargs[16].f = (float)limit(elevator_command, -1, 1);

	// On this model, flap-related trailing-edge surfaces use negative drawarg
	// values for trailing-edge-down deflection. Keep the aerodynamic flap state
	// positive in FM logic and only invert the visual mapping here.
	const double flap_visual = -limit(flaps_pos, 0.0, 1.0);
	drawargs[11].f = (float)limit(flap_visual + aileron_command, -1, 1); // Right flaperon
	drawargs[12].f = (float)limit(flap_visual - aileron_command, -1, 1); // Left flaperon

	// Rudder(s)
	drawargs[17].f = (float)limit(rudder_command, -1, 1);
	drawargs[18].f = (float)limit(rudder_command, -1, 1);

	// Airbrake(s)
	drawargs[21].f = (float)limit(airbrake_pos, 0, 1);
	drawargs[182].f = (float)limit(airbrake_pos, 0, 1);
	drawargs[184].f = (float)limit(airbrake_pos, 0, 1);

	// Afterburner intensity
	drawargs[28].f = (float)limit(left_afterburner_ratio, 0, 1);
	drawargs[29].f = (float)limit(right_afterburner_ratio, 0, 1);

	// Nozzle aperture: MV2-confirmed mapping is 89 = right engine, 90 = left engine.
	drawargs[89].f = (float)limit(right_nozzle_aperture, 0, 1);
	drawargs[90].f = (float)limit(left_nozzle_aperture, 0, 1);

	// Practical model mapping based on in-sim verification:
	// 9/10 behave like the leading-edge slot pieces, while 11/12 are the flaperons.
	const double slat_visual = limit(slats_pos, 0.0, 1.0);
	drawargs[9].f = (float)slat_visual;
	drawargs[10].f = (float)slat_visual;

	// Wheel spin bones: 76 = nose, 101 = left main, 102 = right main.
	drawargs[76].f = (float)wheel_spin[0];
	drawargs[101].f = (float)wheel_spin[1];
	drawargs[102].f = (float)wheel_spin[2];

	/*
	Hints on some aircraft args where applicable

	25 is the tail hook or weapons bay on some aircraft

	115 to 117 are gear doors

	7 is wing sweep

	28 and 29 are left and right afterburners

	89 and 90 are engine nozzle apertures (this module uses 89 = right, 90 = left)

	40 and 41 are helicopter rotors

	407 to 410 are propellers
	*/
}

void ed_fm_configure(const char * cfg_path)
{
	// Not sure what this does.
}

static inline double engine_display_related_rpm(double core_readout)
{
	const double idle_related_rpm = 0.675;
	const double clamped = limit(core_readout, 0.0, 1.0);
	if (clamped <= 0.5)
	{
		return (clamped / 0.5) * idle_related_rpm;
	}
	return idle_related_rpm + ((clamped - 0.5) / 0.5) * (1.0 - idle_related_rpm);
}

// Interface with default parameters like gear and engines
double ed_fm_get_param(unsigned index)
{
	const double nominal_core_rpm = 14710.0;
	const double nominal_fan_rpm = 8215.0;
	const double left_core_related_rpm = engine_display_related_rpm(left_engine_power_readout);
	const double right_core_related_rpm = engine_display_related_rpm(right_engine_power_readout);
	const double left_fan_related_rpm = left_engine_switch ? left_core_related_rpm : 0.0;
	const double right_fan_related_rpm = right_engine_switch ? right_core_related_rpm : 0.0;
	const double left_core_rpm = left_core_related_rpm * nominal_core_rpm;
	const double right_core_rpm = right_core_related_rpm * nominal_core_rpm;
	const double left_fan_rpm = left_fan_related_rpm * nominal_fan_rpm;
	const double right_fan_rpm = right_fan_related_rpm * nominal_fan_rpm;

	switch (index)
	{
		case ED_FM_SUSPENSION_0_WHEEL_YAW: // Nose wheel steering
		{
			const bool wow = has_suspension_feedback() && any_wow();
			const bool nws_valid = wow && (gear_pos > 0.5);
			return nws_valid ? nose_wheel_steering : 0.0;
		}

		case ED_FM_SUSPENSION_0_WHEEL_SELF_ATTITUDE:
			return wheel_spin[0];
		case ED_FM_SUSPENSION_1_WHEEL_SELF_ATTITUDE:
			return wheel_spin[1];
		case ED_FM_SUSPENSION_2_WHEEL_SELF_ATTITUDE:
			return wheel_spin[2];

		case ED_FM_SUSPENSION_0_RELATIVE_BRAKE_MOMENT:
			return 0.0;
		case ED_FM_SUSPENSION_1_RELATIVE_BRAKE_MOMENT:
			return limit(wheel_brake_left, 0.0, 1.0);
		case ED_FM_SUSPENSION_2_RELATIVE_BRAKE_MOMENT:
			return limit(wheel_brake_right, 0.0, 1.0);

		case ED_FM_ANTI_SKID_ENABLE:
			return true;

		case ED_FM_FC3_STICK_PITCH:
			return limit(pitch_input, -1.0, 1.0);

		case ED_FM_FC3_STICK_ROLL:
			return limit(roll_input, -1.0, 1.0);

		case ED_FM_FC3_RUDDER_PEDALS:
			return limit(-yaw_input, -1.0, 1.0);

		case ED_FM_FC3_WHEEL_BRAKE_LEFT:
		case ED_FM_FC3_WHEEL_BRAKE_COMMAND_LEFT:
			return limit(wheel_brake_left, 0.0, 1.0);

		case ED_FM_FC3_WHEEL_BRAKE_RIGHT:
		case ED_FM_FC3_WHEEL_BRAKE_COMMAND_RIGHT:
			return limit(wheel_brake_right, 0.0, 1.0);

		case ED_FM_FC3_THROTTLE_LEFT:
			if (left_engine_switch == false)
				return limit(left_throttle_input, 0.0, 0.1);
			else
				return limit(left_throttle_input, 0.1, 1.0);

		case ED_FM_FC3_THROTTLE_RIGHT:
			if (right_engine_switch == false)
				return limit(right_throttle_input, 0.0, 0.1);
			else
				return limit(right_throttle_input, 0.1, 1.0);

		case ED_FM_FUEL_INTERNAL_FUEL:
			return internal_fuel;
		case ED_FM_FUEL_TOTAL_FUEL:
			return total_fuel;

		case ED_FM_OXYGEN_SUPPLY:
			return 101000.0;

		case ED_FM_FLOW_VELOCITY:
			return 10.0;

		case ED_FM_SUSPENSION_0_GEAR_POST_STATE:
		case ED_FM_SUSPENSION_1_GEAR_POST_STATE:
		case ED_FM_SUSPENSION_2_GEAR_POST_STATE:
			return gear_pos;	// Landing gear states, combined

		if (index <= ED_FM_END_ENGINE_BLOCK)
		{

			// APU, doesn't make sounds.
		case ED_FM_ENGINE_0_RPM:
		case ED_FM_ENGINE_0_RELATED_RPM:
			return 1;
		case ED_FM_ENGINE_0_THRUST:
		case ED_FM_ENGINE_0_RELATED_THRUST:
			return 0;

			// Engine 1, left
		case ED_FM_ENGINE_1_CORE_RPM:
			return left_core_rpm;
		case ED_FM_ENGINE_1_RPM:
			return left_fan_rpm;
		case ED_FM_ENGINE_1_COMBUSTION:
			return left_engine_switch ? limit(left_engine_power_readout * 2.0, 0.0, 1.0) : 0.0;

		case ED_FM_ENGINE_1_RELATED_THRUST: // low frequency rumble
			return left_throttle_output;
		case ED_FM_ENGINE_1_CORE_RELATED_THRUST:
			return left_throttle_output;
		case ED_FM_ENGINE_1_RELATED_RPM:
			return left_fan_related_rpm;
		case ED_FM_ENGINE_1_CORE_RELATED_RPM: // RPM readout and core sound
			return left_core_related_rpm;

		case ED_FM_ENGINE_1_CORE_THRUST:
		case ED_FM_ENGINE_1_THRUST:
			return left_thrust_force;
		case ED_FM_ENGINE_1_TEMPERATURE:
			return (pow(left_engine_power_readout, 3) * 500) + atmosphere_temperature;

			// Engine 2, right
		case ED_FM_ENGINE_2_CORE_RPM:
			return right_core_rpm;
		case ED_FM_ENGINE_2_RPM:
			return right_fan_rpm;
		case ED_FM_ENGINE_2_COMBUSTION:
			return right_engine_switch ? limit(right_engine_power_readout * 2.0, 0.0, 1.0) : 0.0;

		case ED_FM_ENGINE_2_RELATED_THRUST: // low frequency rumble
			return right_throttle_output;
		case ED_FM_ENGINE_2_CORE_RELATED_THRUST:
			return right_throttle_output;
		case ED_FM_ENGINE_2_RELATED_RPM:
			return right_fan_related_rpm;
		case ED_FM_ENGINE_2_CORE_RELATED_RPM: // RPM readout and core sound
			return right_core_related_rpm;

		case ED_FM_ENGINE_2_CORE_THRUST:
		case ED_FM_ENGINE_2_THRUST:
			return right_thrust_force;
		case ED_FM_ENGINE_2_TEMPERATURE:
			return (pow(right_engine_power_readout, 3) * 500) + atmosphere_temperature;
		}
	}
	return 0;
}

void ed_fm_refueling_add_fuel(double fuel)
{
	// Doesn't seem to do anything, maybe it's for mid-air refueling?
}

// Infinite fuel setting
void ed_fm_unlimited_fuel(bool value)
{
	infinite_fuel = value;

	/*
	This setting doesn't do anything on its own.
	In this FM, it simply disables fuel consumption when set to true.
	*/
}

// Easy/"game" flight mode setting
void ed_fm_set_easy_flight(bool value)
{
	easy_flight = value;

	/*
	This setting doesn't do anything on its own.
	The expectation is that the aircraft is a lot more stable and easy to fly when set to true.
	Such is the case with this FM.
	*/
}

// Invincibility setting
void ed_fm_set_immortal(bool value)
{
	invincible = value;

	/*
	When enabled, the aircraft does not register damage.
	When disabled, you have to code what would happen when certain parts are damaged.
	See the function below.
	*/
}

// What happens when certain parts of the aircraft are hit?
void ed_fm_on_damage(int Element, double element_integrity_factor)
{
	if (Element >= 0 && Element < 111)
	{
		element_integrity[Element] = element_integrity_factor;
		// Element integrity is a scale from 0 to 1, 0 is completely broken and 1 is fully intact.
	}

	// See DCSWorld/scripts/Aircrafts/_Common/Damage.lua for a full list of elements.
	if (invincible == false)
	{
		// Left wing
		left_wing_integrity = element_integrity[23] * element_integrity[29] * element_integrity[35];

		// Right wing
		right_wing_integrity = element_integrity[24] * element_integrity[30] * element_integrity[36];

		// Tail
		tail_integrity = element_integrity[53] * element_integrity[54] * element_integrity[55] * element_integrity[56] * element_integrity[57];

		// Left engine
		left_engine_integrity = element_integrity[13] * element_integrity[17] * element_integrity[103];

		// Right engine
		right_engine_integrity = element_integrity[14] * element_integrity[18] * element_integrity[104];
	}

	char buf[160];
	snprintf(
		buf, sizeof(buf),
		"damage element=%d integrity=%.3f invincible=%d",
		Element,
		element_integrity_factor,
		invincible ? 1 : 0
	);
	dbg_susp(buf);

	suspension_debug_log(buf);
}

// ed_fm_suspension_feedback will be defined below with debug logging.

static void dbg_susp(const char* msg)
{
	const char* debug_dir = "C:\\Users\\Ragdoll\\Saved Games\\DCS\\Mods\\aircraft\\F-CK-1C\\debug";
	// Ensure debug directory exists (ignore error if it already does)
	_mkdir(debug_dir);
	char path[1024];
	snprintf(path, sizeof(path), "%s\\fck1c_efm_dbg.txt", debug_dir);
	FILE* f = fopen(path, "a");
	if (!f) return;
	fprintf(f, "%s\n", msg);
	fclose(f);
}

static void susp_probe_log(const char* msg)
{
	const char* userprofile = getenv("USERPROFILE");
	if (!userprofile || userprofile[0] == '\0')
	{
		userprofile = "C:\\Users\\Ragdoll";
	}

	char log_dir[1024];
	snprintf(log_dir, sizeof(log_dir), "%s\\Saved Games\\DCS\\Logs", userprofile);
	_mkdir(log_dir);

	char path[1200];
	snprintf(path, sizeof(path), "%s\\fck_susp_debug.log", log_dir);
	FILE* f = fopen(path, "a");
	if (!f) return;
	fprintf(f, "%.3f %s\n", fm_clock, msg);
	fclose(f);
}

void ed_fm_suspension_feedback(int idx, const ed_fm_suspension_info* info)
{
	if (idx < 0 || idx >= 3 || info == nullptr)
	{
		susp_probe_log("suspension_feedback: invalid idx or null info");
		dbg_susp("suspension_feedback: invalid idx or null info");
		return;
	}

	suspension_feedback_valid[idx] = true;
	suspension_compression[idx] = info->struct_compression;

	const double fx = info->acting_force[0];
	const double fy = info->acting_force[1];
	const double fz = info->acting_force[2];
	suspension_force_vec[idx] = Vec3(fx, fy, fz);
	suspension_force_mag[idx] = sqrt(fx * fx + fy * fy + fz * fz);

	suspension_wow[idx] = (suspension_compression[idx] > 1e-4) || (suspension_force_mag[idx] > 50.0);

	char buf[512];
	snprintf(
		buf, sizeof(buf),
		"idx=%d comp=%.6f force=(%.3f, %.3f, %.3f) mag=%.3f wow=%d",
		idx,
		suspension_compression[idx],
		fx, fy, fz,
		suspension_force_mag[idx],
		suspension_wow[idx] ? 1 : 0
	);
	dbg_susp(buf);

	snprintf(
		buf, sizeof(buf),
		"susp idx=%d comp_norm=%.6f arg_val=%.3f speed=%.3f",
		idx,
		suspension_compression[idx],
		(float)suspension_visual_arg(idx),
		info->wheel_speed_X
	);
	suspension_debug_log(buf);
}

// What should be reset when the aircraft is repaired?
void ed_fm_repair()
{
	reset_damage_state();
}

bool ed_fm_pop_simulation_event(ed_fm_simulation_event& out)
{
	// Catapult launch sequence
	if (carrier_pos == 1)
	{
		if (left_throttle_output > 0.99) // Automatic launch at full throttle
		{
			out.event_type = ED_FM_EVENT_CARRIER_CATAPULT;
			out.event_params[0] = 1;
			out.event_params[1] = 2.0; // Start delay (s)
			out.event_params[2] = 80.0; // Added velocity after takeoff (m/s)
			out.event_params[3] = FM_DATA::max_thrust[1] * 0.5 * 2; // Engine thrust during takeoff (N)? Doesn't seem to work.
			carrier_pos = 2;
			return true;
		}
	}
	return false;
}

// bool ed_fm_push_simulation_event. DCS will call it for your FM when ingame event occurs
bool ed_fm_push_simulation_event(const ed_fm_simulation_event& in)
{
	if (in.event_type == ED_FM_EVENT_CARRIER_CATAPULT)
	{
		if (in.event_params[0] == 1)
		{
			carrier_pos = 1;
		}
		else if (in.event_params[0] == 2) // start launch
		{
			carrier_pos = 3;
		}
		else if (in.event_params[0] == 3) // launch finished
		{
			carrier_pos = 0;
		}
	}
	return false;
	// TO DO: Failure events
}


// What should be set on a cold start on the ground?
void ed_fm_cold_start()
{
	ed_fm_repair();
	reset_suspension_feedback_state();
	reset_startup_susp_probe_state();
	reset_fbw_state();
	reset_wheel_brakes();
	reset_wheel_spin();
	on_ground = false;

	// Landing gear down
	gear_switch = true;
	gear_pos = 1;
	nose_turn_enabled = true;
	carrier_pos = 0;

	// Engines off
	left_engine_switch = false;
	throttle_axis_cmd_left = 0.0;
	throttle_keyboard_cmd_left = 0.0;
	throttle_use_axis_left = false;
	pilot_throttle_cmd_left = 0.0;
	left_throttle_input = 0.0;
	left_throttle_output = 0.0;
	left_engine_power_readout = 0.0;
	left_nozzle_aperture = 0.80;

	right_engine_switch = false;
	throttle_axis_cmd_right = 0.0;
	throttle_keyboard_cmd_right = 0.0;
	throttle_use_axis_right = false;
	pilot_throttle_cmd_right = 0.0;
	right_throttle_input = 0.0;
	right_throttle_output = 0.0;
	right_engine_power_readout = 0.0;
	right_nozzle_aperture = 0.80;
}

// What should be set on a hot start on the ground?
void ed_fm_hot_start()
{	
	ed_fm_repair();
	reset_suspension_feedback_state();
	reset_startup_susp_probe_state();
	reset_fbw_state();
	reset_wheel_brakes();
	reset_wheel_spin();
	on_ground = false;

	// Landing gear down
	gear_switch = true;
	gear_pos = 1;
	nose_turn_enabled = true;
	carrier_pos = 0;

	// Flaps down
	flap_mode = FLAP_MODE_DOWN;
	flaps_pos = 1;
	slats_pos = 1;

	// Engines on at idle/minimum throttle
	left_engine_switch = true;
	throttle_axis_cmd_left = 0.0;
	throttle_keyboard_cmd_left = 0.0;
	throttle_use_axis_left = false;
	pilot_throttle_cmd_left = 0.0;
	left_throttle_input = 0.0;
	left_throttle_output = 0.5;
	left_engine_power_readout = 0.5;
	left_nozzle_aperture = estimate_nozzle_aperture_target(left_throttle_input, left_engine_power_readout, 0.0, left_engine_switch);

	right_engine_switch = true;
	throttle_axis_cmd_right = 0.0;
	throttle_keyboard_cmd_right = 0.0;
	throttle_use_axis_right = false;
	pilot_throttle_cmd_right = 0.0;
	right_throttle_input = 0.0;
	right_throttle_output = 0.5;
	right_engine_power_readout = 0.5; 
	right_nozzle_aperture = estimate_nozzle_aperture_target(right_throttle_input, right_engine_power_readout, 0.0, right_engine_switch);
}

// What should be set on a hot start in the air?
void ed_fm_hot_start_in_air()
{
	ed_fm_repair();
	reset_suspension_feedback_state();
	reset_startup_susp_probe_state();
	reset_fbw_state();
	reset_wheel_brakes();
	reset_wheel_spin();
	on_ground = false;

	// Landing gear up
	gear_switch = false;
	gear_pos = 0;
	nose_turn_enabled = false;
	carrier_pos = 0;

	//Engines on at 50% throttle
	left_engine_switch = true;
	throttle_axis_cmd_left = 0.5;
	throttle_keyboard_cmd_left = 0.5;
	throttle_use_axis_left = false;
	pilot_throttle_cmd_left = 0.5;
	left_throttle_input = 0.5;
	left_throttle_output = 0.5;
	left_engine_power_readout = 0.5;
	left_nozzle_aperture = estimate_nozzle_aperture_target(left_throttle_input, left_engine_power_readout, 0.0, left_engine_switch);

	right_engine_switch = true;
	throttle_axis_cmd_right = 0.5;
	throttle_keyboard_cmd_right = 0.5;
	throttle_use_axis_right = false;
	pilot_throttle_cmd_right = 0.5;
	right_throttle_input = 0.5;
	right_throttle_output = 0.5;
	right_engine_power_readout = 0.5;
	right_nozzle_aperture = estimate_nozzle_aperture_target(right_throttle_input, right_engine_power_readout, 0.0, right_engine_switch);
}

// What should be reset on mission exit?
void ed_fm_release()
{
	reset_suspension_feedback_state();
	reset_fbw_state();
	on_ground = false;

	fm_clock = 0;

	// Reset user inputs
	pitch_input = 0; 
	pitch_trim = 0;
	elevator_command = 0;

	roll_input = 0;
	roll_trim = 0;
	aileron_command = 0;

	yaw_input = 0;
	yaw_trim = 0;
	rudder_command = 0;
	nose_turn_enabled = false;

	throttle_axis_cmd_left = 0.0;
	throttle_axis_cmd_right = 0.0;
	throttle_keyboard_cmd_left = 0.0;
	throttle_keyboard_cmd_right = 0.0;
	throttle_use_axis_left = false;
	throttle_use_axis_right = false;
	pilot_throttle_cmd_left = 0.0;
	pilot_throttle_cmd_right = 0.0;
	fbw_throttle_cmd_left = 0.0;
	fbw_throttle_cmd_right = 0.0;
	fbw_throttle_blend = 0.0;
	fbw_throttle_override = false;
	engine_throttle_cmd_left = 0.0;
	engine_throttle_cmd_right = 0.0;
	left_afterburner_ratio = 0.0;
	right_afterburner_ratio = 0.0;
	left_nozzle_aperture = 0.80;
	right_nozzle_aperture = 0.80;

	// Reset cached AP state
	ap_pitch_cmd_cached  = 0.0;
	ap_roll_cmd_cached   = 0.0;
	ap_throttle_cached   = 0.0;
	ap_master_cached     = false;
	ap_bypass_cached     = false;
	ap_at_engaged_cached = false;

	// Repair
	ed_fm_repair();
}

// Cockpit view shaking
double ed_fm_get_shake_amplitude()
{
	return shake_amplitude;

	//This can be used to give a visual indication of stress on the airframe.
}

// Unused
bool ed_fm_add_local_force_component( double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z )
{
	return false;
}

// Unused
bool ed_fm_add_global_force_component( double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z )
{
	return false;
}

// Unused
bool ed_fm_add_local_moment_component( double & x,double &y,double &z )
{
	return false;
}

// Unused
bool ed_fm_add_global_moment_component( double & x,double &y,double &z )
{
	return false;
}

// Debug force vector and center of mass visualisation
bool ed_fm_enable_debug_info()
{
	return false;

	/*
	When set to true, DCS draws lines on the aircraft.
	The blue box is the center of mass, green line is net force vector, pink line is the velocity vector.
	*/
}

size_t ed_fm_debug_watch(int level, char* buffer, size_t maxlen)
{
	if (buffer == nullptr || maxlen == 0)
	{
		return 0;
	}

	const int wow0 = suspension_wow[0] ? 1 : 0;
	const int wow1 = suspension_wow[1] ? 1 : 0;
	const int wow2 = suspension_wow[2] ? 1 : 0;
	const int wow_any = any_wow() ? 1 : 0;
	const int wow_valid = has_suspension_feedback() ? 1 : 0;
	const int og = on_ground ? 1 : 0;

	int written = 0;
	if (level <= 0)
	{
		written = sprintf_s(
			buffer,
			maxlen,
			"VER:%s DATE:%s ASL:%.1f AGL:%.2f Z:%.1f GEAR:%.2f WOW:%d FG:%.0f MODE:%s ST:%s EN:%d RE:%s HOLD:%d HG:%.2f STK:[%.2f %.2f %.2f] CMD:[%.1f %.1f %.1f] HCMD:[%.1f %.1f] LIM:[A%d R%d S%d AW%d]",
			FCK1C_EFM_VERSION,
			FCK1C_EFM_VERSION_DATE,
			altitude_ASL,
			altitude_AGL,
			position_world_z,
			gear_pos,
			wow_any,
			fallback_ground_force,
			fbw_mode_name(),
			fbw_state_name(),
			fbw_hold_enter_reason,
			fbw_exit_reason_name(),
			fbw_hold_active ? 1 : 0,
			fbw_hold_gain_scale,
			fbw_stick_roll_raw,
			fbw_stick_pitch_raw,
			fbw_stick_yaw_raw,
			deg(fbw_p_cmd),
			deg(fbw_q_cmd),
			deg(fbw_r_cmd),
			deg(fbw_p_cmd_hold),
			deg(fbw_q_cmd_hold),
			fbw_aoa_limit_active ? 1 : 0,
			fbw_rate_limit_active ? 1 : 0,
			fbw_actuator_sat ? 1 : 0,
			fbw_anti_windup_active ? 1 : 0
		);
	}
	else
	{
		written = sprintf_s(
			buffer,
			maxlen,
			"VER:%s DATE:%s MODE:%s ST:%s EN:%d RE:%s CATB:%.2f OG:%d WOW:%d%d%d VALID:%d ASL:%.1f AGL:%.2f Z:%.1f GEAR:%.2f FG:%.0f "
			"ATT:[%.2f %.2f]/[%.2f %.2f] RATE:[%.2f %.2f %.2f]/[%.2f %.2f %.2f] "
			"AERO:[%.1f %.1f %.1f %.2f %.0f]/[%.1f %.1f %.1f %.2f %.0f] "
			"STK:[%.2f %.2f %.2f]/[%.2f %.2f %.2f] CMD_R:[%.1f %.1f %.1f] CMD_H:[%.1f %.1f] CMD:[%.1f %.1f %.1f] "
			"HOLD:%d REF:[%.2f %.2f] HG:%.2f HT:%.2f "
			"ACT_E:[%.1f %.1f %.1f %.1f] ACT_A:[%.1f %.1f %.1f %.1f] ACT_R:[%.1f %.1f %.1f %.1f] "
			"ERR:[%.2f %.2f %.2f %.2f %.2f] LIM:[A%d R%d S%d AW%d] SAT_T:%.2f",
			FCK1C_EFM_VERSION,
			FCK1C_EFM_VERSION_DATE,
			fbw_mode_name(),
			fbw_state_name(),
			fbw_hold_enter_reason,
			fbw_exit_reason_name(),
			fbw_mode_blend,
			og,
			wow0, wow1, wow2,
			wow_valid,
			altitude_ASL,
			altitude_AGL,
			position_world_z,
			gear_pos,
			fallback_ground_force,
			deg(fbw_phi_raw), deg(fbw_theta_raw),
			deg(fbw_phi_f), deg(fbw_theta_f),
			deg(fbw_p_raw), deg(fbw_q_raw), deg(fbw_r_raw),
			deg(fbw_p_f), deg(fbw_q_f), deg(fbw_r_f),
			fbw_alpha_raw, fbw_beta_raw, fbw_ias_raw, fbw_mach_raw, fbw_qbar_raw,
			fbw_alpha_f, fbw_beta_f, fbw_ias_f, fbw_mach_f, fbw_qbar_f,
			fbw_stick_roll_raw, fbw_stick_pitch_raw, fbw_stick_yaw_raw,
			fbw_stick_roll_shaped, fbw_stick_pitch_shaped, fbw_stick_yaw_shaped,
			deg(fbw_p_cmd_rate), deg(fbw_q_cmd_rate), deg(fbw_r_cmd_rate),
			deg(fbw_p_cmd_hold), deg(fbw_q_cmd_hold),
			deg(fbw_p_cmd), deg(fbw_q_cmd), deg(fbw_r_cmd),
			fbw_hold_active ? 1 : 0,
			deg(fbw_phi_ref), deg(fbw_theta_ref),
			fbw_hold_gain_scale,
			fbw_hold_timer,
			fbw_ele_cmd_pre, fbw_ele_cmd_sat, fbw_ele_cmd_rate, fbw_ele_cmd_lag,
			fbw_ail_cmd_pre, fbw_ail_cmd_sat, fbw_ail_cmd_rate, fbw_ail_cmd_lag,
			fbw_rud_cmd_pre, fbw_rud_cmd_sat, fbw_rud_cmd_rate, fbw_rud_cmd_lag,
			deg(fbw_p_err), deg(fbw_q_err), deg(fbw_r_err),
			deg(fbw_phi_err), deg(fbw_theta_err),
			fbw_aoa_limit_active ? 1 : 0,
			fbw_rate_limit_active ? 1 : 0,
			fbw_actuator_sat ? 1 : 0,
			fbw_anti_windup_active ? 1 : 0,
			fbw_actuator_sat_timer
		);
	}

	return written > 0 ? static_cast<size_t>(written) : 0;
}
