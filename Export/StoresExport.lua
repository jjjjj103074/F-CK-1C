-- 以 10 Hz 讀取本機原生機砲彈量，經座艙裝置命令交給 MC 的觀測轉接層。
if _G.FCK1CStoresExportLoaded then return end
local module_root = lfs.writedir() .. "Mods/aircraft/F-CK-1C/"
local function definitions(path)
    local environment = {}
    local chunk = assert(loadfile(module_root .. path))
    setfenv(chunk, environment)
    chunk()
    return environment
end
local commands = definitions("Cockpit/Scripts/command_defs.lua").device_commands
local devices = definitions("Cockpit/Scripts/devices.lua").devices
local protocol = dofile(module_root .. "Cockpit/Scripts/Stores/GunAmmunition.lua")
local previous_start, previous_frame, previous_stop = LuaExportStart, LuaExportAfterNextFrame, LuaExportStop
local next_sample, last_time = 0, -1
local reported_errors = {}

local function protected(callback)
    if type(callback) ~= "function" then return end
    local ok, message = pcall(callback)
    if not ok and not reported_errors[tostring(message)] then
        reported_errors[tostring(message)] = true
        if log and type(log.write) == "function" then log.write("FCK1C_STORES_EXPORT", log.ERROR, tostring(message)) end
    end
end

local function sample()
    if type(LoGetModelTime) ~= "function" then return end
    local now = LoGetModelTime()
    if type(now) ~= "number" or now ~= now then return end
    if now < last_time then next_sample = 0 end
    last_time = now
    if now < next_sample then return end
    next_sample = now + 0.1
    if type(LoGetSelfData) ~= "function" or type(GetDevice) ~= "function" then return end
    local ownship = LoGetSelfData()
    if type(ownship) ~= "table" or ownship.Name ~= "F-CK-1C" then return end
    local count
    if type(LoGetPayloadInfo) == "function" then
        local ok, payload = pcall(LoGetPayloadInfo)
        if ok and type(payload) == "table" and type(payload.Cannon) == "table" then count = payload.Cannon.shells end
    end
    local device = GetDevice(devices.CMS)
    if device and type(device.performClickableAction) == "function" then device:performClickableAction(commands.ReceiveGunAmmunition, protocol.encode(count), false) end
end

function LuaExportStart()
    next_sample, last_time = 0, -1
    reported_errors = {}
    protected(previous_start)
end

function LuaExportAfterNextFrame()
    protected(previous_frame)
    protected(sample)
end

function LuaExportStop()
    protected(previous_stop)
    next_sample, last_time = 0, -1
end

_G.FCK1CStoresExportLoaded = true
