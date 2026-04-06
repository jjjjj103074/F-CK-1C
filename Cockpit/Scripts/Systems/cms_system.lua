local dev = GetSelf()
local sensor_data = get_base_data()

local update_rate = 0.02
make_default_activity(update_rate)

dofile(LockOn_Options.script_path .. "command_defs.lua")

local CMD_TRIGGER_FIRST_STAGE = device_commands.TriggerFirstStage
local CMD_CMS_FORWARD = device_commands.CMSForward
local CMD_CMS_AFT = device_commands.CMSAft
local CMD_CMS_LEFT = device_commands.CMSLeft
local CMD_CMS_RIGHT = device_commands.CMSRight
local CMD_CMS_PRESS = device_commands.CMSPress
local CMD_TRIGGER_SECOND_STAGE = device_commands.TriggerSecondStage
local CMD_MASTER_ARM_ON = device_commands.MasterArmOn
local CMD_MASTER_ARM_OFF = device_commands.MasterArmOff
local CMD_MASTER_ARM_SIM = device_commands.MasterArmSim
local CMD_DOGFIGHT_SWITCH = device_commands.DogfightSwitch
local CMD_MISSILE_UNCAGE = device_commands.MissileUncage
local CMD_WEAPON_RELEASE = device_commands.WeaponRelease
local CMD_TMS_UP = device_commands.TMSUp or device_commands.TargetLock
local CMD_TMS_DOWN = device_commands.TMSDown
local CMD_TMS_LEFT = device_commands.TMSLeft
local CMD_TMS_RIGHT = device_commands.TMSRight
local CMD_NAV_MODE = device_commands.NavMode
local CMD_MISSILE_OVERRIDE = device_commands.MissileOverride

local ICMD_PLANE_FIRE = 84
local ICMD_PLANE_FIRE_OFF = 85
local ICMD_PLANE_RADAR_ON_OFF = 86
local ICMD_PLANE_CHANGE_LOCK = 100
local ICMD_PLANE_CHANGE_WEAPON = 101
local ICMD_PLANE_MODE_BVR = 106
local ICMD_PLANE_MODE_VS = 107
local ICMD_PLANE_MODE_BORE = 108
local ICMD_PLANE_MODE_HELMET = 109
local ICMD_PLANE_MODE_FI0 = 110
local ICMD_PLANE_MODE_CANNON = 113
local ICMD_PLANE_LAUNCH_PERMISSION_OVERRIDE = 349
local ICMD_PLANE_PICKLE_ON = 350
local ICMD_PLANE_PICKLE_OFF = 351
local ICMD_PLANE_LOCKON_START = 509
local ICMD_PLANE_LOCKON_FINISH = 510
local ICMD_PLANE_CHANGE_LOCK_UP = 1627
local ICMD_PLANE_DROP_SNAR_ONCE = 176
local ICMD_PLANE_DROP_SNAR_ONCE_OFF = 536
local ICMD_PLANE_DROP_FLARE_ONCE = 357
local ICMD_PLANE_DROP_CHAFF_ONCE = 358

local MASTER_OFF = 0
local MASTER_SIM = 1
local MASTER_ON = 2

local FC_MODE_NAV = 0
local FC_MODE_DGFT = 1
local FC_MODE_MSL = 2

local AAM_SUBMODE_NONE = 0
local AAM_SUBMODE_HELMET = 1
local AAM_SUBMODE_VERTICAL = 2
local AAM_SUBMODE_HUD = 3
local AAM_SUBMODE_BVR = 4

local AIM9_TONE_OFF = 0
local AIM9_TONE_SEEK = 1
local AIM9_TONE_LOCK = 2
local AIM9_STATUS_OFF = 0
local AIM9_STATUS_COOL = 1
local AIM9_STATUS_RDY = 2
local AIM9_STATUS_TRACK = 3
local AIM9_MISSILE_COUNT_DEFAULT = 1
local GUN_COUNT_CANDIDATES = {
    "getGunAmmoCount",
    "getCannonsAmmoCount",
    "getAmmoCount",
}
local AIM9_COUNT_CANDIDATES = {
    "getAAMMissileCount",
    "getAAMCount",
    "getAirMissileCount",
    "getAirToAirMissileCount",
    "getMissileCount",
    "getMissilesCount",
    "getWeaponCount",
    "getSelectedWeaponCount",
    "getAmmoCount",
}

