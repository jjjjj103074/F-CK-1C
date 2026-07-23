-- ============================================================================
-- IDF F-CK-1C AFCS / Autopilot System  (Phase 1)
-- 戰鬥機式 AFCS 設計：模式分軸、駕駛優先、Hold 抓當前值
-- ============================================================================
local dev = GetSelf()
local sensor_data = get_base_data()

local update_rate = 0.02 -- 50 Hz
make_default_activity(update_rate)

dofile(LockOn_Options.script_path .. "command_defs.lua")
dofile(LockOn_Options.script_path .. "devices.lua")
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")

-- ========================== Constants =======================================
local RAD_TO_DEG = 57.29577951308232
local DEG_TO_RAD = 0.01745329251994
local MPS_TO_KTS = 1.943844
local FT_TO_M = 0.3048
local M_TO_FT = 3.28084

-- Engage conditions
local AP_MIN_SPEED_KTS = 240.0 -- 低於此速度不可接通 AP
local AP_MACH_MAX_AT = 0.95 -- A/T 上限

-- Attitude engage limits (CAT III defaults for Phase 1)
local AP_ENGAGE_ROLL_MAX = 45.0 * DEG_TO_RAD
local AP_ENGAGE_PITCH_MAX = 45.0 * DEG_TO_RAD

-- Control limits
local AP_BANK_LIMIT = 60.0 * DEG_TO_RAD
local AP_PITCH_CMD_LIMIT = 0.6 -- normalised -1..+1 range

-- Altitude capture
local ALT_HOLD_WINDOW_FT = 500.0
local ALT_CAPTURE_K = 2.0
local ALT_CAPTURE_ENTRY_FT = ALT_HOLD_WINDOW_FT * ALT_CAPTURE_K -- 1000 ft
local ALT_FINE_HOLD_FT = ALT_HOLD_WINDOW_FT * 0.10 -- 50 ft
local ALT_COMFORT_UNLOAD_G = 0.6

-- VS limits (m/s)
local VS_MAX = 40.0 -- ~7900 fpm
local VS_CAPTURE_TAPER = 10.0 -- m/s: begin reducing VS when close

-- Override detection
local OVERRIDE_FACTOR = 1.2
local OVERRIDE_HOLD_TIME = 1.0 -- seconds

-- Bypass
local BYPASS_ATTITUDE_THRESHOLD = 1.0 * DEG_TO_RAD

-- Speed (A/T)
local AT_SPEED_STEP_KTS = 5.0
local AT_MIN_SPEED_KTS = 200.0
local AT_MAX_SPEED_KTS = 550.0

-- ALT/HDG step sizes
local ALT_STEP_FT = 100.0
local HDG_STEP_DEG = 1.0

-- Sensor degradation limits
local DEGRADE_PITCH_LIMIT = 13.0 * DEG_TO_RAD
local DEGRADE_ROLL_LIMIT = 15.0 * DEG_TO_RAD

-- ========================== Command IDs =====================================
local CMD = device_commands
local CMD_AP_MASTER_TOGGLE = CMD.APMasterToggle
local CMD_AP_MASTER_ON = CMD.APMasterOn
local CMD_AP_MASTER_OFF = CMD.APMasterOff
local CMD_AP_BYPASS = CMD.APBypass
local CMD_VERT_PITCH_HOLD = CMD.APVertPitchHold
local CMD_VERT_VS_HOLD = CMD.APVertVSHold
local CMD_VERT_ALT_HOLD = CMD.APVertAltHold
local CMD_VERT_INCREASE = CMD.APVertIncrease
local CMD_VERT_DECREASE = CMD.APVertDecrease
local CMD_LAT_HDG_HOLD = CMD.APLatHeadingHold
local CMD_LAT_HDG_SELECT = CMD.APLatHeadingSelect
local CMD_LAT_NAV_TRACK = CMD.APLatNavTrack
local CMD_LAT_INCREASE = CMD.APLatIncrease
local CMD_LAT_DECREASE = CMD.APLatDecrease
local CMD_AT_TOGGLE = CMD.APAutoThrottleToggle
local CMD_AT_ON = CMD.APAutoThrottleOn
local CMD_AT_OFF = CMD.APAutoThrottleOff
local CMD_SPEED_INCREASE = CMD.APSpeedIncrease
local CMD_SPEED_DECREASE = CMD.APSpeedDecrease
local CMD_THRUST_TEST_TOGGLE = CMD.EngineThrustCutTestToggle
local CMD_THRUST_TEST_ENABLE = CMD.EngineThrustCutTestEnable
local CMD_THRUST_TEST_DISABLE = CMD.EngineThrustCutTestDisable

