local dev = GetSelf()

local update_rate = 0.02
make_default_activity(update_rate)

local AIM9_TONE_OFF = 0
local AIM9_TONE_SEEK = 1
local AIM9_TONE_LOCK = 2

local aim9_uncage_held = get_param_handle("AIM9_UNCAGE_HELD")
local aim9_tone_state = get_param_handle("AIM9_TONE_STATE")
local aim9_contact_state = get_param_handle("AIM9_SEEKER_CONTACT")
local aim9_lock_state = get_param_handle("AIM9_SEEKER_LOCK")
local aim9_seeker_azimuth = get_param_handle("AIM9_SEEKER_AZIMUTH")
local aim9_seeker_elevation = get_param_handle("AIM9_SEEKER_ELEVATION")
local aim9_lock_range = get_param_handle("AIM9_LOCK_RANGE")
local hmcs_fc_mode = get_param_handle("HMCS_FC_MODE")
local radar_state = get_param_handle("RADARSTATE")
local radar_power_state = get_param_handle("RADARPOWER_STATE")
local radar_mode = get_param_handle("RADAR_MODE")
local radar_stt_azimuth = get_param_handle("RADAR_STT_AZIMUTH")
local radar_stt_elevation = get_param_handle("RADAR_STT_ELEVATION")
local radar_stt_range = get_param_handle("RADAR_STT_RANGE")
local ws_target_range = get_param_handle("WS_TARGET_RANGE")
local ws_ir_slave_azimuth = get_param_handle("WS_IR_MISSILE_SEEKER_DESIRED_AZIMUTH")
local ws_ir_slave_elevation = get_param_handle("WS_IR_MISSILE_SEEKER_DESIRED_ELEVATION")

local debug_last_line = ""
local debug_timer = 0.0
local debug_period = 0.20

local function dlog(msg)
    if log ~= nil and log.info ~= nil then
        log.info("FCK1C RADAR: " .. tostring(msg))
    end
end

local function num(handle)
    return tonumber(handle:get()) or 0.0
end

local function in_aam_mode()
    local mode = math.floor(num(hmcs_fc_mode) + 0.5)
    return mode == 1 or mode == 2
end

local function seeker_metric(azimuth, elevation)
    return math.abs(azimuth) + math.abs(elevation)
end

local function update_debug(contact_valid, lock_valid, ir_azimuth, ir_elevation, stt_azimuth, stt_elevation, stt_range, ws_range, radar_mode_value)
    debug_timer = debug_timer + update_rate
    if debug_timer < debug_period then
        return
    end

    debug_timer = debug_timer - debug_period

    local line = string.format(
        "DBG uncage=%d aam=%d contact=%d lock=%d radar=%d mode=%.1f ir=(%.4f,%.4f) stt=(%.4f,%.4f,%.1f) ws=%.1f",
        (num(aim9_uncage_held) > 0.5) and 1 or 0,
        in_aam_mode() and 1 or 0,
        contact_valid and 1 or 0,
        lock_valid and 1 or 0,
        (num(radar_state) > 0.5 and num(radar_power_state) > 0.5) and 1 or 0,
        radar_mode_value,
        ir_azimuth,
        ir_elevation,
        stt_azimuth,
        stt_elevation,
        stt_range,
        ws_range
    )

    if line ~= debug_last_line then
        debug_last_line = line
        dlog(line)
    end
end

function post_initialize()
    radar_state:set(1)
    radar_power_state:set(1)
    aim9_tone_state:set(AIM9_TONE_OFF)
    aim9_contact_state:set(0)
    aim9_lock_state:set(0)
    aim9_seeker_azimuth:set(0)
    aim9_seeker_elevation:set(0)
    aim9_lock_range:set(0)
end

function update()
    radar_state:set(1)
    radar_power_state:set(1)

    local uncage_held = num(aim9_uncage_held) > 0.5
    local aam_mode = in_aam_mode()

    local ir_azimuth = num(ws_ir_slave_azimuth)
    local ir_elevation = num(ws_ir_slave_elevation)
    local stt_azimuth = num(radar_stt_azimuth)
    local stt_elevation = num(radar_stt_elevation)
    local stt_range = num(radar_stt_range)
    local ws_range = num(ws_target_range)
    local radar_mode_value = num(radar_mode)

    local ir_contact_valid = seeker_metric(ir_azimuth, ir_elevation) > 0.00001
    local stt_valid = stt_range > 1.0
    local ws_target_valid = ws_range > 1.0
    local contact_valid = aam_mode and uncage_held and (ir_contact_valid or stt_valid or ws_target_valid)
    local lock_valid = aam_mode and uncage_held and (stt_valid or ws_target_valid)

    aim9_contact_state:set(contact_valid and 1 or 0)
    aim9_lock_state:set(lock_valid and 1 or 0)
    aim9_seeker_azimuth:set(ir_azimuth)
    aim9_seeker_elevation:set(ir_elevation)
    aim9_lock_range:set((stt_range > ws_range) and stt_range or ws_range)

    if lock_valid then
        aim9_tone_state:set(AIM9_TONE_LOCK)
    elseif contact_valid then
        aim9_tone_state:set(AIM9_TONE_SEEK)
    else
        aim9_tone_state:set(AIM9_TONE_OFF)
    end

    update_debug(contact_valid, lock_valid, ir_azimuth, ir_elevation, stt_azimuth, stt_elevation, stt_range, ws_range, radar_mode_value)
end

need_to_be_closed = false
