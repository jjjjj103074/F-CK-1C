-- DCS station adapter. Classification, counts and selection policy belong to MC.
local WeaponSystem = GetSelf()
make_default_activity(0.05)
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")

local NUM_STATIONS = 7
local NO_STATION = -1
local MAX_REVISION = 9007199254740991
local OBS_INVALID_NONE = 0
local OBS_INVALID_NOT_PROVIDED = 1
local OBS_INVALID_STATION_API_ERROR = 5
local OBS_INVALID_PAYLOAD = 6
local observation_available = get_param_handle(cockpit_params.WeaponObservationAvailable)
local observation_revision = get_param_handle(cockpit_params.WeaponObservationRevision)
local observation_invalid_reason = get_param_handle(cockpit_params.WeaponObservationInvalidReason)
local observation_selected_station = get_param_handle(cockpit_params.WeaponObservationSelectedStation)
local observation_scanned_count = get_param_handle(cockpit_params.WeaponObservationScannedStationCount)
local observation_ack = get_param_handle(cockpit_params.WeaponSelectionAckRevision)
local observation_rearm = get_param_handle(cockpit_params.WeaponRearmRevision)
local station_01 = get_param_handle(cockpit_params.WeaponStation01)
local station_02 = get_param_handle(cockpit_params.WeaponStation02)
local station_03 = get_param_handle(cockpit_params.WeaponStation03)
local station_04 = get_param_handle(cockpit_params.WeaponStation04)
local station_05 = get_param_handle(cockpit_params.WeaponStation05)
local station_06 = get_param_handle(cockpit_params.WeaponStation06)
local station_07 = get_param_handle(cockpit_params.WeaponStation07)
local mc_generation = get_param_handle(cockpit_params.McGeneration)
local mc_available = get_param_handle(cockpit_params.McAvailable)
local requested_station = get_param_handle(cockpit_params.WeaponRequestedStation)
local request_revision = get_param_handle(cockpit_params.WeaponSelectionRequestRevision)
local revision = 0
local acknowledged_request = 0
local selected_station = NO_STATION
local rearm_revision = 0

local function integer(value, minimum, maximum)
    return type(value) == "number" and value >= minimum and value <= maximum and value == math.floor(value)
end

local function apply_selection_request()
    local generation = mc_generation:get()
    if not integer(generation, 2, MAX_REVISION) or generation % 2 ~= 0 or mc_available:get() ~= 1 then return end
    local station = requested_station:get()
    local requested = request_revision:get()
    if mc_generation:get() ~= generation then return end
    if not integer(station, NO_STATION, NUM_STATIONS - 1) or not integer(requested, 0, MAX_REVISION) then return end
    if requested <= acknowledged_request then return end
    if station ~= NO_STATION then
        local ok, result = pcall(WeaponSystem.select_station, WeaponSystem, station)
        if not ok or result == false then return end
    end
    -- NO_STATION clears our last-request belief; the SDK has no deselect call here.
    selected_station = station
    acknowledged_request = requested
end

local function raw_station_record(info)
    if info == nil then return "0;0;0;0;" end
    if type(info) ~= "table" then return nil end
    local weapon = info.weapon
    if weapon == nil then weapon = {} end
    if type(weapon) ~= "table" then return nil end
    local count = info.count
    if count == nil then count = 0 end
    local level2 = weapon.level2
    if level2 == nil then level2 = 0 end
    local level3 = weapon.level3
    if level3 == nil then level3 = 0 end
    if not integer(count, 0, 9999) or not integer(level2, 0, 2147483647) or not integer(level3, 0, 2147483647) then
        return nil
    end
    local identifier = info.CLSID
    if identifier == nil then identifier = "" end
    if type(identifier) ~= "string" then return nil end
    if identifier:find("%z") then return nil end
    -- The first four fields are bounded numbers; the remainder is a literal identifier.
    local record = string.format("1;%d;%d;%d;%s", count, level2, level3, identifier)
    if #record >= 511 then return nil end
    return record
end

local function publish_station_observation(records, invalid_reason)
    assert(revision <= MAX_REVISION - 2, "station observation revision exhausted")
    revision = revision + 1
    observation_revision:set(revision)
    observation_available:set(invalid_reason == OBS_INVALID_NONE and 1 or 0)
    observation_invalid_reason:set(invalid_reason)
    observation_selected_station:set(selected_station)
    observation_scanned_count:set(NUM_STATIONS)
    observation_ack:set(acknowledged_request)
    observation_rearm:set(rearm_revision)
    station_01:set(records[1] or "0;0;0;0;")
    station_02:set(records[2] or "0;0;0;0;")
    station_03:set(records[3] or "0;0;0;0;")
    station_04:set(records[4] or "0;0;0;0;")
    station_05:set(records[5] or "0;0;0;0;")
    station_06:set(records[6] or "0;0;0;0;")
    station_07:set(records[7] or "0;0;0;0;")
    revision = revision + 1
    observation_revision:set(revision)
end

local function scan_stations()
    local records = {}
    for station = 0, NUM_STATIONS - 1 do
        local ok, info = pcall(WeaponSystem.get_station_info, WeaponSystem, station)
        if not ok then
            publish_station_observation({}, OBS_INVALID_STATION_API_ERROR)
            return
        end
        local record = raw_station_record(info)
        if not record then
            publish_station_observation({}, OBS_INVALID_PAYLOAD)
            return
        end
        records[station + 1] = record
    end
    publish_station_observation(records, OBS_INVALID_NONE)
end

function post_initialize()
    publish_station_observation({}, OBS_INVALID_NOT_PROVIDED)
end

WeaponSystem:listen_event("WeaponRearmComplete")
WeaponSystem:listen_event("UnlimitedWeaponStationRestore")
function CockpitEvent(event)
    if event == "WeaponRearmComplete" or event == "UnlimitedWeaponStationRestore" then
        assert(rearm_revision < MAX_REVISION, "rearm revision exhausted")
        rearm_revision = rearm_revision + 1
    end
end

function update()
    apply_selection_request()
    scan_stations()
end
need_to_be_closed = false