-- ========================== Exported Params ==================================
-- These are read by the EFM C++ side and by HUD/indicators
local p_ap_master_engaged = get_param_handle(cockpit_params.ApMasterEngaged)
local p_ap_vert_mode = get_param_handle("AP_VERT_MODE")
local p_ap_lat_mode = get_param_handle("AP_LAT_MODE")
local p_ap_at_engaged = get_param_handle(cockpit_params.ApAutoThrottleEngaged)
local p_ap_pitch_cmd = get_param_handle(cockpit_params.ApPitchCommand)
local p_ap_roll_cmd = get_param_handle(cockpit_params.ApRollCommand)
local p_ap_throttle_cmd = get_param_handle(cockpit_params.ApThrottleCommand)
local p_ap_bypass_active = get_param_handle(cockpit_params.ApBypassActive)
-- Engine thrust cut bridge. When set to 0, the EFM zeros engine thrust for
-- ground-coupling diagnostics.
local p_maxpower_switch = get_param_handle(cockpit_params.MaxPowerSwitch)
local p_maxpower_ready = get_param_handle(cockpit_params.MaxPowerReady)
-- Default to normal thrust.
p_maxpower_switch:set(1)
p_maxpower_ready:set(1)
local p_ap_target_alt_ft = get_param_handle("AP_TARGET_ALT_FT")
local p_ap_target_hdg_deg = get_param_handle("AP_TARGET_HDG_DEG")
local p_ap_target_spd_kts = get_param_handle("AP_TARGET_SPD_KTS")
local p_ap_target_pitch_deg = get_param_handle("AP_TARGET_PITCH_DEG")
local p_ap_target_vs_fpm = get_param_handle("AP_TARGET_VS_FPM")

-- ========================== Mode Enums ======================================
local VERT_MODE_OFF = 0
local VERT_MODE_PITCH_HOLD = 1
local VERT_MODE_VS_HOLD = 2
local VERT_MODE_ALT_HOLD = 3

local LAT_MODE_OFF = 0
local LAT_MODE_HDG_HOLD = 1
local LAT_MODE_HDG_SELECT = 2
local LAT_MODE_NAV_TRACK = 3

-- ========================== State ==========================================
local ap_master = false
local vert_mode = VERT_MODE_OFF
local lat_mode = LAT_MODE_OFF
local at_engaged = false

-- Targets
local target_pitch = 0.0 -- rad
local target_vs = 0.0 -- m/s
local target_alt = 0.0 -- meters
local target_hdg = 0.0 -- rad
local target_hdg_select = 0.0 -- rad (for Heading Select mode)
local target_speed = 0.0 -- m/s (IAS)

-- Control outputs
local ap_pitch_cmd = 0.0 -- -1..+1
local ap_roll_cmd = 0.0 -- -1..+1
local ap_throttle_cmd = 0.0 -- 0..1

-- Bypass state
local bypass_held = false
local bypass_active = false
local bypass_start_pitch = 0.0
local bypass_start_roll = 0.0
local bypass_start_hdg = 0.0

-- Override detection
local override_timer_pitch = 0.0
local override_timer_roll = 0.0

-- PID integrators
local pid_alt_int = 0.0
local pid_hdg_int = 0.0
local pid_spd_int = 0.0
local pid_pitch_int = 0.0
local pid_vs_int = 0.0

-- ALT capture state
local alt_capture_overshoot_count = 0

-- ========================== Utilities =======================================
local function dlog(msg)
    if log ~= nil and log.info ~= nil then log.info("FCK1C AP: " .. tostring(msg)) end
end

local function set_thrust_cut_test_enabled(enabled)
    local switch_value = enabled and 0 or 1
    p_maxpower_switch:set(switch_value)
    if enabled then
        dlog("Engine thrust cut test ENABLED (engine thrust forced to zero)")
    else
        dlog("Engine thrust cut test DISABLED (normal engine thrust restored)")
    end
end

local function is_thrust_cut_test_enabled()
    return p_maxpower_switch:get() < 0.5
end

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

local function wrap_pi(angle)
    while angle > math.pi do
        angle = angle - 2 * math.pi
    end
    while angle < -math.pi do
        angle = angle + 2 * math.pi
    end
    return angle
end

