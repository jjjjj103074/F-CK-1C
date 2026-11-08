-- 驗證原始彈量採集、不同執行環境及本機識別的隔離。
local checks = 0
local function expect(value, message)
    checks = checks + 1
    assert(value, message)
end
local function read(path)
    local file = assert(io.open(path, "rb"))
    local text = file:read("*a")
    file:close()
    return text
end
local core_path = "Cockpit/Scripts/Stores/AmmoProbe.lua"
local runtime = "tools/dcs-ammo-probe/runtime/"
local Core = dofile(core_path)
local records = {}
local logger = Core.logger("test", function(line)
    records[#records + 1] = line
end)
logger.record("zero", { value = 0, available = false, text = 'quote"\\\n\000' }, 0)
expect(records[1]:find('"value":0', 1, true), "zero ammunition is recorded")
expect(records[1]:find('"available":false', 1, true), "false remains distinct from nil")
expect(not records[1]:find("\n", 1, true), "JSON records occupy one line")
logger.record("zero", { value = 0, available = false, text = 'quote"\\\n\000' }, 1)
expect(#records == 1, "unchanged values wait for the heartbeat")
logger.record("zero", { value = 0, available = false, text = 'quote"\\\n\000' }, 5)
expect(#records == 2, "unchanged source heartbeat is recorded")
local recursive = {}
recursive.self = recursive
logger.record("recursive", recursive, 0)
expect(records[#records]:find("<cycle>", 1, true), "recursive source values are bounded")
logger.record("nonfinite", { value = 0 / 0, infinity = math.huge }, 0)
expect(Core.call({}, "getAmmoCount", "colon").status == "missing", "missing is a separate result")
expect(Core.call({
    getAmmoCount = function()
        error("read failure")
    end,
}, "getAmmoCount", "colon").status == "error", "failed reads are recorded")
expect(Core.call({
    getAmmoCount = function()
        return 0
    end,
}, "getAmmoCount", "colon").value == 0, "native zero passes through")
local calls = 0
local device = newproxy(true)
getmetatable(device).__index = {
    get_selected_gun_ammo_count = function(self)
        expect(self == device, "native methods receive their owning device")
        calls = calls + 1
        return 23
    end,
    launch_station = function()
        error("diagnostic invoked a control method")
    end,
}
local methods = Core.methods(device)
expect(methods.names[1] == "get_selected_gun_ammo_count", "userdata method inventory follows __index")
Core.probe_getters(logger, "weapon", device, { "colon" }, 1)
expect(calls == 1, "read-only candidate executes once")

local files = {
    ["/fake-dcs/Scripts/FCK1CAmmoProbe/AmmoProbe.lua"] = core_path,
    ["/fake-dcs/Scripts/FCK1CAmmoProbe/host.lua"] = runtime .. "host.lua",
    ["/fake-dcs/Scripts/FCK1CAmmoProbe/mission-query.lua"] = runtime .. "mission-query.lua",
}
local function fixture()
    local env = setmetatable({}, { __index = _G })
    env._G = env
    env.lines, env.file_lines, env.state = {}, {}, { time = 0, shells = 523, aircraft = "F-CK-1C", id = 101 }
    env.log = {
        INFO = 3,
        write = function(_, _, line)
            env.lines[#env.lines + 1] = line
        end,
    }
    env.lfs = {
        writedir = function()
            return "/fake-dcs/"
        end,
        mkdir = function()
            return true
        end,
    }
    env.io = {
        open = function(path, mode)
            if mode == "rb" then
                expect(files[path] ~= nil, "runtime reads only its known query modules")
                return io.open(files[path], mode)
            end
            return {
                write = function(_, line)
                    env.file_lines[#env.file_lines + 1] = line
                    return true
                end,
                flush = function()
                    return true
                end,
                close = function()
                    return true
                end,
            }
        end,
    }
    env.get_model_time = function()
        return env.state.time
    end
    env.dofile = function(path)
        local chunk = assert(loadfile(files[path] or path))
        setfenv(chunk, env)
        return chunk()
    end
    env.LoGetSelfData = function()
        return { Name = env.state.aircraft }
    end
    env.LoGetPlayerPlaneId = function()
        return env.state.id
    end
    env.LoGetPilotName = function()
        return "Local Pilot"
    end
    env.LoGetPayloadInfo = function()
        return { Cannon = { shells = env.state.shells }, Stations = {} }
    end
    env.GetDevice = function(id)
        return {
            get_selected_gun_ammo_count = function()
                return env.state.shells + id
            end,
        }
    end
    return env
end
local function load_in(path, env)
    local chunk = assert(loadfile(path))
    setfenv(chunk, env)
    return chunk()
end
local function contains(lines, fragment)
    for _, line in ipairs(lines) do
        if line:find(fragment, 1, true) then return true end
    end
    return false
end
local env = fixture()
local old_start, old_frame, old_stop = 0, 0, 0
env.LuaExportStart = function()
    old_start = old_start + 1
end
env.LuaExportAfterNextFrame = function()
    old_frame = old_frame + 1
end
env.LuaExportStop = function()
    old_stop = old_stop + 1
end
load_in(runtime .. "export.lua", env)
env.LuaExportStart()
env.LuaExportAfterNextFrame()
expect(contains(env.lines, '"shells":523'), "Export reads initial native count")
env.state.time, env.state.shells = 0.2, 0
env.LuaExportAfterNextFrame()
expect(contains(env.lines, '"shells":0'), "Export preserves native zero")
env.state.time, env.state.shells = 0.4, 261
env.LuaExportAfterNextFrame()
expect(contains(env.lines, '"shells":261'), "Export records changed native ammunition")
env.LuaExportStop()
env.state.time, env.state.shells = 0, 523
local before = #env.file_lines
env.LuaExportStart()
env.LuaExportAfterNextFrame()
expect(#env.file_lines > before, "the second mission reopens file recording")
expect(old_start == 2 and old_frame == 4 and old_stop == 1, "existing Export callbacks retain their calls")
local prior = env.LuaExportAfterNextFrame
load_in(runtime .. "export.lua", env)
expect(env.LuaExportAfterNextFrame == prior, "duplicate registration installs one observer")
env.state.aircraft, env.state.time = "F-15C", 1
env.LoGetPayloadInfo = function()
    error("other aircraft payload was read")
end
env.LuaExportAfterNextFrame()
expect(not contains(env.lines, "other aircraft payload was read"), "Export restricts data reads to F-CK-1C")
for _, line in ipairs(env.lines) do
    records[#records + 1] = line
end

env = fixture()
env.LoGetSelfData = function()
    return nil
end
env.Export = {
    LoGetSelfData = function()
        return { Name = "F-CK-1C" }
    end,
    LoGetPlayerPlaneId = function()
        return 101
    end,
    LoGetPilotName = function()
        return "Local Pilot"
    end,
}
local identity = load_in(runtime .. "host.lua", env).identity()
expect(identity and identity.object_id == 101, "namespaced identity remains available when the global returns nil")

-- 任務來源在 Export 缺少時仍獨立執行，並鎖定本機玩家。
env = fixture()
env.LoGetSelfData, env.LoGetPayloadInfo, env.GetDevice = false, false, false
local mission = setmetatable({}, { __index = _G })
local ammo = 261
local enemy_reads = 0
local function unit(id, pilot)
    return {
        getTypeName = function()
            return "F-CK-1C"
        end,
        getID = function()
            return id
        end,
        getPlayerName = function()
            return pilot
        end,
        getName = function()
            return "Player 1"
        end,
        getAmmo = function()
            if id ~= 101 then enemy_reads = enemy_reads + 1 end
            return { { count = ammo, desc = { category = 0, typeName = "M61_HEI", displayName = "20 mm" } } }
        end,
        getDrawArgumentValue = function(_, index)
            return index == 308 and -1 or 0
        end,
    }
end
local self, other = unit(101, "Local Pilot"), unit(202, "Other Pilot")
mission.coalition = {
    getPlayers = function(side)
        if side == 0 then error("neutral player scan unavailable") end
        return side == 2 and { other, self } or {}
    end,
}
mission.world = {
    getPlayer = function()
        return self
    end,
}
mission.timer = {
    getTime = function()
        return env.state.time
    end,
}
mission.env = { mission = { coalition = { blue = { country = { { plane = { group = { { units = { { name = "Player 1", payload = { gun = 50, pylons = { [2] = { CLSID = "<CLEAN>" } } } } } } } } } } } } } }
env.net = {
    get_my_player_id = function()
        return 1
    end,
    get_player_info = function()
        return { name = "Local Pilot" }
    end,
    dostring_in = function(state, script)
        expect(state == "mission", "GameGUI uses the mission state")
        local chunk = assert(loadstring(script))
        setfenv(chunk, mission)
        return chunk()
    end,
}
env.DCS = {
    isMultiplayer = function()
        return false
    end,
    setUserCallbacks = function(callbacks)
        env.callbacks = callbacks
    end,
}
load_in(runtime .. "hook.lua", env)
env.callbacks.onSimulationStart()
env.callbacks.onSimulationFrame()
expect(contains(env.lines, '\\"count\\":261'), "mission getAmmo works independently of Export")
expect(contains(env.lines, '\\"gun\\":50'), "initial native load percentage is captured")
expect(contains(env.lines, "<CLEAN>"), "initial CLEAN configuration is preserved")
expect(contains(env.lines, '\\"argument\\":308'), "CLEAN draw argument uses the aircraft definition")
expect(enemy_reads == 0, "another player's ammunition is never queried")
ammo, env.state.time = 0, 1.1
env.callbacks.onSimulationFrame()
expect(contains(env.lines, '\\"count\\":0'), "mission source records depleted ammunition")
ammo, env.state.time = 523, 2.2
env.callbacks.onSimulationFrame()
expect(contains(env.lines, '\\"count\\":523'), "mission source records rearming")
env.net.get_player_info = function()
    return { name = "Unmatched" }
end
env.DCS.isMultiplayer = function()
    return true
end
env.state.time = 3.3
env.callbacks.onSimulationFrame()
expect(contains(env.lines, "ownship_unmatched"), "unmatched multiplayer identity stays explicit")
expect(enemy_reads == 0, "unmatched identity never falls back to another aircraft")
env.net.dostring_in = function()
    return nil, "mission unavailable"
end
env.state.time = 4.4
env.callbacks.onSimulationFrame()
expect(contains(env.lines, "mission unavailable"), "cross-state failure is preserved")
env.callbacks.onSimulationStop()
local count = #env.lines
env.state.time = 7
env.callbacks.onSimulationFrame()
expect(#env.lines == count, "mission observer stops with the simulation")
for _, line in ipairs(env.lines) do
    records[#records + 1] = line
end

env = fixture()
env.FCK1C_AMMO_PROBE_ENABLED = true
local instance = load_in(core_path, env).cockpit("weapon", device)
instance.update(0.05)
expect(contains(env.lines, '"source":"device.methods"'), "enabled cockpit captures real method inventory")
expect(contains(env.lines, '"value":23'), "enabled cockpit records a native getter result")
for _, line in ipairs(env.lines) do
    records[#records + 1] = line
end
env = fixture()
env.FCK1C_AMMO_PROBE_ENABLED = false
load_in(core_path, env).cockpit("weapon", device).update(0.1)
expect(#env.lines == 0, "disabled cockpit leaves adapters alone")

if arg[1] then
    local file = assert(io.open(arg[1], "wb"))
    for _, line in ipairs(records) do
        file:write(line .. "\n")
    end
    file:close()
end
print(string.format("DCS ammo probe: %d checks, 0 failures", checks))
