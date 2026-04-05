dofile(LockOn_Options.script_path .. "HMCS/HMCS_VR_definitions.lua")

local hmcs_color = MakeMaterial(nil, { 80, 255, 120, 220 })
local line_width = 0.6
local text_size = { 8.0, 8.0, 0.0, 0.0 }
local hmcs_root_name = "hmcs_vr_root"

local hmcs_root = CreateElement "ceSimple"
hmcs_root.name = hmcs_root_name
hmcs_root.parent_element = "hmcs_vr_tfov"
hmcs_root.element_params = { "HMCS_ENABLED", "HMCS_DISPLAY_MODE" }
hmcs_root.controllers = {
	{ "parameter_in_range", 0, 0.9, 1.1 },
	{ "parameter_in_range", 1, 0.9, 1.1 },
}
Add(hmcs_root)

local function AddElement(object)
	if object.parent_element == nil then
		object.parent_element = hmcs_root_name
	end
	object.h_clip_relation = h_clip_relations.COMPARE
	object.level = HMCS_VR_DEFAULT_LEVEL
	object.collimated = true
	object.use_mipfilter = true
	Add(object)
end

local function add_line(name, pos, length, width, rotation_deg)
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
	elem.init_pos = { pos[1], pos[2], 0.0 }
	if rotation_deg ~= nil then
		elem.init_rot = { rotation_deg, 0.0, 0.0 }
	end
	AddElement(elem)
	return elem
end

local function add_text(name, value, pos)
	local elem = CreateElement "ceStringPoly"
	elem.name = name
	elem.material = "font_hmcs_small"
	elem.alignment = "CenterCenter"
	elem.stringdefs = text_size
	elem.init_pos = { pos[1], pos[2], 0.0 }
	elem.value = value
	AddElement(elem)
	return elem
end

add_line("hmcs_vr_reticle_h", { -4.0, 0.0 }, 8.0, line_width, 0)
add_line("hmcs_vr_reticle_v", { 0.0, -4.0 }, 8.0, line_width, 90)