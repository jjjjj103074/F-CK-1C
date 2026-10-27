-- MC owns modes, HOTAS decisions, gun/pickle/lock timing and store count policy.
-- This device executes ordered DCS requests, publishes raw API counts, and runs CM programs.
local dev = GetSelf()
local sensor_data = get_base_data()
local update_rate = 0.02
make_default_activity(update_rate)
dofile(LockOn_Options.script_path .. "command_defs.lua")
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")

local adapter_available = get_param_handle(cockpit_params.McAdapterAvailable)
local missile_available = get_param_handle(cockpit_params.McMissileCountAvailable)
local missile_count = get_param_handle(cockpit_params.McMissileCount)
local gun_available = get_param_handle(cockpit_params.McGunCountAvailable)
local gun_count = get_param_handle(cockpit_params.McGunCount)
local master_mode = get_param_handle(cockpit_params.HmcsMasterMode)
local radar_power = get_param_handle(cockpit_params.RadarState)

local ICMD_PLANE_DROP_SNAR_ONCE = dcs_commands.iCommandPlaneDropSnarOnce
local ICMD_PLANE_DROP_SNAR_ONCE_OFF = dcs_commands.iCommandPlaneDropSnarOnceOff
local ICMD_PLANE_DROP_FLARE_ONCE = dcs_commands.iCommandPlaneDropFlareOnce
local ICMD_PLANE_DROP_CHAFF_ONCE = dcs_commands.iCommandPlaneDropChaffOnce
local cms_program_active = false
local cms_program_name = "idle"
local cms_program_steps_remaining = 0
local cms_program_timer = 0.0
local cms_program_interval = 1.0
local cms_program_flare_per_step = 0
local cms_program_chaff_per_step = 0
local cms_pair_release_pending = 0
local cms_pair_release_latched = false
local cms_pair_release_hold_timer = 0.0
local cms_pair_release_hold_time = 0.04
local cms_pair_release_gap_timer = 0.0
local cms_pair_release_gap_time = 0.12

local function dlog(message)
    if log and log.info then log.info("FCK1C CMS: " .. tostring(message)) end
end

local function cms_release_allowed()
    return dispatch_action ~= nil and master_mode:get() > 1.5
end

-- Preserve the existing API names and precedence; confirm them in DCS before changing this path.
local function read_count(candidates, maximum)
    for _, name in ipairs(candidates) do
        local method = sensor_data and sensor_data[name]
        if type(method) == "function" then
            local ok, value = pcall(method, sensor_data)
            if ok and type(value) == "number" and value >= 0 and value <= maximum then
                return true, math.floor(value + 0.5)
            end
        end
    end
    return false, 0
end

local function publish_counts()
    local has_gun, guns = read_count({ "getGunAmmoCount", "getCannonsAmmoCount", "getAmmoCount" }, 9999)
    local has_missiles, missiles = read_count({
        "getAAMMissileCount",
        "getAAMCount",
        "getAirMissileCount",
        "getAirToAirMissileCount",
        "getMissileCount",
        "getMissilesCount",
        "getWeaponCount",
        "getSelectedWeaponCount",
        "getAmmoCount",
    }, 12)
    gun_available:set(has_gun and 1 or 0)
    gun_count:set(guns)
    missile_available:set(has_missiles and 1 or 0)
    missile_count:set(missiles)
    adapter_available:set(dispatch_action ~= nil and 1 or 0)
end

local function flare_once()
    dlog("dispatch flare once")
    local ok = dispatch_action(nil, ICMD_PLANE_DROP_FLARE_ONCE)
    if ok ~= nil then dlog("dispatch flare result=" .. tostring(ok)) end
end

local function chaff_once()
    dlog("dispatch chaff once")
    local ok = dispatch_action(nil, ICMD_PLANE_DROP_CHAFF_ONCE)
    if ok ~= nil then dlog("dispatch chaff result=" .. tostring(ok)) end
end

local function pair_release_off()
    if not cms_pair_release_latched then return end

    dlog("dispatch cm release OFF")
    local ok = dispatch_action(nil, ICMD_PLANE_DROP_SNAR_ONCE_OFF)
    if ok ~= nil then dlog("dispatch cm release OFF result=" .. tostring(ok)) end

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
    if count == nil or count <= 0 then return end

    cms_pair_release_pending = cms_pair_release_pending + count
    dlog("queue cm release x" .. tostring(count) .. " pending=" .. tostring(cms_pair_release_pending))