local function wrap_2pi(angle)
    while angle >= 2 * math.pi do
        angle = angle - 2 * math.pi
    end
    while angle < 0 do
        angle = angle + 2 * math.pi
    end
    return angle
end

local function sign(x)
    if x > 0 then
        return 1
    elseif x < 0 then
        return -1
    else
        return 0
    end
end

local function safe_sensor(method_name, default)
    local method = sensor_data and sensor_data[method_name]
    if type(method) ~= "function" then return default end
    local ok, val = pcall(method, sensor_data)
    if ok and type(val) == "number" then return val end
    return default
end

-- ========================== Sensor Reads ====================================
local function get_pitch()
    return safe_sensor("getPitch", 0)
end
local function get_roll()
    return safe_sensor("getRoll", 0)
end
local function get_heading()
    return safe_sensor("getHeading", 0)
end
local function get_baro_alt()
    return safe_sensor("getBarometricAltitude", 0)
end
local function get_radar_alt()
    return safe_sensor("getRadarAltitude", 0)
end
local function get_ias()
    return safe_sensor("getIndicatedAirSpeed", 0)
end
local function get_tas()
    return safe_sensor("getTrueAirSpeed", 0)
end
local function get_mach()
    return safe_sensor("getMachNumber", 0)
end
local function get_vs()
    return safe_sensor("getVerticalVelocity", 0)
end
local function get_aoa()
    return safe_sensor("getAngleOfAttack", 0)
end
local function get_wow()
    -- check any weight-on-wheels
    local n = safe_sensor("getWOW_NoseLandingGear", 0)
    local l = safe_sensor("getWOW_LeftMainLandingGear", 0)
    local r = safe_sensor("getWOW_RightMainLandingGear", 0)
    return (n > 0.5 or l > 0.5 or r > 0.5)
end

-- ========================== Engage Check ====================================
local function can_engage_ap()
    local ias_kts = get_ias() * MPS_TO_KTS
    if ias_kts < AP_MIN_SPEED_KTS then
        dlog("engage denied: speed " .. string.format("%.0f", ias_kts) .. " < " .. AP_MIN_SPEED_KTS)
        return false
    end
    if get_wow() then
        dlog("engage denied: weight on wheels")
        return false
    end
    local abs_roll = math.abs(get_roll())
    if abs_roll > AP_ENGAGE_ROLL_MAX then
        dlog("engage denied: roll " .. string.format("%.1f", abs_roll * RAD_TO_DEG) .. " > limit")
        return false
    end
    local abs_pitch = math.abs(get_pitch())
    if abs_pitch > AP_ENGAGE_PITCH_MAX then
        dlog("engage denied: pitch " .. string.format("%.1f", abs_pitch * RAD_TO_DEG) .. " > limit")
        return false
    end
    return true
end

local function can_engage_at()
    local mach = get_mach()
    if mach > AP_MACH_MAX_AT then
        dlog("A/T engage denied: Mach " .. string.format("%.2f", mach) .. " > " .. AP_MACH_MAX_AT)
        return false
    end
    if get_wow() then
        dlog("A/T engage denied: weight on wheels")
        return false
    end
    return true
end

-- ========================== Reset Helpers ====================================
local function reset_pid_all()
    pid_alt_int = 0.0
    pid_hdg_int = 0.0
    pid_spd_int = 0.0
    pid_pitch_int = 0.0
    pid_vs_int = 0.0
    alt_capture_overshoot_count = 0
end

local function reset_vert()
    vert_mode = VERT_MODE_OFF
    ap_pitch_cmd = 0.0
    pid_alt_int = 0.0
    pid_pitch_int = 0.0
    pid_vs_int = 0.0
    alt_capture_overshoot_count = 0
end

local function reset_lat()
    lat_mode = LAT_MODE_OFF
    ap_roll_cmd = 0.0
    pid_hdg_int = 0.0
end

local function reset_at()
    at_engaged = false
    ap_throttle_cmd = 0.0
    pid_spd_int = 0.0
end

local function disengage_all()
    ap_master = false
    reset_vert()
    reset_lat()
    reset_at()
    bypass_held = false
    bypass_active = false
    override_timer_pitch = 0.0
    override_timer_roll = 0.0
    dlog("AP disengaged (all)")
end

-- ========================== Mode Engage =====================================
local function engage_pitch_hold()
    vert_mode = VERT_MODE_PITCH_HOLD
    target_pitch = get_pitch()
    pid_pitch_int = 0.0
    dlog("PITCH HOLD @ " .. string.format("%.1f", target_pitch * RAD_TO_DEG) .. " deg")
