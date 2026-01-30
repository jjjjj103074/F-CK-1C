local cscripts = folder .. "../../../Cockpit/Scripts/"
-- dofile(cscripts.."devices.lua")
dofile(cscripts .. "command_defs.lua")


-- 載入 DCS 通用鍵盤設定 (包含視角、系統選單等)
local res = external_profile("Config/Input/Aircrafts/common_keyboard_binding.lua")

join(res.keyCommands,
    {
        -- ============================================================
        -- 俯仰控制 (Pitch / Elevator) - 使用 上/下 箭頭
        -- ============================================================
        -- iCommandPlanePitchDown = 機頭向下 (推桿)
        -- iCommandPlanePitchUp   = 機頭向上 (拉桿)
        { combos = defaultDeviceAssignmentFor("pitch_up"),     down = iCommandPlaneUpStart,           up = iCommandPlaneUpStop,             name = _('Aircraft Pitch Down'),                           category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("pitch_down"),   down = iCommandPlaneDownStart,         up = iCommandPlaneDownStop,           name = _('Aircraft Pitch Up'),                             category = { _('Flight Control') } },

        -- ============================================================
        -- 滾轉控制 (Roll / Aileron) - 使用 左/右 箭頭
        -- ============================================================
        -- iCommandPlaneRollLeft  = 向左滾轉
        -- iCommandPlaneRollRight = 向右滾轉
        { combos = defaultDeviceAssignmentFor("roll_left"),    down = iCommandPlaneLeftStart,         up = iCommandPlaneLeftStop,           name = _('Aircraft Bank Left'),                            category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("roll_right"),   down = iCommandPlaneRightStart,        up = iCommandPlaneRightStop,          name = _('Aircraft Bank Right'),                           category = { _('Flight Control') } },

        -- ============================================================
        -- 偏航控制 (Yaw / Rudder) - 使用 Z / X 鍵
        -- ============================================================
        -- iCommandPlaneRudDistLeft  = 左舵
        -- iCommandPlaneRudDistRight = 右舵
        { combos = defaultDeviceAssignmentFor("rudder_left"),  down = iCommandPlaneLeftRudderStart,   up = iCommandPlaneLeftRudderStop,     name = _('Aircraft Rudder Left'),                          category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("rudder_right"), down = iCommandPlaneRightRudderStart,  up = iCommandPlaneRightRudderStop,    name = _('Aircraft Rudder Right'),                         category = { _('Flight Control') } },

        -- ============================================================
        -- 油門控制 (Thrust / Throttle) - 使用 PageUp / PageDown (Num+ / Num-)
        -- ============================================================
        { combos = defaultDeviceAssignmentFor("thrust_up"),    down = iCommandThrottleIncrease,       up = iCommandThrottleStop,            name = _('Throttle Smoothly - Increase'),                  category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("thrust_down"),  down = iCommandThrottleDecrease,       up = iCommandThrottleStop,            name = _('Throttle Smoothly - Decrease'),                  category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = { { key = 'PageUp' } },                     down = iCommandPlaneAUTIncreaseRegime, name = _('Throttle Step - Increase'), category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = { { key = 'PageDown' } },                   down = iCommandPlaneAUTDecreaseRegime, name = _('Throttle Step - Decrease'), category = { _('Throttle Quadrant'), _('Flight Control') } },

        -- 煞車
        { combos = defaultDeviceAssignmentFor("wheel_brake"),  down = iCommandPlaneWheelBrakeOn,      up = iCommandPlaneWheelBrakeOff,      name = _('Wheel Brake - ON/OFF'),                          category = { _('Systems') } },
        { combos = { { key = 'W', reformers = { 'LCtrl' } } }, down = iCommandPlaneWheelBrakeLeftOn,  up = iCommandPlaneWheelBrakeLeftOff,  name = _('Wheel Brake Left - ON/OFF'),                     category = { _('Systems') } },
        { combos = { { key = 'W', reformers = { 'LAlt' } } },  down = iCommandPlaneWheelBrakeRightOn, up = iCommandPlaneWheelBrakeRightOff, name = _('Wheel Brake Right - ON/OFF'),                    category = { _('Systems') } },
    }
)


return res
