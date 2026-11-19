-- 在座艙替身執行實際紀錄器與面板，核對取樣、檔名、CSV 與釋放流程。
local root = assert(arg[1], "Pass the flight-test tool directory")
local output = assert(arg[2], "Pass a fixture directory")
local values = {
    CPP_MC_GENERATION = 2,
    OBS_WEAPON_STATIONS_REVISION = 2,
    CPP_MC_AVAILABLE = 1,
    OBS_WEAPON_STATION_01 = '1;1;4;4;7;1;RT;{quoted,"weapon"}',
}
local original_dofile, original_io = dofile, io
local time = 1
local reads = 0
local mutate_generation = false
local messages = {}
LockOn_Options = { script_path = 'fixture/', common_script_path = 'common/', screen = { aspect = 16 / 9 } }
function make_default_activity(rate) assert(rate == 0.02) end
function get_param_handle(name)
    return { get = function()
        if name == 'CPP_MC_GENERATION' then
            reads = reads + 1
            if mutate_generation and reads % 3 == 0 then values[name] = values[name] + 2 end
        end
        return values[name]
    end }
end
function get_model_time() return time end
log = { info = function(message) messages[#messages + 1] = message end }
lfs = { writedir = function() return output .. '/' end, mkdir = function() return true end }
function dofile(path)
    if path:match('Build.lua$') then return { source_commit = string.rep('a',40), dll_sha256 = string.rep('b',64) } end
    if path:match('Parameters.lua$') then return { 'CPP_MC_GENERATION', 'CPP_MC_AVAILABLE', 'OBS_WEAPON_STATION_01', 'UNAVAILABLE_HANDLE' } end
    return original_dofile(path)
end
original_dofile(root .. '/Recorder.lua')
post_initialize()
assert(#messages == 1 and messages[1]:match('recording'))
local first = assert(messages[1]:match('recording (.+)$'))
-- 依裝置生命週期排程取樣，關閉狀態只留下初始化表頭。
if need_to_be_closed == false then
    update(); time = 2; update(); mutate_generation = true; time = 3; update(); mutate_generation = false
end
release()
local f=assert(original_io.open(first,'rb')); local content=f:read('*a'); f:close()
assert(content:match('"source_commit"'))
assert(select(2, content:gsub('\n', '')) > 1, 'Recorder produced only a header; runtime updates did not run')
assert(content:match('quoted,""weapon""'))
assert(content:match(',"1","1","2","1",'))
assert(content:match(',""\r\n'))
assert(content:match(',"3","3","0","1",'))
post_initialize(); assert(messages[#messages]:match('recording'))
assert(messages[#messages]:match('recording (.+)$') ~= first)
update(); release()
local count=#messages
local saved_io = io
io = nil
post_initialize()
assert(#messages == count + 1 and messages[#messages]:match('unavailable'))
update(); release()
io = saved_io
-- 以 DCS 的零回傳值 I/O 包裝重放實際寫入流程，並核對標準 I/O 錯誤。
for _, mode in ipairs({ "void-write", "void-flush", "void-both", "write-error", "flush-error", "row-error", "write-throws" }) do
    io = { open = function(path, access)
        local handle, reason = original_io.open(path, access)
        if not handle or access ~= "wb" then return handle, reason end
        local writes = 0
        return {
            write = function(_, text)
                writes = writes + 1
                if mode == "write-error" or (mode == "row-error" and writes == 2) then
                    return nil, "fixture write failure"
                end
                if mode == "write-throws" then error("fixture write exception") end
                local result, failure = handle:write(text)
                if not result then return nil, failure end
                if mode == "void-write" or mode == "void-both" then return end
                return result
            end,
            flush = function()
                if mode == "flush-error" then return nil, "fixture flush failure" end
                local result, failure = handle:flush()
                if not result then return nil, failure end
                if mode == "void-flush" or mode == "void-both" then return end
                return result
            end,
            close = function() return handle:close() end
        }
    end }
    original_dofile(root .. '/Recorder.lua')
    post_initialize()
    if mode:match('^void') or mode == "row-error" then
        assert(messages[#messages]:match('recording'), mode .. ': ' .. messages[#messages])
        local path = assert(messages[#messages]:match('recording (.+)$'))
        if mode == "row-error" then
            update()
            assert(messages[#messages]:match('recorder stopped:') and messages[#messages]:match('fixture write failure'))
            local before = #messages
            update()
            assert(#messages == before)
        else
            for i = 1, 51 do time = time + 0.02; update() end
        end
        release()
        local handle = assert(original_io.open(path, 'rb'))
        local rows = 0
        for _ in handle:lines() do rows = rows + 1 end
        handle:close()
        assert(rows == (mode == "row-error" and 1 or 52), mode .. ': row count ' .. rows)
    else
        assert(messages[#messages]:match('recorder unavailable:') and messages[#messages]:match('fixture'))
        update(); release()
    end
    io = original_io
end
print('Recorder DCS I/O return and failure fixtures passed.')
-- 用既有指示器介面建立實際面板元素並核對邊界與參數唯讀。
function dofile(path) if path:match('elements_defs.lua$') then return end; return original_dofile(path) end
function SetCustomScale(value) assert(value==1) end
ScreenType = { SCREENSPACE_TRUE = 1 }
default_box_indices = { 0, 1, 2, 0, 2, 3 }
function MakeMaterial(_,color) return color end
function CreateElement(kind) return { kind = kind } end
local elements = {}
function Add(element) elements[#elements + 1] = element end
original_dofile(root .. '/Indicator_page.lua')
assert(#elements == 36)
local names = {}
for _,element in ipairs(elements) do assert(not names[element.name]); names[element.name]=true end
assert(elements[1].vertices[3][1] < LockOn_Options.screen.aspect)
print('MC flight-test recorder/indicator fixtures passed.')

-- 取樣器保留有效零值，並分別標示缺少的原生 getter。
local methods = { get_flare_count = function() return 90 end, get_chaff_count = function() return 80 end }
function get_param_handle(name)
    return { set = function(_, value) values[name] = value end }
end
local observer = original_dofile(root .. '/NativeCountermeasureObservation.lua').new(methods)
observer.update()
assert(values.FCK1C_TEST_CM_OBSERVATION_AVAILABLE == 3)
assert(values.FCK1C_TEST_CM_FLARE_COUNT == 90 and values.FCK1C_TEST_CM_CHAFF_COUNT == 80)
methods.get_flare_count = function() return 0 end
methods.get_chaff_count = function() error('missing') end
observer.update()
assert(values.FCK1C_TEST_CM_OBSERVATION_AVAILABLE == 1)
assert(values.FCK1C_TEST_CM_FLARE_COUNT == 0 and values.FCK1C_TEST_CM_CHAFF_COUNT == -1)
methods.get_flare_count = function() return 0/0 end
methods.get_chaff_count = nil
observer.update()
assert(values.FCK1C_TEST_CM_OBSERVATION_AVAILABLE == 0)
-- 執行實際鍵位加入器，核對四種測試入口及既有配置保留。
function _(s) return s end
local commands = { { name = 'existing' } }
local ids = { NativeCmFlareDigital = 3150, NativeCmChaffDigital = 3151, NativeCmFlareAnalog = 3152, NativeCmChaffAnalog = 3153 }
original_dofile(root .. '/NativeCountermeasureKeys.lua')(commands, ids)
assert(#commands == 5 and commands[1].name == 'existing')
for i = 2,5 do
    assert(commands[i].down == 3148 + i and commands[i].up == commands[i].down)
    assert(commands[i].value_down == 1 and commands[i].value_up == 0)
    assert(commands[i].combos[1].key == (i % 2 == 0 and 'Insert' or 'Delete'))
end
print('Native countermeasure observer/key fixtures passed.')
