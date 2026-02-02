-- local cscripts = folder .. "../../../Cockpit/Scripts/"
-- dofile(cscripts.."devices.lua")
-- dofile(cscripts .. "command_defs.lua")


-- 載入 DCS 通用鍵盤設定 (包含視角、系統選單等)
local res = external_profile("Config/Input/Aircrafts/common_keyboard_binding.lua")

join(res.keyCommands,
    {
        -- ============================================================
        -- 俯仰控制 (Pitch / Elevator) - 使用 上/下 箭頭
        -- ============================================================
        -- iCommandPlanePitchDown = 機頭向下 (推桿)
        -- iCommandPlanePitchUp   = 機頭向上 (拉桿)
        { combos = defaultDeviceAssignmentFor("pitch_up"),      down = iCommandPlaneUpStart,           up = iCommandPlaneUpStop,             name = _('Aircraft Pitch Down'),                           category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("pitch_down"),    down = iCommandPlaneDownStart,         up = iCommandPlaneDownStop,           name = _('Aircraft Pitch Up'),                             category = { _('Flight Control') } },

        -- ============================================================
        -- 滾轉控制 (Roll / Aileron) - 使用 左/右 箭頭
        -- ============================================================
        -- iCommandPlaneRollLeft  = 向左滾轉
        -- iCommandPlaneRollRight = 向右滾轉
        { combos = defaultDeviceAssignmentFor("roll_left"),     down = iCommandPlaneLeftStart,         up = iCommandPlaneLeftStop,           name = _('Aircraft Bank Left'),                            category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("roll_right"),    down = iCommandPlaneRightStart,        up = iCommandPlaneRightStop,          name = _('Aircraft Bank Right'),                           category = { _('Flight Control') } },

        -- ============================================================
        -- 偏航控制 (Yaw / Rudder) - 使用 Z / X 鍵
        -- ============================================================
        -- iCommandPlaneRudDistLeft  = 左舵
        -- iCommandPlaneRudDistRight = 右舵
        { combos = defaultDeviceAssignmentFor("rudder_left"),   down = iCommandPlaneLeftRudderStart,   up = iCommandPlaneLeftRudderStop,     name = _('Aircraft Rudder Left'),                          category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("rudder_right"),  down = iCommandPlaneRightRudderStart,  up = iCommandPlaneRightRudderStop,    name = _('Aircraft Rudder Right'),                         category = { _('Flight Control') } },

        -- ============================================================
        -- 油門控制 (Thrust / Throttle) - 使用 PageUp / PageDown (Num+ / Num-)
        -- ============================================================
        { combos = defaultDeviceAssignmentFor("thrust_up"),     pressed = iCommandThrottleIncrease,    up = iCommandThrottleStop,            name = _('Throttle Smoothly - Increase'),                  category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("thrust_down"),   pressed = iCommandThrottleDecrease,    up = iCommandThrottleStop,            name = _('Throttle Smoothly - Decrease'),                  category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = { { key = 'PageUp' } },                      down = iCommandPlaneAUTIncreaseRegime, name = _('Throttle Step - Increase'), category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = { { key = 'PageDown' } },                    down = iCommandPlaneAUTDecreaseRegime, name = _('Throttle Step - Decrease'), category = { _('Throttle Quadrant'), _('Flight Control') } },

        -- ============================================================
        -- 煞車系統 (Wheel Brakes)
        -- ============================================================
        { combos = defaultDeviceAssignmentFor("wheel_brake"),   down = iCommandPlaneWheelBrakeOn,      up = iCommandPlaneWheelBrakeOff,      name = _('Wheel Brake - ON/OFF'),                          category = { _('Systems') } },
        { combos = { { key = 'W', reformers = { 'LCtrl' } } },  down = iCommandPlaneWheelBrakeLeftOn,  up = iCommandPlaneWheelBrakeLeftOff,  name = _('Wheel Brake Left - ON/OFF'),                     category = { _('Systems') } },
        { combos = { { key = 'W', reformers = { 'LAlt' } } },   down = iCommandPlaneWheelBrakeRightOn, up = iCommandPlaneWheelBrakeRightOff, name = _('Wheel Brake Right - ON/OFF'),                    category = { _('Systems') } },

        -- ============================================================
        -- 起落架系統 (Landing Gear)
        -- ============================================================
        { combos = defaultDeviceAssignmentFor("plane_gear"),    down = iCommandPlaneGear,              name = _('LG Handle - UP/DN'),        category = { _('Left Auxiliary Console') } },
        { combos = { { key = 'G', reformers = { 'LCtrl' } } },  down = iCommandPlaneGearUp,            name = _('LG Handle - UP'),           category = { _('Left Auxiliary Console') } },
        { combos = { { key = 'G', reformers = { 'LShift' } } }, down = iCommandPlaneGearDown,          name = _('LG Handle - DN'),           category = { _('Left Auxiliary Console') } },

        -- ============================================================
        -- 襟翼系統 (Flaps)
        -- ============================================================
        { combos = { { key = 'F' } },                           down = iCommandPlaneFlaps,             name = _('Flap Handle - UP/DOWN'),    category = { _('Throttle Panel'), _('Flight Control') } },
        { combos = { { key = 'F', reformers = { 'LCtrl' } } },  down = iCommandPlaneFlapsOn,           name = _('Flap Handle - DOWN'),       category = { _('Throttle Panel'), _('Flight Control') } },
        { combos = { { key = 'F', reformers = { 'LShift' } } }, down = iCommandPlaneFlapsOff,          name = _('Flap Handle - UP'),         category = { _('Throttle Panel'), _('Flight Control') } },

        -- -- ============================================================
        -- -- 座艙蓋系統 (Canopy)
        -- -- ============================================================
        -- { combos = { { key = 'C', reformers = { 'LCtrl' } } },  down = iCommandPlaneCanopyOpenClose,   name = _('Canopy - Open/Close'),      category = { _('Systems') } },
        -- { combos = { { key = 'C', reformers = { 'LShift' } } }, down = iCommandPlaneCanopyOpen,        name = _('Canopy - Open'),            category = { _('Systems') } },
        -- { combos = { { key = 'C', reformers = { 'LAlt' } } },   down = iCommandPlaneCanopyClose,       name = _('Canopy - Close'),           category = { _('Systems') } },

        -- -- ============================================================
        -- -- 尾鉤系統 (Arresting Hook)
        -- -- ============================================================
        -- { combos = { { key = 'H' } },                              down = iCommandPlaneHook,              name = _('Tail Hook - Up/Down'),      category = { _('Systems') } },

        -- -- ============================================================
        -- -- 引擎控制 (Engine)
        -- -- ============================================================
        -- { combos = { { key = 'Home', reformers = { 'RShift' } } }, down = iCommandPlaneEngineStart,       name = _('Engine - Start'),           category = { _('Systems') } },
        -- { combos = { { key = 'End', reformers = { 'RShift' } } },  down = iCommandPlaneEngineStop,        name = _('Engine - Stop'),            category = { _('Systems') } },
    }
)


return res
