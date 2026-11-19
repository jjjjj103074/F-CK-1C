-- 執行 DcsBridge 的有序武器、選站與雷達操作，發布 Lua 轉接介面可用性。
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

local function publish_adapter_availability()
    adapter_available:set(dispatch_action ~= nil and 1 or 0)
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
end

need_to_be_closed = false