local cms_program_active = false
local cms_program_name = "idle"
local cms_program_steps_remaining = 0
local cms_program_timer = 0.0
local cms_program_interval = 1.0
local cms_program_flare_per_step = 0
local cms_program_chaff_per_step = 0
local master_arm_mode = MASTER_ON
local fire_control_mode = FC_MODE_NAV
local aam_submode = AAM_SUBMODE_NONE
local trigger_second_latched = false
local trigger_release_pending = false
local trigger_hold_timer = 0.0
local trigger_min_hold_time = 0.08
local missile_uncage_held = false
local weapon_release_latched = false
local weapon_release_pending = false
local weapon_release_hold_timer = 0.0
local weapon_release_min_hold_time = 0.08
local aim9_missile_count = AIM9_MISSILE_COUNT_DEFAULT
local aim9_missile_live_source = nil
local aim9_missile_live_last = -1
local gun_ammo_live_source = nil
local gun_ammo_live_last = -1
local store_probe_dumped = false
local store_probe_summary_timer = 0.0
local store_probe_summary_period = 5.0
local target_lock_latched = false
local target_lock_release_pending = false
local target_lock_hold_timer = 0.0
local target_lock_min_hold_time = 0.08
local aim9_target_designated = false
local dgft_auto_lock_timer = 0.0
local dgft_auto_lock_interval = 1.0
local cms_connected = true
local cms_pair_release_pending = 0
local cms_pair_release_latched = false
local cms_pair_release_hold_timer = 0.0
local cms_pair_release_hold_time = 0.04
local cms_pair_release_gap_timer = 0.0
local cms_pair_release_gap_time = 0.12
local debug_state_period = 0.20
local debug_state_timer = 0.0
local debug_state_last_line = ""
local stores_release_allowed
local cms_release_allowed
local hmcs_gun_firing = get_param_handle("HMCS_GUN_FIRING")
local hmcs_master_mode = get_param_handle("HMCS_MASTER_MODE")
local hmcs_dogfight_mode = get_param_handle("HMCS_DOGFIGHT_MODE")
local hmcs_fc_mode = get_param_handle("HMCS_FC_MODE")
local hmcs_aam_submode = get_param_handle("HMCS_AAM_SUBMODE")
local aim9_uncage_state = get_param_handle("AIM9_UNCAGE_HELD")
local aim9_tone_state = get_param_handle("AIM9_TONE_STATE")
local aim9_target_designated_state = get_param_handle("AIM9_TARGET_DESIGNATED")
local aim9_missile_status_state = get_param_handle("AIM9_MISSILE_STATUS")
local aim9_missile_count_state = get_param_handle("AIM9_MISSILE_COUNT")
local radar_state_param = get_param_handle("RADARSTATE")

local function dlog(msg)
    if log ~= nil and log.info ~= nil then
        log.info("FCK1C CMS: " .. tostring(msg))
    end
end

local function safe_sensor_call(method_name, default_value)
    local method = sensor_data and sensor_data[method_name]
    if type(method) ~= "function" then
        return default_value
    end

    local ok, value = pcall(method, sensor_data)
    if ok and type(value) == "number" then
        return value
    end

    return default_value
end

local function describe_sensor_candidate(method_name)
    local method = sensor_data and sensor_data[method_name]
    if type(method) ~= "function" then
        return "missing"
    end

    local ok, value = pcall(method, sensor_data)
    if not ok then
        return "error"
    end
    if value == nil then
        return "nil"
    end
    if type(value) == "number" then
        return string.format("number(%0.3f)", value)
    end

    return type(value)
end

local function dump_store_probe_candidates_once()
    if store_probe_dumped then
        return
    end

    store_probe_dumped = true
    for _, method_name in ipairs(GUN_COUNT_CANDIDATES) do
        dlog("probe gun " .. tostring(method_name) .. " -> " .. describe_sensor_candidate(method_name))
    end
    for _, method_name in ipairs(AIM9_COUNT_CANDIDATES) do
        dlog("probe aim9 " .. tostring(method_name) .. " -> " .. describe_sensor_candidate(method_name))
    end
end

local function first_sensor_count(candidates, min_value, max_value)
    for _, method_name in ipairs(candidates) do
        local value = safe_sensor_call(method_name, -1.0)
        if value >= min_value and value <= max_value then
            return method_name, value
        end
    end
    return nil, -1.0
end

local function refresh_live_store_counts()
    dump_store_probe_candidates_once()

    local gun_source, gun_value = first_sensor_count(GUN_COUNT_CANDIDATES, 0.0, 9999.0)

    if gun_source ~= nil then
        local rounded = math.max(0, math.floor(gun_value + 0.5))
        if gun_source ~= gun_ammo_live_source or rounded ~= gun_ammo_live_last then
            gun_ammo_live_source = gun_source
            gun_ammo_live_last = rounded
            dlog("live gun count -> " .. tostring(rounded) .. " via " .. tostring(gun_source))
        end
    end

    local aim9_source, aim9_value = first_sensor_count(AIM9_COUNT_CANDIDATES, 0.0, 12.0)

    if aim9_source ~= nil then
        local rounded = math.max(0, math.floor(aim9_value + 0.5))
        aim9_missile_count = rounded
        if aim9_source ~= aim9_missile_live_source or rounded ~= aim9_missile_live_last then
            aim9_missile_live_source = aim9_source
            aim9_missile_live_last = rounded
            dlog("live aim9 count -> " .. tostring(rounded) .. " via " .. tostring(aim9_source))
        end
    end

    if gun_source == nil and aim9_source == nil then
        store_probe_summary_timer = store_probe_summary_timer + update_rate
        if store_probe_summary_timer >= store_probe_summary_period then
            store_probe_summary_timer = 0.0
            dlog("live store probe: no candidate hit yet (gun=nil aim9=nil)")
        end
    else
        store_probe_summary_timer = 0.0
    end
