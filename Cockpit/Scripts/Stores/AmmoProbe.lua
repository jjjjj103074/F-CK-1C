-- 蒐集彈藥介面的原始結果、方法清單及時間，供不同 Lua 執行環境對照。
local M = {}
local MAX_DEPTH = 7
local MAX_ITEMS = 128
local MAX_STRING = 16384
M.stations = {
    { name = "RT", draw = 307 },
    { name = "RO", draw = 308 },
    { name = "RI", draw = 309 },
    { name = "MM", draw = 310 },
    { name = "MF", draw = 316 },
    { name = "MB", draw = 317 },
    { name = "LI", draw = 311 },
    { name = "LO", draw = 312 },
    { name = "LT", draw = 313 },
}
local getters = {
    "getGunAmmoCount",
    "getCannonsAmmoCount",
    "getAmmoCount",
    "get_selected_gun_ammo_count",
    "get_gun_ammo_count",
    "get_cannon_ammo_count",
    "get_ammo_count",
    "get_shell_count",
    "get_shells_count",
    "get_chaff_count",
    "get_flare_count",
}

local function quote(value)
    return '"' .. value:gsub('[%z\1-\31\\"]', function(c)
        local replacements = { ['"'] = '\\"', ["\\"] = "\\\\", ["\n"] = "\\n", ["\r"] = "\\r", ["\t"] = "\\t" }
        return replacements[c] or string.format("\\u%04x", string.byte(c))
    end) .. '"'
end