end

local function engage_vs_hold()
    vert_mode = VERT_MODE_VS_HOLD
    target_vs = get_vs()
    pid_vs_int = 0.0
    dlog("VS HOLD @ " .. string.format("%.1f", target_vs * M_TO_FT * 60) .. " fpm")
end

local function engage_alt_hold()
    vert_mode = VERT_MODE_ALT_HOLD
    target_alt = get_baro_alt()
    pid_alt_int = 0.0
    pid_vs_int = 0.0
    alt_capture_overshoot_count = 0
    dlog("ALT HOLD @ " .. string.format("%.0f", target_alt * M_TO_FT) .. " ft")
end

local function engage_hdg_hold()
    lat_mode = LAT_MODE_HDG_HOLD
    target_hdg = get_heading()
    pid_hdg_int = 0.0
    dlog("HDG HOLD @ " .. string.format("%.1f", target_hdg * RAD_TO_DEG) .. " deg")
end

local function engage_hdg_select()
    lat_mode = LAT_MODE_HDG_SELECT
    -- Keep current selected heading, or init to current if first time
    if target_hdg_select == 0.0 then target_hdg_select = get_heading() end
    pid_hdg_int = 0.0
    dlog("HDG SELECT @ " .. string.format("%.1f", target_hdg_select * RAD_TO_DEG) .. " deg")
end

local function engage_at()
    if not can_engage_at() then return end
    at_engaged = true
    target_speed = get_ias()
    pid_spd_int = 0.0
    dlog("A/T ON @ " .. string.format("%.0f", target_speed * MPS_TO_KTS) .. " kts")
end

-- ========================== PID Controllers =================================

-- Pitch Hold: P controller (FBW handles inner loop)
local function update_pitch_hold(dt)
    local pitch_err = target_pitch - get_pitch()
    -- P + small D (pitch rate damping)
    local kp = 2.5
    local kd = 0.3
    local pitch_rate = safe_sensor("getAngularVelocityX", 0) -- body-axis pitch rate
    local cmd = kp * pitch_err - kd * pitch_rate
    ap_pitch_cmd = clamp(cmd, -AP_PITCH_CMD_LIMIT, AP_PITCH_CMD_LIMIT)
end

-- VS Hold: PI on VS error -> pitch command
local function update_vs_hold(dt)
    local vs = get_vs()
    local vs_err = target_vs - vs
    local kp = 0.08
    local ki = 0.02
    pid_vs_int = clamp(pid_vs_int + vs_err * dt, -5.0, 5.0)
    local cmd = kp * vs_err + ki * pid_vs_int
    ap_pitch_cmd = clamp(cmd, -AP_PITCH_CMD_LIMIT, AP_PITCH_CMD_LIMIT)
end

