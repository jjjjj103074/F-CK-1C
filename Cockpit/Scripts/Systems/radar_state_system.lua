local dev = GetSelf()

local update_rate = 0.02
make_default_activity(update_rate)

local AIM9_TONE_OFF = 0
local AIM9_TONE_SEEK = 1
local AIM9_TONE_ACQUIRE = 2
local AIM9_TONE_LOCK = 3
local AIM9_SEEKER_OFF = 0
local AIM9_SEEKER_SEARCH_CAGED = 1
local AIM9_SEEKER_SEARCH_UNCAGED = 2
local AIM9_SEEKER_TRACK = 3
local AIM9_STATUS_OFF = 0
local AIM9_STATUS_COOL = 1
local AIM9_STATUS_RDY = 2
local AIM9_STATUS_TRACK = 3

local aim9_uncage_held = get_param_handle("AIM9_UNCAGE_HELD")
local aim9_tone_state = get_param_handle("AIM9_TONE_STATE")
local aim9_seeker_state = get_param_handle("AIM9_SEEKER_STATE")
local aim9_contact_state = get_param_handle("AIM9_SEEKER_CONTACT")
local aim9_lock_state = get_param_handle("AIM9_SEEKER_LOCK")
local aim9_target_designated = get_param_handle("AIM9_TARGET_DESIGNATED")
local aim9_weapon_active = get_param_handle("AIM9_WEAPON_ACTIVE")
local aim9_missile_status = get_param_handle("AIM9_MISSILE_STATUS")
local aim9_missile_count = get_param_handle("AIM9_MISSILE_COUNT")
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
local radar_stt_azimuth_stab = get_param_handle("RADAR_STT_AZIMUTH_STAB")
local radar_stt_elevation_stab = get_param_handle("RADAR_STT_ELEVATION_STAB")
local radar_tdc_azimuth = get_param_handle("RADAR_TDC_AZIMUTH")
local radar_tdc_range_scaled = get_param_handle("RADAR_TDC_RANGE_SCALED")
local radar_gate_range_scaled = get_param_handle("RADAR_GATE_RANGE_SCALED")
local radar_contact_01_azimuth = get_param_handle("RADAR_CONTACT_01_AZIMUTH")
local radar_contact_01_range_scaled = get_param_handle("RADAR_CONTACT_01_RANGE_SCALED")
local ws_target_range = get_param_handle("WS_TARGET_RANGE")
local ws_ir_slave_azimuth = get_param_handle("WS_IR_MISSILE_SEEKER_DESIRED_AZIMUTH")
local ws_ir_slave_elevation = get_param_handle("WS_IR_MISSILE_SEEKER_DESIRED_ELEVATION")
local ws_ir_lock = get_param_handle("WS_IR_MISSILE_LOCK")
local ws_ir_tgt_azimuth = get_param_handle("WS_IR_MISSILE_TARGET_AZIMUTH")
local ws_ir_tgt_elevation = get_param_handle("WS_IR_MISSILE_TARGET_ELEVATION")

local debug_last_line = ""
local debug_timer = 0.0
local debug_period = 0.20

local function dlog(msg)
    if log ~= nil and log.info ~= nil then log.info("FCK1C RADAR: " .. tostring(msg)) end
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

local function telemetry_metric()
    local total = 0.0
    total = total + seeker_metric(num(ws_ir_slave_azimuth), num(ws_ir_slave_elevation))
    total = total + seeker_metric(num(radar_stt_azimuth), num(radar_stt_elevation))
    total = total + math.abs(num(radar_stt_range))
    total = total + seeker_metric(num(radar_stt_azimuth_stab), num(radar_stt_elevation_stab))
    total = total + math.abs(num(radar_tdc_azimuth))
    total = total + math.abs(num(radar_tdc_range_scaled))
    total = total + math.abs(num(radar_gate_range_scaled))
    total = total + math.abs(num(radar_contact_01_azimuth))
    total = total + math.abs(num(radar_contact_01_range_scaled))
    return total
end

local function update_debug(state)
    debug_timer = debug_timer + update_rate
    if debug_timer < debug_period then return end

    debug_timer = debug_timer - debug_period

    local line = string.format("DBG uncage=%d aam=%d weapon=%d contact=%d lock=%d desig=%d status=%d aim9=%d src=%s radar=%d mode=%.1f ir=(%.4f,%.4f) stt=(%.4f,%.4f,%.1f) stab=(%.4f,%.4f) tdc=(%.4f,%.4f) gate=%.4f c1=(%.4f,%.4f) ws=%.1f", state.uncage_held and 1 or 0, state.aam_mode and 1 or 0, state.weapon_active and 1 or 0, state.contact_valid and 1 or 0, state.lock_valid and 1 or 0, state.target_designated and 1 or 0, state.missile_status, state.missile_count, state.use_fallback and "fallback" or "telemetry", (num(radar_state) > 0.5 and num(radar_power_state) > 0.5) and 1 or 0, state.radar_mode_value, state.ir_azimuth, state.ir_elevation, state.stt_azimuth, state.stt_elevation, state.stt_range, num(radar_stt_azimuth_stab), num(radar_stt_elevation_stab), num(radar_tdc_azimuth), num(radar_tdc_range_scaled), num(radar_gate_range_scaled), num(radar_contact_01_azimuth), num(radar_contact_01_range_scaled), num(ws_target_range))

    if line ~= debug_last_line then
        debug_last_line = line
        dlog(line)
    end
