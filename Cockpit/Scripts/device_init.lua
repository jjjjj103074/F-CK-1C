dofile(LockOn_Options.script_path .. "devices.lua") -- 取得設備編號定義


-- MainPanel = {
--     "ccMainPanel",
--     LockOn_Options.script_path .. "mainpanel_init.lua",
--     {
--         --{"INTERCOM", devices.INTERCOM},
--         --{"UHF_Radio", devices.UHF_RADIO},
--         --{"OxygenSystem", devices.TEMP3},
--         --{"avSimpleElectricSystem", devices.ELECTRIC_SYSTEM}, -- DCS.log: ERROR   COCKPITBASE: devices_keeper::link_all: unable to find link target 'avSimpleElectricSystem' for device 'MAIN_PANEL'
--         --{"LightSystem", devices.TEMP1},
--         --[[
--     {
--     devices.INTERCOM, -- DCS.log: ERROR   COCKPITBASE: devices_keeper::link_all: unable to find link target '28' for device 'MAIN_PANEL'
--     devices.UHF_RADIO,
--     devices.ELECTRIC_SYSTEM,
--     },
-- --]]
--     },
-- }

-- ---------------------------------------------------------
-- 設備
-- ---------------------------------------------------------
creators = {}

-- 測試用設備
creators[devices.GEAR] = {
    "avLuaDevice",
    LockOn_Options.script_path .. "Systems/gear_system.lua",
}

-- ---------------------------------------------------------
-- 指示器
-- ---------------------------------------------------------
indicators = {}

-- 控制指示器
indicators[#indicators + 1] = {
    "ccIndicator",
    LockOn_Options.script_path .. "ControlsIndicator/ControlsIndicator.lua",
}