-- ALT Hold: Three-zone capture -> VS command -> pitch command
local function update_alt_hold(dt)
    local alt = get_baro_alt()
    local alt_err_m = target_alt - alt
    local alt_err_ft = alt_err_m * M_TO_FT
    local abs_err_ft = math.abs(alt_err_ft)
    local vs = get_vs()

    local desired_vs = 0.0

    if abs_err_ft <= ALT_FINE_HOLD_FT then
        -- Fine hold region: very gentle correction
        local kp_fine = 0.15
        desired_vs = kp_fine * alt_err_m
    elseif abs_err_ft <= ALT_HOLD_WINDOW_FT then
        -- Hold region: proportional
        local kp_hold = 0.5
        desired_vs = kp_hold * alt_err_m
    elseif abs_err_ft <= ALT_CAPTURE_ENTRY_FT then
        -- Capture region: reduce VS progressively
        -- Scale: at window edge VS should be small, at capture entry can be larger
        local capture_fraction = (abs_err_ft - ALT_HOLD_WINDOW_FT) / (ALT_CAPTURE_ENTRY_FT - ALT_HOLD_WINDOW_FT)
        local max_vs_capture = VS_CAPTURE_TAPER * capture_fraction + 2.0
        desired_vs = clamp(1.5 * alt_err_m, -max_vs_capture, max_vs_capture)

        -- Comfort check: can we stop in time at 0.6G unload?
        -- 停止距離 = vs^2 / (2 * a), where a = (0.6 * 9.81) m/s^2
        local a_comfort = ALT_COMFORT_UNLOAD_G * 9.81
        local stop_dist_m = (vs * vs) / (2.0 * a_comfort)
        local remaining_m = math.abs(alt_err_m)
        if stop_dist_m > remaining_m * 0.8 and math.abs(vs) > 2.0 then
            -- Need to start reducing VS early
            desired_vs = sign(alt_err_m) * math.sqrt(2.0 * a_comfort * remaining_m * 0.8)
            desired_vs = clamp(desired_vs, -max_vs_capture, max_vs_capture)
        end
    else
        -- Outside capture: use proportional approach with VS limit
        local kp_approach = 0.8
        desired_vs = clamp(kp_approach * alt_err_m, -VS_MAX, VS_MAX)

        -- Comfort gate
        local a_comfort = ALT_COMFORT_UNLOAD_G * 9.81
        local remaining_m = math.abs(alt_err_m)
        local stop_dist_m = (vs * vs) / (2.0 * a_comfort)
        if stop_dist_m > remaining_m * 0.6 then
            desired_vs = sign(alt_err_m) * math.sqrt(2.0 * a_comfort * remaining_m * 0.6)
            desired_vs = clamp(desired_vs, -VS_MAX, VS_MAX)
        end
    end

    -- Ensure no negative G commands: desired_vs change rate should be bounded
    -- VS -> pitch command (inner loop)
    local vs_err = desired_vs - vs
    local kp_vs = 0.08
    local ki_vs = 0.015
    pid_vs_int = clamp(pid_vs_int + vs_err * dt, -3.0, 3.0)
    local cmd = kp_vs * vs_err + ki_vs * pid_vs_int
    ap_pitch_cmd = clamp(cmd, -AP_PITCH_CMD_LIMIT, AP_PITCH_CMD_LIMIT)

    -- Overshoot tracking
    if abs_err_ft <= ALT_HOLD_WINDOW_FT and math.abs(vs) < 2.0 then
        -- Settled
    elseif abs_err_ft > ALT_HOLD_WINDOW_FT and sign(alt_err_ft) ~= sign(vs) then
        -- Moving away from target (overshoot recovery)
        alt_capture_overshoot_count = alt_capture_overshoot_count + 1
        if alt_capture_overshoot_count > 2 then dlog("ALT capture unstable: overshoot count " .. alt_capture_overshoot_count) end
    end
end

-- Heading Hold: heading error -> bank command -> roll command
local function update_heading_control(target, dt)
    local hdg = get_heading()
    local hdg_err = wrap_pi(target - hdg)

    -- Outer loop: heading error -> desired bank angle
    local kp_hdg = 2.5
    local ki_hdg = 0.1
    pid_hdg_int = clamp(pid_hdg_int + hdg_err * dt, -0.5, 0.5)
    local desired_bank = kp_hdg * hdg_err + ki_hdg * pid_hdg_int
    desired_bank = clamp(desired_bank, -AP_BANK_LIMIT, AP_BANK_LIMIT)

    -- Inner loop: bank error -> roll command
    local bank = get_roll()
    local bank_err = desired_bank - bank
    local kp_bank = 1.8
    local kd_bank = 0.2
    local roll_rate = safe_sensor("getAngularVelocityY", 0) -- body-axis roll rate
    local cmd = kp_bank * bank_err - kd_bank * roll_rate
    ap_roll_cmd = clamp(cmd, -1.0, 1.0)
end

-- Auto Throttle: PI on speed error -> throttle command
local function update_auto_throttle(dt)
    if not at_engaged then
        ap_throttle_cmd = 0.0
        return
    end

    -- Check Mach limit
    if get_mach() > AP_MACH_MAX_AT then
        dlog("A/T: Mach limit reached, reducing thrust")
        ap_throttle_cmd = clamp(ap_throttle_cmd - 0.01, 0.0, 1.0)
        return
    end

    local ias = get_ias()
    local speed_err = target_speed - ias -- m/s
    local kp = 0.015
    local ki = 0.003
    pid_spd_int = clamp(pid_spd_int + speed_err * dt, -30.0, 30.0)
    local cmd = 0.5 + kp * speed_err + ki * pid_spd_int
    ap_throttle_cmd = clamp(cmd, 0.0, 0.95) -- Don't command full AB via A/T
end

-- ========================== Bypass Logic ====================================
local function enter_bypass()
    if not ap_master then return end
    bypass_active = true
    bypass_start_pitch = get_pitch()
    bypass_start_roll = get_roll()
    bypass_start_hdg = get_heading()
    -- Pause AP outputs, let pilot control directly
    ap_pitch_cmd = 0.0
    ap_roll_cmd = 0.0
    dlog("BYPASS entered")
