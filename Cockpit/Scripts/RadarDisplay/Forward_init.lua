dofile(LockOn_Options.common_script_path .. "devices_defs.lua")

indicator_type = indicator_types.COLLIMATOR
purposes = { render_purpose.GENERAL, render_purpose.HUD_ONLY_VIEW }
init_pageID = 1
page_subsets = { [1] = LockOn_Options.script_path .. "RadarDisplay/Forward_page.lua" }
pages = { [1] = { 1 } }
collimator_default_distance_factor = { 0.15, 0, 0 }
need_to_be_closed = true
