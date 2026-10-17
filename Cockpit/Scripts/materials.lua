dofile(LockOn_Options.common_script_path .. "Fonts/fonts_cmn.lua")

dbg_drawStrokesAsWire = false

-- 材質定義
materials = {}

materials["INDICATION_COMMON_RED"] = { 255, 0, 0, 255 }
materials["INDICATION_COMMON_WHITE"] = { 255, 255, 255, 255 }
materials["MASK_MATERIAL"] = { 255, 0, 255, 50 }

-- 貼圖定義
textures = {}

textures["ARCADE"] = { "arcade.tga", materials["INDICATION_COMMON_RED"] } -- Control Indicator
textures["ARCADE_WHITE"] = { "arcade.tga", materials["INDICATION_COMMON_WHITE"] } -- Control Indicator

-- 字體定義
fonts = {}

-- 測試字體
fonts["font_kneeboard"] = { fontdescription_cmn["font_general_loc"], 10, { 0, 0, 0, 255 } }
fonts["font_hmcs"] = { fontdescription_cmn["font_general_loc"], 6, { 80, 255, 120, 255 } }
fonts["font_hmcs_small"] = { fontdescription_cmn["font_general_loc"], 5, { 80, 255, 120, 255 } }
