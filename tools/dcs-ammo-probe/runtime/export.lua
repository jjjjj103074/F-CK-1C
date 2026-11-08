-- 在 Export 執行環境觀測原生 payload 與武器裝置的彈量方法。
if _G.FCK1CAmmoProbeExportLoaded then return end
local root = lfs.writedir() .. "Scripts/FCK1CAmmoProbe/"
local Host = dofile(root .. "host.lua")
local Core = Host.Core
local logger = Host.logger("export")
_G.FCK1CAmmoProbeExportLoaded = true
local previous_start = LuaExportStart
local previous_frame = LuaExportAfterNextFrame
local previous_stop = LuaExportStop
local next_sample = 0
local last_time = -1
local active = false
local inventory_done = false

local function sample()
    local now = Core.time()
    if now < last_time then
        next_sample = 0
        inventory_done = false
    end
    last_time = now
    if now < next_sample then return end
    next_sample = now + 0.1
    local identity, status = Host.identity()
    if not identity then
        logger.record("ownship.visibility", { status = status.status, result_type = status.result_type }, now)
        if active then logger.record("aircraft.departed", status, now, true) end
        active, inventory_done = false, false
        return
    end
    if not active then logger.record("aircraft.entered", identity, now, true) end
    active = true
    Core.payload(logger, _G, "global.LoGetPayloadInfo", now)
    Core.payload(logger, Core.member(_G, "Export"), "Export.LoGetPayloadInfo", now)
    Core.draw(logger, _G, "LoGetAircraftDrawArgumentValue", now)
    logger.record("LoGetSnares", Core.call(_G, "LoGetSnares", "dot"), now)
    for _, id in ipairs({ 3, 4 }) do
        local get_device = Core.member(_G, "GetDevice") or Core.member(Core.member(_G, "Export"), "GetDevice")
        if type(get_device) == "function" then
            local ok, device = pcall(get_device, id)
            if ok and device then
                if not inventory_done then logger.record("device." .. id .. ".methods", Core.methods(device), now, true) end
                Core.probe_getters(logger, "device." .. id, device, { "colon" }, now)
            else
                logger.record("device." .. id, { call_ok = ok, result_type = type(device), value = device }, now)
            end
        else
            logger.record("GetDevice", { status = "missing" }, now)
        end
    end
    inventory_done = true
end

function LuaExportStart()
    if previous_start then previous_start() end
    logger.close()
    logger = Host.logger("export")
    next_sample, last_time, active, inventory_done = 0, -1, false, false
end

function LuaExportAfterNextFrame()
    if previous_frame then previous_frame() end
    local ok, err = pcall(sample)
    if not ok then pcall(logger.record, "probe.error", { message = tostring(err) }) end
end

function LuaExportStop()
    local ok, err = pcall(function()
        if previous_stop then previous_stop() end
    end)
    pcall(logger.record, "probe.stop", {}, Core.time(), true)
    logger.close()
    if not ok then error(err) end
end
