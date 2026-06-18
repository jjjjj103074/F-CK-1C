dofile(LockOn_Options.common_script_path .. "elements_defs.lua")

SetCustomScale(1.0)

local hmcs_color = MakeMaterial(nil, { 80, 255, 120, 220 })
local line_width = 0.0035
local thin_box_line_width = 0.0028
local text_size_numeric = { 0.0033, 0.0033, 0.0, 0.0 }
local text_size_numeric_box = { 0.0031, 0.0031, 0.0, 0.0 }
local text_size_title = { 0.0032, 0.0032, 0.0, 0.0 }
local text_size_value = { 0.0026, 0.0026, 0.0, 0.0 }
local heading_slot_spacing = 0.048
local heading_slot_count = 13
local hmcs_root_name = "hmcs_2d_root"

local hmcs_root = CreateElement "ceSimple"
hmcs_root.name = hmcs_root_name
hmcs_root.element_params = { "HMCS_ENABLED", "HMCS_DISPLAY_MODE" }
hmcs_root.controllers = {
	{ "parameter_in_range", 0, 0.9, 1.1 },
	{ "parameter_in_range", 1, -0.1, 0.1 },
}
Add(hmcs_root)

local function AddElement(object)
	if object.parent_element == nil then
		object.parent_element = hmcs_root_name
	end
	object.screenspace = ScreenType.SCREENSPACE_TRUE
	object.use_mipfilter = true
	Add(object)
end

local function add_line(name, pos, length, width, rotation_deg, parent, element_params, controllers)
	local elem = CreateElement "ceMeshPoly"
	elem.name = name
	elem.primitivetype = "triangles"
	elem.material = hmcs_color
	elem.vertices = {
		{ 0.0, -width },
		{ 0.0, width },
		{ length, width },
		{ length, -width },
	}
	elem.indices = default_box_indices
	elem.init_pos = { pos[1], pos[2], 0 }
	if rotation_deg ~= nil then
		elem.init_rot = { rotation_deg, 0, 0 }
	end
	if parent ~= nil then
		elem.parent_element = parent
	end
	if element_params ~= nil then
		elem.element_params = element_params
	end
	if controllers ~= nil then
		elem.controllers = controllers
	end
	AddElement(elem)
	return elem
end

local function add_box(name, center, width, height, box_line_width)
	local stroke_width = box_line_width or line_width
	local left = center[1] - width * 0.5
	local bottom = center[2] - height * 0.5
	add_line(name .. "_top", { left, bottom + height }, width, stroke_width, 0)
	add_line(name .. "_bottom", { left, bottom }, width, stroke_width, 0)
	add_line(name .. "_left", { left, bottom }, height, stroke_width, 90)
	add_line(name .. "_right", { left + width, bottom }, height, stroke_width, 90)
end

local function add_text(name, value, pos, alignment, material, size, parent, element_params, controllers, formats)
	local elem = CreateElement "ceStringPoly"
	elem.name = name
	elem.material = material or "font_hmcs"
	elem.alignment = alignment or "CenterCenter"
	elem.stringdefs = size or { 0.010, 0.010, 0.0, 0.0 }
	elem.init_pos = { pos[1], pos[2], 0 }
	elem.value = value
	if parent ~= nil then
		elem.parent_element = parent
	end
	if element_params ~= nil then
		elem.element_params = element_params
	end
	if controllers ~= nil then
		elem.controllers = controllers
	end
	if formats ~= nil then
		elem.formats = formats
	end
	AddElement(elem)
	return elem
end

local function add_numeric_text(name, pos, alignment, format_string, param_name, size)
	return add_text(
		name,
		"000",
		pos,
		alignment,
		"font_hmcs",
		size,
		nil,
		{ param_name },
		{ { "text_using_parameter", 0, 0 } },
		{ format_string }
	)
end

local function add_numeric_text_range(name, pos, alignment, format_string, param_name, gate_param_name, min_value, max_value, size)
	return add_text(
		name,
		"0",
		pos,
		alignment,
		"font_hmcs_small",
		size,
		nil,
		{ param_name, gate_param_name },
		{ { "text_using_parameter", 0, 0 }, { "parameter_in_range", 1, min_value, max_value } },
		{ format_string }
	)
end

local function add_param_text(name, value, pos, alignment, param_name, min_value, max_value, size)
	return add_text(
		name,
		value,
		pos,
		alignment,
		"font_hmcs_small",
		size,
		nil,
		{ param_name },
		{ { "parameter_in_range", 0, min_value, max_value } }
	)
