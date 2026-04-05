dofile(LockOn_Options.script_path .. "devices.lua")

update_time_step = 0.02
device_timer_dt = 0.01

local radar = GetSelf()

make_default_activity(update_time_step)

perfomance =
{
    roll_compensation_limits = { math.rad(-180.0), math.rad(180.0) },
    pitch_compensation_limits = { math.rad(-45.0), math.rad(45.0) },
    tracking_azimuth = { -math.rad(70.0), math.rad(70.0) },
    tracking_elevation = { -math.rad(45.0), math.rad(45.0) },
    scan_volume_azimuth = math.rad(140.0),
    scan_volume_elevation = math.rad(60.0),
    scan_beam = math.rad(60.0),
    scan_speed = math.rad(240.0),
    max_available_distance = 90000.0,
    {
        sea = { 0, 0, 0 },
        land = { 0, 2, 2 },
        artificial = { 1, 4, 4 },
        rays_density = 0.1,
        max_distance = 40000,
    }
}

function update()
    radar:set_power(true)
end

need_to_be_closed = true