end

local function bool_to_num(v)
    if v then
        return 1
    end
    return 0
end

local function master_mode_name()
    if master_arm_mode == MASTER_ON then
        return "ON"
    end
    if master_arm_mode == MASTER_SIM then
        return "SIM"
    end
    return "OFF"
end

local function fc_mode_name()
    if fire_control_mode == FC_MODE_DGFT then
        return "DGFT"
    end
    if fire_control_mode == FC_MODE_MSL then
        return "MSL"
    end
    return "NAV"
end

local function aam_submode_name()
    if aam_submode == AAM_SUBMODE_HELMET then
        return "HMD"
    end
    if aam_submode == AAM_SUBMODE_VERTICAL then
        return "VS"
    end
    if aam_submode == AAM_SUBMODE_HUD then
        return "HUD"
    end
    if aam_submode == AAM_SUBMODE_BVR then
        return "BVR"
    end
    return "NONE"
end

local function dogfight_active()
    return fire_control_mode == FC_MODE_DGFT
end

local function aam_mode_active()
    return fire_control_mode == FC_MODE_DGFT or fire_control_mode == FC_MODE_MSL
end

local function push_hmcs_gun_state()
    if trigger_second_latched and master_arm_mode == MASTER_ON and dogfight_active() then
        hmcs_gun_firing:set(1)
        return
    end
    hmcs_gun_firing:set(0)
end

local function push_hmcs_mode_state()
    hmcs_master_mode:set(master_arm_mode)
    hmcs_dogfight_mode:set(dogfight_active() and 1 or 0)
    hmcs_fc_mode:set(fire_control_mode)
    hmcs_aam_submode:set(aam_submode)
end

local function current_aim9_status()
    if not aam_mode_active() or aim9_missile_count <= 0 then
        return AIM9_STATUS_OFF
    end
    if missile_uncage_held and aim9_target_designated then
        return AIM9_STATUS_TRACK
    end
    if missile_uncage_held then
        return AIM9_STATUS_RDY
    end
    return AIM9_STATUS_COOL
end

local function push_aim9_store_state()
    aim9_missile_status_state:set(current_aim9_status())
    aim9_missile_count_state:set(math.max(0, aim9_missile_count))
end

local function push_aim9_uncage_state()
    aim9_uncage_state:set(missile_uncage_held and 1 or 0)
    push_aim9_store_state()
end

local function push_aim9_designation_state()
    aim9_target_designated_state:set(aim9_target_designated and 1 or 0)
    push_aim9_store_state()
end

local function fire_gate_reason()
    if dispatch_action == nil then
        return "dispatch_nil"
    end
    if master_arm_mode ~= MASTER_ON then
        return "master_not_on"
    end
    if fire_control_mode ~= FC_MODE_DGFT then
        return "not_in_dgft"
    end
    return "ok"
end

local function aam_release_reason()
    if dispatch_action == nil then
        return "dispatch_nil"
    end
    if master_arm_mode ~= MASTER_ON then
        return "master_not_on"
    end
    if fire_control_mode ~= FC_MODE_DGFT and fire_control_mode ~= FC_MODE_MSL then
        return "not_in_aam_mode"
    end
    if not missile_uncage_held then
        return "uncage_not_held"
    end
    return "ok"
end

local function log_gate_snapshot(tag)
    local stores_ok = (master_arm_mode == MASTER_ON)
    local cms_ok = (cms_connected and stores_ok)
    local gun_reason = fire_gate_reason()
    local aam_reason = aam_release_reason()
    local line = string.format(
        "DBG[%s] master=%s fcmode=%s sub=%s stores=%d cms=%d trig=%d relpend=%d hold=%.2f uncage=%d pickle=%d pickrel=%d pickhold=%.2f lock=%d lockrel=%d lockhold=%.2f desig=%d aim9=%d tone=%1.0f cmsprog=%d mode=%s cmsrem=%d gun=%s aam=%s",
        tostring(tag),
        master_mode_name(),
        fc_mode_name(),
        aam_submode_name(),
        bool_to_num(stores_ok),
        bool_to_num(cms_ok),
        bool_to_num(trigger_second_latched),
        bool_to_num(trigger_release_pending),
        trigger_hold_timer,
        bool_to_num(missile_uncage_held),
        bool_to_num(weapon_release_latched),
        bool_to_num(weapon_release_pending),
        weapon_release_hold_timer,
        bool_to_num(target_lock_latched),
        bool_to_num(target_lock_release_pending),
        target_lock_hold_timer,
        bool_to_num(aim9_target_designated),
        aim9_missile_count,
        aim9_tone_state:get(),
        bool_to_num(cms_program_active),
        tostring(cms_program_name),
        cms_program_steps_remaining,
        gun_reason,
        aam_reason
    )

    if line ~= debug_state_last_line then
        dlog(line)
        debug_state_last_line = line
    end