local function encode(value, depth, seen)
    local kind = type(value)
    if kind == "nil" then return "null" end
    if kind == "boolean" then return value and "true" or "false" end
    if kind == "number" then
        if value ~= value or value == math.huge or value == -math.huge then return quote(tostring(value)) end
        return string.format("%.17g", value)
    end
    if kind == "string" then
        if #value > MAX_STRING then value = value:sub(1, MAX_STRING) .. "<truncated>" end
        return quote(value)
    end
    if kind ~= "table" then return quote("<" .. kind .. ">") end
    if seen[value] then return quote("<cycle>") end
    if depth >= MAX_DEPTH then return quote("<depth-limit>") end
    seen[value] = true
    local keys = {}
    for key in pairs(value) do
        if type(key) == "string" or type(key) == "number" then keys[#keys + 1] = key end
    end
    table.sort(keys, function(a, b)
        if type(a) == type(b) then return a < b end
        return type(a) < type(b)
    end)
    local parts = {}
    for i = 1, math.min(#keys, MAX_ITEMS) do
        local key = keys[i]
        parts[#parts + 1] = quote(tostring(key)) .. ":" .. encode(value[key], depth + 1, seen)
    end
    if #keys > MAX_ITEMS then parts[#parts + 1] = '"_truncated_entries":' .. #keys end
    seen[value] = nil
    return "{" .. table.concat(parts, ",") .. "}"
end

function M.encode(value)
    return encode(value, 0, {})
end

-- 以受保護的屬性查詢讀取 table 或原生裝置物件。
function M.member(object, name)
    if object == nil then return nil end
    local ok, value = pcall(function()
        return object[name]
    end)
    if ok then return value end
    return nil
end

function M.call(object, name, style)
    local fn = M.member(object, name)
    if type(fn) ~= "function" then return { status = "missing", member_type = type(fn) } end
    local ok, value
    if style == "colon" then
        ok, value = pcall(fn, object)
    else
        ok, value = pcall(fn)
    end
    return { status = ok and "returned" or "error", result_type = type(value), value = value }
end

-- 列出裝置與其索引表提供的方法，保留原生介面的實際名稱。
function M.methods(object)
    local names, seen, visited = {}, {}, {}
    local function visit(value, depth)
        if value == nil or visited[value] or depth > 4 then return end
        visited[value] = true
        if type(value) == "table" then
            for name, fn in pairs(value) do
                if type(name) == "string" and type(fn) == "function" and not seen[name] then
                    seen[name] = true
                    names[#names + 1] = name
                end
            end
        end
        local ok, meta = pcall(getmetatable, value)
        if ok and type(meta) == "table" then visit(rawget(meta, "__index"), depth + 1) end
    end
    visit(object, 0)
    table.sort(names)
    return { object_type = type(object), names = names }
end

function M.time()
    local result = M.call(_G, "get_model_time", "dot")
    if result.status == "returned" and type(result.value) == "number" then return result.value, "model" end
    result = M.call(_G, "LoGetModelTime", "dot")
    if result.status == "returned" and type(result.value) == "number" then return result.value, "model" end
    result = M.call(M.member(_G, "Export"), "LoGetModelTime", "dot")
    if result.status == "returned" and type(result.value) == "number" then return result.value, "model" end
    if os and type(os.clock) == "function" then return os.clock(), "process" end
    return 0, "unavailable"
end

function M.logger(channel, write)
    local previous, last_write = {}, {}
    local sequence = 0
    local logger = {}
    function logger.record(source, result, now, force)
        local content = M.encode(result)
        local default_time, clock = M.time()
        now = now or default_time
        if not force and previous[source] == content and last_write[source] and now >= last_write[source] and now - last_write[source] < 5 then return end
        sequence = sequence + 1
        local record = M.encode({ channel = channel, sequence = sequence, time = now, clock = clock, source = source, result = result })
        local ok = pcall(write, record)
        if ok then
            previous[source] = content
            last_write[source] = now
        end
    end
    return logger
end

function M.log_line(line)
    if log and type(log.write) == "function" then
        log.write("FCK1C_AMMO", log.INFO, line)
    elseif log and type(log.info) == "function" then
        log.info("FCK1C AMMO " .. line)
    end
end

function M.probe_getters(logger, owner, object, styles, now)
    for _, name in ipairs(getters) do
        for _, style in ipairs(styles) do
            logger.record(owner .. "." .. name .. ":" .. style, M.call(object, name, style), now)
        end
    end
end

function M.payload(logger, root, label, now)
    logger.record(label, M.call(root, "LoGetPayloadInfo", "dot"), now)
end

function M.draw(logger, root, method, now)
    local fn = M.member(root, method)
    if type(fn) ~= "function" then
        logger.record(method, { status = "missing" }, now)
        return
    end
    local values = {}
    for _, station in ipairs(M.stations) do
        local ok, value = pcall(fn, station.draw)
        values[station.name] = { argument = station.draw, call_ok = ok, value = value }
    end
    logger.record(method, values, now)
end

-- 座艙裝置固定取樣候選讀取介面，並記錄機砲現有輸入與估算值。
function M.cockpit(label, device, sensor, parameters, options)
    local enabled = rawget(_G, "FCK1C_AMMO_PROBE_ENABLED")
    if enabled == nil and LockOn_Options and LockOn_Options.script_path then
        local ok, value = pcall(dofile, LockOn_Options.script_path .. "Stores/AmmoProbeEnabled.lua")
        enabled = ok and value == true
    end
    if not enabled then return { update = function() end } end
    local logger = M.logger("cockpit." .. label, M.log_line)
    local elapsed, ticks = 0, 0
    local handles = {}
    if type(get_param_handle) == "function" then
        for name, id in pairs(parameters or {}) do
            local ok, handle = pcall(get_param_handle, id)
            if ok then handles[name] = handle end
        end
    end
    local function sample()
        local now = M.time()
        if ticks == 0 or ticks == 50 then
            logger.record("device.methods", M.methods(device), now, true)
            logger.record("sensor.methods", M.methods(sensor), now, true)
        end
        M.probe_getters(logger, "device", device, { "colon" }, now)
        if sensor then M.probe_getters(logger, "get_base_data", sensor, { "dot", "colon" }, now) end
        if not options or options.global_payload ~= false then M.payload(logger, _G, "global.LoGetPayloadInfo", now) end
        M.payload(logger, M.member(_G, "Export"), "Export.LoGetPayloadInfo", now)
        M.draw(logger, _G, "get_aircraft_draw_argument_value", now)
        local values = {}
        for name, handle in pairs(handles) do
            values[name] = M.call(handle, "get", "colon")
        end
        if next(values) then logger.record("mc.parameters", values, now) end
        ticks = ticks + 1
    end
    return {
        update = function(dt)
            elapsed = elapsed + dt
            if ticks == 0 or elapsed >= 0.1 then
                elapsed = 0
                local ok, err = pcall(sample)
                if not ok then pcall(logger.record, "probe.error", { message = tostring(err) }) end
            end
        end,
    }
end

return M
