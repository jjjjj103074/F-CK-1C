------------------------------------------------------------
-- F-CK-1C Weapon System  (avSimpleWeaponSystem context)
--
-- 職責：
--   1. 偵測 FC mode → 呼叫 select_station() 啟動 DCS IR 尋標器
--   2. 掃描掛點取得真實飛彈數量
--
-- 注意：AIM9_TONE_STATE 由 radar_state_system.lua 負責
--       radar_state_system 直接讀取 WS_IR_MISSILE_LOCK 判斷鎖定
--       本系統只負責觸發 select_station() 讓 DCS 啟動尋標器
------------------------------------------------------------
local WeaponSystem = GetSelf()

local update_rate = 0.05 -- 20 Hz
make_default_activity(update_rate)
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")

------------------------------------------------------------
-- DCS 內建常數 (wsType levels)
-- AIM-9L 實測: level2=4(Missile), level3=7, level4=267
------------------------------------------------------------
local wsType_Missile = 4
local wsType_AA_Missile = 7

------------------------------------------------------------
-- FC mode 常數
------------------------------------------------------------
local FC_MODE_NAV = 0
local FC_MODE_DGFT = 1
local FC_MODE_MSL = 2

------------------------------------------------------------
-- 掛點數量
------------------------------------------------------------
local NUM_STATIONS = 7
local NO_STATION = -1
local UNKNOWN_FC_MODE = -1
local OBS_INVALID_NONE = 0
local OBS_INVALID_NOT_PROVIDED = 1
local OBS_INVALID_STATION_API_ERROR = 5

------------------------------------------------------------
-- Param handles
------------------------------------------------------------
local cms_fc_mode_param = get_param_handle("HMCS_FC_MODE")
local observation_available = get_param_handle(cockpit_params.WeaponObservationAvailable)
local observation_revision = get_param_handle(cockpit_params.WeaponObservationRevision)
local observation_invalid_reason = get_param_handle(cockpit_params.WeaponObservationInvalidReason)
local observation_aim9_count = get_param_handle(cockpit_params.WeaponObservationAim9Count)
local observation_selected_station = get_param_handle(cockpit_params.WeaponObservationSelectedStation)
local observation_scanned_count = get_param_handle(cockpit_params.WeaponObservationScannedStationCount)

------------------------------------------------------------
-- 內部狀態
------------------------------------------------------------
local aim9_selected = false
local selected_station = NO_STATION
local prev_fc_mode = UNKNOWN_FC_MODE
local station_select_done = false
local scan_dump_done = false
local debug_timer = 0.0
local DEBUG_INTERVAL = 2.0
local station_observation_revision = 0

------------------------------------------------------------
local function dlog(msg)
    if log and log.info then log.info("FCK1C WPN: " .. tostring(msg)) end
end

local function publish_station_observation(available, count, station)
    station_observation_revision = station_observation_revision + 1
    observation_available:set(available and 1 or 0)
    observation_revision:set(station_observation_revision)
    observation_invalid_reason:set(available and OBS_INVALID_NONE or OBS_INVALID_STATION_API_ERROR)
    observation_aim9_count:set(count)
    observation_selected_station:set(station)
    observation_scanned_count:set(NUM_STATIONS)
end

------------------------------------------------------------
-- 掃描所有掛點，dump 詳細資訊 (只做一次，用於診斷)
------------------------------------------------------------
local function dump_all_stations()
    if scan_dump_done then return end
    scan_dump_done = true

    for i = 0, NUM_STATIONS - 1 do
        local ok, info = pcall(WeaponSystem.get_station_info, WeaponSystem, i)
        if ok and info then
            local w = info.weapon or {}
            dlog(string.format("station[%d] CLSID=%s count=%s L2=%s L3=%s L4=%s", i, tostring(info.CLSID or "?"), tostring(info.count or "?"), tostring(w.level2 or "?"), tostring(w.level3 or "?"), tostring(w.level4 or "?")))
        elseif not ok then
            dlog("station[" .. i .. "] ERROR: " .. tostring(info))
        else
            dlog("station[" .. i .. "] = nil")
        end
    end
end

