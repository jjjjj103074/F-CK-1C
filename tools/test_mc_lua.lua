-- Run from the repository root with Lua 5.1: lua tools/test_mc_lua.lua
local checks = 0
local function expect(condition, message)
    checks = checks + 1
    assert(condition, message)
end

local function fixture(execute)
    local values = {}
    function get_param_handle(name)
        return {
            get = function()
                local value = values[name]
                return type(value) == "function" and value() or value or 0
            end,
            set = function(_, value) values[name] = value end,
        }
    end
    dofile("Cockpit/Scripts/generated/CockpitParams.g.lua")
    local transport = dofile("Cockpit/Scripts/Systems/mc_io.lua").new(execute)
    local function batch(first, actions, generation)
        values[cockpit_params.McGeneration] = generation or 2
        values[cockpit_params.McAvailable] = 1
        values[cockpit_params.McActionFirstSequence] = first
        values[cockpit_params.McActionCount] = #actions
        for i, action in ipairs(actions) do
            values[cockpit_params[string.format("McAction%02d", i)]] = action
        end
    end
    return values, transport, batch
end

local delivered = {}
local values, transport, batch = fixture(function(action)
    delivered[#delivered + 1] = action
    return true
end)
batch(1, { 1, 2, 13, 15 })
transport.update()
transport.update()
expect(#delivered == 4, "a repeated window must not repeat successful actions")
expect(values[cockpit_params.McAcknowledgedSequence] == 4, "acknowledge the consumed prefix")
batch(6, { 1 })
transport.update()
expect(#delivered == 4, "a sequence gap must not execute")
batch(5, { 1 }, 3)
transport.update()
expect(#delivered == 4, "an unfinished odd generation must not execute")
batch(5, { 21 })
transport.update()
expect(#delivered == 4, "unknown action values must not execute")
batch(5, { 1 })
values[cockpit_params.McAction01] = function()
    values[cockpit_params.McGeneration] = 4
    return 1
end
transport.update()
expect(#delivered == 4, "a window changed while reading must not execute")

local fail = true
local calls = {}
values, transport, batch = fixture(function(action)
    if action == 2 and fail then return false end
    calls[#calls + 1] = action
    return true
end)
batch(1, { 1, 2, 3 })
transport.update()
expect(values[cockpit_params.McAcknowledgedSequence] == 1, "failed callback must leave its action unacknowledged")
fail = false
transport.update()
expect(
    #calls == 3 and calls[1] == 1 and calls[2] == 2 and calls[3] == 3,
    "retry must retain order and skip the successful prefix"
)

local count = 0
values, transport, batch = fixture(function()
    count = count + 1
    return true
end)
for first = 1, 40, 16 do
    local actions = {}
    for i = first, math.min(first + 15, 40) do
        actions[#actions + 1] = (i % 2 == 0) and 2 or 1
    end
    batch(first, actions)
    transport.update()
end
expect(count == 40, "a backlog larger than one window must drain in order")

-- Exercise the real CMS native adapter against every semantic enum value.
local header = assert(io.open("src/efm/F-CK-1C_EFM/Core/Contracts/MissionActions.h", "r"))
local text = header:read("*a")
header:close()
local enum = assert(text:match("enum class MissionActionKind%s*(%b{})"))
local kinds = {}
for name, value in enum:gmatch("(%w+)%s*=%s*(%d+)") do
    kinds[name] = tonumber(value)
end
local native = {}
values = fixture(function() return true end)
LockOn_Options = { script_path = "Cockpit/Scripts/" }
GetSelf = function() return {} end
local count_sensors = {}
get_base_data = function() return count_sensors end
make_default_activity = function(rate) expect(rate == 0.02, "CMS retains its adapter/CM update period") end
dispatch_action = function(_, command) native[#native + 1] = command end
dofile("Cockpit/Scripts/Systems/cms_system.lua")
post_initialize()
values[cockpit_params.HmcsMasterMode] = 2
local mapping = {
    { "GunOn", "iCommandPlaneFire" },
    { "GunOff", "iCommandPlaneFireOff" },
    { "LaunchPermissionOverride", "iCommandPlaneLaunchPermissionOverride" },
    { "PickleOn", "iCommandPlanePickleOn" },
    { "PickleOff", "iCommandPlanePickleOff" },
    { "EnsureRadarOn", "iCommandPlaneRadarOnOff" },
    { "CycleWeapon", "iCommandPlaneChangeWeapon" },
    { "ModeHelmet", "iCommandPlaneModeHelmet" },
    { "ModeVerticalScan", "iCommandPlaneModeVS" },
    { "ModeBeyondVisualRange", "iCommandPlaneModeBVR" },
    { "ModeFi0", "iCommandPlaneModeFI0" },
    { "ModeCannon", "iCommandPlaneModeCannon" },
    { "LockStart", "iCommandPlaneLockOnStart" },
    { "ChangeLock", "iCommandPlaneChangeLock" },
    { "LockFinish", "iCommandPlaneLockOnFinish" },
    { "ChangeLockUp", "iCommandPlaneChangeLockUp" },
}
values[cockpit_params.McGeneration] = 2
values[cockpit_params.McAvailable] = 1
values[cockpit_params.McActionFirstSequence] = 1
values[cockpit_params.McActionCount] = #mapping
for i, entry in ipairs(mapping) do
    values[cockpit_params[string.format("McAction%02d", i)]] = kinds[entry[1]]
end
update()
for i, entry in ipairs(mapping) do
    expect(native[i] == dcs_commands[entry[2]], "semantic/native mapping: " .. entry[1])
end
update()
expect(#native == 16, "the real CMS adapter must not replay an acknowledged window")
values[cockpit_params.McGeneration] = 4
values[cockpit_params.McActionFirstSequence] = 17
values[cockpit_params.McActionCount] = 4
for i, name in ipairs({ "StartCmsForward", "StartCmsAft", "StartCmsLeft", "AbortCms" }) do
    values[cockpit_params[string.format("McAction%02d", i)]] = kinds[name]
end
update()
expect(values[cockpit_params.McAcknowledgedSequence] == 20, "CM requests are handled by the Lua CM subsystem")
expect(#native == 17 and native[17] == dcs_commands.iCommandPlaneDropChaffOnce, "abort clears queued paired releases")
for _ = 1, 100 do
    update()
end
expect(#native == 17, "an aborted CM program must not release later")
values[cockpit_params.McGeneration] = 6
values[cockpit_params.McActionFirstSequence] = 21
values[cockpit_params.McActionCount] = 1
values[cockpit_params.McAction01] = kinds.EnsureRadarOn
values[cockpit_params.RadarState] = 1
update()
expect(
    values[cockpit_params.McAcknowledgedSequence] == 21 and #native == 17,
    "an ON request must not toggle an already ON radar"
)
-- Exercise the existing count API success path without claiming DCS exposes the fixtures.
local function clear_count_sensors()
    for name in pairs(count_sensors) do
        count_sensors[name] = nil
    end
end
expect(values[cockpit_params.McGunCountAvailable] == 0, "absent gun methods remain unavailable")
expect(values[cockpit_params.McMissileCountAvailable] == 0, "absent missile methods remain unavailable")
for _, name in ipairs({ "getGunAmmoCount", "getCannonsAmmoCount", "getAmmoCount" }) do
    clear_count_sensors()
    count_sensors[name] = function(self)
        expect(self == count_sensors, "gun count methods receive the sensor object")
        return 240
    end
    update()
    expect(
        values[cockpit_params.McGunCountAvailable] == 1 and values[cockpit_params.McGunCount] == 240,
        "preserve existing gun count API: " .. name
    )
end
for _, name in ipairs({
    "getAAMMissileCount",
    "getAAMCount",
    "getAirMissileCount",
    "getAirToAirMissileCount",
    "getMissileCount",
    "getMissilesCount",
    "getWeaponCount",
    "getSelectedWeaponCount",
    "getAmmoCount",
}) do
    clear_count_sensors()
    count_sensors[name] = function(self)
        expect(self == count_sensors, "missile count methods receive the sensor object")
        return 4
    end
    update()
    expect(
        values[cockpit_params.McMissileCountAvailable] == 1 and values[cockpit_params.McMissileCount] == 4,
        "preserve existing missile count API: " .. name
    )
end
clear_count_sensors()
local secondary_count_called = false
count_sensors.getGunAmmoCount = function() return 0 end
count_sensors.getAAMMissileCount = function() return 0 end
count_sensors.getAmmoCount = function()
    secondary_count_called = true
    return 5
end
update()
expect(
    values[cockpit_params.McGunCountAvailable] == 1 and values[cockpit_params.McGunCount] == 0,
    "zero gun count is valid"
)
expect(
    values[cockpit_params.McMissileCountAvailable] == 1 and values[cockpit_params.McMissileCount] == 0,
    "zero missile count is valid"
)
expect(not secondary_count_called, "valid primary zero counts retain precedence over secondary APIs")
clear_count_sensors()
count_sensors.getGunAmmoCount = function() error("count API fixture failure") end
count_sensors.getCannonsAmmoCount = function() return false end
count_sensors.getAmmoCount = function() return 37 end
count_sensors.getAAMMissileCount = function() return 13 end
count_sensors.getAAMCount = function() return 4 end
update()
expect(
    values[cockpit_params.McGunCountAvailable] == 1 and values[cockpit_params.McGunCount] == 37,
    "failed gun APIs try the next existing name"
)
expect(
    values[cockpit_params.McMissileCountAvailable] == 1 and values[cockpit_params.McMissileCount] == 4,
    "invalid missile counts try the next existing name"
)
clear_count_sensors()
count_sensors.getGunAmmoCount = function() return -1 end
count_sensors.getAAMMissileCount = function() return 13 end
update()
expect(
    values[cockpit_params.McGunCountAvailable] == 0 and values[cockpit_params.McGunCount] == 0,
    "invalid gun data clears availability and stale count"
)
expect(
    values[cockpit_params.McMissileCountAvailable] == 0 and values[cockpit_params.McMissileCount] == 0,
    "invalid missile data clears availability and stale count"
)
-- The real station adapter publishes raw samples and executes only MC selection requests.
values = fixture(function() return true end)
local stations = {
    [2] = { count = 2, weapon = { level2 = 4, level3 = 7 }, CLSID = "{AIM-9;literal}" },
    [4] = { count = 3, weapon = { level2 = 1, level3 = 2 }, CLSID = "bomb" },
}
local select_calls = {}
local selection_fails = true
local failed_station = nil
local station_device = {
    listen_event = function() end,
    get_station_info = function(_, station)
        if station == failed_station then error("station API failed") end
        return stations[station]
    end,
    select_station = function(_, station)
        select_calls[#select_calls + 1] = station
        if selection_fails then error("select failed") end
    end,
}
GetSelf = function() return station_device end
make_default_activity = function(rate) expect(rate == 0.05, "station adapter retains 20 Hz API sampling") end
dofile("Cockpit/Scripts/Systems/weapon_system.lua")
post_initialize()
expect(values[cockpit_params.WeaponObservationAvailable] == 0, "unobserved inventory must not be available")
update()
expect(values[cockpit_params.WeaponObservationAvailable] == 1, "a complete raw scan is available")
expect(values[cockpit_params.WeaponObservationRevision] % 2 == 0, "scan commits an even revision")
expect(
    values[cockpit_params.WeaponStation03] == "1;2;4;7;{AIM-9;literal}",
    "publish raw native type fields and literal identifier"
)
expect(values[cockpit_params.WeaponStation05] == "1;3;1;2;bomb", "adapter must not classify or filter non-missiles")
expect(values[cockpit_params.WeaponStation01] == "0;0;0;0;", "nil mount is an absent station sample")
values[cockpit_params.McGeneration] = 2
values[cockpit_params.McAvailable] = 1
values[cockpit_params.WeaponRequestedStation] = 2
values[cockpit_params.WeaponSelectionRequestRevision] = 1
update()
expect(values[cockpit_params.WeaponSelectionAckRevision] == 0, "failed selection must remain unacknowledged")
selection_fails = false
update()
expect(
    values[cockpit_params.WeaponSelectionAckRevision] == 1
        and values[cockpit_params.WeaponObservationSelectedStation] == 2,
    "successful retry acknowledges the actual request"
)
update()
expect(#select_calls == 2, "a repeated request must not repeat successful selection")
values[cockpit_params.McGeneration] = 3
values[cockpit_params.WeaponSelectionRequestRevision] = 2
update()
expect(#select_calls == 2, "odd MC generation must not select")
values[cockpit_params.McGeneration] = 4
values[cockpit_params.WeaponRequestedStation] = function()
    values[cockpit_params.McGeneration] = 6
    return 4
end
update()
expect(#select_calls == 2, "torn MC selection request must not select")
values[cockpit_params.WeaponRequestedStation] = 4
update()
expect(
    #select_calls == 3 and select_calls[3] == 4,
    "the adapter must execute the station MC requests without reclassification"
)
CockpitEvent("WeaponRearmComplete")
update()
expect(
    #select_calls == 3 and values[cockpit_params.WeaponRearmRevision] == 1,
    "rearm publishes an event revision; MC decides reselection"
)
failed_station = 2
update()
expect(
    values[cockpit_params.WeaponObservationAvailable] == 0
        and values[cockpit_params.WeaponObservationInvalidReason] == 5,
    "station API failure invalidates the entire inventory"
)
expect(values[cockpit_params.WeaponStation03] == "0;0;0;0;", "failed scans cannot publish old inventory as current")
failed_station = nil
stations[2].count = 1.5
update()
expect(
    values[cockpit_params.WeaponObservationAvailable] == 0
        and values[cockpit_params.WeaponObservationInvalidReason] == 6,
    "invalid native record must be unavailable"
)
local invalid_records = {
    { count = false, weapon = { level2 = 4, level3 = 7 }, CLSID = "AIM-9" },
    { count = 1, weapon = { level2 = false, level3 = 7 }, CLSID = "AIM-9" },
    { count = 1, weapon = { level2 = 4, level3 = false }, CLSID = "AIM-9" },
    { count = 1, weapon = false, CLSID = "AIM-9" },
    { count = 1, weapon = { level2 = 4, level3 = 7 }, CLSID = false },
}
for _, record in ipairs(invalid_records) do
    stations[2] = record
    update()
    expect(
        values[cockpit_params.WeaponObservationAvailable] == 0
            and values[cockpit_params.WeaponObservationInvalidReason] == 6,
        "false must not become a valid empty inventory field"
    )
end
stations[2] = { count = 0, weapon = { level2 = 4, level3 = 7 }, CLSID = "AIM-9" }
values[cockpit_params.WeaponRequestedStation] = -1
values[cockpit_params.WeaponSelectionRequestRevision] = 3
update()
expect(
    values[cockpit_params.WeaponSelectionAckRevision] == 3
        and values[cockpit_params.WeaponObservationSelectedStation] == -1
        and #select_calls == 3,
    "clearing the last-request belief uses no invented SDK deselect call"
)
-- Radar/HMCS preserve MC mission state while publishing observations and presentation.
values = fixture(function() return true end)
local mission_values = {
    [cockpit_params.Aim9ToneState] = 3,
    [cockpit_params.Aim9SeekerState] = 3,
    [cockpit_params.Aim9SeekerContact] = 1,
    [cockpit_params.Aim9SeekerLock] = 1,
    [cockpit_params.Aim9WeaponActive] = 1,
    [cockpit_params.Aim9SeekerAzimuth] = 0.1,
    [cockpit_params.Aim9SeekerElevation] = -0.2,
    [cockpit_params.Aim9LockRange] = 1200,
}
for name, value in pairs(mission_values) do
    values[name] = value
end
make_default_activity = function(rate) expect(rate == 0.02, "radar adapter retains its update period") end
dofile("Cockpit/Scripts/Systems/radar_state_system.lua")
post_initialize()
update()
expect(
    values[cockpit_params.RadarState] == 1 and values[cockpit_params.RadarPowerState] == 1,
    "preserve the constant-on DCS radar adapter"
)
local native_sensors = {
    getIndicatedAirSpeed = function() return 100 end,
    getBarometricAltitude = function() return 1000 end,
    getMagneticHeading = function() return -0.1 end,
}
get_base_data = function() return native_sensors end
GetSelf = function()
    return { listen_command = function() error("HMCS must not register MC command listeners") end }
end
get_aircraft_draw_argument_value = function(argument) return argument == 510 and 1 or 0 end
make_default_activity = function(rate) expect(rate == 1 / 64, "HMCS native observation remains 64 Hz") end
dofile("Cockpit/Scripts/Systems/hmcs_system.lua")
post_initialize()
update()
expect(
    values[cockpit_params.PressureAltitudeAvailable] == 1 and values[cockpit_params.PressureAltitudeM] == 1000,
    "publish raw barometric altitude"
)
expect(
    values[cockpit_params.MagneticHeadingAvailable] == 1
        and math.abs(values[cockpit_params.MagneticHeadingRad] - (2 * math.pi - 0.1)) < 1e-12,
    "publish normalized magnetic heading"
)
expect(math.abs(values[cockpit_params.HmcsIasKts] - 194.3844) < 1e-8, "retain IAS presentation conversion")
expect(math.abs(values[cockpit_params.HmcsAltitudeFt] - 3280.84) < 1e-8, "retain altitude presentation conversion")
expect(
    values[cockpit_params.HmcsEnabled] == 1 and values[cockpit_params.HmcsDisplayMode] == 1,
    "retain helmet presentation draw arguments"
)
native_sensors.getBarometricAltitude = nil
native_sensors.getMagneticHeading = nil
update()
expect(
    values[cockpit_params.PressureAltitudeAvailable] == 0 and values[cockpit_params.MagneticHeadingAvailable] == 0,
    "missing native observations remain unavailable"
)
for name, value in pairs(mission_values) do
    expect(values[name] == value, "native presentation adapters must not overwrite MC state: " .. name)
end

-- Exercise real HMCS presentation against committed MC facts, without command listeners.
values = fixture(function() error("presentation must not execute weapon actions") end)
dofile("Cockpit/Scripts/Systems/hmcs_system.lua")
values[cockpit_params.HmcsWeaponClass] = 5
values[cockpit_params.HmcsWeaponQuantity] = 77
post_initialize()
expect(values[cockpit_params.HmcsWeaponQuantity] == 77, "unavailable MC must not invent a display quantity")
values[cockpit_params.McGeneration] = 2
values[cockpit_params.McAvailable] = 1
values[cockpit_params.McStoresGunCount] = 523
values[cockpit_params.Aim9MissileCount] = 4
update()
expect(values[cockpit_params.HmcsWeaponClass] == 5 and values[cockpit_params.HmcsWeaponQuantity] == 523,
    "initial NAV presentation uses authoritative MC gun quantity")
values[cockpit_params.McNavigationPressSequence] = 1
values[cockpit_params.McMissileUncagePressSequence] = 2
-- Both uncage edges have already been consumed by MC; the last press is retained.
values[cockpit_params.Aim9UncageHeld] = 0
update()
expect(values[cockpit_params.HmcsWeaponClass] == 1 and values[cockpit_params.HmcsWeaponQuantity] == 4,
    "a completed short NAV uncage pulse retains the old AAM preference")
values[cockpit_params.McTriggerSecondStageHeld] = 1
update()
expect(values[cockpit_params.HmcsWeaponClass] == 5, "a rejected raw trigger still affects presentation")
values[cockpit_params.McTriggerSecondStageHeld] = 0
update()
expect(values[cockpit_params.HmcsWeaponClass] == 1, "trigger release restores the retained AAM preference")
values[cockpit_params.McNavigationPressSequence] = 3
update()
expect(values[cockpit_params.HmcsWeaponClass] == 5, "a later NAV press restores gun preference")
values[cockpit_params.McWeaponReleasePressSequence] = 4
update()
expect(values[cockpit_params.HmcsWeaponClass] == 1, "a rejected release press retains its old display effect")
values[cockpit_params.McMasterArmPressSequence] = 5
values[cockpit_params.HmcsFireControlMode] = 1
update()
expect(values[cockpit_params.HmcsWeaponClass] == 1, "AA mode has precedence over gun preference")
values[cockpit_params.HmcsFireControlMode] = 0
values[cockpit_params.McStoresGunCount] = 0
update()
expect(values[cockpit_params.HmcsWeaponClass] == 5 and values[cockpit_params.HmcsWeaponQuantity] == 0,
    "valid zero gun quantity must not fall back to an estimate")
values[cockpit_params.Aim9SeekerLock] = 1
values[cockpit_params.Aim9MissileCount] = 0
update()
expect(values[cockpit_params.HmcsWeaponClass] == 1 and values[cockpit_params.HmcsWeaponQuantity] == 0,
    "lock indication has precedence while zero missile quantity remains authoritative")
values[cockpit_params.Aim9SeekerLock] = 0
values[cockpit_params.HmcsWeaponClass] = 1
values[cockpit_params.HmcsWeaponQuantity] = 77
values[cockpit_params.McGeneration] = 3
update()
expect(values[cockpit_params.HmcsWeaponQuantity] == 77, "an odd MC generation must not change presentation")
values[cockpit_params.McGeneration] = 4
values[cockpit_params.McStoresGunCount] = function()
    values[cockpit_params.McGeneration] = 6
    return 2
end
update()
expect(values[cockpit_params.HmcsWeaponClass] == 1 and values[cockpit_params.HmcsWeaponQuantity] == 77,
    "a generation changed during reading must not publish mixed presentation")
values[cockpit_params.McStoresGunCount] = 523
values[cockpit_params.McAvailable] = 0
update()
expect(values[cockpit_params.HmcsWeaponQuantity] == 77, "unavailable MC retains the last complete presentation")
values[cockpit_params.McAvailable] = 1

-- Replay the pre-refactor DisplayManagement rules as a reference, with consumer
-- frames deliberately skipped. Inputs cover raw rejected presses and FIFO pulses.
local legacy_preferred, legacy_trigger = 5, false
local input_sequence = 0
local seed = 314159
local function random(maximum)
    seed = (seed * 48271) % 2147483647
    return seed % maximum
end
local presses = {
    cockpit_params.McMasterArmPressSequence, cockpit_params.McNavigationPressSequence,
    cockpit_params.McDogfightPressSequence, cockpit_params.McMissileOverridePressSequence,
    cockpit_params.McMissileUncagePressSequence, cockpit_params.McWeaponReleasePressSequence,
}
for _, parameter in ipairs(presses) do values[parameter] = 0 end
for frame = 1, 8192 do
    for _ = 1, random(7) do
        input_sequence = input_sequence + 1
        local command, pressed = random(9) + 1, random(2) == 1
        if command == 7 then
            legacy_trigger = pressed
        elseif command <= 6 and pressed then
            values[presses[command]] = input_sequence
            legacy_preferred = command <= 2 and 5 or 1
        end
    end
    local mode = random(3)
    local uncage, contact, locked = random(5) == 0, random(7) == 0, random(7) == 0
    local readiness = random(5) == 0 and 1 or 0
    local firing = random(7) == 0
    local gun, missiles = random(524), random(7)
    values[cockpit_params.McGeneration] = frame * 2 + 6
    values[cockpit_params.HmcsFireControlMode] = mode
    values[cockpit_params.Aim9UncageHeld] = uncage and 1 or 0
    values[cockpit_params.Aim9SeekerContact] = contact and 1 or 0
    values[cockpit_params.Aim9SeekerLock] = locked and 1 or 0
    values[cockpit_params.Aim9MissileStatus] = readiness
    values[cockpit_params.HmcsGunFiring] = firing and 1 or 0
    values[cockpit_params.McTriggerSecondStageHeld] = legacy_trigger and 1 or 0
    values[cockpit_params.McStoresGunCount] = gun
    values[cockpit_params.Aim9MissileCount] = missiles
    local expected_class
    if mode ~= 0 or uncage or contact or locked or readiness ~= 0 then
        expected_class = 1
    elseif firing or legacy_trigger or legacy_preferred == 5 then
        expected_class = 5
    else
        expected_class = legacy_preferred
    end
    if frame % 3 == 0 or frame == 8192 then
        update()
        expect(values[cockpit_params.HmcsWeaponClass] == expected_class,
            "legacy display class replay frame " .. frame)
        expect(values[cockpit_params.HmcsWeaponQuantity] == (expected_class == 5 and gun or missiles),
            "legacy display quantity replay frame " .. frame)
    end
end

-- Execute both real DCS input profiles: every MC/HOTAS binding reaches EFM with both edges.
for name, value in pairs(dcs_commands) do
    _G[name] = value
end
external_profile = function()
    return {
        keyCommands = {
            {
                down = iCommandPlaneFire,
                up = iCommandPlaneFireOff,
                combos = { { key = "Space" } },
                name = "Inherited Gun",
            },
            {
                down = iCommandPlanePickleOn,
                up = iCommandPlanePickleOff,
                combos = { { key = "InheritedRelease" } },
                name = "Inherited Release",
            },
            {
                pressed = iCommandPlaneFire,
                up = iCommandPlaneFireOff,
                combos = { { key = "HeldGun" } },
                name = "Inherited Held Gun",
            },
            {
                down = 999001,
                up = 999002,
                value_down = 0.7,
                value_up = 0.2,
                cockpit_device_id = 777,
                name = "Unrelated",
            },
        },
        axisCommands = {},
    }
end
defaultDeviceAssignmentFor = function(role)
    return role == "weapon_release_button" and { { key = "DeviceRelease" } } or {}
end
_ = function(text) return text end
join = function(destination, entries)
    for _, entry in ipairs(entries) do
        destination[#destination + 1] = entry
    end
end
local mission_commands = {
    "TriggerFirstStage",
    "TriggerSecondStage",
    "MasterArmOn",
    "MasterArmOff",
    "MasterArmSim",
    "NavMode",
    "DogfightSwitch",
    "MissileOverride",
    "MissileUncage",
    "WeaponRelease",
    "TMSUp",
    "TMSDown",
    "TMSLeft",
    "TMSRight",
    "CMSForward",
    "CMSAft",
    "CMSLeft",
    "CMSRight",
    "CMSPress",
}
for _, profile in ipairs({ "keyboard", "joystick" }) do
    folder = "Input/F-CK-1C/" .. profile .. "/"
    local bindings = dofile(folder .. "default.lua")
    for _, name in ipairs(mission_commands) do
        local matches = 0
        for _, binding in ipairs(bindings.keyCommands) do
            if binding.down == device_commands[name] then
                matches = matches + 1
                expect(
                    binding.cockpit_device_id == nil
                        and binding.up == binding.down
                        and binding.value_down == 1
                        and binding.value_up == 0,
                    profile .. ": MC command must reach EFM with press/release: " .. name
                )
            end
        end
        expect(matches == 1, profile .. ": expected one MC command after merging aliases: " .. name)
    end
    local forbidden = {
        [dcs_commands.iCommandPlaneFire] = true,
        [dcs_commands.iCommandPlaneFireOff] = true,
        [dcs_commands.iCommandPlanePickleOn] = true,
        [dcs_commands.iCommandPlanePickleOff] = true,
    }
    local inherited_release, held_gun, unrelated, space_gun = 0, 0, 0, 0
    local joystick_gun, joystick_release, keyboard_release, device_release = 0, 0, 0, 0
    for _, binding in ipairs(bindings.keyCommands) do
        for _, edge in ipairs({ "down", "up", "pressed" }) do
            expect(not forbidden[binding[edge]], profile .. ": firing/release input must not bypass MC: " .. edge)
        end
        if binding.name == "Unrelated" then
            unrelated = unrelated + 1
            expect(
                binding.down == 999001
                    and binding.up == 999002
                    and binding.value_down == 0.7
                    and binding.value_up == 0.2
                    and binding.cockpit_device_id == 777,
                profile .. ": unrelated bindings remain unchanged"
            )
        end
        for _, combo in ipairs(binding.combos or {}) do
            if combo.key == "DeviceRelease" then
                device_release = device_release + 1
                expect(binding.down == device_commands.WeaponRelease, "device-assigned release combo reaches MC")
            elseif combo.key == "InheritedRelease" then
                inherited_release = inherited_release + 1
                expect(binding.down == device_commands.WeaponRelease, "inherited release combo reaches MC")
            elseif combo.key == "HeldGun" then
                held_gun = held_gun + 1
                expect(
                    binding.down == device_commands.TriggerSecondStage and binding.pressed == nil,
                    "inherited held fire becomes one MC press and release"
                )
            elseif
                combo.key == "Space"
                and not combo.reformers
                and binding.down == device_commands.TriggerSecondStage
            then
                space_gun = space_gun + 1
            end
            if profile == "joystick" and combo.key == "JOY_BTN1" then
                joystick_gun = joystick_gun + 1
                expect(binding.down == device_commands.TriggerSecondStage, "JOY_BTN1 must reach MC gun control")
            elseif profile == "joystick" and combo.key == "JOY_BTN2" then
                joystick_release = joystick_release + 1
                expect(binding.down == device_commands.WeaponRelease, "JOY_BTN2 must reach MC weapon release")
            elseif
                profile == "keyboard"
                and combo.key == "Space"
                and combo.reformers
                and #combo.reformers == 1
                and combo.reformers[1] == "RAlt"
            then
                keyboard_release = keyboard_release + 1
                expect(binding.down == device_commands.WeaponRelease, "RAlt+Space must reach MC weapon release")
            end
        end
    end
    expect(
        inherited_release == 1 and held_gun == 1 and unrelated == 1,
        profile .. ": preserve inherited keys and unrelated bindings while merging aliases"
    )
    expect(space_gun == 1, profile .. ": merge duplicate plain Space combos into one gun action")
    if profile == "joystick" then
        expect(joystick_gun == 1 and joystick_release == 1, "preserve default joystick gun/release buttons")
    else
        expect(keyboard_release == 1 and device_release == 1, "preserve keyboard and device-assigned release combos")
    end
end
-- Explicit release-only inputs retain their zero-valued MC edge.
local release_only = {
    { down = dcs_commands.iCommandPlaneFireOff, cockpit_device_id = 999 },
    { down = dcs_commands.iCommandPlanePickleOff, cockpit_device_id = 999 },
}
dofile("Input/F-CK-1C/mission_weapon_input.lua")(release_only, device_commands, dcs_commands)
expect(
    release_only[1].down == device_commands.TriggerSecondStage and release_only[1].value_down == 0,
    "a native gun stop must remain a release request"
)
expect(
    release_only[2].down == device_commands.WeaponRelease and release_only[2].value_down == 0,
    "a native pickle stop must remain a release request"
)
expect(
    release_only[1].cockpit_device_id == nil and release_only[2].cockpit_device_id == nil,
    "release-only input must reach EFM"
)
print(string.format("MC Lua adapter: %d checks, 0 failures", checks))
