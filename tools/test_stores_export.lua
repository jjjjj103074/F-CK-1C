-- 執行實際 Export、座艙接收及 HMCS Interface，重播原生彈量與資料中斷。
local checks = 0
local function expect(condition, message)
    checks = checks + 1
    assert(condition, message)
end
local function environment()
    local env = setmetatable({}, { __index = _G })
    env._G = env
    env.loadfile = function(path)
        path = path:gsub("^fixture/Mods/aircraft/F%-CK%-1C/", "")
        local chunk, err = loadfile(path)
        if not chunk then return nil, err end
        setfenv(chunk, env)
        return chunk
    end
    env.dofile = function(path)
        return assert(env.loadfile(path))()
    end
    return env
end
local function fixture()
    local state = { time = 0, count = 523, aircraft = "F-CK-1C", calls = 0, frames = 0, errors = 0 }
    local values = {}
    local cockpit = environment()
    cockpit.LockOn_Options = { script_path = "Cockpit/Scripts/" }
    cockpit.get_param_handle = function(name)
        return {
            get = function()
                return values[name] or 0
            end,
            set = function(_, value)
                values[name] = value
            end,
        }
    end
    cockpit.get_model_time = function()
        return state.time
    end
    cockpit.get_base_data = function()
        return {}
    end
    cockpit.GetSelf = function()
        return { listen_command = function() end }
    end
    cockpit.make_default_activity = function() end
    cockpit.dispatch_action = function() end
    cockpit.dofile("Cockpit/Scripts/Systems/cms_system.lua")
    cockpit.post_initialize()
    local export = environment()
    export.lfs = {
        writedir = function()
            return "fixture/"
        end,
    }
    export.log = {
        ERROR = 1,
        write = function()
            state.errors = state.errors + 1
        end,
    }
    export.LoGetModelTime = function()
        return state.time
    end
    export.LoGetSelfData = function()
        return { Name = state.aircraft }
    end
    export.LoGetPayloadInfo = function()
        if state.payload_error then error("fixture: unavailable payload") end
        return { Cannon = { shells = state.count } }
    end
    export.GetDevice = function(id)
        expect(id == 3, "Export uses the registered CMS device")
        if state.missing_device then return nil end
        return {
            performClickableAction = function(_, command, value, animated)
                expect(command == cockpit.device_commands.ReceiveGunAmmunition, "generated command connects both environments")
                expect(animated == false, "data reception uses the device command Interface")
                state.calls = state.calls + 1
                cockpit.SetCommand(command, value)
            end,
        }
    end
    export.LuaExportAfterNextFrame = function()
        state.frames = state.frames + 1
        if state.previous_error then error("fixture: previous export error") end
    end
    export.dofile("Export/StoresExport.lua")
    export.LuaExportStart()
    return state, values, cockpit, export
end

local protocol = dofile("Cockpit/Scripts/Stores/GunAmmunition.lua")
for _, count in ipairs({ 0, 1, 216, 261, 523, 9999 }) do
    expect(protocol.decode(protocol.encode(count)) == count, "native integer roundtrip")
    expect(protocol.decode(protocol.encode(count) + 0.00000001) == count, "roundtrip tolerates command float precision")
end
for _, value in ipairs({ -1, 0.5, 10000, math.huge, -math.huge, 0 / 0, "523", false }) do
    expect(protocol.encode(value) == -1, "invalid native quantity has an unavailable encoding")
end
for _, value in ipairs({ -1, 1, math.huge, -math.huge, 0 / 0, "0.0523", false }) do
    expect(protocol.decode(value) == nil, "invalid received value stays unavailable")
end

local state, values, cockpit, export = fixture()
local params = cockpit.cockpit_params
expect(values[params.McGunCountAvailable] == 0, "initial gun count is unavailable")
export.LuaExportAfterNextFrame()
expect(values[params.McGunCount] == 523 and values[params.McGunCountAvailable] == 1, "Export supplies native full ammunition")
expect(values[params.McGunCountRevision] == 2, "first complete sample is published")
export.LuaExportAfterNextFrame()
expect(state.calls == 1 and state.frames == 2, "read rate preserves the previous per-frame callback")
state.time = 0.11
state.count = 261
export.LuaExportAfterNextFrame()
expect(values[params.McGunCount] == 261, "half ammunition replaces the observed count")
state.time = 0.22
state.count = 216
export.LuaExportAfterNextFrame()
expect(values[params.McGunCount] == 216, "native shot decrement reaches the cockpit")
state.time = 0.33
state.count = 0
export.LuaExportAfterNextFrame()
expect(values[params.McGunCount] == 0 and values[params.McGunCountAvailable] == 1, "zero native rounds remain valid")
state.time = 0.44
state.count = 523
export.LuaExportAfterNextFrame()
expect(values[params.McGunCount] == 523, "an ammunition increase replaces the observed count")
state.time = 0.55
state.payload_error = true
export.LuaExportAfterNextFrame()
expect(values[params.McGunCount] == 523 and values[params.McGunCountAvailable] == 0, "failed payload preserves the last number and clears availability")
state.payload_error = false
state.time = 0.66
export.LuaExportAfterNextFrame()
expect(values[params.McGunCountAvailable] == 1, "a new sample recovers availability")
state.time = 1.17
cockpit.update()
expect(values[params.McGunCountAvailable] == 0 and values[params.McGunCount] == 523, "missing heartbeats expire without inventing zero rounds")
state.time = 1.28
state.count = 216
export.LuaExportAfterNextFrame()
expect(values[params.McGunCountAvailable] == 1 and values[params.McGunCount] == 216, "resumed export updates the current observation")
local before = state.calls
state.aircraft = "OtherAircraft"
state.time = 1.39
export.LuaExportAfterNextFrame()
expect(state.calls == before, "another aircraft receives no F-CK-1C data command")
state.aircraft = "F-CK-1C"
state.previous_error = true
state.time = 1.5
export.LuaExportAfterNextFrame()
expect(state.calls == before + 1 and state.errors == 1, "an existing export failure preserves this route")
state.time = 1.61
export.LuaExportAfterNextFrame()
expect(state.errors == 1, "repeated callback errors are bounded")
local callback = export.LuaExportAfterNextFrame
export.dofile("Export/StoresExport.lua")
expect(export.LuaExportAfterNextFrame == callback, "loading twice preserves one callback chain")
state.previous_error = false
state.missing_device = true
state.time = 1.72
before = state.calls
export.LuaExportAfterNextFrame()
expect(state.calls == before, "device creation can finish after Export starts")
state.missing_device = false
state.time = 0
state.count = 261
cockpit.post_initialize()
export.LuaExportStart()
export.LuaExportAfterNextFrame()
expect(values[params.McGunCount] == 261 and values[params.McGunCountRevision] == 2, "a new flight starts with its native ammunition")

