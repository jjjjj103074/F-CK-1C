-- 以實際發布器及指示器驗證選站、來源失效與文字欄位位置。
return function(expect)
    local env = setmetatable({}, { __index = _G })
    env.LockOn_Options = { script_path = "Cockpit/Scripts/", common_script_path = "fixture/common/", screen = { aspect = 16 / 9 } }
    local function execute(path)
        local chunk = assert(loadfile(path)); setfenv(chunk, env); return chunk()
    end
    env.dofile = function(path)
        if path:match("^fixture/common/") then return end
        return execute(path)
    end
    execute("Cockpit/Scripts/generated/CockpitParams.g.lua")
    local values, writes = {}, {}
    env.get_param_handle = function(name)
        return {
            get = function() local v = values[name]; return type(v) == "function" and v() or v or 0 end,
            set = function(_, value) values[name] = type(value) == "string" and (tonumber(value) or value) or value; writes[name] = true end,
        }
    end
    local status = execute("Cockpit/Scripts/FlightStatus/Status.lua")()
    local p = env.cockpit_params
    local function display(parameter)
        local value = values[parameter]
        expect(type(value) == "string" and value:sub(1, 2) == ": ", "published text carries its aligned colon")
        return value:sub(3)
    end
    local function update() for _ = 1, 4 do status.update() end end
    update()
    expect(display(p.FlightStatusMissile) == "---", "unavailable MC displays unknown selected weapon")
    values[p.HmcsIasKts], values[p.HmcsAltitudeFt], values[p.PressureAltitudeAvailable] = 420, 12000, 1
    values[p.McGeneration], values[p.McAvailable] = 2, 1
    values[p.McMasterMode], values[p.McOverrideMode], values[p.HmcsMasterMode] = 0, 1, 2
    values[p.McSelectedStoreState], values[p.McSelectedStoreCount] = 3, 1
    values[p.McSelectedStoreIdentifier] = "{6CEB49FC-DED8-4DED-B053-E1F033FF72D3}"
    values[p.McSelectedStoreStation] = "LT"
    values[p.McStoresGunCountAvailable], values[p.McStoresGunCount] = 1, 240
    values[p.McStoresIrSeekerAvailable], values[p.McStoresIrSeekerLock] = 1, 1
    values[p.McMissileUncageHeld], values[p.HmcsAamSubmode] = 1, 5
    values[p.RadarDisplaySensorAvailable], values[env.dcs_cockpit_params.RadarMode] = 1, 3
    values[p.CmInventoryAvailable], values[p.CmFlareCount], values[p.CmChaffCount] = 1, 70, 83
    update()
    expect(display(p.FlightStatusIas) == "420 kt" and display(p.FlightStatusAltitude) == "12000 ft", "flight rows state their units")
    expect(display(p.FlightStatusMasterMode) == "NAV" and display(p.FlightStatusOverride) == "DGFT", "master mode remains distinct from override")
    expect(display(p.FlightStatusArm) == "ON", "master arm has a readable label")
    expect(display(p.FlightStatusMissile) == "AIM-9M x1" and display(p.FlightStatusStation) == "LT", "selected weapon identity and station come from the SM publication")
    expect(display(p.FlightStatusGun) == "240", "gun quantity is independent of selected missile")
    expect(display(p.FlightStatusFlare) == "70" and display(p.FlightStatusChaff) == "83", "countermeasure counts retain their separate identities")
    expect(display(p.FlightStatusRadar) == "ACM/BORE STT" and display(p.FlightStatusIr) == "LOCK", "radar tracking and IR lock have distinct rows")
    expect(display(p.FlightStatusUncage) == "ON", "uncage command state is readable")
    values[p.McSelectedStoreState] = 2
    values[p.McSelectedStoreIdentifier] = "{40EF17B7-F508-45de-8566-6FFECC0C1AB8}"
    update()
    expect(display(p.FlightStatusMissile) == "SELECTING" and display(p.FlightStatusStation) == "SELECTING", "pending selection never presents a weapon as confirmed")
    values[p.McSelectedStoreState], values[p.McSelectedStoreStation], values[p.McMasterMode], values[p.McOverrideMode] = 3, "MF", 1, 0
    update()
    expect(display(p.FlightStatusMissile) == "AIM-120C x1" and display(p.FlightStatusStation) == "MF", "confirmation switches model and station together")
    expect(display(p.FlightStatusMasterMode) == "A-A" and display(p.FlightStatusOverride) == "CENTER", "normal air-to-air mode is readable")
    values[p.McSelectedStoreState] = 1
    values[p.McStoresGunCount], values[p.McStoresIrSeekerLock] = 0, 0
    update()
    expect(display(p.FlightStatusMissile) == "NONE" and display(p.FlightStatusGun) == "0", "no selection and zero ammunition are distinct from unavailable data")
    expect(display(p.FlightStatusIr) == "NO LOCK", "valid unlocked IR observation is explicit")
    values[p.PressureAltitudeAvailable], values[p.McStoresGunCountAvailable], values[p.CmInventoryAvailable] = 0, 0, 0
    values[p.McStoresIrSeekerAvailable], values[p.RadarDisplaySensorAvailable] = 0, 0
    update()
    expect(display(p.FlightStatusAltitude) == "---" and display(p.FlightStatusGun) == "---", "invalid sources clear old readings")
    expect(display(p.FlightStatusIr) == "---" and display(p.FlightStatusRadar) == "---", "invalid sensor state differs from no lock")
    expect(display(p.FlightStatusFlare) == "---" and display(p.FlightStatusChaff) == "---", "invalid CM inventory clears both counts")
    for _, generation in ipairs({ 3, 0 / 0, math.huge }) do
        values[p.McGeneration] = generation
        update()
        expect(display(p.FlightStatusArm) == "---", "incomplete MC generation clears old operational text")
    end
    local reads = 0
    values[p.McGeneration] = function() reads = reads + 1; return reads == 1 and 2 or 4 end
    update()
    expect(display(p.FlightStatusMissile) == "---", "changing generation cannot publish mixed selection data")
    for name in pairs(writes) do expect(name:match("^FLIGHT_STATUS_"), "presentation writes only its own text parameters") end

    -- 在不同長寬比核對冒號固定欄位及左上角位置。
    for _, aspect in ipairs({ 4 / 3, 16 / 9, 21 / 9 }) do
        env.LockOn_Options.screen.aspect = aspect
        env.ScreenType, env.SetCustomScale = { SCREENSPACE_TRUE = 1 }, function() end
        env.MakeMaterial = function() return {} end
        env.CreateElement = function(kind) return { kind = kind } end
        local elements = {}
        env.Add = function(element) expect(not elements[element.name], "status element names are unique"); elements[element.name] = element end
        execute("Cockpit/Scripts/FlightStatus/Indicator_page.lua")
        local visibility = elements.flight_status_root
        expect(visibility.element_params[1] == p.DebugIndicatorVisible
            and visibility.controllers[1][1] == "parameter_in_range"
            and visibility.controllers[1][4] < 1, "full debug view hides the status panel")
        local colon_x, rows = nil, 0
        for name, element in pairs(elements) do
            expect(element.screenspace == 1, "status remains fixed to the screen")
            if name ~= "flight_status_root" then
                expect(element.parent_element == "flight_status_root", "all status drawing follows debug visibility")
            end
            if name:match("_value$") then
                rows = rows + 1
                colon_x = colon_x or element.init_pos[1]
                expect(element.init_pos[1] == colon_x, "all colons share the same horizontal coordinate")
                expect(element.init_pos[2] > 0 and element.init_pos[2] < 1, "status row stays in the upper viewport")
                local label = elements[name:gsub("_value$", "_label")]
                expect(label.init_pos[1] < colon_x and label.init_pos[2] == element.init_pos[2], "name precedes its aligned colon and value")
            end
        end
        expect(rows == 13, "status panel includes every intended row")
        expect(not elements.flight_status_background, "status panel leaves the outside view transparent")
        for name, item in pairs(elements) do
            if name:match("_value$") then
                expect(item.init_pos[1] > -aspect and item.init_pos[1] < 0, "status values fit the left viewport")
            end
        end
    end
end