end

local function dispatch_with_log(command_id, tag)
    if dispatch_action == nil then
        return nil
    end

    dlog("dispatch " .. tostring(tag))
    local ok = dispatch_action(nil, command_id)
    if ok ~= nil then
        dlog("dispatch " .. tostring(tag) .. " result=" .. tostring(ok))
    end
    return ok
end

local function ensure_radar_on(reason_tag)
    local radar_state = tonumber(radar_state_param:get()) or 0.0
    if radar_state > 0.5 then
        return
    end

    dlog("ensure radar on -> " .. tostring(reason_tag))
    dispatch_with_log(ICMD_PLANE_RADAR_ON_OFF, "radar on/off")
end

local function fire_on()
    dispatch_with_log(ICMD_PLANE_FIRE, "fire ON")
end

local function fire_off()
    dispatch_with_log(ICMD_PLANE_FIRE_OFF, "fire OFF")
end

local function pickle_on()
    dispatch_with_log(ICMD_PLANE_LAUNCH_PERMISSION_OVERRIDE, "launch permission override")
    dispatch_with_log(ICMD_PLANE_PICKLE_ON, "pickle ON")
end

local function pickle_off()
    dispatch_with_log(ICMD_PLANE_PICKLE_OFF, "pickle OFF")
end

local function cycle_aam_weapon(reason_tag)
    dlog("cycle aam weapon -> " .. tostring(reason_tag))
    dispatch_with_log(ICMD_PLANE_CHANGE_WEAPON, "change weapon")
end

local function apply_aam_submode(mode, reason_tag)
    aam_submode = mode
    dlog("aam submode -> " .. aam_submode_name() .. " by " .. tostring(reason_tag))

    if mode == AAM_SUBMODE_HELMET then
        dispatch_with_log(ICMD_PLANE_MODE_HELMET, "mode Helmet")
        return
    end

    if mode == AAM_SUBMODE_VERTICAL then
        dispatch_with_log(ICMD_PLANE_MODE_VS, "mode VS")
        return
    end

    if mode == AAM_SUBMODE_BVR then
        dispatch_with_log(ICMD_PLANE_MODE_BVR, "mode BVR")
        dispatch_with_log(ICMD_PLANE_MODE_FI0, "mode FI0")
        return
    end

    if mode == AAM_SUBMODE_HUD then
        dlog("hud scan reserved; no dispatch implemented yet")
        return
    end
end

local function target_lock_begin(reason_tag)
    if dispatch_action == nil then
        return
    end

    if fire_control_mode == FC_MODE_DGFT or fire_control_mode == FC_MODE_MSL then
        apply_aam_submode(aam_submode, reason_tag .. "_refresh")
    end

    dispatch_with_log(ICMD_PLANE_LOCKON_START, "lockon start")
    dispatch_with_log(ICMD_PLANE_CHANGE_LOCK, "change lock")
    target_lock_latched = true
    target_lock_release_pending = false
    target_lock_hold_timer = target_lock_min_hold_time
    aim9_target_designated = true
    push_aim9_designation_state()
end

local function target_lock_end(reason_tag)
    if dispatch_action == nil then
        return
    end

    dispatch_with_log(ICMD_PLANE_LOCKON_FINISH, "lockon finish")
    dispatch_with_log(ICMD_PLANE_CHANGE_LOCK_UP, "change lock up")
    target_lock_latched = false
    target_lock_release_pending = false
    target_lock_hold_timer = 0.0
    dlog("target lock end -> " .. tostring(reason_tag))
end

local function target_lock_pulse(reason_tag)
    target_lock_begin(reason_tag)
    target_lock_release_pending = true
end

local function unlock_target(reason_tag)
    dlog("unlock target -> " .. tostring(reason_tag))

    if target_lock_latched then
        if target_lock_hold_timer <= 0.0 then
            target_lock_end(reason_tag)
        else
            target_lock_release_pending = true
        end
    else
        dispatch_with_log(ICMD_PLANE_LOCKON_FINISH, "lockon finish")
        dispatch_with_log(ICMD_PLANE_CHANGE_LOCK_UP, "change lock up")
    end

    aim9_target_designated = false
    push_aim9_designation_state()

end

local function switch_target(reason_tag)
    dlog("switch target -> " .. tostring(reason_tag))
    dispatch_with_log(ICMD_PLANE_CHANGE_LOCK, "change lock")
    aim9_target_designated = true
    push_aim9_designation_state()
end

