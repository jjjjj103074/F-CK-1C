dofile(LockOn_Options.common_script_path .. "devices_defs.lua")

indicator_type = indicator_types.COLLIMATOR
purposes = { render_purpose.GENERAL, render_purpose.HUD_ONLY_VIEW }

id_Page = {
    MAIN = 0,
}

id_pagesubset = {
    BASE = 0,
    COMMON = 1,
}

page_subsets = {}
page_subsets[id_pagesubset.BASE] = LockOn_Options.script_path .. "HMCS/HMCS_VR_base_page.lua"
page_subsets[id_pagesubset.COMMON] = LockOn_Options.script_path .. "HMCS/HMCS_VR_page.lua"

pages = {}
pages[id_Page.MAIN] = { id_pagesubset.BASE, id_pagesubset.COMMON }

init_pageID = id_Page.MAIN

need_to_be_closed = true
