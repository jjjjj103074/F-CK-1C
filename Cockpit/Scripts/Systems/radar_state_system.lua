-- Preserve the existing constant-on DCS radar adapter. MC owns seeker/tone decisions.
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")
make_default_activity(0.02)
local radar_state = get_param_handle(cockpit_params.RadarState)
local radar_power_state = get_param_handle(cockpit_params.RadarPowerState)
local function publish_radar_power()
    radar_state:set(1)
    radar_power_state:set(1)
end
function post_initialize()
    publish_radar_power()
end
function update()
    publish_radar_power()
end
need_to_be_closed = false