local function consume_aim9_missile(reason_tag)
    if aim9_missile_live_source == nil then
        if aim9_missile_count <= 0 then
            return
        end
        aim9_missile_count = math.max(0, aim9_missile_count - 1)
        dlog("aim9 consume -> " .. tostring(reason_tag) .. " remaining=" .. tostring(aim9_missile_count) .. " via fallback")
    else
        dlog("aim9 consume -> " .. tostring(reason_tag) .. " awaiting live sync via " .. tostring(aim9_missile_live_source))
    end

    unlock_target("aim9_launch")
    push_aim9_store_state()
end

local function stop_gun_and_pickle()
    if trigger_second_latched then
        fire_off()
        trigger_second_latched = false
        trigger_release_pending = false
        trigger_hold_timer = 0.0
    end

    if weapon_release_latched then
        pickle_off()
        weapon_release_latched = false
        weapon_release_pending = false
        weapon_release_hold_timer = 0.0
    end

    push_hmcs_gun_state()
end

local function set_fire_control_mode(mode, reason_tag, cycle_weapon)
    fire_control_mode = mode
    dgft_auto_lock_timer = 0.0

    if mode == FC_MODE_NAV then
        aam_submode = AAM_SUBMODE_NONE
        aim9_target_designated = false
        push_aim9_designation_state()
        unlock_target("nav_mode")
        dispatch_with_log(ICMD_PLANE_MODE_CANNON, "mode cannon")
    elseif mode == FC_MODE_DGFT then
        master_arm_mode = MASTER_ON
        aam_submode = AAM_SUBMODE_HELMET
        ensure_radar_on(reason_tag)
        if cycle_weapon then
            cycle_aam_weapon(reason_tag)
        end
        apply_aam_submode(AAM_SUBMODE_HELMET, reason_tag)
        target_lock_pulse(reason_tag .. "_auto_lock")
    elseif mode == FC_MODE_MSL then
        master_arm_mode = MASTER_ON
        aam_submode = AAM_SUBMODE_BVR
        aim9_target_designated = false
        push_aim9_designation_state()
        unlock_target("msl_mode_enter")
        ensure_radar_on(reason_tag)
        if cycle_weapon then
            cycle_aam_weapon(reason_tag)
        end
        apply_aam_submode(AAM_SUBMODE_BVR, reason_tag)
    end

    push_hmcs_mode_state()
    push_aim9_uncage_state()
    log_gate_snapshot(reason_tag)
end

local function flare_once()
    dlog("dispatch flare once")
    local ok = dispatch_action(nil, ICMD_PLANE_DROP_FLARE_ONCE)
    if ok ~= nil then
        dlog("dispatch flare result=" .. tostring(ok))
    end
end

local function chaff_once()
    dlog("dispatch chaff once")
    local ok = dispatch_action(nil, ICMD_PLANE_DROP_CHAFF_ONCE)
    if ok ~= nil then
        dlog("dispatch chaff result=" .. tostring(ok))
    end
end

local function pair_release_off()
    if not cms_pair_release_latched then
        return
    end

    dlog("dispatch cm release OFF")
    local ok = dispatch_action(nil, ICMD_PLANE_DROP_SNAR_ONCE_OFF)
    if ok ~= nil then
        dlog("dispatch cm release OFF result=" .. tostring(ok))
    end

    cms_pair_release_latched = false
    cms_pair_release_hold_timer = 0.0
end

local function clear_pair_release_queue(reason_tag)
    if cms_pair_release_latched or cms_pair_release_pending > 0 then
        dlog("clear cm release queue -> " .. tostring(reason_tag) .. " pending=" .. tostring(cms_pair_release_pending))
    end

    pair_release_off()
    cms_pair_release_pending = 0
    cms_pair_release_gap_timer = 0.0
end

local function queue_pair_release(count)
    if count == nil or count <= 0 then
        return
    end

    cms_pair_release_pending = cms_pair_release_pending + count
    dlog("queue cm release x" .. tostring(count) .. " pending=" .. tostring(cms_pair_release_pending))
end

local function release_countermeasures(flare_count, chaff_count)
    if flare_count == nil then
        flare_count = 0
    end
    if chaff_count == nil then
        chaff_count = 0
    end

    if flare_count <= 0 and chaff_count <= 0 then
        return
    end

    if not cms_release_allowed() then
        dlog("countermeasure release blocked by arm/cms state")
        return
    end

    local paired_release_count = math.min(flare_count, chaff_count)
    if paired_release_count > 0 then
        queue_pair_release(paired_release_count)
        flare_count = flare_count - paired_release_count
        chaff_count = chaff_count - paired_release_count
    end

    local i = 0
    while i < flare_count do
        flare_once()
        i = i + 1
    end

    i = 0
    while i < chaff_count do
        chaff_once()
        i = i + 1
    end
end

