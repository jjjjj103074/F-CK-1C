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
get_base_data = function() return {} end
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
print(string.format("MC Lua adapter: %d checks, 0 failures", checks))
