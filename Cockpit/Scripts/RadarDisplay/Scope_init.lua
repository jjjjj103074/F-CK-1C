dofile(LockOn_Options.common_script_path .. "devices_defs.lua")

indicator_type = indicator_types.COMMON
purposes = { render_purpose.SCREENSPACE_INSIDE_COCKPIT, render_purpose.HUD_ONLY_VIEW }
screenspace_scale = 1
init_pageID = 1
page_subsets = { [1] = LockOn_Options.script_path .. "RadarDisplay/Scope_page.lua" }
pages = { [1] = { 1 } }
need_to_be_closed = true