local function stop_cms_program(reason_tag)
    if cms_program_active then
        dlog("cms program stop -> " .. tostring(reason_tag))
    end
    cms_program_active = false
    cms_program_name = "idle"
    cms_program_steps_remaining = 0
    cms_program_timer = 0.0
    cms_program_interval = 1.0
    cms_program_flare_per_step = 0
    cms_program_chaff_per_step = 0
    log_gate_snapshot(reason_tag)
end

local function abort_cms_program(reason_tag)
    stop_cms_program(reason_tag)
    clear_pair_release_queue(reason_tag)
end

local function start_cms_program(name, steps, interval, flare_per_step, chaff_per_step, immediate_release)
    if not cms_release_allowed() then
        dlog("cms program " .. tostring(name) .. " blocked by arm/cms state")
        log_gate_snapshot("cms_" .. tostring(name) .. "_block")
        return
    end

    cms_program_active = true
    cms_program_name = tostring(name)
    cms_program_steps_remaining = steps
    cms_program_timer = 0.0
    cms_program_interval = interval
    cms_program_flare_per_step = flare_per_step
    cms_program_chaff_per_step = chaff_per_step

    if immediate_release and cms_program_steps_remaining > 0 then
        release_countermeasures(cms_program_flare_per_step, cms_program_chaff_per_step)
        cms_program_steps_remaining = cms_program_steps_remaining - 1
    end

    log_gate_snapshot("cms_" .. tostring(name) .. "_start")
end

local function handle_tms_up(value)
    if fire_control_mode == FC_MODE_DGFT then
        if value > 0.5 then
            apply_aam_submode(AAM_SUBMODE_HELMET, "tms_up_dgft")
            target_lock_pulse("tms_up_dgft")
            log_gate_snapshot("tms_up_dgft")
        end
        return
    end

    if value > 0.5 then
        target_lock_begin("tms_up")
        log_gate_snapshot("tms_up_down")
        return
    end

    if target_lock_latched then
        if target_lock_hold_timer <= 0.0 then
            target_lock_end("tms_up_release")
        else
            target_lock_release_pending = true
        end
    end
    log_gate_snapshot("tms_up_up")
end

local function handle_tms_down(value)
    if value <= 0.5 then
        return
    end

    if fire_control_mode == FC_MODE_DGFT then
        apply_aam_submode(AAM_SUBMODE_VERTICAL, "tms_down_dgft")
        target_lock_pulse("tms_down_dgft")
        log_gate_snapshot("tms_down_dgft")
        return
    end

    unlock_target("tms_down")
    log_gate_snapshot("tms_down")
end

local function handle_tms_left(value)
    if value <= 0.5 then
        return
    end

    if fire_control_mode == FC_MODE_MSL then
        dlog("tms left -> IFF reserved")
        log_gate_snapshot("tms_left_iff_reserved")
        return
    end

    dlog("tms left -> no function in " .. fc_mode_name())
    log_gate_snapshot("tms_left_noop")
end

local function handle_tms_right(value)
    if value <= 0.5 then
        return
    end

    if fire_control_mode == FC_MODE_DGFT then
        apply_aam_submode(AAM_SUBMODE_HUD, "tms_right_dgft")
        log_gate_snapshot("tms_right_hud_reserved")
        return
    end

    if fire_control_mode == FC_MODE_MSL then
        switch_target("tms_right_msl")
        log_gate_snapshot("tms_right_switch_target")
        return
    end

    dlog("tms right -> no function in NAV")
    log_gate_snapshot("tms_right_noop")
end

dev:listen_command(CMD_TRIGGER_FIRST_STAGE)
dev:listen_command(CMD_CMS_FORWARD)
dev:listen_command(CMD_CMS_AFT)
dev:listen_command(CMD_CMS_LEFT)
dev:listen_command(CMD_CMS_RIGHT)
dev:listen_command(CMD_CMS_PRESS)
dev:listen_command(CMD_TRIGGER_SECOND_STAGE)
dev:listen_command(CMD_MASTER_ARM_ON)
dev:listen_command(CMD_MASTER_ARM_OFF)
dev:listen_command(CMD_MASTER_ARM_SIM)
dev:listen_command(CMD_DOGFIGHT_SWITCH)
dev:listen_command(CMD_MISSILE_UNCAGE)
dev:listen_command(CMD_WEAPON_RELEASE)
dev:listen_command(CMD_TMS_UP)
dev:listen_command(CMD_TMS_DOWN)
dev:listen_command(CMD_TMS_LEFT)
dev:listen_command(CMD_TMS_RIGHT)
dev:listen_command(CMD_NAV_MODE)
dev:listen_command(CMD_MISSILE_OVERRIDE)

stores_release_allowed = function()
    return master_arm_mode == MASTER_ON
end

cms_release_allowed = function()
    return cms_connected and stores_release_allowed()
end

function post_initialize()
    dlog("loaded; CMS connected=" .. tostring(cms_connected) .. ", master=" .. tostring(master_arm_mode))
    push_hmcs_gun_state()
    push_hmcs_mode_state()
    push_aim9_uncage_state()
    push_aim9_designation_state()
    push_aim9_store_state()
    log_gate_snapshot("init")
