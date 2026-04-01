local res = external_profile("Config/Input/Aircrafts/common_keyboard_binding.lua")
local cscripts = folder .. "../../../Cockpit/Scripts/"
dofile(cscripts .. "devices.lua")
dofile(cscripts .. "command_defs.lua")

join(res.keyCommands,
    {
        { combos = defaultDeviceAssignmentFor("pitch_up"),          down = iCommandPlaneUpStart,                   up = iCommandPlaneUpStop,                   name = _('Aircraft Pitch Down'),                                            category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("pitch_down"),        down = iCommandPlaneDownStart,                 up = iCommandPlaneDownStop,                 name = _('Aircraft Pitch Up'),                                              category = { _('Flight Control') } },

        { combos = defaultDeviceAssignmentFor("roll_left"),         down = iCommandPlaneLeftStart,                 up = iCommandPlaneLeftStop,                 name = _('Aircraft Bank Left'),                                             category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("roll_right"),        down = iCommandPlaneRightStart,                up = iCommandPlaneRightStop,                name = _('Aircraft Bank Right'),                                            category = { _('Flight Control') } },

        { combos = defaultDeviceAssignmentFor("rudder_left"),       down = iCommandPlaneLeftRudderStart,           up = iCommandPlaneLeftRudderStop,           name = _('Aircraft Rudder Left'),                                           category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("rudder_right"),      down = iCommandPlaneRightRudderStart,          up = iCommandPlaneRightRudderStop,          name = _('Aircraft Rudder Right'),                                          category = { _('Flight Control') } },

        { combos = defaultDeviceAssignmentFor("thrust_up"),         pressed = iCommandThrottleIncrease,            up = iCommandThrottleStop,                  name = _('Throttle Smoothly - Increase'),                                   category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("thrust_down"),       pressed = iCommandThrottleDecrease,            up = iCommandThrottleStop,                  name = _('Throttle Smoothly - Decrease'),                                   category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = { { key = 'PageUp' } },                          down = iCommandPlaneAUTIncreaseRegime,         name = _('Throttle Step - Increase'),       category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = { { key = 'PageDown' } },                        down = iCommandPlaneAUTDecreaseRegime,         name = _('Throttle Step - Decrease'),       category = { _('Throttle Quadrant'), _('Flight Control') } },

        { combos = defaultDeviceAssignmentFor("wheel_brake"),       down = iCommandPlaneWheelBrakeOn,              up = iCommandPlaneWheelBrakeOff,            name = _('Wheel Brake - ON/OFF'),                                           category = { _('Systems') } },
        { combos = { { key = 'W', reformers = { 'LCtrl' } } },      down = iCommandPlaneWheelBrakeLeftOn,          up = iCommandPlaneWheelBrakeLeftOff,        name = _('Wheel Brake Left - ON/OFF'),                                      category = { _('Systems') } },
        { combos = { { key = 'W', reformers = { 'LAlt' } } },       down = iCommandPlaneWheelBrakeRightOn,         up = iCommandPlaneWheelBrakeRightOff,       name = _('Wheel Brake Right - ON/OFF'),                                     category = { _('Systems') } },

        { combos = defaultDeviceAssignmentFor("plane_gear"),        down = iCommandPlaneGear,                      name = _('LG Handle - UP/DN'),              category = { _('Left Auxiliary Console') } },
        { combos = { { key = 'G', reformers = { 'LCtrl' } } },      down = iCommandPlaneGearUp,                    name = _('LG Handle - UP'),                 category = { _('Left Auxiliary Console') } },
        { combos = { { key = 'G', reformers = { 'LShift' } } },     down = iCommandPlaneGearDown,                  name = _('LG Handle - DN'),                 category = { _('Left Auxiliary Console') } },

        { combos = { { key = 'F' } },                               down = iCommandPlaneFlaps,                     name = _('Flap Handle - UP/DOWN'),          category = { _('Throttle Panel'), _('Flight Control') } },
        { combos = { { key = 'F', reformers = { 'LCtrl' } } },      down = iCommandPlaneFlapsOn,                   name = _('Flap Handle - DOWN'),             category = { _('Throttle Panel'), _('Flight Control') } },
        { combos = { { key = 'F', reformers = { 'LShift' } } },     down = iCommandPlaneFlapsOff,                  name = _('Flap Handle - UP'),               category = { _('Throttle Panel'), _('Flight Control') } },

        { combos = { { key = 'B' } },                               down = iCommandPlaneAirBrake,                  name = _('Speedbrake Switch - OPEN/CLOSE'), category = { _('Throttle Panel'), _('Throttle Grip'), _('Flight Control') } },
        { combos = { { key = 'B', reformers = { 'RCtrl' } } },      down = 3016,                                   name = _('Speedbrake - Close'),             category = { _('Throttle Panel'), _('Throttle Grip'), _('Flight Control') } },
        { combos = { { key = 'B', reformers = { 'RAlt' } } },       down = 3010,                                   name = _('Speedbrake - Hold'),              category = { _('Throttle Panel'), _('Throttle Grip'), _('Flight Control') } },
        { combos = { { key = 'B', reformers = { 'RShift' } } },     down = 3017,                                   name = _('Speedbrake - Extend'),            category = { _('Throttle Panel'), _('Throttle Grip'), _('Flight Control') } },

        { combos = { { key = 'G', reformers = { 'RCtrl' } } },      down = 3018,                                   name = _('LG Handle - UP'),                 category = { _('Left Auxiliary Console') } },
        { combos = { { key = 'G', reformers = { 'RShift' } } },     down = 3019,                                   name = _('LG Handle - DOWN'),               category = { _('Left Auxiliary Console') } },

        { combos = { { key = 'F', reformers = { 'RCtrl' } } },      down = 3020,                                   name = _('Flap Handle - UP (3-pos)'),       category = { _('Throttle Panel'), _('Flight Control') } },
        { combos = { { key = 'F', reformers = { 'RAlt' } } },       down = 3012,                                   name = _('Flap Handle - AUTO (3-pos)'),     category = { _('Throttle Panel'), _('Flight Control') } },
        { combos = { { key = 'F', reformers = { 'RShift' } } },     down = 3021,                                   name = _('Flap Handle - DOWN (3-pos)'),     category = { _('Throttle Panel'), _('Flight Control') } },

        { combos = { { key = 'N' } },                               down = 3022,                                   name = _('Nose Turn Switch - TOGGLE'),      category = { _('Systems') } },

        { combos = { { key = 'Space', reformers = { 'LAlt' } } },   down = device_commands.TriggerFirstStage,      up = device_commands.TriggerFirstStage,     cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('HOTAS Trigger - First Stage (Hold, Reserved)'),  category = { _('Weapons') } },
        { combos = { { key = 'Space' } },                           down = device_commands.TriggerSecondStage,     up = device_commands.TriggerSecondStage,    cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('HOTAS Trigger - Second Stage (Gun Fire)'),       category = { _('Weapons') } },

        { combos = { { key = 'M', reformers = { 'RCtrl' } } },      down = device_commands.MasterArmOn,            up = device_commands.MasterArmOn,           cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('Master Arm - ON (Momentary)'),                   category = { _('Weapons') } },
        { combos = { { key = 'M', reformers = { 'RShift' } } },     down = device_commands.MasterArmOff,           up = device_commands.MasterArmOff,          cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('Master Arm - OFF (Momentary)'),                  category = { _('Weapons') } },
        { combos = { { key = 'M', reformers = { 'RAlt' } } },       down = device_commands.MasterArmSim,           up = device_commands.MasterArmSim,          cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('Master Arm - SIM (Momentary)'),                  category = { _('Weapons') } },
        { combos = { { key = 'D', reformers = { 'RCtrl' } } },      down = device_commands.DogfightSwitch,         up = device_commands.DogfightSwitch,        cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('Dogfight Switch (Momentary)'),                   category = { _('Weapons') } },

        { combos = { { key = 'Delete', reformers = { 'RCtrl' } } }, down = device_commands.CMSForward,             up = device_commands.CMSForward,            cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('HOTAS CMS - Forward (Program: 10x Flare / 1s)'), category = { _('Countermeasures') }, features = { 'Countermeasures' } },
        { combos = { { key = 'Home', reformers = { 'RCtrl' } } },   down = device_commands.CMSAft,                 up = device_commands.CMSAft,                cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('HOTAS CMS - Aft (Reserved)'),                    category = { _('Countermeasures') }, features = { 'Countermeasures' } },
        { combos = { { key = 'Insert', reformers = { 'RCtrl' } } }, down = device_commands.CMSLeft,                up = device_commands.CMSLeft,               cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('HOTAS CMS - Left (Single Flare)'),               category = { _('Countermeasures') }, features = { 'Countermeasures' } },
        { combos = { { key = 'End', reformers = { 'RCtrl' } } },    down = device_commands.CMSRight,               up = device_commands.CMSRight,              cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('HOTAS CMS - Right (Reserved)'),                  category = { _('Countermeasures') }, features = { 'Countermeasures' } },
        { combos = { { key = 'PgDn', reformers = { 'RCtrl' } } },   down = device_commands.CMSPress,               up = device_commands.CMSPress,              cockpit_device_id = devices.CMS,                                            value_down = 1.0,                                          value_up = 0.0, name = _('HOTAS CMS - Press (Reserved)'),                  category = { _('Countermeasures') }, features = { 'Countermeasures' } },
        { down = iCommandPlaneFire,                                 up = iCommandPlaneFireOff,                     name = _('Gun Fire - Direct (Fallback)'),   category = { _('Weapons') } },
        { down = iCommandPlaneDropFlareOnce,                        name = _('Flare Release - Direct (Fallback)'), category = { _('Countermeasures') },        features = { 'Countermeasures' } },

        { combos = { { key = 'C', reformers = { 'RCtrl' } } },      down = 3007,                                   name = _('FBW CAT - Toggle'),               category = { _('Flight Control') } },
        { combos = { { key = '1', reformers = { 'RAlt' } } },       down = 3008,                                   name = _('FBW CAT - CAT I'),                category = { _('Flight Control') } },
        { combos = { { key = '3', reformers = { 'RAlt' } } },       down = 3009,                                   name = _('FBW CAT - CAT III'),              category = { _('Flight Control') } },
    }
)

return res