end

local function exit_bypass()
    if not bypass_active then return end
    bypass_active = false

    -- Check if meaningful change occurred (>1 deg)
    local d_pitch = math.abs(get_pitch() - bypass_start_pitch)
    local d_roll = math.abs(get_roll() - bypass_start_roll)
    local d_hdg = math.abs(wrap_pi(get_heading() - bypass_start_hdg))
    local meaningful = (d_pitch > BYPASS_ATTITUDE_THRESHOLD) or (d_roll > BYPASS_ATTITUDE_THRESHOLD) or (d_hdg > BYPASS_ATTITUDE_THRESHOLD)

    if meaningful then
        -- Re-capture current values as new references (Hold modes)
        if vert_mode == VERT_MODE_PITCH_HOLD then
            target_pitch = get_pitch()
            pid_pitch_int = 0.0
            dlog("BYPASS exit: new pitch " .. string.format("%.1f", target_pitch * RAD_TO_DEG))
        elseif vert_mode == VERT_MODE_VS_HOLD then
            target_vs = get_vs()
            pid_vs_int = 0.0
            dlog("BYPASS exit: new VS " .. string.format("%.1f", target_vs * M_TO_FT * 60) .. " fpm")
        elseif vert_mode == VERT_MODE_ALT_HOLD then
            target_alt = get_baro_alt()
            pid_alt_int = 0.0
            pid_vs_int = 0.0
            alt_capture_overshoot_count = 0
            dlog("BYPASS exit: new alt " .. string.format("%.0f", target_alt * M_TO_FT) .. " ft")
        end

        if lat_mode == LAT_MODE_HDG_HOLD then
            target_hdg = get_heading()
            pid_hdg_int = 0.0
            dlog("BYPASS exit: new heading " .. string.format("%.1f", target_hdg * RAD_TO_DEG))
        end
        -- Note: NAV Track mode does NOT re-capture; it continues to waypoint target
    end

    dlog("BYPASS exited (meaningful=" .. tostring(meaningful) .. ")")
end

-- ========================== Override Detection ===============================
local function check_override(dt)
    -- For Phase 1, override detection compares AP output vs pilot stick position
    -- Full implementation requires reading pilot raw inputs from EFM
    -- For now, we just track the timers for future use
end

-- ========================== Main Update =====================================
function update()
    local dt = update_rate

    -- Check disengage conditions while AP is on
    if ap_master then
        local ias_kts = get_ias() * MPS_TO_KTS
        if ias_kts < AP_MIN_SPEED_KTS or get_wow() then
            dlog("AP auto-disengage: speed/ground condition")
            disengage_all()
        end
    end

    -- A/T Mach guard
    if at_engaged and get_mach() > AP_MACH_MAX_AT + 0.05 then
        dlog("A/T auto-disengage: Mach exceeded")
        reset_at()
    end

    -- Bypass: freeze outputs
    if bypass_active then
        ap_pitch_cmd = 0.0
        ap_roll_cmd = 0.0
        -- A/T remains active during bypass (independent subsystem)
        if at_engaged then update_auto_throttle(dt) end
        push_params()
        return
    end

    -- Run controllers
    if ap_master then
        -- Vertical channel
        if vert_mode == VERT_MODE_PITCH_HOLD then
            update_pitch_hold(dt)
        elseif vert_mode == VERT_MODE_VS_HOLD then
            update_vs_hold(dt)
        elseif vert_mode == VERT_MODE_ALT_HOLD then
            update_alt_hold(dt)
        else
            ap_pitch_cmd = 0.0
        end

        -- Lateral channel
        if lat_mode == LAT_MODE_HDG_HOLD then
            update_heading_control(target_hdg, dt)
        elseif lat_mode == LAT_MODE_HDG_SELECT then
            update_heading_control(target_hdg_select, dt)
        else
            ap_roll_cmd = 0.0
        end

        -- Override detection
        check_override(dt)
    else
        ap_pitch_cmd = 0.0
        ap_roll_cmd = 0.0
    end

    -- Auto throttle (independent of AP master)
    if at_engaged then
        update_auto_throttle(dt)
    else
        ap_throttle_cmd = 0.0
    end

    push_params()
end

