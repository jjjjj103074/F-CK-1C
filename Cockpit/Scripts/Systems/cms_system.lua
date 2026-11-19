-- 執行 DcsBridge 的共用有序 DCS 操作，發布介面可用性，並運行反制程序。
local update_rate = 0.02
make_default_activity(update_rate)
dofile(LockOn_Options.script_path .. "command_defs.lua")
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")

local adapter_available = get_param_handle(cockpit_params.McAdapterAvailable)
local radar_power = get_param_handle(cockpit_params.RadarState)
local mc_generation = get_param_handle(cockpit_params.McGeneration)
local selection_sequence = get_param_handle(cockpit_params.WeaponSelectionActionSequence)
local selection_request = get_param_handle(cockpit_params.WeaponSelectionRequestRevision)
local requested_station = get_param_handle(cockpit_params.WeaponRequestedStation)
local observation_revision = get_param_handle(cockpit_params.WeaponObservationRevision)
local selection_ack = get_param_handle(cockpit_params.WeaponSelectionAckRevision)
local selected_station = get_param_handle(cockpit_params.WeaponObservationSelectedStation)

local ICMD_PLANE_DROP_SNAR_ONCE = dcs_commands.iCommandPlaneDropSnarOnce
local ICMD_PLANE_DROP_SNAR_ONCE_OFF = dcs_commands.iCommandPlaneDropSnarOnceOff
local cms_program_active = false
local cms_program_steps_remaining = 0
local cms_program_timer = 0.0
local cms_forward_rounds = 10
local cms_forward_interval = 1.0
local cms_pair_release_pending = 0
local cms_pair_release_latched = false
local cms_pair_release_hold_timer = 0.0
local cms_pair_release_hold_time = 0.04
local cms_pair_release_gap_timer = 0.0
local cms_pair_release_gap_time = 0.12

local function dlog(message)
    if log and log.info then log.info("FCK1C CMS: " .. tostring(message)) end
end

local function cms_adapter_ready()
    return dispatch_action ~= nil
end

local function publish_adapter_availability()
    adapter_available:set(cms_adapter_ready() and 1 or 0)
end

local function pair_release_off()
    if not cms_pair_release_latched then return end
    if cms_adapter_ready() then
        dlog("dispatch cm release OFF")
        dispatch_action(nil, ICMD_PLANE_DROP_SNAR_ONCE_OFF)
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

local function stop_cms_program(reason_tag)
    if cms_program_active then dlog("cms program stop -> " .. tostring(reason_tag)) end
    cms_program_active = false
    cms_program_steps_remaining = 0
    cms_program_timer = 0.0
end

local function abort_cms_program(reason_tag)
    stop_cms_program(reason_tag)
    clear_pair_release_queue(reason_tag)
end

-- 每輪要求熱焰彈與干擾絲各一發，沿用原生配對施放脈衝。
local function queue_pair_release()
    cms_pair_release_pending = cms_pair_release_pending + 1
end

local function start_cms_program()
    abort_cms_program("restart")
    cms_program_active = true
    cms_program_steps_remaining = cms_forward_rounds - 1
    cms_program_timer = 0.0
    queue_pair_release()
    dlog("cms program start -> forward rounds=" .. tostring(cms_forward_rounds))
end

-- Stable semantic action values -> generated DCS IDs. No native IDs enter Core.
local native_actions = {
    [1] = dcs_commands.iCommandPlaneFire,
    [2] = dcs_commands.iCommandPlaneFireOff,
    [3] = dcs_commands.iCommandPlaneLaunchPermissionOverride,
    [4] = dcs_commands.iCommandPlanePickleOn,
    [5] = dcs_commands.iCommandPlanePickleOff,
    [6] = dcs_commands.iCommandPlaneRadarOnOff,
    [8] = dcs_commands.iCommandPlaneModeHelmet,
    [22] = dcs_commands.iCommandPlaneModeBore,
    [9] = dcs_commands.iCommandPlaneModeVS,
    [10] = dcs_commands.iCommandPlaneModeBVR,
    [11] = dcs_commands.iCommandPlaneModeFI0,
    [12] = dcs_commands.iCommandPlaneModeCannon,
    [13] = dcs_commands.iCommandPlaneLockOnStart,
    [14] = dcs_commands.iCommandPlaneChangeLock,
    [15] = dcs_commands.iCommandPlaneLockOnFinish,
    [16] = dcs_commands.iCommandPlaneChangeLockUp,
}

local function integer(value, minimum, maximum)
    return type(value) == "number" and value >= minimum and value <= maximum and value == math.floor(value)
end

local function selection_completed(sequence, generation)
    local before = observation_revision:get()
    if not integer(before, 2, 9007199254740991) or before % 2 ~= 0 then return false end
    local command_sequence = selection_sequence:get()
    local request = selection_request:get()
    local station = requested_station:get()
    local acknowledged = selection_ack:get()
    local selected = selected_station:get()
    return observation_revision:get() == before and mc_generation:get() == generation
        and command_sequence == sequence and integer(request, 1, 9007199254740991)
        and integer(station, -1, 8)
        and acknowledged == request and selected == station
end

local function execute_action(action, sequence, generation)
    if action == 0 then return true end
    if action == 21 then return selection_completed(sequence, generation) end
    if dispatch_action == nil then return false end
    -- MC requests the ON state; DCS exposes only a toggle. Recheck the current adapter state.
    if action == 6 and radar_power:get() > 0.5 then return true end
    local native = native_actions[action]
    if native then
        local ok, result = pcall(dispatch_action, nil, native)
        return ok and result ~= false
    elseif action == 17 then
        start_cms_program()
    else
        return false
    end
    return true
end

local mc_io = dofile(LockOn_Options.script_path .. "Systems/mc_io.lua").new(execute_action)

function post_initialize()
    publish_adapter_availability()
end

function update()
    publish_adapter_availability()
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
        elseif cms_adapter_ready() then
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
    if cms_program_timer >= cms_forward_interval then
        cms_program_timer = cms_program_timer - cms_forward_interval
        if cms_adapter_ready() then
            queue_pair_release()
            cms_program_steps_remaining = cms_program_steps_remaining - 1
        else
            abort_cms_program("cms_prog_block")
        end
    end
end

need_to_be_closed = false