end

function SetCommand(command, value)
    dlog("SetCommand cmd=" .. tostring(command) .. " val=" .. string.format("%.2f", value))

    if command == CMD_TRIGGER_FIRST_STAGE then
        log_gate_snapshot((value > 0.5) and "trigger1_down" or "trigger1_up")
        return
    end

    if command == CMD_MISSILE_UNCAGE then
        missile_uncage_held = value > 0.5
        if missile_uncage_held and (fire_control_mode == FC_MODE_DGFT or fire_control_mode == FC_MODE_MSL) then
            ensure_radar_on("uncage_cmd")
            apply_aam_submode(aam_submode, "uncage_cmd")
        elseif not missile_uncage_held and fire_control_mode == FC_MODE_DGFT then
            unlock_target("uncage_release_dgft")
        end
        push_aim9_uncage_state()
        log_gate_snapshot(missile_uncage_held and "uncage_down" or "uncage_up")
        return
    end

    if command == CMD_TMS_UP then
        handle_tms_up(value)
        return
    end

    if command == CMD_TMS_DOWN then
        handle_tms_down(value)
        return
    end

    if command == CMD_TMS_LEFT then
        handle_tms_left(value)
        return
    end

    if command == CMD_TMS_RIGHT then
        handle_tms_right(value)
        return
    end

    if command == CMD_WEAPON_RELEASE then
        if value > 0.5 then
            if fire_control_mode == FC_MODE_DGFT or fire_control_mode == FC_MODE_MSL then
                apply_aam_submode(aam_submode, "pickle")
            end
            log_gate_snapshot("pickle_down")
            if aam_release_reason() == "ok" then
                pickle_on()
                weapon_release_latched = true
                weapon_release_pending = false
                weapon_release_hold_timer = weapon_release_min_hold_time
            else
                dlog("pickle blocked reason=" .. tostring(aam_release_reason()))
                weapon_release_latched = false
                weapon_release_pending = false
                weapon_release_hold_timer = 0.0
            end
        else
            log_gate_snapshot("pickle_up")
            if weapon_release_latched then
                if weapon_release_hold_timer <= 0.0 then
                    pickle_off()
                    weapon_release_latched = false
                    weapon_release_pending = false
                    consume_aim9_missile("pickle_up")
                else
                    weapon_release_pending = true
                end
            end
        end
        return
    end

    if command == CMD_TRIGGER_SECOND_STAGE then
        if value > 0.5 then
            if fire_control_mode ~= FC_MODE_DGFT then
                dlog("trigger blocked by fire control mode=" .. fc_mode_name())
                log_gate_snapshot("trigger_block_mode")
                return
            end
            if missile_uncage_held then
                dlog("trigger second stage ignored while uncage held; use weapon release for AIM-9")
                log_gate_snapshot("trigger_ignored_aam")
                return
            end
            log_gate_snapshot("trigger_down")
            if stores_release_allowed() then
                fire_on()
                trigger_second_latched = true
                trigger_release_pending = false
                trigger_hold_timer = trigger_min_hold_time
                push_hmcs_gun_state()
            else
                dlog("trigger blocked by master arm mode=" .. tostring(master_arm_mode))
                trigger_second_latched = false
                trigger_release_pending = false
                trigger_hold_timer = 0.0
                push_hmcs_gun_state()
            end
        else
            log_gate_snapshot("trigger_up")
            if trigger_second_latched then
                if trigger_hold_timer <= 0.0 then
                    fire_off()
                    trigger_second_latched = false
                    trigger_release_pending = false
                    push_hmcs_gun_state()
                else
                    trigger_release_pending = true
                end
            end
        end
        return
    end

    if value <= 0.5 then
        return
    end

    if command == CMD_MASTER_ARM_ON then
        master_arm_mode = MASTER_ON
        set_fire_control_mode(FC_MODE_NAV, "master_on", false)
        dlog("master arm -> ON")
        return
    end

    if command == CMD_MASTER_ARM_OFF then
        master_arm_mode = MASTER_OFF
        stop_gun_and_pickle()
        abort_cms_program("master_off_abort")
        set_fire_control_mode(FC_MODE_NAV, "master_off", false)
        dlog("master arm -> OFF")
        return
    end

    if command == CMD_MASTER_ARM_SIM then
        master_arm_mode = MASTER_SIM
        stop_gun_and_pickle()
        abort_cms_program("master_sim_abort")
        set_fire_control_mode(FC_MODE_NAV, "master_sim", false)
        dlog("master arm -> SIM")
        return
    end

    if command == CMD_NAV_MODE then
        set_fire_control_mode(FC_MODE_NAV, "nav_mode", false)
        return
    end

    if command == CMD_DOGFIGHT_SWITCH then
        dlog("dogfight switch pressed; force master ON")
        set_fire_control_mode(FC_MODE_DGFT, "dogfight", true)
        return
    end

    if command == CMD_MISSILE_OVERRIDE then
        dlog("missile override selected; default AIM-120C, fallback AIM-9")
        set_fire_control_mode(FC_MODE_MSL, "missile_override", true)
        return
    end

    if command == CMD_CMS_FORWARD then
        start_cms_program("forward", 10, 1.0, 1, 1, true)
        return
    end

    if command == CMD_CMS_LEFT then
        start_cms_program("left", 2, 0.12, 1, 1, true)
        return
    end

    if command == CMD_CMS_AFT then
        start_cms_program("aft", 7, 0.5, 0, 1, true)
        return
    end

    if command == CMD_CMS_RIGHT then
        abort_cms_program("cms_right_abort")
        return
    end
