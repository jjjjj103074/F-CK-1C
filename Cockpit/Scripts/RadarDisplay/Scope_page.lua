dofile(LockOn_Options.common_script_path .. "elements_defs.lua")
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")
SetCustomScale(1.0)

local draw = dofile(LockOn_Options.script_path .. "RadarDisplay/Drawing.lua")({ screenspace = true, collimated = false })
local p = cockpit_params
local green = MakeMaterial(nil, { 80, 255, 120, 245 })
local dim = MakeMaterial(nil, { 40, 150, 75, 130 })
local white = MakeMaterial(nil, { 230, 255, 230, 245 })
local warning = MakeMaterial(nil, { 255, 100, 70, 240 })
local width, height = 0.60, 0.48
local right, bottom = LockOn_Options.screen.aspect - 0.045, -0.94
local center = right - width / 2
local root = draw.node("radar_scope_root", nil, { center, bottom, 0 },
    { p.RadarDisplayAvailable, p.RadarDisplayVisible },
    { { "parameter_in_range", 0, 0.9, 1.1 }, { "parameter_in_range", 1, 0.9, 1.1 } })

local background = CreateElement("ceMeshPoly")
background.name = "radar_scope_background"
background.parent_element = root
background.primitivetype = "triangles"
background.material = MakeMaterial(nil, { 0, 12, 4, 175 })
background.vertices = { { -width / 2 - 0.025, -0.035 }, { -width / 2 - 0.025, height + 0.055 },
    { width / 2 + 0.025, height + 0.055 }, { width / 2 + 0.025, -0.035 } }
background.indices = { 0, 1, 2, 0, 2, 3 }
background.screenspace = ScreenType.SCREENSPACE_TRUE
Add(background)

for i = 0, 4 do
    local y = height * i / 4
    draw.line("radar_grid_h_" .. i, root, -width / 2, y, width / 2, y, 0.0012, dim)
end
for i = 0, 4 do
    local x = width * (i / 4 - 0.5)
    draw.line("radar_grid_v_" .. i, root, x, 0, x, height, 0.0012, dim)
end
draw.text("radar_range_label", root, "", { -width / 2, height + 0.04, 0 }, 0.0023,
    "font_hmcs_small", p.RadarDisplayRangeScaleNm, "%.0f NM")
draw.text("radar_az_left", root, "-60", { -width / 2, -0.012, 0 }, 0.0018, "font_hmcs_small")
draw.text("radar_az_center", root, "0", { -0.006, -0.012, 0 }, 0.0018, "font_hmcs_small")
draw.text("radar_az_right", root, "+60", { width / 2 - 0.035, -0.012, 0 }, 0.0018, "font_hmcs_small")

local live = draw.node("radar_scope_live", root, nil, { p.RadarDisplaySensorAvailable },
    { { "parameter_in_range", 0, 0.9, 1.1 } })
for _, edge in ipairs({ { "left", p.RadarDisplayScanLeft }, { "right", p.RadarDisplayScanRight } }) do
    local parent = draw.node("radar_scan_" .. edge[1], live, nil, { edge[2] },
        { { "move_left_right_using_parameter", 0, width / 2 } })
    draw.line("radar_scan_line_" .. edge[1], parent, 0, 0, 0, height, 0.0025, green)
end

-- 虛線代表估算掃描進度，回波使用來源量測位置。
local sweep = draw.node("radar_estimated_sweep", live, nil,
    { p.RadarDisplaySweepVisible, p.RadarDisplaySweepX },
    { { "parameter_in_range", 0, 0.9, 1.1 }, { "move_left_right_using_parameter", 1, width / 2 } })
for i = 0, 11 do
    local y = height * i / 12
    draw.line("radar_sweep_dash_" .. i, sweep, 0, y, 0, y + height / 24, 0.002, green)
end

local cursor = draw.node("radar_cursor", live, nil,
    { p.RadarDisplayCursorVisible, p.RadarDisplayCursorX, p.RadarDisplayCursorY },
    { { "parameter_in_range", 0, 0.9, 1.1 }, { "move_left_right_using_parameter", 1, width / 2 },
      { "move_up_down_using_parameter", 2, height } })
for _, side in ipairs({ -1, 1 }) do
    local x = side * 0.012
    draw.line("radar_cursor_side_" .. side, cursor, x, -0.014, x, 0.014, 0.0025, white)
    draw.line("radar_cursor_top_" .. side, cursor, x, 0.014, x - side * 0.006, 0.014, 0.0025, white)
    draw.line("radar_cursor_bottom_" .. side, cursor, x, -0.014, x - side * 0.006, -0.014, 0.0025, white)
end

for i = 1, 32 do
    local prefix = "RadarDisplayPlot" .. string.format("%02d", i)
    local opacity = p[prefix .. "Opacity"]
    local point = draw.node("radar_plot_" .. i, live, nil,
        { p[prefix .. "X"], p[prefix .. "Y"], opacity },
        { { "move_left_right_using_parameter", 0, width / 2 }, { "move_up_down_using_parameter", 1, height },
          { "parameter_in_range", 2, 0.001, 1.001 } })
    draw.line("radar_plot_mark_" .. i, point, -0.008, 0, 0.008, 0, 0.004, green,
        { opacity }, { { "opacity_using_parameter", 0 } })
end

local target = draw.node("radar_scope_stt", live, nil,
    { p.RadarDisplayTargetVisible, p.RadarDisplayTargetX, p.RadarDisplayTargetY },
    { { "parameter_in_range", 0, 0.9, 1.1 }, { "move_left_right_using_parameter", 1, width / 2 },
      { "move_up_down_using_parameter", 2, height } })
draw.box("radar_scope_stt_box", target, 0.014, 0.014, 0.003, green)

local unavailable = draw.node("radar_scope_unavailable", root, { 0, height / 2, 0 },
    { p.RadarDisplaySensorAvailable }, { { "parameter_in_range", 0, -0.1, 0.1 } })
draw.line("radar_unavailable_a", unavailable, -0.04, -0.04, 0.04, 0.04, 0.004, warning)
draw.line("radar_unavailable_b", unavailable, -0.04, 0.04, 0.04, -0.04, 0.004, warning)
need_to_be_closed = true
