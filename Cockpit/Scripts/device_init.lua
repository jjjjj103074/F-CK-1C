attributes = {
    "cockpit_game_interface",
    "check_simplification",
}

layout = {
    { 0.0, 0.0 },
    {
        cockpit_local_point = { 3.2, 0.6, 0.0 },
    },
}

dofile(LockOn_Options.script_path .. "devices.lua")
dofile(LockOn_Options.script_path .. "materials.lua")

creators = {}
local gear_device_id = devices and devices.Gear
local actuators_device_id = devices and devices.Actuators
local cms_device_id = devices and devices.CMS

if gear_device_id == nil then
    gear_device_id = 1
end
if actuators_device_id == nil then
    actuators_device_id = 2
end
if cms_device_id == nil then
    cms_device_id = 3
end

creators[gear_device_id] = {
    "avLuaDevice",
    LockOn_Options.script_path .. "Systems/gear_system.lua",
}

creators[actuators_device_id] = {
    "avLuaDevice",
    LockOn_Options.script_path .. "Systems/actuators_system.lua",
}

creators[cms_device_id] = {
    "avLuaDevice",
    LockOn_Options.script_path .. "Systems/cms_system.lua",
}

indicators = {}
indicators[#indicators + 1] = { "ccControlsIndicatorBase", LockOn_Options.script_path .. "ControlsIndicator/ControlsIndicator.lua" }

need_to_be_closed = true
