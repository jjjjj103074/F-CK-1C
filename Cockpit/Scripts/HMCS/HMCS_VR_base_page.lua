dofile(LockOn_Options.script_path .. "HMCS/HMCS_VR_definitions.lua")

local mask_material = MakeMaterial(nil, { 0, 0, 0, 0 })
local show_masks = false
local tfov_radius = 105
local points_count = 32
local step = math.rad(360.0 / points_count)

local glass = CreateElement "ceMeshPoly"
glass.name = "hmcs_vr_glass"
glass.primitivetype = "triangles"
glass.vertices = {
	{ 0.0, 105.0 },
	{ -105.0, -105.0 },
	{ -105.0, 105.0 },
	{ 105.0, 105.0 },
	{ 105.0, -105.0 },
}
glass.indices = {
	0, 1, 2,
	0, 3, 4,
	0, 4, 1,
}
glass.init_pos = { 0.0, 0.0, -0.07 / GetScale() }
glass.material = mask_material
glass.h_clip_relation = h_clip_relations.REWRITE_LEVEL
glass.level = HMCS_VR_NOCLIP_LEVEL
glass.isdraw = true
glass.change_opacity = false
glass.isvisible = show_masks
Add(glass)

local verts = {
	{ 0.0, 0.0 },
}
for i = 1, points_count do
	verts[#verts + 1] = { tfov_radius * math.cos(i * step), tfov_radius * math.sin(i * step) }
end

local inds = {}
local index = 1
for i = 1, points_count - 1 do
	inds[index] = 0
	inds[index + 1] = i
	inds[index + 2] = i + 1
	index = index + 3
end
	inds[index] = 0
	inds[index + 1] = points_count
	inds[index + 2] = 1

local total_field_of_view = CreateElement "ceMeshPoly"
total_field_of_view.name = "hmcs_vr_tfov"
total_field_of_view.primitivetype = "triangles"
total_field_of_view.vertices = verts
total_field_of_view.indices = inds
total_field_of_view.init_pos = { 0.0, 0.0, 0.0 }
total_field_of_view.material = mask_material
total_field_of_view.h_clip_relation = h_clip_relations.INCREASE_IF_LEVEL
total_field_of_view.level = HMCS_VR_DEFAULT_LEVEL - 1
total_field_of_view.change_opacity = false
total_field_of_view.collimated = true
total_field_of_view.isvisible = show_masks
Add(total_field_of_view)