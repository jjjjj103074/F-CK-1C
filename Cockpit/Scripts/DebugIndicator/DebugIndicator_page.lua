dofile(LockOn_Options.common_script_path .. "elements_defs.lua")
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")

SetCustomScale(1.0)

local column_count = 2
local blocks_per_column = 3
local rows_per_block = 10
local reference_aspect = 16.0 / 9.0
local base_font_size = 0.0019
local horizontal_margin_ratio = 0.015
local vertical_margin_ratio = 0.02
local column_gap_ratio = 0.02
local inner_padding_ratio = 0.01
local status_height_ratio = 0.07
local separator_thickness_ratio = 0.001

local text_parameters = {
    cockpit_params.DebugIndicatorText1,
    cockpit_params.DebugIndicatorText2,
    cockpit_params.DebugIndicatorText3,
    cockpit_params.DebugIndicatorText4,
    cockpit_params.DebugIndicatorText5,
    cockpit_params.DebugIndicatorText6,
}

assert(#text_parameters == column_count * blocks_per_column)
assert(rows_per_block == 10)

local aspect = LockOn_Options.screen.aspect
local panel_left = -aspect + (2.0 * aspect * horizontal_margin_ratio)
local panel_right = aspect - (2.0 * aspect * horizontal_margin_ratio)
local panel_top = 1.0 - (2.0 * vertical_margin_ratio)
local panel_bottom = -1.0 + (2.0 * vertical_margin_ratio)
local panel_width = panel_right - panel_left
local panel_height = panel_top - panel_bottom
local column_gap = panel_width * column_gap_ratio
local padding = panel_width * inner_padding_ratio
local status_height = panel_height * status_height_ratio
local data_top = panel_top - status_height
local column_width = (panel_width - column_gap) / column_count
local block_height = (data_top - panel_bottom) / blocks_per_column
local font_size = base_font_size * math.min(1.0, aspect / reference_aspect)
local separator_thickness = panel_height * separator_thickness_ratio

local root = CreateElement("ceSimple")
root.name = "efm_debug_indicator_root"
root.element_params = { cockpit_params.DebugIndicatorVisible }
root.controllers = { { "parameter_in_range", 0, 0.9, 1.1 } }
root.screenspace = ScreenType.SCREENSPACE_TRUE
Add(root)

local panel_material = MakeMaterial("", { 0, 0, 0, 170 })
local separator_material = MakeMaterial("", { 70, 70, 70, 170 })

local function add_quad(name, bounds, material)
    local element = CreateElement("ceMeshPoly")
    element.name = name
    element.primitivetype = "triangles"
    element.material = material
    element.vertices = {
        { bounds.left, bounds.bottom },
        { bounds.left, bounds.top },
        { bounds.right, bounds.top },
        { bounds.right, bounds.bottom },
    }
    element.indices = default_box_indices
    element.parent_element = root.name
    element.screenspace = ScreenType.SCREENSPACE_TRUE
    Add(element)
end

local function add_text(name, parameter, position)
    local element = CreateElement("ceStringPoly")
    element.name = name
    element.material = "font_hmcs_small"
    element.value = ""
    element.alignment = "LeftTop"
    element.stringdefs = { font_size, font_size, 0.0, 0.0 }
    element.init_pos = position
    element.parent_element = root.name
    element.element_params = { parameter }
    element.controllers = { { "text_using_parameter", 0, 0 } }
    element.formats = { "%s" }
    element.screenspace = ScreenType.SCREENSPACE_TRUE
    element.use_mipfilter = true
    Add(element)
end

add_quad("efm_debug_indicator_background", {
    left = panel_left,
    right = panel_right,
    top = panel_top,
    bottom = panel_bottom,
}, panel_material)

local column_separator_x = panel_left + column_width + (column_gap / 2.0)
add_quad("efm_debug_indicator_column_separator", {
    left = column_separator_x - separator_thickness,
    right = column_separator_x + separator_thickness,
    top = data_top,
    bottom = panel_bottom,
}, separator_material)

add_quad("efm_debug_indicator_status_separator", {
    left = panel_left,
    right = panel_right,
    top = data_top + separator_thickness,
    bottom = data_top - separator_thickness,
}, separator_material)

for row = 1, blocks_per_column - 1 do
    local y = data_top - (row * block_height)
    add_quad("efm_debug_indicator_row_separator_" .. row, {
        left = panel_left,
        right = panel_right,
        top = y + separator_thickness,
        bottom = y - separator_thickness,
    }, separator_material)
end

for index, parameter in ipairs(text_parameters) do
    local column = math.floor((index - 1) / blocks_per_column)
    local row = (index - 1) % blocks_per_column
    local x = panel_left + (column * (column_width + column_gap)) + padding
    local y = data_top - (row * block_height) - padding
    add_text("efm_debug_indicator_text_" .. index, parameter, { x, y, 0.0 })
end

add_text("efm_debug_indicator_status", cockpit_params.DebugIndicatorStatus, {
    panel_left + padding,
    panel_top - padding,
    0.0,
})

need_to_be_closed = true
