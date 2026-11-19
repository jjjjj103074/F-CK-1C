local res = external_profile("Config/Input/Aircrafts/common_keyboard_binding.lua")
local cscripts = folder .. "../../../Cockpit/Scripts/"
dofile(cscripts .. "devices.lua")
dofile(cscripts .. "command_defs.lua")

join(res.keyCommands, {
    { down = device_commands.DebugIndicatorToggle, value_down = 1.0, name = _("[Developer] EFM Debug Indicator - Toggle"), category = { _("Developer Tools") } },
    { combos = defaultDeviceAssignmentFor("pitch_up"), down = iCommandPlaneUpStart, up = iCommandPlaneUpStop, name = _("Aircraft Pitch Down"), category = { _("Flight Control") } },
    { combos = defaultDeviceAssignmentFor("pitch_down"), down = iCommandPlaneDownStart, up = iCommandPlaneDownStop, name = _("Aircraft Pitch Up"), category = { _("Flight Control") } },

    { combos = defaultDeviceAssignmentFor("roll_left"), down = iCommandPlaneLeftStart, up = iCommandPlaneLeftStop, name = _("Aircraft Bank Left"), category = { _("Flight Control") } },
    { combos = defaultDeviceAssignmentFor("roll_right"), down = iCommandPlaneRightStart, up = iCommandPlaneRightStop, name = _("Aircraft Bank Right"), category = { _("Flight Control") } },

    { combos = defaultDeviceAssignmentFor("rudder_left"), down = iCommandPlaneLeftRudderStart, up = iCommandPlaneLeftRudderStop, name = _("Aircraft Rudder Left"), category = { _("Flight Control") } },
    { combos = defaultDeviceAssignmentFor("rudder_right"), down = iCommandPlaneRightRudderStart, up = iCommandPlaneRightRudderStop, name = _("Aircraft Rudder Right"), category = { _("Flight Control") } },

    { combos = defaultDeviceAssignmentFor("thrust_up"), pressed = iCommandThrottleIncrease, up = iCommandThrottleStop, name = _("Throttle Smoothly - Increase"), category = { _("Throttle Quadrant"), _("Flight Control") } },
    { combos = defaultDeviceAssignmentFor("thrust_down"), pressed = iCommandThrottleDecrease, up = iCommandThrottleStop, name = _("Throttle Smoothly - Decrease"), category = { _("Throttle Quadrant"), _("Flight Control") } },
    { combos = { { key = "PageUp" } }, pressed = iCommandThrottleIncrease, up = iCommandThrottleStop, name = _("Throttle Smoothly - Increase"), category = { _("Throttle Quadrant"), _("Flight Control") } },
    { combos = { { key = "PageDown" } }, pressed = iCommandThrottleDecrease, up = iCommandThrottleStop, name = _("Throttle Smoothly - Decrease"), category = { _("Throttle Quadrant"), _("Flight Control") } },
    { combos = { { key = "PageUp", reformers = { "RCtrl" } } }, down = iCommandThrottleIncrease, name = _("Throttle Step - Increase"), category = { _("Throttle Quadrant"), _("Flight Control") } },
    { combos = { { key = "PageDown", reformers = { "RCtrl" } } }, down = iCommandThrottleDecrease, name = _("Throttle Step - Decrease"), category = { _("Throttle Quadrant"), _("Flight Control") } },

    { combos = defaultDeviceAssignmentFor("wheel_brake"), down = iCommandPlaneWheelBrakeOn, up = iCommandPlaneWheelBrakeOff, name = _("Wheel Brake - ON/OFF"), category = { _("Systems") } },
    { combos = { { key = "W", reformers = { "LCtrl" } } }, down = iCommandPlaneWheelBrakeLeftOn, up = iCommandPlaneWheelBrakeLeftOff, name = _("Wheel Brake Left - ON/OFF"), category = { _("Systems") } },
    { combos = { { key = "W", reformers = { "LAlt" } } }, down = iCommandPlaneWheelBrakeRightOn, up = iCommandPlaneWheelBrakeRightOff, name = _("Wheel Brake Right - ON/OFF"), category = { _("Systems") } },

    { combos = defaultDeviceAssignmentFor("plane_gear"), down = iCommandPlaneGear, name = _("LG Handle - UP/DN"), category = { _("Left Auxiliary Console") } },
    { combos = { { key = "G", reformers = { "LCtrl" } } }, down = iCommandPlaneGearUp, name = _("LG Handle - UP"), category = { _("Left Auxiliary Console") } },
    { combos = { { key = "G", reformers = { "LShift" } } }, down = iCommandPlaneGearDown, name = _("LG Handle - DN"), category = { _("Left Auxiliary Console") } },

    { combos = { { key = "F" } }, down = iCommandPlaneFlaps, name = _("Flap Handle - UP/DOWN"), category = { _("Throttle Panel"), _("Flight Control") } },
    { combos = { { key = "F", reformers = { "LCtrl" } } }, down = iCommandPlaneFlapsOn, name = _("Flap Handle - DOWN"), category = { _("Throttle Panel"), _("Flight Control") } },
    { combos = { { key = "F", reformers = { "LShift" } } }, down = iCommandPlaneFlapsOff, name = _("Flap Handle - UP"), category = { _("Throttle Panel"), _("Flight Control") } },

    { combos = { { key = "B" } }, down = iCommandPlaneAirBrake, name = _("Speedbrake Switch - OPEN/CLOSE"), category = { _("Throttle Panel"), _("Throttle Grip"), _("Flight Control") } },
    { combos = { { key = "B", reformers = { "RCtrl" } } }, down = device_commands.AirBrakesUp, name = _("Speedbrake 3-Pos - Close"), category = { _("Throttle Panel"), _("Throttle Grip"), _("Flight Control") } },
    { combos = { { key = "B", reformers = { "RAlt" } } }, down = device_commands.AirBrakesAuto, name = _("Speedbrake 3-Pos - Hold"), category = { _("Throttle Panel"), _("Throttle Grip"), _("Flight Control") } },
    { combos = { { key = "B", reformers = { "RShift" } } }, down = device_commands.AirBrakesDown, name = _("Speedbrake 3-Pos - Extend"), category = { _("Throttle Panel"), _("Throttle Grip"), _("Flight Control") } },

    { combos = { { key = "F", reformers = { "RCtrl" } } }, down = device_commands.FlapsUpCmd, name = _("Flap 3-Pos - UP"), category = { _("Throttle Panel"), _("Flight Control") } },
    { combos = { { key = "F", reformers = { "RAlt" } } }, down = device_commands.FlapsAuto, name = _("Flap 3-Pos - AUTO"), category = { _("Throttle Panel"), _("Flight Control") } },
    { combos = { { key = "F", reformers = { "RShift" } } }, down = device_commands.FlapsDownCmd, name = _("Flap 3-Pos - DOWN"), category = { _("Throttle Panel"), _("Flight Control") } },

    { combos = { { key = "N" } }, down = device_commands.NoseTurnToggle, name = _("Nose Wheel Steering - Toggle"), category = { _("Systems") } },
    { combos = { { key = "N", reformers = { "LCtrl" } } }, down = device_commands.NoseTurnUp, name = _("Nose Wheel Steering - OFF"), category = { _("Systems") } },
    { combos = { { key = "N", reformers = { "LShift" } } }, down = device_commands.NoseTurnDown, name = _("Nose Wheel Steering - ON"), category = { _("Systems") } },

    { combos = { { key = "Space", reformers = { "LAlt" } } }, down = device_commands.TriggerFirstStage, up = device_commands.TriggerFirstStage, value_down = 1.0, value_up = 0.0, name = _("HOTAS Trigger - First Stage"), category = { _("Weapons") } },
    { combos = { { key = "Space" } }, down = device_commands.TriggerSecondStage, up = device_commands.TriggerSecondStage, value_down = 1.0, value_up = 0.0, name = _("HOTAS Trigger - Second Stage (Gun Fire)"), category = { _("Weapons") } },

    { combos = { { key = "M", reformers = { "RCtrl" } } }, down = device_commands.MasterArmOn, up = device_commands.MasterArmOn, value_down = 1.0, value_up = 0.0, name = _("Master Arm - ON (Momentary)"), category = { _("Weapons") } },
    { combos = { { key = "M", reformers = { "RShift" } } }, down = device_commands.MasterArmOff, up = device_commands.MasterArmOff, value_down = 1.0, value_up = 0.0, name = _("Master Arm - OFF (Momentary)"), category = { _("Weapons") } },
    { combos = { { key = "M", reformers = { "RAlt" } } }, down = device_commands.MasterArmSim, up = device_commands.MasterArmSim, value_down = 1.0, value_up = 0.0, name = _("Master Arm - SIM (Momentary)"), category = { _("Weapons") } },
    { combos = { { key = "T", reformers = { "RShift" } } }, down = device_commands.SoundTestCycle, up = device_commands.SoundTestCycle, cockpit_device_id = devices.AAM_AUDIO, value_down = 1.0, value_up = 0.0, name = _("[Developer] Audio Test - Next Sound"), category = { _("Developer Tools") } },
    { combos = { { key = "1" } }, down = device_commands.NavMode, up = device_commands.NavMode, value_down = 1.0, value_up = 0.0, name = _("Master Mode - NAV"), category = { _("Modes") } },
    { combos = { { key = "D", reformers = { "RCtrl" } } }, down = device_commands.DogfightSwitch, up = device_commands.DogfightSwitch, value_down = 1.0, value_up = 0.0, name = _("Dogfight Switch - DGFT"), category = { _("Modes") } },
    { combos = { { key = "D", reformers = { "RAlt" } } }, down = device_commands.MissileOverride, up = device_commands.MissileOverride, value_down = 1.0, value_up = 0.0, name = _("Dogfight Switch - MSL OVRD"), category = { _("Modes") } },
    { combos = { { key = "C" } }, down = device_commands.MissileUncage, value_down = 1.0, name = _("Missile Cage / Uncage - Toggle"), category = { _("Weapons") } },
    { combos = { { key = "2" } }, down = device_commands.AirToAirMode, up = device_commands.AirToAirMode, value_down = 1.0, value_up = 0.0, name = _("Master Mode Button - A-A (Toggle to NAV)"), category = { _("Modes") } },
    { combos = { { key = "3" } }, down = device_commands.AirToGroundMode, up = device_commands.AirToGroundMode, value_down = 1.0, value_up = 0.0, name = _("Master Mode Button - A-G (Toggle to NAV)"), category = { _("Modes") } },
    { combos = { { key = "D", reformers = { "RShift" } } }, down = device_commands.DogfightCenter, up = device_commands.DogfightCenter, value_down = 1.0, value_up = 0.0, name = _("Dogfight Switch - CENTER (Restore Master Mode)"), category = { _("Modes") } },
    { combos = { { key = "Q" } }, down = device_commands.MissileStep, up = device_commands.MissileStep, value_down = 1.0, value_up = 0.0, name = _("MSL STEP - Station (Short) / Missile Type (Hold)"), category = { _("Weapons") } },
    { down = device_commands.WeaponRelease, up = device_commands.WeaponRelease, value_down = 1.0, value_up = 0.0, name = _("Weapon Release (AAM, Uncage-Gated)"), category = { _("Weapons") } },
    { combos = { { key = "O" } }, down = iCommandPlaneEOSOnOff, name = _("[Developer] IRST Test - Toggle"), category = { _("Developer Tools") } },
    { combos = { { key = "I" } }, down = device_commands.TMSUp, up = device_commands.TMSUp, value_down = 1.0, value_up = 0.0, name = _("HOTAS TMS - Up"), category = { _("Sensors") } },
    { combos = { { key = "K" } }, down = device_commands.TMSDown, up = device_commands.TMSDown, value_down = 1.0, value_up = 0.0, name = _("HOTAS TMS - Down"), category = { _("Sensors") } },
    { combos = { { key = "J" } }, down = device_commands.TMSLeft, up = device_commands.TMSLeft, value_down = 1.0, value_up = 0.0, name = _("HOTAS TMS - Left"), category = { _("Sensors") } },
    { combos = { { key = "L" } }, down = device_commands.TMSRight, up = device_commands.TMSRight, value_down = 1.0, value_up = 0.0, name = _("HOTAS TMS - Right"), category = { _("Sensors") } },
    { combos = { { key = "I", reformers = { "RShift" } } }, down = device_commands.DMSUp, up = device_commands.DMSUp, value_down = 1.0, value_up = 0.0, name = _("HOTAS DMS - Up"), category = { _("Displays") } },
    { combos = { { key = "K", reformers = { "RShift" } } }, down = device_commands.DMSDown, up = device_commands.DMSDown, value_down = 1.0, value_up = 0.0, name = _("HOTAS DMS - Down"), category = { _("Displays") } },
    { combos = { { key = "J", reformers = { "RShift" } } }, down = device_commands.DMSLeft, up = device_commands.DMSLeft, value_down = 1.0, value_up = 0.0, name = _("HOTAS DMS - Left"), category = { _("Displays") } },
    { combos = { { key = "L", reformers = { "RShift" } } }, down = device_commands.DMSRight, up = device_commands.DMSRight, value_down = 1.0, value_up = 0.0, name = _("HOTAS DMS - Right"), category = { _("Displays") } },

    { combos = { { key = "I", reformers = { "RCtrl" } } }, down = device_commands.CMSForward, up = device_commands.CMSForward, value_down = 1.0, value_up = 0.0, name = _("HOTAS CMS - Forward (10x Flare + Chaff / 1s)"), category = { _("Countermeasures") }, features = { "Countermeasures" } },
    { combos = { { key = "K", reformers = { "RCtrl" } } }, down = device_commands.CMSAft, up = device_commands.CMSAft, value_down = 1.0, value_up = 0.0, name = _("HOTAS CMS - Aft (ECM Consent - Reserved)"), category = { _("Countermeasures") }, features = { "Countermeasures" } },
    { combos = { { key = "J", reformers = { "RCtrl" } } }, down = device_commands.CMSLeft, up = device_commands.CMSLeft, value_down = 1.0, value_up = 0.0, name = _("HOTAS CMS - Left (Program 6 - 7x Chaff / 0.5s)"), category = { _("Countermeasures") }, features = { "Countermeasures" } },
    { combos = { { key = "L", reformers = { "RCtrl" } } }, down = device_commands.CMSRight, up = device_commands.CMSRight, value_down = 1.0, value_up = 0.0, name = _("HOTAS CMS - Right (ECM Standby - Reserved)"), category = { _("Countermeasures") }, features = { "Countermeasures" } },
    { combos = { { key = "PgDn", reformers = { "RAlt" } } }, down = device_commands.CMSPress, up = device_commands.CMSPress, value_down = 1.0, value_up = 0.0, name = _("HOTAS CMS - Press (Reserved)"), category = { _("Countermeasures") }, features = { "Countermeasures" } },
    { down = iCommandPlaneFire, up = iCommandPlaneFireOff, name = _("Gun Fire (MC)"), category = { _("Weapons") } },
    { combos = defaultDeviceAssignmentFor("weapon_release_button"), down = iCommandPlanePickleOn, up = iCommandPlanePickleOff, name = _("Weapon Release"), category = { _("Weapons") } },
    { combos = { { key = "Space", reformers = { "RAlt" } } }, down = iCommandPlanePickleOn, up = iCommandPlanePickleOff, name = _("Weapon Release (MC)"), category = { _("Weapons") } },

    { combos = { { key = "C", reformers = { "RCtrl" } } }, down = device_commands.FBWCatToggle, name = _("FBW CAT - Toggle"), category = { _("Flight Control") } },
    { combos = { { key = "1", reformers = { "RAlt" } } }, down = device_commands.FBWCat1, name = _("FBW CAT - CAT I"), category = { _("Flight Control") } },
    { combos = { { key = "3", reformers = { "RAlt" } } }, down = device_commands.FBWCat3, name = _("FBW CAT - CAT III"), category = { _("Flight Control") } },
    { combos = { { key = "G", reformers = { "RCtrl" } } }, down = device_commands.FBWGLimiterOverride, up = device_commands.FBWGLimiterOverride, value_down = 1.0, value_up = 0.0, name = _("[Developer] FBW G-Limiter Override (Hold)"), category = { _("Flight Control") } },
    { combos = { { key = "G", reformers = { "RAlt" } } }, down = device_commands.FBWGLimiterOverrideToggle, name = _("[Developer] FBW G-Limiter Override (Toggle)"), category = { _("Flight Control") } },

    -- F-16A/B-reference controls: a separate AUTOPILOT master plus two-position
    -- PITCH and ROLL mode switches. This is not a confirmed F-CK-1C panel.
    { combos = { { key = "A", reformers = { "LAlt" } } }, down = device_commands.APMasterOn, value_down = 1.0, name = _("AP AUTOPILOT Switch - ON"), category = { _("Autopilot") } },
    { combos = { { key = "A", reformers = { "LAlt", "LShift" } } }, down = device_commands.APMasterOff, value_down = 1.0, name = _("AP AUTOPILOT Switch - OFF"), category = { _("Autopilot") } },
    { combos = { { key = "A", reformers = { "LAlt", "LCtrl" } } }, down = device_commands.APBypass, up = device_commands.APBypass, value_down = 1.0, value_up = 0.0, name = _("AP Bypass (Hold)"), category = { _("Autopilot") } },

    { combos = { { key = "H", reformers = { "LCtrl" } } }, down = device_commands.APPitchAttitudeHold, value_down = 1.0, name = _("AP PITCH Switch - ATT HOLD"), category = { _("Autopilot") } },
    { combos = { { key = "H", reformers = { "LAlt" } } }, down = device_commands.APPitchAltitudeHold, value_down = 1.0, name = _("AP PITCH Switch - ALT HOLD"), category = { _("Autopilot") } },

    { combos = { { key = "U", reformers = { "LAlt" } } }, down = device_commands.APRollAttitudeHold, value_down = 1.0, name = _("AP ROLL Switch - ATT HOLD"), category = { _("Autopilot") } },
    { combos = { { key = "U", reformers = { "LCtrl" } } }, down = device_commands.APRollHeadingSelect, value_down = 1.0, name = _("AP ROLL Switch - HDG SEL"), category = { _("Autopilot") } },
    { combos = { { key = "Right", reformers = { "LAlt" } } }, down = device_commands.APHeadingSetIncrease, value_down = 1.0, name = _("Heading Set - Increase 1 Degree"), category = { _("Autopilot") } },
    { combos = { { key = "Left", reformers = { "LAlt" } } }, down = device_commands.APHeadingSetDecrease, value_down = 1.0, name = _("Heading Set - Decrease 1 Degree"), category = { _("Autopilot") } },

    { combos = { { key = "T", reformers = { "LAlt" } } }, down = device_commands.APAutoThrottleToggle, up = device_commands.APAutoThrottleToggle, value_down = 1.0, value_up = 0.0, name = _("[Developer] Experimental Auto-Throttle - Toggle"), category = { _("Developer Tools") } },
    { down = device_commands.APAutoThrottleOn, up = device_commands.APAutoThrottleOn, value_down = 1.0, value_up = 0.0, name = _("[Developer] Experimental Auto-Throttle - ON"), category = { _("Developer Tools") } },
    { down = device_commands.APAutoThrottleOff, up = device_commands.APAutoThrottleOff, value_down = 1.0, value_up = 0.0, name = _("[Developer] Experimental Auto-Throttle - OFF"), category = { _("Developer Tools") } },
    { combos = { { key = "T", reformers = { "LAlt", "LCtrl" } } }, pressed = device_commands.APSpeedIncrease, up = device_commands.APSpeedIncrease, value_pressed = 1.0, value_up = 0.0, name = _("[Developer] Experimental A/T Speed Ref - Increase"), category = { _("Developer Tools") } },
    { combos = { { key = "T", reformers = { "LAlt", "LShift" } } }, pressed = device_commands.APSpeedDecrease, up = device_commands.APSpeedDecrease, value_pressed = 1.0, value_up = 0.0, name = _("[Developer] Experimental A/T Speed Ref - Decrease"), category = { _("Developer Tools") } },

    { combos = { { key = "Y", reformers = { "RAlt" } } }, down = device_commands.EngineThrustCutTestToggle, up = device_commands.EngineThrustCutTestToggle, value_down = 1.0, value_up = 0.0, name = _("Engine Thrust Cut Test - Toggle"), category = { _("Flight Test") } },
    { down = device_commands.EngineThrustCutTestEnable, up = device_commands.EngineThrustCutTestEnable, value_down = 1.0, value_up = 0.0, name = _("Engine Thrust Cut Test - Enable (Thrust = 0)"), category = { _("Flight Test") } },
    { down = device_commands.EngineThrustCutTestDisable, up = device_commands.EngineThrustCutTestDisable, value_down = 1.0, value_up = 0.0, name = _("Engine Thrust Cut Test - Disable (Restore Thrust)"), category = { _("Flight Test") } },
})

dofile(folder .. "../mission_input.lua")(res.keyCommands, device_commands, dcs_commands)

return res
