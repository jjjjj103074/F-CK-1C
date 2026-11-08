-- DCS station adapter. Classification, counts and selection policy belong to MC.
local WeaponSystem = GetSelf()
make_default_activity(0.05)
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")

local station_definitions = dofile(LockOn_Options.script_path .. "Stores/StationDefinitions.lua")
local diagnose = dofile(LockOn_Options.script_path .. "Stores/ObservationDiagnostics.lua").new("STORES")
local NUM_STATIONS = #station_definitions
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
local station_08 = get_param_handle(cockpit_params.WeaponStation08)
local station_09 = get_param_handle(cockpit_params.WeaponStation09)
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
    -- NO_STATION 清除轉接層保存的最近選站紀錄。
    selected_station = station
    acknowledged_request = requested
end

local function empty_record(name)
    return "0;0;0;0;0;0;" .. name .. ";"
end

local function raw_station_record(info, name)
    if info == nil then return empty_record(name) end
    if type(info) ~= "table" then return nil end
    local weapon = info.weapon
    if weapon == nil then weapon = {} end
    if type(weapon) ~= "table" then return nil end
    local count = info.count
    if count == nil then count = 0 end
    if not integer(count, 0, 9999) then return nil end
    local levels = {}
    for i = 1, 4 do
        local value = weapon["level" .. i]
        if value == nil then value = 0 end
        if not integer(value, 0, 2147483647) then return nil end
        levels[i] = value
    end
    local identifier = info.CLSID
    if identifier == nil then identifier = "" end
    if type(identifier) ~= "string" or identifier:find("%z") then return nil end
    -- 固定欄位後保留完整識別碼，識別碼內的分號原樣傳遞。
    local record = string.format("1;%d;%d;%d;%d;%d;%s;%s", count, levels[1], levels[2], levels[3], levels[4], name, identifier)
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
    station_01:set(records[1] or empty_record(station_definitions[1].name))
    station_02:set(records[2] or empty_record(station_definitions[2].name))
    station_03:set(records[3] or empty_record(station_definitions[3].name))
    station_04:set(records[4] or empty_record(station_definitions[4].name))
    station_05:set(records[5] or empty_record(station_definitions[5].name))
    station_06:set(records[6] or empty_record(station_definitions[6].name))
    station_07:set(records[7] or empty_record(station_definitions[7].name))
    station_08:set(records[8] or empty_record(station_definitions[8].name))
    station_09:set(records[9] or empty_record(station_definitions[9].name))
    revision = revision + 1
    observation_revision:set(revision)
end

local function scan_stations()
    local records = {}
    for station = 0, NUM_STATIONS - 1 do
        local ok, info = pcall(WeaponSystem.get_station_info, WeaponSystem, station)
        diagnose("station=" .. station .. " name=" .. station_definitions[station + 1].name, {
            call_ok = ok,
            result_type = type(info),
            result = info,
        }, "batch=" .. (revision + 2) .. " rearm=" .. rearm_revision)
        if not ok then
            publish_station_observation({}, OBS_INVALID_STATION_API_ERROR)
            return
        end
        local record = raw_station_record(info, station_definitions[station + 1].name)
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
        diagnose("rearm", { event = event, revision = rearm_revision })
    end
end

function update()
    apply_selection_request()
    scan_stations()
end
need_to_be_closed = false
