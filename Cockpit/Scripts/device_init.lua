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
local weapon_system_device_id = devices and devices.WEAPON_SYSTEM
local hmcs_device_id = devices and devices.HMCS
local aam_audio_device_id = devices and devices.AAM_AUDIO
local radar_device_id = devices and devices.RADAR
local radar_state_device_id = devices and devices.RADAR_STATE

if gear_device_id == nil then
    gear_device_id = 1
end
if actuators_device_id == nil then
    actuators_device_id = 2
end
if cms_device_id == nil then
    cms_device_id = 3
end
if weapon_system_device_id == nil then
    weapon_system_device_id = 4
end
if hmcs_device_id == nil then
    hmcs_device_id = 5
end
if aam_audio_device_id == nil then
    aam_audio_device_id = 6
end
if radar_device_id == nil then
    radar_device_id = 7
end
if radar_state_device_id == nil then
    radar_state_device_id = 8
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

creators[weapon_system_device_id] = {
    "avSimpleWeaponSystem",
    LockOn_Options.script_path .. "Systems/weapon_system.lua",
}

creators[radar_device_id] = {
    "avSimpleRadar",
    LockOn_Options.script_path .. "RADAR/FCK1C_Radar.lua",
}

creators[radar_state_device_id] = {
    "avLuaDevice",
    LockOn_Options.script_path .. "Systems/radar_state_system.lua",
}

creators[hmcs_device_id] = {
    "avLuaDevice",
    LockOn_Options.script_path .. "Systems/hmcs_system.lua",
}

creators[aam_audio_device_id] = {
    "avLuaDevice",
    LockOn_Options.script_path .. "Systems/aam_audio_system.lua",
}

indicators = {}
indicators[#indicators + 1] = { "ccControlsIndicatorBase", LockOn_Options.script_path .. "ControlsIndicator/ControlsIndicator.lua" }
indicators[#indicators + 1] = { "ccControlsIndicatorBase", LockOn_Options.script_path .. "HMCS/HMCS_init.lua" }
indicators[#indicators + 1] = { "ccIndicator", LockOn_Options.script_path .. "HMCS/HMCS_VR_init.lua" }

need_to_be_closed = true
