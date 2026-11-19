dofile(LockOn_Options.common_script_path .. "devices_defs.lua")
indicator_type = indicator_types.COMMON
purposes = { render_purpose.SCREENSPACE_INSIDE_COCKPIT }
screenspace_scale = 4
page_subsets = { [0] = LockOn_Options.script_path .. "MCFlightTest/Indicator_page.lua" }
pages = { [0] = { 0 } }
init_pageID = 0
need_to_be_closed = true