------------------------------------------------------------
-- 掃描掛點找 AIM-9
------------------------------------------------------------
local function scan_aim9_stations()
    local first_found = NO_STATION
    local total_count = 0
    local available = true

    for i = 0, NUM_STATIONS - 1 do
        local ok, info = pcall(WeaponSystem.get_station_info, WeaponSystem, i)
        if not ok then available = false end
        if ok and info and info.count and info.count > 0 then
            local is_aim9 = false
            local w = info.weapon

            -- 方法 1: wsType 匹配
            if w and w.level2 == wsType_Missile and w.level3 == wsType_AA_Missile then is_aim9 = true end

            -- 方法 2: CLSID 名稱匹配 (備用)
            if not is_aim9 and info.CLSID then
                local clsid = tostring(info.CLSID)
                if clsid:find("AIM%-9") or clsid:find("AIM_9") or clsid:find("CATM%-9") then is_aim9 = true end
            end

            if is_aim9 then
                total_count = total_count + info.count
                if first_found == NO_STATION then first_found = i end
            end
        end
    end

    return (first_found ~= NO_STATION), first_found, total_count, available
end

------------------------------------------------------------
-- 選擇 AIM-9 掛點
------------------------------------------------------------
local function select_aim9()
    dump_all_stations()

    local found, station, count, observation_is_available = scan_aim9_stations()

    if found then
        local ok, err = pcall(WeaponSystem.select_station, WeaponSystem, station)
        if ok then
            aim9_selected = true
            selected_station = station
            dlog("select_station(" .. station .. ") OK, count=" .. count)
        else
            dlog("select_station(" .. station .. ") FAILED: " .. tostring(err))
            aim9_selected = false
            selected_station = NO_STATION
            observation_is_available = false
        end
    else
        dlog("scan: no AIM-9 found")
        aim9_selected = false
        selected_station = NO_STATION
    end
    publish_station_observation(observation_is_available, count, selected_station)
end

------------------------------------------------------------
function post_initialize()
    prev_fc_mode = UNKNOWN_FC_MODE
    observation_available:set(0)
    observation_revision:set(0)
    observation_invalid_reason:set(OBS_INVALID_NOT_PROVIDED)
    observation_aim9_count:set(0)
    observation_selected_station:set(NO_STATION)
    observation_scanned_count:set(NUM_STATIONS)
    dlog("initialized")
end

------------------------------------------------------------
WeaponSystem:listen_event("WeaponRearmComplete")
WeaponSystem:listen_event("UnlimitedWeaponStationRestore")

function CockpitEvent(event, val)
    if event == "WeaponRearmComplete" or event == "UnlimitedWeaponStationRestore" then
        dlog("rearm: " .. tostring(event))
        scan_dump_done = false
        select_aim9()
    end
end

------------------------------------------------------------
function update()
    local fc_mode = math.floor((cms_fc_mode_param:get() or 0) + 0.5)
    local mode_changed = (fc_mode ~= prev_fc_mode)
    prev_fc_mode = fc_mode

    local is_aam = (fc_mode == FC_MODE_DGFT or fc_mode == FC_MODE_MSL)

    if mode_changed then
        if is_aam then
            select_aim9()
            station_select_done = true
        else
            aim9_selected = false
            selected_station = NO_STATION
            station_select_done = false
        end
    end

    if is_aam and not station_select_done then
        select_aim9()
        station_select_done = true
    end

    if not is_aam or not aim9_selected then return end

    -- 確認掛點仍有飛彈
    local ok, info = pcall(WeaponSystem.get_station_info, WeaponSystem, selected_station)
    if not ok or info == nil then
        dlog("selected station read failed: " .. tostring(info))
        aim9_selected = false
        selected_station = NO_STATION
        station_select_done = false
        publish_station_observation(false, 0, NO_STATION)
        return
    end
    if info.count ~= nil and info.count <= 0 then
        dlog("station " .. selected_station .. " empty, re-scanning")
        select_aim9()
    end

    -- Periodic AIM-9 seeker telemetry log.
    debug_timer = debug_timer + update_rate
    if debug_timer >= DEBUG_INTERVAL then
        debug_timer = 0.0
        local ir_lock = get_param_handle("WS_IR_MISSILE_LOCK"):get() or 0
        local ir_az = get_param_handle("WS_IR_MISSILE_TARGET_AZIMUTH"):get() or 0
        local ir_el = get_param_handle("WS_IR_MISSILE_TARGET_ELEVATION"):get() or 0
        dlog(string.format("IR: stn=%d lock=%.1f az=%.4f el=%.4f fc=%d", selected_station, ir_lock, ir_az, ir_el, fc_mode))
    end
end

need_to_be_closed = false