-- ========================== Param Export =====================================
function push_params()
    p_ap_master_engaged:set(ap_master and 1.0 or 0.0)
    p_ap_vert_mode:set(vert_mode)
    p_ap_lat_mode:set(lat_mode)
    p_ap_at_engaged:set(at_engaged and 1.0 or 0.0)
    p_ap_pitch_cmd:set(ap_pitch_cmd)
    p_ap_roll_cmd:set(ap_roll_cmd)
    p_ap_throttle_cmd:set(ap_throttle_cmd)
    p_ap_bypass_active:set(bypass_active and 1.0 or 0.0)
    p_ap_target_alt_ft:set(target_alt * M_TO_FT)
    p_ap_target_hdg_deg:set((lat_mode == LAT_MODE_HDG_SELECT and target_hdg_select or target_hdg) * RAD_TO_DEG)
    p_ap_target_spd_kts:set(target_speed * MPS_TO_KTS)
    p_ap_target_pitch_deg:set(target_pitch * RAD_TO_DEG)
    p_ap_target_vs_fpm:set(target_vs * M_TO_FT * 60.0)
end

-- ========================== Command Handler =================================
function SetCommand(command, value)
    dlog("SetCommand cmd=" .. tostring(command) .. " val=" .. string.format("%.2f", value))

    -- ---- AP Master ----
    if command == CMD_AP_MASTER_TOGGLE then
        if value > 0.5 then
            if ap_master then
                disengage_all()
            else
                if can_engage_ap() then
                    ap_master = true
                    -- Default: engage Pitch Hold + Heading Hold
                    engage_pitch_hold()
                    engage_hdg_hold()
                    dlog("AP MASTER ON (default: PH + HH)")
                end
            end
        end
        return
    end
    if command == CMD_AP_MASTER_ON then
        if value > 0.5 and not ap_master and can_engage_ap() then
            ap_master = true
            engage_pitch_hold()
            engage_hdg_hold()
            dlog("AP MASTER ON")
        end
        return
    end
    if command == CMD_AP_MASTER_OFF then
        if value > 0.5 then disengage_all() end
        return
    end

    -- ---- Bypass ----
    if command == CMD_AP_BYPASS then
        if value > 0.5 then
            bypass_held = true
            enter_bypass()
        else
            bypass_held = false
            exit_bypass()
        end
        return
    end

    -- ---- Vertical Channel ----
    if command == CMD_VERT_PITCH_HOLD then
        if value > 0.5 and ap_master then engage_pitch_hold() end
        return
    end
    if command == CMD_VERT_VS_HOLD then
        if value > 0.5 and ap_master then engage_vs_hold() end
        return
    end
    if command == CMD_VERT_ALT_HOLD then
        if value > 0.5 and ap_master then engage_alt_hold() end
        return
    end
    if command == CMD_VERT_INCREASE then
        if value > 0.5 and ap_master then
            if vert_mode == VERT_MODE_ALT_HOLD then
                target_alt = target_alt + ALT_STEP_FT * FT_TO_M
                pid_alt_int = 0.0
                alt_capture_overshoot_count = 0
                dlog("ALT target -> " .. string.format("%.0f", target_alt * M_TO_FT) .. " ft")
            elseif vert_mode == VERT_MODE_VS_HOLD then
                target_vs = target_vs + 1.0 -- +1 m/s ≈ +200 fpm
                dlog("VS target -> " .. string.format("%.0f", target_vs * M_TO_FT * 60) .. " fpm")
            elseif vert_mode == VERT_MODE_PITCH_HOLD then
                target_pitch = target_pitch + 1.0 * DEG_TO_RAD
                dlog("Pitch target -> " .. string.format("%.1f", target_pitch * RAD_TO_DEG) .. " deg")
            end
        end
        return
    end
    if command == CMD_VERT_DECREASE then
        if value > 0.5 and ap_master then
            if vert_mode == VERT_MODE_ALT_HOLD then
                target_alt = target_alt - ALT_STEP_FT * FT_TO_M
                pid_alt_int = 0.0
                alt_capture_overshoot_count = 0
                dlog("ALT target -> " .. string.format("%.0f", target_alt * M_TO_FT) .. " ft")
            elseif vert_mode == VERT_MODE_VS_HOLD then
                target_vs = target_vs - 1.0
                dlog("VS target -> " .. string.format("%.0f", target_vs * M_TO_FT * 60) .. " fpm")
            elseif vert_mode == VERT_MODE_PITCH_HOLD then
                target_pitch = target_pitch - 1.0 * DEG_TO_RAD
                dlog("Pitch target -> " .. string.format("%.1f", target_pitch * RAD_TO_DEG) .. " deg")
            end
        end
        return
    end

    -- ---- Lateral Channel ----
    if command == CMD_LAT_HDG_HOLD then
        if value > 0.5 and ap_master then engage_hdg_hold() end
        return
    end
    if command == CMD_LAT_HDG_SELECT then
        if value > 0.5 and ap_master then engage_hdg_select() end
        return
    end
    if command == CMD_LAT_NAV_TRACK then
        if value > 0.5 and ap_master then
            -- Phase 3: NAV Track (placeholder)
            lat_mode = LAT_MODE_NAV_TRACK
            pid_hdg_int = 0.0
            dlog("NAV TRACK: not yet implemented (Phase 3)")
        end
        return
    end
    if command == CMD_LAT_INCREASE then
        if value > 0.5 and ap_master then
            if lat_mode == LAT_MODE_HDG_HOLD then
                target_hdg = wrap_2pi(target_hdg + HDG_STEP_DEG * DEG_TO_RAD)
                dlog("HDG target -> " .. string.format("%.1f", target_hdg * RAD_TO_DEG))
            elseif lat_mode == LAT_MODE_HDG_SELECT then
                target_hdg_select = wrap_2pi(target_hdg_select + HDG_STEP_DEG * DEG_TO_RAD)
                dlog("HDG SELECT target -> " .. string.format("%.1f", target_hdg_select * RAD_TO_DEG))
            end
        end
        return
    end
    if command == CMD_LAT_DECREASE then
        if value > 0.5 and ap_master then
            if lat_mode == LAT_MODE_HDG_HOLD then
                target_hdg = wrap_2pi(target_hdg - HDG_STEP_DEG * DEG_TO_RAD)
                dlog("HDG target -> " .. string.format("%.1f", target_hdg * RAD_TO_DEG))
            elseif lat_mode == LAT_MODE_HDG_SELECT then
                target_hdg_select = wrap_2pi(target_hdg_select - HDG_STEP_DEG * DEG_TO_RAD)
                dlog("HDG SELECT target -> " .. string.format("%.1f", target_hdg_select * RAD_TO_DEG))
            end
        end
        return
    end

    -- ---- Auto Throttle ----
    if command == CMD_AT_TOGGLE then
        if value > 0.5 then
            if at_engaged then
                reset_at()
                dlog("A/T OFF (toggle)")
            else
                engage_at()
            end
        end
        return
    end
    if command == CMD_AT_ON then
        if value > 0.5 and not at_engaged then engage_at() end
        return
    end
    if command == CMD_AT_OFF then
        if value > 0.5 then
            reset_at()
            dlog("A/T OFF")
        end
        return
    end
    if command == CMD_SPEED_INCREASE then
        if value > 0.5 and at_engaged then
            local new_kts = clamp(target_speed * MPS_TO_KTS + AT_SPEED_STEP_KTS, AT_MIN_SPEED_KTS, AT_MAX_SPEED_KTS)
            target_speed = new_kts / MPS_TO_KTS
            dlog("A/T speed target -> " .. string.format("%.0f", new_kts) .. " kts")
        end
        return
    end
    if command == CMD_SPEED_DECREASE then
        if value > 0.5 and at_engaged then
            local new_kts = clamp(target_speed * MPS_TO_KTS - AT_SPEED_STEP_KTS, AT_MIN_SPEED_KTS, AT_MAX_SPEED_KTS)
            target_speed = new_kts / MPS_TO_KTS
            dlog("A/T speed target -> " .. string.format("%.0f", new_kts) .. " kts")
        end
        return
    end

    -- Engine thrust cut flight-test commands.
    if command == CMD_THRUST_TEST_TOGGLE then
        if value > 0.5 then set_thrust_cut_test_enabled(not is_thrust_cut_test_enabled()) end
        return
    end
    if command == CMD_THRUST_TEST_ENABLE then
        if value > 0.5 then set_thrust_cut_test_enabled(true) end
        return
    end
    if command == CMD_THRUST_TEST_DISABLE then
        if value > 0.5 then set_thrust_cut_test_enabled(false) end
        return
    end
end

-- ========================== Init ============================================
function post_initialize()
    dlog("Autopilot system initialised (Phase 1)")
    disengage_all()
    p_maxpower_ready:set(1)
    set_thrust_cut_test_enabled(false)
    push_params()
end
