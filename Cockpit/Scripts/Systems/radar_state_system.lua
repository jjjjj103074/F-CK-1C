-- 維持原生交流／直流供電，發布雷達實際供電觀測。
dofile(LockOn_Options.script_path .. "devices.lua")
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")
make_default_activity(0.02)

local radar_state = get_param_handle(cockpit_params.RadarState)
local radar_power_state = get_param_handle(cockpit_params.RadarPowerState)

local function update_power()
    local electric = GetDevice(devices.ELECTRIC_SYSTEM)
    if electric then
        electric:AC_Generator_1_on(true)
        electric:AC_Generator_2_on(true)
        electric:DC_Battery_on(true)
    end

    local radar = GetDevice(devices.RADAR)
    local powered = radar and radar:get_power() or false
    local value = powered and 1 or 0
    radar_state:set(value)
    radar_power_state:set(value)
end

function post_initialize()
    update_power()
end

function update()
    update_power()
end

need_to_be_closed = false
