-- 將已發布的飛行、MC 與反制狀態轉成固定畫面文字。
return function()
    local ias = get_param_handle(cockpit_params.HmcsIasKts)
    local altitude = get_param_handle(cockpit_params.HmcsAltitudeFt)
    local altitude_available = get_param_handle(cockpit_params.PressureAltitudeAvailable)
    local generation = get_param_handle(cockpit_params.McGeneration)
    local available = get_param_handle(cockpit_params.McAvailable)
    local master_mode = get_param_handle(cockpit_params.McMasterMode)
    local override = get_param_handle(cockpit_params.McOverrideMode)
    local arm = get_param_handle(cockpit_params.HmcsMasterMode)
    local submode = get_param_handle(cockpit_params.HmcsAamSubmode)
    local store_state = get_param_handle(cockpit_params.McSelectedStoreState)
    local store_count = get_param_handle(cockpit_params.McSelectedStoreCount)
    local store_identifier = get_param_handle(cockpit_params.McSelectedStoreIdentifier)
    local store_station = get_param_handle(cockpit_params.McSelectedStoreStation)
    local gun_available = get_param_handle(cockpit_params.McStoresGunCountAvailable)
    local gun_count = get_param_handle(cockpit_params.McStoresGunCount)
    local ir_available = get_param_handle(cockpit_params.McStoresIrSeekerAvailable)
    local ir_locked = get_param_handle(cockpit_params.McStoresIrSeekerLock)
    local uncage = get_param_handle(cockpit_params.McMissileUncageHeld)
    local cm_available = get_param_handle(cockpit_params.CmInventoryAvailable)
    local flare_count = get_param_handle(cockpit_params.CmFlareCount)
    local chaff_count = get_param_handle(cockpit_params.CmChaffCount)
    local radar_available = get_param_handle(cockpit_params.RadarDisplaySensorAvailable)
    local radar_mode = get_param_handle(dcs_cockpit_params.RadarMode)

    local ias_text = get_param_handle(cockpit_params.FlightStatusIas)
    local altitude_text = get_param_handle(cockpit_params.FlightStatusAltitude)
    local master_mode_text = get_param_handle(cockpit_params.FlightStatusMasterMode)
    local override_text = get_param_handle(cockpit_params.FlightStatusOverride)
    local arm_text = get_param_handle(cockpit_params.FlightStatusArm)
    local missile_text = get_param_handle(cockpit_params.FlightStatusMissile)
    local station_text = get_param_handle(cockpit_params.FlightStatusStation)
    local gun_text = get_param_handle(cockpit_params.FlightStatusGun)
    local flare_text = get_param_handle(cockpit_params.FlightStatusFlare)
    local chaff_text = get_param_handle(cockpit_params.FlightStatusChaff)
    local radar_text = get_param_handle(cockpit_params.FlightStatusRadar)
    local ir_text = get_param_handle(cockpit_params.FlightStatusIr)
    local uncage_text = get_param_handle(cockpit_params.FlightStatusUncage)
    local store_name = dofile(LockOn_Options.script_path .. "FlightStatus/StoreNames.lua")

    local function number(value, suffix)
        if type(value) ~= "number" or value ~= value or value < 0 or value == math.huge then return "---" end
        return string.format("%.0f%s", value, suffix or "")
    end

    local function update()
        ias_text:set(": " .. (number(ias:get(), " kt")))
        altitude_text:set(": " .. (altitude_available:get() == 1 and number(altitude:get(), " ft") or "---"))
        flare_text:set(": " .. (cm_available:get() == 1 and number(flare_count:get()) or "---"))
        chaff_text:set(": " .. (cm_available:get() == 1 and number(chaff_count:get()) or "---"))

        -- 在同一 MC generation 內讀取模式、已確認選站與武器觀測。
        local before = generation:get()
        local coherent = type(before) == "number" and before >= 2 and before <= 9007199254740991
            and before % 2 == 0 and available:get() == 1
        local mode = ({ [0] = "NAV", "A-A", "A-G" })[master_mode:get()] or "---"
        local ovrd = ({ [0] = "CENTER", "DGFT", "MSL OVRD" })[override:get()] or "---"
        local safety = ({ [0] = "SAFE", "SIM", "ON" })[arm:get()] or "---"
        local missile, station = "---", "---"
        if store_state:get() == 1 then
            missile, station = "NONE", "NONE"
        elseif store_state:get() == 2 then
            missile, station = "SELECTING", "SELECTING"
        elseif store_state:get() == 3 then
            missile = store_name(store_identifier:get()) .. " x" .. number(store_count:get())
            station = store_station:get()
            if type(station) ~= "string" or station == "" then station = "---" end
        end
        local gun = gun_available:get() == 1 and number(gun_count:get()) or "---"
        local ir = ir_available:get() == 1 and (ir_locked:get() == 1 and "LOCK" or "NO LOCK") or "---"
        local uncaged = uncage:get() == 1 and "ON" or "OFF"
        local scan = submode:get() == 5 and "ACM/BORE" or (submode:get() == 4 and "CRM/RWS" or "---")
        local track = ({ [0] = "OFF", "SEARCH", "ACQUIRING", "STT" })[radar_mode:get()] or "---"
        local radar = radar_available:get() == 1 and (scan .. " " .. track) or "---"
        coherent = coherent and generation:get() == before and available:get() == 1
        master_mode_text:set(": " .. (coherent and mode or "---"))
        override_text:set(": " .. (coherent and ovrd or "---"))
        arm_text:set(": " .. (coherent and safety or "---"))
        missile_text:set(": " .. (coherent and missile or "---"))
        station_text:set(": " .. (coherent and station or "---"))
        gun_text:set(": " .. (coherent and gun or "---"))
        radar_text:set(": " .. (coherent and radar or "---"))
        ir_text:set(": " .. (coherent and ir or "---"))
        uncage_text:set(": " .. (coherent and uncaged or "---"))
    end

    -- 建立時先發布文字，再隨既有 64 Hz 觀測每四次更新一次。
    update()
    local ticks = 3
    return { update = function()
        ticks = ticks + 1
        if ticks == 4 then ticks = 0; update() end
    end }
end
