dofile(LockOn_Options.common_script_path .. "elements_defs.lua")
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")
SetScale(MILLYRADIANS)

-- 將毫弧度參數換為原生控制器的模型位移。
local parameter_scale = GetScale()

local draw = dofile(LockOn_Options.script_path .. "RadarDisplay/Drawing.lua")({ collimated = true, clip_relation = h_clip_relations.REWRITE_LEVEL, level = 0 })
local color = MakeMaterial(nil, { 0, 255, 0, 255 })
local line_width_mrad = 2.0
local p = cockpit_params
local root = draw.node("radar_forward_root", nil, nil,
    { p.RadarDisplayAvailable, p.RadarDisplayVisible, p.RadarDisplaySensorAvailable },
    { { "parameter_in_range", 0, 0.9, 1.1 }, { "parameter_in_range", 1, 0.9, 1.1 },
      { "parameter_in_range", 2, 0.9, 1.1 } })

-- 四角框以機體準直參考視線定位，捕獲閃爍由顯示快照提供。
local bore = draw.node("radar_bore", root, nil,
    { p.RadarDisplayCueKind, p.RadarDisplayCueVisible, p.RadarDisplayCaptureCenterYMrad },
    { { "parameter_in_range", 0, 0.9, 2.1 }, { "parameter_in_range", 1, 0.9, 1.1 },
      { "move_up_down_using_parameter", 2, parameter_scale } })
for _, x in ipairs({ -1, 1 }) do
    for _, y in ipairs({ -1, 1 }) do
        local name = "radar_bore_corner_" .. x .. "_" .. y
        local corner = draw.node(name, bore, nil,
            { p.RadarDisplayBoreHalfWidthMrad, p.RadarDisplayBoreHalfHeightMrad },
            { { "move_left_right_using_parameter", 0, x * parameter_scale }, { "move_up_down_using_parameter", 1, y * parameter_scale } })
        draw.line(name .. "_horizontal", corner, 0, 0, -x * 9, 0, line_width_mrad, color)
        draw.line(name .. "_vertical", corner, 0, 0, 0, -y * 9, line_width_mrad, color)
    end
end

-- 實線方框的中心沿來源 STT 視線移動。
local target = draw.node("radar_stt", root, nil,
    { p.RadarDisplayCueKind, p.RadarDisplayCueVisible, p.RadarDisplayTargetXMrad, p.RadarDisplayTargetYMrad },
    { { "parameter_in_range", 0, 2.9, 3.1 }, { "parameter_in_range", 1, 0.9, 1.1 },
      { "move_left_right_using_parameter", 2, parameter_scale }, { "move_up_down_using_parameter", 3, parameter_scale } })
draw.box("radar_stt_box", target, 14, 14, line_width_mrad, color)
-- 圓圈顯示原生要求方向，菱形顯示原生 IR 捕獲目標。
local ir = draw.node("infrared_forward", nil, nil,
    { p.InfraredDisplayAvailable }, { { "parameter_in_range", 0, 0.9, 1.1 } })
local desired = draw.node("infrared_desired", ir, nil,
    { p.InfraredDisplayDesiredVisible, p.InfraredDisplayDesiredX, p.InfraredDisplayDesiredY },
    { { "parameter_in_range", 0, 0.9, 1.1 }, { "move_left_right_using_parameter", 1, parameter_scale },
      { "move_up_down_using_parameter", 2, parameter_scale } })
for i = 0, 23 do
    local a, b = i * math.pi / 12, (i + 1) * math.pi / 12
    draw.line("infrared_circle_" .. i, desired, math.cos(a) * 12, math.sin(a) * 12,
        math.cos(b) * 12, math.sin(b) * 12, 1.5, color)
end
local acquired = draw.node("infrared_acquired", ir, nil,
    { p.InfraredDisplayTargetVisible, p.InfraredDisplayTargetX, p.InfraredDisplayTargetY },
    { { "parameter_in_range", 0, 0.9, 1.1 }, { "move_left_right_using_parameter", 1, parameter_scale },
      { "move_up_down_using_parameter", 2, parameter_scale } })
local points = { {0, 16}, {12, 0}, {0, -16}, {-12, 0}, {0, 16} }
for i = 1, 4 do
    draw.line("infrared_diamond_" .. i, acquired, points[i][1], points[i][2],
        points[i + 1][1], points[i + 1][2], 1.8, color)
end
need_to_be_closed = true
