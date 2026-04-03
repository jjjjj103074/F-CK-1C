local res = external_profile("Config/Input/Aircrafts/common_joystick_binding.lua")
local cscripts = folder .. "../../../Cockpit/Scripts/"
dofile(cscripts .. "devices.lua")
dofile(cscripts .. "command_defs.lua")

join(res.axisCommands,
    {
        { combos = defaultDeviceAssignmentFor("pitch"),             action = 2001, name = _('Pitch Axis'),                category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("roll"),              action = 2002, name = _('Roll Axis'),                 category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("rudder"),            action = 2003, name = _('Yaw Axis'),                  category = { _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("thrust"),            action = 2004, name = _('Throttle Axis - Both'),      category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("thrust_left"),       action = 2005, name = _('Throttle Axis - Left'),      category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("thrust_right"),      action = 2006, name = _('Throttle Axis - Right'),     category = { _('Throttle Quadrant'), _('Flight Control') } },
        { combos = defaultDeviceAssignmentFor("wheel_brake"),       action = 3023, name = _('Wheel Brake Axis - Both'), category = { _('Systems') } },
        { combos = defaultDeviceAssignmentFor("wheel_brake_left"),  action = 3024, name = _('Wheel Brake Axis - Left'), category = { _('Systems') } },
        { combos = defaultDeviceAssignmentFor("wheel_brake_right"), action = 3025, name = _('Wheel Brake Axis - Right'),category = { _('Systems') } },
    }
)

join(res.keyCommands,
    {
        { down = device_commands.TriggerFirstStage, up = device_commands.TriggerFirstStage, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS Trigger - First Stage (Hold, Reserved)'), category = { _('Weapons') } },
        { down = device_commands.TriggerSecondStage, up = device_commands.TriggerSecondStage, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS Trigger - Second Stage (Gun Fire)'), category = { _('Weapons') } },

        { down = device_commands.MasterArmOn, up = device_commands.MasterArmOn, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Master Arm - ON (Momentary)'), category = { _('Weapons') } },
        { down = device_commands.MasterArmOff, up = device_commands.MasterArmOff, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Master Arm - OFF (Momentary)'), category = { _('Weapons') } },
        { down = device_commands.MasterArmSim, up = device_commands.MasterArmSim, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Master Arm - SIM (Momentary)'), category = { _('Weapons') } },
        { down = device_commands.DogfightSwitch, up = device_commands.DogfightSwitch, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Dogfight Switch (Momentary)'), category = { _('Weapons') } },

        { down = device_commands.CMSForward, up = device_commands.CMSForward, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Forward (Program: 10x Flare / 1s)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = device_commands.CMSAft, up = device_commands.CMSAft, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Aft (Reserved)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = device_commands.CMSLeft, up = device_commands.CMSLeft, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Left (Single Flare)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = device_commands.CMSRight, up = device_commands.CMSRight, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Right (Reserved)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = device_commands.CMSPress, up = device_commands.CMSPress, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Press (Reserved)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = iCommandPlaneFire, up = iCommandPlaneFireOff, name = _('Gun Fire - Direct (Fallback)'), category = { _('Weapons') } },
        { down = iCommandPlaneDropFlareOnce, name = _('Flare Release - Direct (Fallback)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },

        { down = 3007, name = _('FBW CAT - Toggle'),   category = { _('Flight Control') } },
        { down = 3008, name = _('FBW CAT - CAT I'),    category = { _('Flight Control') } },
        { down = 3009, name = _('FBW CAT - CAT III'),  category = { _('Flight Control') } },
        { down = 3026, up = 3026, value_down = 1.0, value_up = 0.0, name = _('FBW G-Limiter Override (Hold)'), category = { _('Flight Control') } },

        { down = 3016, name = _('Speedbrake 3-Pos - Close'),      category = { _('Throttle Panel'), _('Flight Control') } },
        { down = 3010, name = _('Speedbrake 3-Pos - Hold'),       category = { _('Throttle Panel'), _('Flight Control') } },
        { down = 3017, name = _('Speedbrake 3-Pos - Extend'),     category = { _('Throttle Panel'), _('Flight Control') } },

        { down = 3018, name = _('LG Handle 3-Pos - UP'),          category = { _('Left Auxiliary Console') } },
        { down = 3019, name = _('LG Handle 3-Pos - DOWN'),        category = { _('Left Auxiliary Console') } },

        { down = 3020, name = _('Flap 3-Pos - UP'),               category = { _('Throttle Panel'), _('Flight Control') } },
        { down = 3012, name = _('Flap 3-Pos - AUTO'),             category = { _('Throttle Panel'), _('Flight Control') } },
        { down = 3021, name = _('Flap 3-Pos - DOWN'),             category = { _('Throttle Panel'), _('Flight Control') } },

        { down = 3022, name = _('Nose Wheel Steering - Toggle'),  category = { _('Systems') } },

    }
)

return res