end

local function read_tracking_input()
    local input = {
        uncage_held = num(aim9_uncage_held) > 0.5,
        aam_mode = in_aam_mode(),
        target_designated = num(aim9_target_designated) > 0.5,
        missile_status = math.floor(num(aim9_missile_status) + 0.5),
        missile_count = math.max(0, math.floor(num(aim9_missile_count) + 0.5)),
        ir_azimuth = num(ws_ir_slave_azimuth),
        ir_elevation = num(ws_ir_slave_elevation),
        stt_azimuth = num(radar_stt_azimuth),
        stt_elevation = num(radar_stt_elevation),
        stt_range = num(radar_stt_range),
        radar_mode_value = num(radar_mode),
        dcs_ir_lock = num(ws_ir_lock) > 0.5,
        dcs_ir_target_azimuth = num(ws_ir_tgt_azimuth),
        dcs_ir_target_elevation = num(ws_ir_tgt_elevation),
    }
    input.weapon_active = input.aam_mode and input.missile_count > 0 and input.missile_status ~= AIM9_STATUS_OFF
    input.dcs_ir_has_signal = math.abs(input.dcs_ir_target_azimuth) > 0.001 or math.abs(input.dcs_ir_target_elevation) > 0.001
    input.real_telemetry_active = telemetry_metric() > 0.00001
    return input
end

local function resolve_tracking(input)
    if input.dcs_ir_lock or input.dcs_ir_has_signal then
        return input.weapon_active and (input.dcs_ir_has_signal or input.dcs_ir_lock), input.weapon_active and input.dcs_ir_lock, false
    end
    if input.real_telemetry_active then
        local ir_contact = seeker_metric(input.ir_azimuth, input.ir_elevation) > 0.00001
        return input.weapon_active and input.uncage_held and ir_contact, input.weapon_active and input.uncage_held and input.stt_range > 1.0, false
    end
    local contact = input.weapon_active and input.uncage_held
    return contact, contact and input.target_designated, true
end

local function publish_tracking(input, contact_valid, lock_valid)
    aim9_weapon_active:set(input.weapon_active and 1 or 0)
    aim9_contact_state:set(contact_valid and 1 or 0)
    aim9_lock_state:set(lock_valid and 1 or 0)
    aim9_seeker_azimuth:set(input.ir_azimuth)
    aim9_seeker_elevation:set(input.ir_elevation)
    aim9_lock_range:set(input.stt_range)

    if input.missile_status == AIM9_STATUS_OFF then
        aim9_seeker_state:set(AIM9_SEEKER_OFF)
    elseif input.missile_status == AIM9_STATUS_TRACK or lock_valid then
        aim9_seeker_state:set(AIM9_SEEKER_TRACK)
    elseif input.missile_status == AIM9_STATUS_RDY then
        aim9_seeker_state:set(AIM9_SEEKER_SEARCH_UNCAGED)
    else
        aim9_seeker_state:set(AIM9_SEEKER_SEARCH_CAGED)
    end

    if not input.weapon_active then
        aim9_tone_state:set(AIM9_TONE_OFF)
    elseif lock_valid then
        aim9_tone_state:set(AIM9_TONE_LOCK)
    elseif contact_valid then
        aim9_tone_state:set(AIM9_TONE_ACQUIRE)
    elseif input.missile_status == AIM9_STATUS_COOL or input.missile_status == AIM9_STATUS_RDY then
        aim9_tone_state:set(AIM9_TONE_SEEK)
    else
        aim9_tone_state:set(AIM9_TONE_OFF)
    end
end

function post_initialize()
    radar_state:set(1)
    radar_power_state:set(1)
    aim9_tone_state:set(AIM9_TONE_OFF)
    aim9_seeker_state:set(AIM9_SEEKER_OFF)
    aim9_contact_state:set(0)
    aim9_lock_state:set(0)
    aim9_weapon_active:set(0)
    aim9_seeker_azimuth:set(0)
    aim9_seeker_elevation:set(0)
    aim9_lock_range:set(0)
end

function update()
    radar_state:set(1)
    radar_power_state:set(1)

    local input = read_tracking_input()
    local contact_valid, lock_valid, use_fallback = resolve_tracking(input)
    publish_tracking(input, contact_valid, lock_valid)
    input.contact_valid = contact_valid
    input.lock_valid = lock_valid
    input.use_fallback = use_fallback
    update_debug(input)
end

need_to_be_closed = false
