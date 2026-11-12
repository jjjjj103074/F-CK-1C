-- 將既有座艙參數保存為飛行測試紀錄。
make_default_activity(0.02)
local build = dofile(LockOn_Options.script_path .. "MCFlightTest/Build.lua")
local schema = dofile(LockOn_Options.script_path .. "MCFlightTest/Parameters.lua")
local handles = {}
for i, name in ipairs(schema) do handles[i] = get_param_handle(name) end
local generation = get_param_handle("CPP_MC_GENERATION")
local station_revision = get_param_handle("OBS_WEAPON_STATIONS_REVISION")
local file = nil
local updates = 0
local function report(message)
    if log and log.info then log.info("FCK1C MC FLIGHT TEST: " .. message) end
end
local function quote(value)
    return '"' .. tostring(value):gsub('"', '""') .. '"'
end
local function close_file()
    if file then pcall(function() file:flush(); file:close() end); file = nil end
end
local function open_file()
    assert(io and io.open and lfs and lfs.writedir and lfs.mkdir, "Cockpit file APIs unavailable")
    local directory = lfs.writedir() .. "Logs/FCK1C-MC-TEST"
    lfs.mkdir(directory)
    local stamp = os.date("%Y%m%d-%H%M%S")
    local index = 1
    local path
    repeat
        path = string.format("%s/mc-%s-%03d.csv", directory, stamp, index)
        local previous = io.open(path, "rb")
        if not previous then break end
        previous:close()
        index = index + 1
    until false
    local reason
    file, reason = io.open(path, "wb")
    assert(file, reason)
    local header = { "source_commit", "dll_sha256", "sample", "model_time_s", "mc_coherent", "stations_coherent" }
    for _, name in ipairs(schema) do header[#header + 1] = name end
    for i, value in ipairs(header) do header[i] = quote(value) end
    assert(file:write(table.concat(header, ",") .. "\r\n"))
    assert(file:flush())
    report("recording " .. path)
end
function post_initialize()
    local ok, reason = pcall(open_file)
    if not ok then close_file(); report("recorder unavailable: " .. tostring(reason)) end
end
function update()
    if not file then return end
    local ok, reason = pcall(function()
        local mc_before, stations_before = generation:get(), station_revision:get()
        local readings = {}
        for i, handle in ipairs(handles) do
            local value = handle:get()
            readings[i] = value == nil and "" or value
        end
        local mc_after, stations_after = generation:get(), station_revision:get()
        local function coherent(before, after)
            return type(before) == "number" and before >= 2 and before % 2 == 0 and before == after
        end
        updates = updates + 1
        local time = get_model_time and get_model_time() or updates * 0.02
        local row = { build.source_commit, build.dll_sha256, updates, time,
            coherent(mc_before, mc_after) and 1 or 0, coherent(stations_before, stations_after) and 1 or 0 }
        for _, value in ipairs(readings) do row[#row + 1] = value end
        for i, value in ipairs(row) do row[i] = quote(value) end
        assert(file:write(table.concat(row, ",") .. "\r\n"))
        if updates % 50 == 0 then assert(file:flush()) end
    end)
    if not ok then close_file(); report("recorder stopped: " .. tostring(reason)) end
end
function release() close_file() end
need_to_be_closed = true
