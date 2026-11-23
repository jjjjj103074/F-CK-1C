-- 重播 DCS 參數的數字轉型，核對文字格式與實際指示器字級。
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
    local values, elements = {}, {}
    env.get_param_handle = function(name)
        return {
            get = function() return values[name] or 0 end,
            set = function(_, value)
                values[name] = type(value) == "string" and (tonumber(value) or value) or value
            end,
        }
    end
    env.ScreenType = { SCREENSPACE_TRUE = 1 }
    env.SetCustomScale = function() end
    env.MakeMaterial = function(_, color) return { color = color } end
    env.CreateElement = function(kind) return { kind = kind } end
    env.Add = function(item) elements[item.name] = item end
    local p = env.cockpit_params
    local status = execute("Cockpit/Scripts/FlightStatus/Status.lua")()
    execute("Cockpit/Scripts/FlightStatus/Indicator_page.lua")

    local function validate_text()
        for _, item in pairs(elements) do
            for _, controller in ipairs(item.controllers or {}) do
                if controller[1] == "text_using_parameter" then
                    local value = values[item.element_params[controller[2] + 1]] or 0
                    local format = item.formats[controller[3] + 1]
                    expect(type(value) == "string" or not format:find("%%s"),
                        item.name .. ": numeric DCS parameter must match the native printf argument type")
                end
            end
        end
    end
    validate_text()
    values[p.McGeneration], values[p.McAvailable] = 2, 1
    values[p.McStoresGunCountAvailable], values[p.McStoresGunCount] = 1, 240
    values[p.CmInventoryAvailable], values[p.CmFlareCount], values[p.CmChaffCount] = 1, 90, 90
    for _ = 1, 4 do status.update() end
    validate_text()
    values[p.McStoresGunCount], values[p.CmFlareCount], values[p.CmChaffCount] = 0, 0, 0
    for _ = 1, 4 do status.update() end
    validate_text()
    expect(not elements.flight_status_background, "status text has a transparent background")
    for _, item in pairs(elements) do
        if item.kind == "ceStringPoly" then
            expect(item.material == "font_hmcs_small", "status uses the existing green cockpit font")
            expect(item.stringdefs[1] <= 0.0019, "status text uses the compact requested size")
        end
    end

    elements = {}
    env.default_box_indices = { 0, 1, 2, 0, 2, 3 }
    execute("Cockpit/Scripts/HMCS/HMCS_page.lua")
    local labels = 0
    for name, item in pairs(elements) do
        if name:match("^hmcs_tape_label_") then
            labels = labels + 1
            expect(item.stringdefs[1] == 0.0026 and item.stringdefs[2] == 0.0026,
                "HMCS heading labels retain their established size")
        end
    end
    expect(labels > 0, "heading size check exercises the actual HMCS tape")
end
