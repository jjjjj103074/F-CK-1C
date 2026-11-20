-- 檢查實際指示器圖元、參數控制器與狀態切換。
return function(expect)
    local function scene(path, aspect)
        local elements, by_name = {}, {}
        local env = setmetatable({}, { __index = _G })
        env.LockOn_Options = { script_path = "Cockpit/Scripts/", common_script_path = "fixture/common/", screen = { aspect = aspect } }
        env.MILLYRADIANS = 1
        env.ScreenType = { SCREENSPACE_TRUE = 1 }
        env.SetScale = function() end
        env.SetCustomScale = function() end
        env.MakeMaterial = function(_, color) return { color = color } end
        env.CreateElement = function(kind) return { kind = kind } end
        env.Add = function(element)
            expect(not by_name[element.name], "radar element name is unique: " .. element.name)
            expect(not element.parent_element or by_name[element.parent_element], "radar parent is added before its child")
            for _, parameter in ipairs(element.element_params or {}) do expect(type(parameter) == "string", "radar parameter is declared") end
            if element.vertices then
                for _, v in ipairs(element.vertices) do
                    expect(v[1] == v[1] and v[2] == v[2], "radar vertices are finite")
                end
                local a, b, c = element.vertices[1], element.vertices[2], element.vertices[3]
                expect((b[1] - a[1]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[1] - a[1]) < 0,
                    "radar mesh follows cockpit triangle winding")
            end
            by_name[element.name] = element
            elements[#elements + 1] = element
        end
        env.dofile = function(file)
            if file:match("^fixture/common/") then return end
            local chunk = assert(loadfile(file)); setfenv(chunk, env); return chunk()
        end
        env.dofile(path)
        return env, elements, by_name
    end
    local function transform(name, elements, values)
        local element = assert(elements[name], name)
        local x, y, visible, opacity = 0, 0, true, 1
        if element.parent_element then x, y, visible, opacity = transform(element.parent_element, elements, values) end
        x, y = x + ((element.init_pos or {})[1] or 0), y + ((element.init_pos or {})[2] or 0)
        for _, controller in ipairs(element.controllers or {}) do
            local value = values[element.element_params[controller[2] + 1]] or 0
            if controller[1] == "parameter_in_range" then visible = visible and value >= controller[3] and value <= controller[4] end
            if controller[1] == "move_left_right_using_parameter" then x = x + value * controller[3] end
            if controller[1] == "move_up_down_using_parameter" then y = y + value * controller[3] end
            if controller[1] == "opacity_using_parameter" then opacity = opacity * value end
        end
        return x, y, visible, opacity
    end
    local env, elements, names = scene("Cockpit/Scripts/RadarDisplay/Forward_page.lua", 16 / 9)
    local p, values = env.cockpit_params, {}
    values[p.RadarDisplayAvailable], values[p.RadarDisplayVisible], values[p.RadarDisplaySensorAvailable] = 1, 1, 1
    values[p.RadarDisplayCueKind], values[p.RadarDisplayCueVisible] = 1, 1
    values[p.RadarDisplayBoreHalfWidthMrad], values[p.RadarDisplayBoreHalfHeightMrad] = 28.8, 40.1
    local count = 0
    for _, element in ipairs(elements) do
        expect(element.collimated == true, "forward cue uses native collimation")
        if element.kind == "ceMeshPoly" then
            local _, _, visible = transform(element.name, names, values)
            if visible then count = count + 1 end
        end
    end
    expect(count == 8, "ready BORE shows exactly eight corner segments")
    values[p.RadarDisplayCueVisible] = 0
    local _, _, visible = transform("radar_bore_corner_-1_-1_horizontal", names, values)
    expect(not visible, "capture blink hides corner geometry")
    values[p.RadarDisplayCueKind], values[p.RadarDisplayCueVisible] = 3, 1
    values[p.RadarDisplayTargetXMrad], values[p.RadarDisplayTargetYMrad] = 100, -30
    local x, y, target_visible = transform("radar_stt_box_top", names, values)
    expect(target_visible and x == 100 and y == -30, "solid STT box follows source projection")
    local _, _, bore_visible = transform("radar_bore_corner_-1_-1_horizontal", names, values)
    expect(not bore_visible, "STT replaces BORE corner geometry")

    for _, aspect in ipairs({ 4 / 3, 16 / 9, 21 / 9 }) do
        env, elements, names = scene("Cockpit/Scripts/RadarDisplay/Scope_page.lua", aspect)
        p, values = env.cockpit_params, {}
        values[p.RadarDisplayAvailable], values[p.RadarDisplayVisible], values[p.RadarDisplaySensorAvailable] = 1, 1, 1
        values[p.RadarDisplaySweepVisible], values[p.RadarDisplaySweepX] = 1, 0.25
        values[p.RadarDisplayPlot01X], values[p.RadarDisplayPlot01Y], values[p.RadarDisplayPlot01Opacity] = -0.5, 0.4, 0.6
        x, y, visible, opacity = transform("radar_plot_mark_1", names, values)
        expect(visible and math.abs(opacity - 0.6) < 0.00001, "source return opacity reaches its drawn mesh")
        expect(math.abs(x - (aspect - 0.045 - 0.30 - 0.15)) < 0.00001, "scope maps azimuth to horizontal position")
        expect(math.abs(y - (-0.94 + 0.48 * 0.4)) < 0.00001, "scope maps range to vertical position")
        local _, _, hidden = transform("radar_plot_mark_2", names, values)
        expect(not hidden, "unused return slots remain hidden")
        for i = 0, 11 do expect(names["radar_sweep_dash_" .. i] ~= nil, "estimated sweep uses separated dashes") end
        local panel = names.radar_scope_background
        for _, v in ipairs(panel.vertices) do
            expect(v[1] + aspect - 0.045 - 0.30 < aspect and v[1] + aspect - 0.045 - 0.30 > -aspect,
                "radar panel fits the viewport width")
            expect(v[2] - 0.94 > -1 and v[2] - 0.94 < 1, "radar panel fits the viewport height")
        end
        values[p.RadarDisplaySensorAvailable] = 0
        local _, _, invalid_visible = transform("radar_unavailable_a", names, values)
        local _, _, points_visible = transform("radar_plot_mark_1", names, values)
        expect(invalid_visible and not points_visible, "unavailable sensor shows a cross and hides old returns")
    end
end
