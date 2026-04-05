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
        { down = iCommandPlaneGear, name = _('LG Handle - UP/DN'), category = { _('Left Auxiliary Console') } },
        { down = iCommandPlaneGearUp, name = _('LG Handle - UP'), category = { _('Left Auxiliary Console') } },
        { down = iCommandPlaneGearDown, name = _('LG Handle - DN'), category = { _('Left Auxiliary Console') } },

        { down = iCommandPlaneFlaps, name = _('Flap Handle - UP/DOWN'), category = { _('Throttle Panel'), _('Flight Control') } },
        { down = iCommandPlaneFlapsOn, name = _('Flap Handle - DOWN'), category = { _('Throttle Panel'), _('Flight Control') } },
        { down = iCommandPlaneFlapsOff, name = _('Flap Handle - UP'), category = { _('Throttle Panel'), _('Flight Control') } },

        { down = iCommandPlaneAirBrake, name = _('Speedbrake Switch - OPEN/CLOSE'), category = { _('Throttle Panel'), _('Flight Control') } },
        { down = iCommandPlaneAirBrakeOn, up = iCommandPlaneAirBrakeOff, name = _('Speedbrake Switch - OPEN else CLOSE'), category = { _('Throttle Panel'), _('Flight Control') } },
        { down = iCommandPlaneAirBrakeOff, up = iCommandPlaneAirBrakeOn, name = _('Speedbrake Switch - CLOSE else OPEN'), category = { _('Throttle Panel'), _('Flight Control') } },
        { down = iCommandPlaneAirBrakeOn, name = _('Speedbrake Switch - OPEN'), category = { _('Throttle Panel'), _('Flight Control') } },
        { down = iCommandPlaneAirBrakeOff, name = _('Speedbrake Switch - CLOSE'), category = { _('Throttle Panel'), _('Flight Control') } },

        { down = iCommandPlaneWheelBrakeOn, up = iCommandPlaneWheelBrakeOff, name = _('Wheel Brake - ON/OFF'), category = { _('Systems') } },
        { down = iCommandPlaneWheelBrakeLeftOn, up = iCommandPlaneWheelBrakeLeftOff, name = _('Wheel Brake Left - ON/OFF'), category = { _('Systems') } },
        { down = iCommandPlaneWheelBrakeRightOn, up = iCommandPlaneWheelBrakeRightOff, name = _('Wheel Brake Right - ON/OFF'), category = { _('Systems') } },

        { down = device_commands.TriggerFirstStage, up = device_commands.TriggerFirstStage, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS Trigger - First Stage'), category = { _('Weapons') } },
        { down = device_commands.TriggerSecondStage, up = device_commands.TriggerSecondStage, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS Trigger - Second Stage (Gun Fire)'), category = { _('Weapons') } },

        { down = device_commands.MasterArmOn, up = device_commands.MasterArmOn, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Master Arm - ON (Momentary)'), category = { _('Weapons') } },
        { down = device_commands.MasterArmOff, up = device_commands.MasterArmOff, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Master Arm - OFF (Momentary)'), category = { _('Weapons') } },
        { down = device_commands.MasterArmSim, up = device_commands.MasterArmSim, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Master Arm - SIM (Momentary)'), category = { _('Weapons') } },
        { down = device_commands.SoundTestCycle, up = device_commands.SoundTestCycle, cockpit_device_id = devices.AAM_AUDIO, value_down = 1.0, value_up = 0.0, name = _('Audio Test - Next Sound'), category = { _('Systems') } },
        { down = device_commands.NavMode, up = device_commands.NavMode, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Fire Control Mode - NAV'), category = { _('Weapons') } },
        { down = device_commands.DogfightSwitch, up = device_commands.DogfightSwitch, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Fire Control Mode - DGFT'), category = { _('Weapons') } },
        { down = device_commands.MissileOverride, up = device_commands.MissileOverride, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Fire Control Mode - MSL OVRD'), category = { _('Weapons') } },
        { down = device_commands.MissileUncage, up = device_commands.MissileUncage, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Missile Uncage (Hold)'), category = { _('Weapons') } },
        { down = device_commands.WeaponRelease, up = device_commands.WeaponRelease, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('Weapon Release (AAM, Uncage-Gated)'), category = { _('Weapons') } },
        { down = iCommandPlaneModeBVR, name = _('Air-to-Air Mode - BVR (Direct)'), category = { _('Modes') } },
        { down = iCommandPlaneModeBore, name = _('Air-to-Air Mode - Bore (Direct)'), category = { _('Modes') } },
        { down = iCommandPlaneModeHelmet, name = _('Air-to-Air Mode - Helmet (Direct)'), category = { _('Modes') } },
        { down = iCommandPlaneModeFI0, name = _('Air-to-Air Mode - FI0 (Direct)'), category = { _('Modes') } },
        { down = iCommandPlaneChangeWeapon, name = _('Weapon Change - Direct'), category = { _('Weapons') } },
        { down = device_commands.TMSUp, up = device_commands.TMSUp, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS TMS - Up'), category = { _('Sensors') } },
        { down = device_commands.TMSDown, up = device_commands.TMSDown, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS TMS - Down'), category = { _('Sensors') } },
        { down = device_commands.TMSLeft, up = device_commands.TMSLeft, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS TMS - Left'), category = { _('Sensors') } },
        { down = device_commands.TMSRight, up = device_commands.TMSRight, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS TMS - Right'), category = { _('Sensors') } },

        { down = device_commands.CMSForward, up = device_commands.CMSForward, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Forward (10x Flare + Chaff / 1s)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = device_commands.CMSAft, up = device_commands.CMSAft, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Aft (7x Chaff / 0.5s)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = device_commands.CMSLeft, up = device_commands.CMSLeft, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Left (2x Flare + Chaff)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = device_commands.CMSRight, up = device_commands.CMSRight, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Right (Abort Program)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = device_commands.CMSPress, up = device_commands.CMSPress, cockpit_device_id = devices.CMS, value_down = 1.0, value_up = 0.0, name = _('HOTAS CMS - Press (Reserved)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { combos = {{ key = 'JOY_BTN1' }}, down = iCommandPlaneFire, up = iCommandPlaneFireOff, name = _('Gun Fire'), category = { _('Weapons') } },
        { down = iCommandPlaneFire, up = iCommandPlaneFireOff, name = _('Gun Fire - Direct (Fallback)'), category = { _('Weapons') } },
        { combos = {{ key = 'JOY_BTN2' }}, down = iCommandPlanePickleOn, up = iCommandPlanePickleOff, name = _('Weapon Release'), category = { _('Weapons') } },
        { down = iCommandPlanePickleOn, up = iCommandPlanePickleOff, name = _('Weapon Release - Direct (Pickle Fallback)'), category = { _('Weapons') } },
        { down = iCommandPlaneLaunchPermissionOverride, name = _('Launch Permission Override - Direct'), category = { _('Weapons') } },
        { down = iCommandPlaneDropSnarOnce, up = iCommandPlaneDropSnarOnceOff, name = _('Countermeasures Release - Direct (Standard)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = iCommandPlaneDropFlareOnce, name = _('Flare Release - Direct (Fallback)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },
        { down = iCommandPlaneDropChaffOnce, name = _('Chaff Release - Direct (Fallback)'), category = { _('Countermeasures') }, features = {"Countermeasures"} },

        { down = 3007, name = _('FBW CAT - Toggle'),   category = { _('Flight Control') } },
        { down = 3008, name = _('FBW CAT - CAT I'),    category = { _('Flight Control') } },
        { down = 3009, name = _('FBW CAT - CAT III'),  category = { _('Flight Control') } },
        { down = 3026, up = 3026, value_down = 1.0, value_up = 0.0, name = _('FBW G-Limiter Override (Hold)'), category = { _('Flight Control') } },
        { down = 3027, name = _('FBW G-Limiter Override (Toggle)'), category = { _('Flight Control') } },

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

