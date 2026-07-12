#pragma once

#include "../Common/Units.h"
#include "../Systems/FBWController.h"
#include <cstdio>

namespace Diagnostics
{
struct DebugWatchSnapshot
{
	const char* version = "";
	const char* version_date = "";
	double altitude_asl = 0.0;
	double altitude_agl = 0.0;
	double position_world_z = 0.0;
	double gear_pos = 0.0;
	bool wow[3] = { false, false, false };
	bool wow_any = false;
	bool wow_valid = false;
	bool on_ground = false;
	double fallback_ground_force = 0.0;
	const Systems::FBWControllerState* fbw = nullptr;
};

inline size_t format_debug_watch(
	int level,
	const DebugWatchSnapshot& snapshot,
	char* buffer,
	size_t maxlen)
{
	if (buffer == nullptr || maxlen == 0 || snapshot.fbw == nullptr)
	{
		return 0;
	}

	const Systems::FBWControllerState& fbw = *snapshot.fbw;
	int written = 0;
	if (level <= 0)
	{
		written = sprintf_s(
			buffer,
			maxlen,
			"VER:%s DATE:%s ASL:%.1f AGL:%.2f Z:%.1f GEAR:%.2f WOW:%d FG:%.0f MODE:%s ST:%s EN:%d RE:%s HOLD:%d HG:%.2f STK:[%.2f %.2f %.2f] CMD:[%.1f %.1f %.1f] HCMD:[%.1f %.1f] LIM:[A%d R%d S%d AW%d]",
			snapshot.version,
			snapshot.version_date,
			snapshot.altitude_asl,
			snapshot.altitude_agl,
			snapshot.position_world_z,
			snapshot.gear_pos,
			snapshot.wow_any ? 1 : 0,
			snapshot.fallback_ground_force,
			Systems::fbw_mode_name(fbw),
			Systems::fbw_state_name(fbw),
			fbw.hold_enter_reason,
			Systems::fbw_exit_reason_name(fbw),
			fbw.hold_active ? 1 : 0,
			fbw.hold_gain_scale,
			fbw.stick_roll_raw,
			fbw.stick_pitch_raw,
			fbw.stick_yaw_raw,
			Common::deg(fbw.p_cmd),
			Common::deg(fbw.q_cmd),
			Common::deg(fbw.r_cmd),
			Common::deg(fbw.p_cmd_hold),
			Common::deg(fbw.q_cmd_hold),
			fbw.aoa_limit_active ? 1 : 0,
			fbw.rate_limit_active ? 1 : 0,
			fbw.actuator_sat ? 1 : 0,
			fbw.anti_windup_active ? 1 : 0);
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
			snapshot.version,
			snapshot.version_date,
			Systems::fbw_mode_name(fbw),
			Systems::fbw_state_name(fbw),
			fbw.hold_enter_reason,
			Systems::fbw_exit_reason_name(fbw),
			fbw.mode_blend,
			snapshot.on_ground ? 1 : 0,
			snapshot.wow[0] ? 1 : 0,
			snapshot.wow[1] ? 1 : 0,
			snapshot.wow[2] ? 1 : 0,
			snapshot.wow_valid ? 1 : 0,
			snapshot.altitude_asl,
			snapshot.altitude_agl,
			snapshot.position_world_z,
			snapshot.gear_pos,
			snapshot.fallback_ground_force,
			Common::deg(fbw.phi_raw), Common::deg(fbw.theta_raw),
			Common::deg(fbw.phi_f), Common::deg(fbw.theta_f),
			Common::deg(fbw.p_raw), Common::deg(fbw.q_raw), Common::deg(fbw.r_raw),
			Common::deg(fbw.p_f), Common::deg(fbw.q_f), Common::deg(fbw.r_f),
			fbw.alpha_raw, fbw.beta_raw, fbw.ias_raw, fbw.mach_raw, fbw.qbar_raw,
			fbw.alpha_f, fbw.beta_f, fbw.ias_f, fbw.mach_f, fbw.qbar_f,
			fbw.stick_roll_raw, fbw.stick_pitch_raw, fbw.stick_yaw_raw,
			fbw.stick_roll_shaped, fbw.stick_pitch_shaped, fbw.stick_yaw_shaped,
			Common::deg(fbw.p_cmd_rate), Common::deg(fbw.q_cmd_rate), Common::deg(fbw.r_cmd_rate),
			Common::deg(fbw.p_cmd_hold), Common::deg(fbw.q_cmd_hold),
			Common::deg(fbw.p_cmd), Common::deg(fbw.q_cmd), Common::deg(fbw.r_cmd),
			fbw.hold_active ? 1 : 0,
			Common::deg(fbw.phi_ref), Common::deg(fbw.theta_ref),
			fbw.hold_gain_scale,
			fbw.hold_timer,
			fbw.ele_cmd_pre, fbw.ele_cmd_sat, fbw.ele_cmd_rate, fbw.ele_cmd_lag,
			fbw.ail_cmd_pre, fbw.ail_cmd_sat, fbw.ail_cmd_rate, fbw.ail_cmd_lag,
			fbw.rud_cmd_pre, fbw.rud_cmd_sat, fbw.rud_cmd_rate, fbw.rud_cmd_lag,
			Common::deg(fbw.p_err), Common::deg(fbw.q_err), Common::deg(fbw.r_err),
			Common::deg(fbw.phi_err), Common::deg(fbw.theta_err),
			fbw.aoa_limit_active ? 1 : 0,
			fbw.rate_limit_active ? 1 : 0,
			fbw.actuator_sat ? 1 : 0,
			fbw.anti_windup_active ? 1 : 0,
			fbw.actuator_sat_timer);
	}

	return written > 0 ? static_cast<size_t>(written) : 0;
}
}
