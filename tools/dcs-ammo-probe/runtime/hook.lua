-- 透過 GameGUI 回呼讀取 Export，並查詢任務環境的本機原生彈藥。
local root = lfs.writedir() .. "Scripts/FCK1CAmmoProbe/"
local Host = dofile(root .. "host.lua")
local Core = Host.Core
local logger = Host.logger("hook-mission")
local function read_file(path)
    local file = assert(io.open(path, "rb"))
    local text = file:read("*a")
    file:close()
    return text
end
local query_prefix = "local Core = (function()\n" .. read_file(root .. "AmmoProbe.lua") .. "\nend)()\nlocal query = (function()\n" .. read_file(root .. "mission-query.lua") .. "\nend)()\n"
local next_sample, last_time = 0, -1
local active = false
local running = false

local function sample()
    if not running then return end
    local now = Core.time()
    if now < last_time then next_sample = 0 end
    last_time = now
    if now < next_sample then return end
    next_sample = now + 1
    local identity, visibility = Host.identity()
    if identity and not active then logger.record("aircraft.entered", identity, now, true) end
    active = identity ~= nil
    if not identity then logger.record("ownship.visibility", { status = visibility.status, result_type = visibility.result_type }, now) end
    local export = Core.member(_G, "Export")
    if identity then Core.payload(logger, export, "Export.LoGetPayloadInfo", now) end
    logger.record("Export.LoIsOwnshipExportAllowed", Core.call(export, "LoIsOwnshipExportAllowed", "dot"), now)
    if not net or type(net.dostring_in) ~= "function" then
        logger.record("mission.query", { status = "missing_net_dostring_in" }, now)
        return
    end
    local single_player = false
    if DCS and type(DCS.isMultiplayer) == "function" then single_player = not DCS.isMultiplayer() end
    local object_id = identity and type(identity.object_id) == "number" and string.format("%.0f", identity.object_id) or "nil"
    local player_name = identity and type(identity.player_name) == "string" and identity.player_name or ""
    if net and type(net.get_my_player_id) == "function" and type(net.get_player_info) == "function" then
        local info = net.get_player_info(net.get_my_player_id())
        if type(info) == "table" and type(info.name) == "string" then player_name = info.name end
    end
    local arguments = string.format("{object_id=%s,player_name=%q,single_player=%s}", object_id, player_name, tostring(single_player))
    local script = query_prefix .. "return Core.encode(query(" .. arguments .. "))"
    local ok, result, error_text = pcall(net.dostring_in, "mission", script)
    logger.record("mission.Unit.getAmmo", { transport_ok = ok, response_type = type(result), response = result, error = error_text }, now)
end

local callbacks = {}
function callbacks.onSimulationStart()
    next_sample, last_time, active = 0, -1, false
    running = true
    logger.record("simulation.start", {}, Core.time(), true)
end
function callbacks.onSimulationFrame()
    local ok, err = pcall(sample)
    if not ok then pcall(logger.record, "probe.error", { message = tostring(err) }) end
end
function callbacks.onSimulationStop()
    logger.record("simulation.stop", {}, Core.time(), true)
    active = false
    running = false
end
DCS.setUserCallbacks(callbacks)