end

local function release_countermeasures(flare_count, chaff_count)
    if flare_count == nil then flare_count = 0 end
    if chaff_count == nil then chaff_count = 0 end

    if flare_count <= 0 and chaff_count <= 0 then return end

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
    if cms_program_active then dlog("cms program stop -> " .. tostring(reason_tag)) end
    cms_program_active = false
    cms_program_name = "idle"
    cms_program_steps_remaining = 0
    cms_program_timer = 0.0
    cms_program_interval = 1.0
    cms_program_flare_per_step = 0
    cms_program_chaff_per_step = 0
end

local function abort_cms_program(reason_tag)
    stop_cms_program(reason_tag)
    clear_pair_release_queue(reason_tag)
end

local function start_cms_program(name, steps, interval, flare_per_step, chaff_per_step, immediate_release)
    if not cms_release_allowed() then
        dlog("cms program " .. tostring(name) .. " blocked by arm/cms state")
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
end

-- Stable semantic MC values -> generated DCS IDs. No native IDs enter Core.
local native_actions = {
    [1] = dcs_commands.iCommandPlaneFire,
    [2] = dcs_commands.iCommandPlaneFireOff,
    [3] = dcs_commands.iCommandPlaneLaunchPermissionOverride,
    [4] = dcs_commands.iCommandPlanePickleOn,
    [5] = dcs_commands.iCommandPlanePickleOff,
    [6] = dcs_commands.iCommandPlaneRadarOnOff,
    [7] = dcs_commands.iCommandPlaneChangeWeapon,
    [8] = dcs_commands.iCommandPlaneModeHelmet,
    [9] = dcs_commands.iCommandPlaneModeVS,
    [10] = dcs_commands.iCommandPlaneModeBVR,
    [11] = dcs_commands.iCommandPlaneModeFI0,
    [12] = dcs_commands.iCommandPlaneModeCannon,
    [13] = dcs_commands.iCommandPlaneLockOnStart,
    [14] = dcs_commands.iCommandPlaneChangeLock,
    [15] = dcs_commands.iCommandPlaneLockOnFinish,
    [16] = dcs_commands.iCommandPlaneChangeLockUp,
}

local function execute_action(action)
    if dispatch_action == nil then return false end
    -- MC requests the ON state; DCS exposes only a toggle. Recheck the current adapter state.
    if action == 6 and radar_power:get() > 0.5 then return true end
    local native = native_actions[action]
    if native then
        local ok, result = pcall(dispatch_action, nil, native)
        return ok and result ~= false
    elseif action == 17 then
        start_cms_program("forward", 10, 1.0, 1, 1, true)
    elseif action == 18 then
        start_cms_program("aft", 7, 0.5, 0, 1, true)
    elseif action == 19 then
        start_cms_program("left", 2, 0.12, 1, 1, true)
    elseif action == 20 then
        abort_cms_program("mc_abort")
    else
        return false
    end
    return true
end

local mc_io = dofile(LockOn_Options.script_path .. "Systems/mc_io.lua").new(execute_action)

function post_initialize()
    publish_counts()
end

function update()
    publish_counts()
    mc_io.update()
    if cms_pair_release_latched then
        cms_pair_release_hold_timer = cms_pair_release_hold_timer - update_rate
        if cms_pair_release_hold_timer <= 0.0 then
            pair_release_off()
            cms_pair_release_pending = cms_pair_release_pending - 1
            if cms_pair_release_pending < 0 then cms_pair_release_pending = 0 end
            cms_pair_release_gap_timer = cms_pair_release_gap_time
        end
    elseif cms_pair_release_pending > 0 then
        if cms_pair_release_gap_timer > 0.0 then
            cms_pair_release_gap_timer = cms_pair_release_gap_timer - update_rate
            if cms_pair_release_gap_timer < 0.0 then cms_pair_release_gap_timer = 0.0 end
        elseif cms_release_allowed() then
            dlog("dispatch cm release ON")
            local ok = dispatch_action(nil, ICMD_PLANE_DROP_SNAR_ONCE)
            if ok ~= nil then dlog("dispatch cm release ON result=" .. tostring(ok)) end
            cms_pair_release_latched = true
            cms_pair_release_hold_timer = cms_pair_release_hold_time
        else
            clear_pair_release_queue("cms_pair_block")
        end
    end

    if not cms_program_active then return end

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
        else
            abort_cms_program("cms_prog_block")
        end
    end
end

need_to_be_closed = false