end

local function add_dashed_circle(name_prefix, radius, dash_count, dash_length, dash_width)
	for i = 1, dash_count do
		local angle = ((i - 1) / dash_count) * 2.0 * math.pi
		local x = math.cos(angle) * radius
		local y = math.sin(angle) * radius
		local rot = math.deg(angle) + 90.0
		add_line(name_prefix .. tostring(i), { x, y }, dash_length, dash_width, rot)
	end
	end

local function add_heading_tape()
	local tape = CreateElement "ceSimple"
	tape.name = "hmcs_heading_tape"
	tape.init_pos = { 0.0, -0.275, 0 }
	tape.element_params = { "HMCS_HDG_MINOR_OFFSET" }
	tape.controllers = { { "move_left_right_using_parameter", 0, 1.0 } }
	AddElement(tape)

	local center_index = math.floor(heading_slot_count / 2)
	for i = 1, heading_slot_count do
		local slot_x = (i - center_index - 1) * heading_slot_spacing
		local tick_param = "HMCS_HDG_SLOT_TICK_" .. tostring(i)
		local label_param = "HMCS_HDG_SLOT_LABEL_" .. tostring(i)

		add_line(
			"hmcs_tape_tick_small_" .. tostring(i),
			{ slot_x, 0.0 },
			0.008,
			line_width * 0.65,
			90,
			tape.name,
			{ tick_param },
			{ { "parameter_in_range", 0, 0.9, 1.1 } }
		)
		add_line(
			"hmcs_tape_tick_med_" .. tostring(i),
			{ slot_x, 0.0 },
			0.012,
			line_width * 0.75,
			90,
			tape.name,
			{ tick_param },
			{ { "parameter_in_range", 0, 1.9, 2.1 } }
		)
		add_line(
			"hmcs_tape_tick_large_" .. tostring(i),
			{ slot_x, 0.0 },
			0.016,
			line_width * 0.95,
			90,
			tape.name,
			{ tick_param },
			{ { "parameter_in_range", 0, 2.9, 3.1 } }
		)

		local label_specs = {
			{ 1, "N" },
			{ 2, "03" },
			{ 3, "06" },
			{ 4, "E" },
			{ 5, "12" },
			{ 6, "15" },
			{ 7, "S" },
			{ 8, "21" },
			{ 9, "24" },
			{ 10, "W" },
			{ 11, "30" },
			{ 12, "33" },
		}

		for _, label_spec in ipairs(label_specs) do
			add_text(
				"hmcs_tape_label_" .. tostring(i) .. "_" .. tostring(label_spec[1]),
				label_spec[2],
				{ slot_x, 0.022 },
				"CenterCenter",
				"font_hmcs_small",
				text_size_value,
				tape.name,
				{ label_param },
				{ { "parameter_in_range", 0, label_spec[1] - 0.1, label_spec[1] + 0.1 } }
			)
		end
	end

	add_line("hmcs_tape_caret_l", { -0.012, 0.030 }, 0.012, line_width * 0.85, 45, tape.name)
	add_line("hmcs_tape_caret_r", { 0.012, 0.030 }, 0.012, line_width * 0.85, 135, tape.name)
end

add_dashed_circle("hmcs_outer_dash_", 0.39, 30, 0.052, line_width * 0.85)
add_dashed_circle("hmcs_inner_ring_", 0.055, 20, 0.015, line_width * 0.75)

add_line("hmcs_reticle_h", { -0.018, 0.0 }, 0.036, line_width, 0)
add_line("hmcs_reticle_v", { 0.0, -0.018 }, 0.036, line_width, 90)

add_box("hmcs_spd_box", { -0.29, 0.11 }, 0.10, 0.032, thin_box_line_width)
add_box("hmcs_alt_box", { 0.29, 0.11 }, 0.12, 0.032, thin_box_line_width)
add_box("hmcs_hdg_box", { 0.0, -0.325 }, 0.08, 0.030)

add_text("hmcs_spd_label", "SPD", { -0.29, 0.135 }, "CenterCenter", "font_hmcs_small", text_size_title)
add_text("hmcs_alt_label", "ALT", { 0.29, 0.135 }, "CenterCenter", "font_hmcs_small", text_size_title)
add_text("hmcs_hdg_label", "HDG", { 0.0, -0.301 }, "CenterCenter", "font_hmcs_small", text_size_title)