end

function update()
    refresh_live_store_counts()

    debug_state_timer = debug_state_timer + update_rate
    if debug_state_timer >= debug_state_period then
        debug_state_timer = debug_state_timer - debug_state_period
        log_gate_snapshot("tick")
    end

    if trigger_hold_timer > 0.0 then
        trigger_hold_timer = trigger_hold_timer - update_rate
        if trigger_hold_timer < 0.0 then
            trigger_hold_timer = 0.0
        end
    end

    if weapon_release_hold_timer > 0.0 then
        weapon_release_hold_timer = weapon_release_hold_timer - update_rate
        if weapon_release_hold_timer < 0.0 then
            weapon_release_hold_timer = 0.0
        end
    end

    if target_lock_hold_timer > 0.0 then
        target_lock_hold_timer = target_lock_hold_timer - update_rate
        if target_lock_hold_timer < 0.0 then
            target_lock_hold_timer = 0.0
        end
    end

    if trigger_release_pending and trigger_second_latched and trigger_hold_timer <= 0.0 then
        fire_off()
        trigger_second_latched = false
        trigger_release_pending = false
        push_hmcs_gun_state()
        log_gate_snapshot("trigger_release")
    end

    if weapon_release_pending and weapon_release_latched and weapon_release_hold_timer <= 0.0 then
        pickle_off()
        weapon_release_latched = false
        weapon_release_pending = false
        consume_aim9_missile("pickle_release")
        log_gate_snapshot("pickle_release")
    end

    if target_lock_release_pending and target_lock_latched and target_lock_hold_timer <= 0.0 then
        target_lock_end("target_lock_release")
        log_gate_snapshot("target_lock_release")
    end

    push_hmcs_gun_state()

    if cms_pair_release_latched then
        cms_pair_release_hold_timer = cms_pair_release_hold_timer - update_rate
        if cms_pair_release_hold_timer <= 0.0 then
            pair_release_off()
            cms_pair_release_pending = cms_pair_release_pending - 1
            if cms_pair_release_pending < 0 then
                cms_pair_release_pending = 0
            end
            cms_pair_release_gap_timer = cms_pair_release_gap_time
        end
    elseif cms_pair_release_pending > 0 then
        if cms_pair_release_gap_timer > 0.0 then
            cms_pair_release_gap_timer = cms_pair_release_gap_timer - update_rate
            if cms_pair_release_gap_timer < 0.0 then
                cms_pair_release_gap_timer = 0.0
            end
        elseif cms_release_allowed() then
            dlog("dispatch cm release ON")
            local ok = dispatch_action(nil, ICMD_PLANE_DROP_SNAR_ONCE)
            if ok ~= nil then
                dlog("dispatch cm release ON result=" .. tostring(ok))
            end
            cms_pair_release_latched = true
            cms_pair_release_hold_timer = cms_pair_release_hold_time
        else
            clear_pair_release_queue("cms_pair_block")
        end
    end

    if fire_control_mode == FC_MODE_DGFT and master_arm_mode == MASTER_ON and missile_uncage_held and aim9_missile_count > 0 then
        dgft_auto_lock_timer = dgft_auto_lock_timer - update_rate
        if dgft_auto_lock_timer <= 0.0 then
            target_lock_pulse("dgft_auto")
            dgft_auto_lock_timer = dgft_auto_lock_interval
        end
    else
        dgft_auto_lock_timer = dgft_auto_lock_interval
    end

    push_hmcs_mode_state()
    push_aim9_uncage_state()

    if not cms_program_active then
        return
    end

    if cms_program_steps_remaining <= 0 then
        stop_cms_program("cms_done")
        return
    end

    cms_program_timer = cms_program_timer + update_rate
    if cms_program_timer >= cms_program_interval then
        cms_program_timer = cms_program_timer - cms_program_interval
        if cms_release_allowed() then
            release_countermeasures(cms_program_flare_per_step, cms_program_chaff_per_step)
            cms_program_steps_remaining = cms_program_steps_remaining - 1
            log_gate_snapshot("cms_prog_drop")
        else
            abort_cms_program("cms_prog_block")
        end
    end
end

need_to_be_closed = false
