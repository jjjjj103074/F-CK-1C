local dev = GetSelf()
local sensor_data = get_base_data()

dofile(LockOn_Options.script_path .. "argument.lua")
local Actuator = dofile(LockOn_Options.script_path .. "Systems/actuators.lua")

-- 設定更新頻率
local update_rate = 0.01
make_default_activity(update_rate)

-- actuators列表
local actuators = {}
-- 方向舵
actuators["rudder"] = Actuator:new(
    "angle_of_draw_right_rudder",
    { -1, 1 },
    function() return sensor_data:getRudderPosition() end,
    { 85, -85 }
)

function update_actuators()
    for _, act in pairs(actuators) do
        act:update()
    end
end

function update()
    update_actuators()
end