add_numeric_text("hmcs_speed", { -0.29, 0.11 }, "CenterCenter", "%03.0f", "HMCS_IAS_KTS", text_size_numeric_box)
add_numeric_text("hmcs_altitude", { 0.29, 0.11 }, "CenterCenter", "%05.0f", "HMCS_ALT_FT", text_size_numeric_box)
add_numeric_text("hmcs_heading", { 0.0, -0.325 }, "CenterCenter", "%03.0f", "HMCS_HDG_DEG", text_size_numeric)

add_text("hmcs_weapon_title", "WPN", { -0.255, -0.018 }, "RightCenter", "font_hmcs_small", text_size_title)
add_numeric_text_range("hmcs_weapon_aam", { -0.245, -0.048 }, "LeftCenter", "AAM-%1.0f", "HMCS_WEAPON_QTY", "HMCS_WEAPON_CLASS", 0.9, 1.1, text_size_value)
add_numeric_text_range("hmcs_weapon_ag", { -0.245, -0.048 }, "LeftCenter", "AG-%1.0f", "HMCS_WEAPON_QTY", "HMCS_WEAPON_CLASS", 1.9, 2.1, text_size_value)
add_numeric_text_range("hmcs_weapon_bomb", { -0.245, -0.048 }, "LeftCenter", "B%02.0f", "HMCS_WEAPON_QTY", "HMCS_WEAPON_CLASS", 2.9, 3.1, text_size_value)
add_numeric_text_range("hmcs_weapon_rkt", { -0.245, -0.048 }, "LeftCenter", "R%1.0f", "HMCS_WEAPON_QTY", "HMCS_WEAPON_CLASS", 3.9, 4.1, text_size_value)
add_numeric_text_range("hmcs_weapon_gun", { -0.245, -0.048 }, "LeftCenter", "GUN %03.0f", "HMCS_WEAPON_QTY", "HMCS_WEAPON_CLASS", 4.9, 5.1, text_size_value)

add_text("hmcs_master_title", "SYS", { -0.255, -0.095 }, "RightCenter", "font_hmcs_small", text_size_title)
add_param_text("hmcs_master_safe", "SAFE", { -0.245, -0.125 }, "LeftCenter", "HMCS_MASTER_MODE", -0.1, 0.1, text_size_value)
add_param_text("hmcs_master_sim", "SIM", { -0.245, -0.125 }, "LeftCenter", "HMCS_MASTER_MODE", 0.9, 1.1, text_size_value)
add_param_text("hmcs_master_arm", "ARM", { -0.245, -0.125 }, "LeftCenter", "HMCS_MASTER_MODE", 1.9, 2.1, text_size_value)

add_text("hmcs_mode_title", "MODE", { -0.255, -0.172 }, "RightCenter", "font_hmcs_small", text_size_title)
add_param_text("hmcs_mode_nav", "NAV", { -0.245, -0.202 }, "LeftCenter", "HMCS_FC_MODE", -0.1, 0.1, text_size_value)
add_param_text("hmcs_mode_dgf", "DGF", { -0.245, -0.202 }, "LeftCenter", "HMCS_FC_MODE", 0.9, 1.1, text_size_value)
add_param_text("hmcs_mode_msl", "MSL", { -0.245, -0.202 }, "LeftCenter", "HMCS_FC_MODE", 1.9, 2.1, text_size_value)

add_text("hmcs_sub_title", "SUB", { -0.255, -0.232 }, "RightCenter", "font_hmcs_small", text_size_title)
add_param_text("hmcs_sub_none", "---", { -0.245, -0.262 }, "LeftCenter", "HMCS_AAM_SUBMODE", -0.1, 0.1, text_size_value)
add_param_text("hmcs_sub_hmd", "HMD", { -0.245, -0.262 }, "LeftCenter", "HMCS_AAM_SUBMODE", 0.9, 1.1, text_size_value)
add_param_text("hmcs_sub_vs", "VS", { -0.245, -0.262 }, "LeftCenter", "HMCS_AAM_SUBMODE", 1.9, 2.1, text_size_value)
add_param_text("hmcs_sub_hud", "HUD", { -0.245, -0.262 }, "LeftCenter", "HMCS_AAM_SUBMODE", 2.9, 3.1, text_size_value)
add_param_text("hmcs_sub_bvr", "BVR", { -0.245, -0.262 }, "LeftCenter", "HMCS_AAM_SUBMODE", 3.9, 4.1, text_size_value)

add_heading_tape()