-- HMCS 依 MC 提供的有效性選擇數字或未知指示。
cockpit.dofile("Cockpit/Scripts/Systems/hmcs_system.lua")
values[params.McGeneration] = 2
values[params.McAvailable] = 1
values[params.McStoresGunCount] = 216
values[params.McStoresGunCountAvailable] = 1
cockpit.update()
expect(values[params.HmcsWeaponClass] == 5 and values[params.HmcsWeaponQuantity] == 216, "HMCS receives the latest MC gun count")
expect(values[params.HmcsWeaponQuantityAvailable] == 1, "HMCS marks a native quantity available")
values[params.McStoresGunCountAvailable] = 0
cockpit.update()
expect(values[params.HmcsWeaponQuantity] == 216 and values[params.HmcsWeaponQuantityAvailable] == 0, "HMCS retains the last quantity and marks it unavailable")
values[params.McStoresGunCount] = 0
values[params.McStoresGunCountAvailable] = 1
cockpit.update()
expect(values[params.HmcsWeaponQuantity] == 0 and values[params.HmcsWeaponQuantityAvailable] == 1, "HMCS distinguishes zero rounds from unavailable data")

-- 在 Add 註冊時保存畫面條件，驗證機砲數字與未知文字的顯示選擇。
local registered = {}
local function copy(value)
    if type(value) ~= "table" then return value end
    local result = {}
    for key, item in pairs(value) do
        result[key] = copy(item)
    end
    return result
end
local real_dofile = cockpit.dofile
cockpit.LockOn_Options.common_script_path = "fixture-common/"
cockpit.dofile = function(path)
    if path == "fixture-common/elements_defs.lua" then return end
    return real_dofile(path)
end
cockpit.SetCustomScale = function() end
cockpit.MakeMaterial = function()
    return "fixture-material"
end
cockpit.CreateElement = function(kind)
    return { kind = kind }
end
cockpit.ScreenType = { SCREENSPACE_TRUE = 1 }
cockpit.Add = function(element)
    registered[element.name] = copy(element)
end
cockpit.dofile("Cockpit/Scripts/HMCS/HMCS_page.lua")
local function visible(element)
    for _, controller in ipairs(element.controllers or {}) do
        if controller[1] == "parameter_in_range" then
            local value = values[element.element_params[controller[2] + 1]] or 0
            if value < controller[3] or value > controller[4] then return false end
        end
    end
    return true
end
local numeric, unknown = registered.hmcs_weapon_gun, registered.hmcs_weapon_gun_unknown
expect(numeric ~= nil and unknown ~= nil, "both gun representations are registered")
values[params.HmcsWeaponClass] = 5
values[params.HmcsWeaponQuantityAvailable] = 1
values[params.HmcsWeaponQuantity] = 216
expect(visible(numeric) and not visible(unknown), "valid rounds select only the numeric indicator")
values[params.HmcsWeaponQuantity] = 0
expect(visible(numeric) and not visible(unknown), "valid zero selects the numeric indicator")
values[params.HmcsWeaponQuantityAvailable] = 0
expect(not visible(numeric) and visible(unknown), "unavailable rounds select only GUN ---")
values[params.HmcsWeaponClass] = 1
expect(not visible(numeric) and not visible(unknown), "AA presentation hides both gun representations")

-- 重播 2026-11-02 實機測試的三組原生彈量變化。
local native_traces = {
    { 523, 518, 508, 497, 487, 482, 472, 462, 451, 441, 437, 427, 416, 406, 395, 383, 382, 371, 360, 350, 340, 329, 323, 312, 301, 290, 278, 268, 258, 248, 237, 227, 216, 206, 195, 185, 174, 164, 153, 142, 132, 122, 110, 98, 86, 74, 62, 51, 39, 27, 15, 3, 0 },
    { 523 },
    { 261, 257, 247, 236, 225, 216 },
}
for _, trace in ipairs(native_traces) do
    local replay, recorded, receiver, sender = fixture()
    for i, count in ipairs(trace) do
        replay.time = (i - 1) * 0.11
        replay.count = count
        sender.LuaExportAfterNextFrame()
        expect(recorded[receiver.cockpit_params.McGunCount] == count, "captured native count reaches the receiver")
        expect(recorded[receiver.cockpit_params.McGunCountAvailable] == 1, "captured native sample stays available")
    end
end

print("Stores Export: " .. checks .. " checks, 0 failures")
