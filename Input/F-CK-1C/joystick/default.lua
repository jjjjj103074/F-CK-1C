local res = external_profile("Config/Input/Aircrafts/common_joystick_binding.lua")

join(res.keyCommands,
    {
        { combos = defaultDeviceAssignmentFor("roll"),   action = iCommandPlaneRoll,         name = _('Roll') },
        { combos = defaultDeviceAssignmentFor("pitch"),  action = iCommandPlanePitch,        name = _('Pitch') },
        { combos = defaultDeviceAssignmentFor("rudder"), action = iCommandPlaneRudder,       name = _('Rudder') },
        { combos = defaultDeviceAssignmentFor("thrust"), action = iCommandPlaneThrustCommon, name = _('Thrust') },

        { action = iCommandWheelBrake,                   name = _('Wheel Brake'),            category = { _('Systems') } },
        { action = iCommandLeftWheelBrake,               name = _('Wheel Brake Left'),       category = { _('Systems') } },
        { action = iCommandRightWheelBrake,              name = _('Wheel Brake Right'),      category = { _('Systems') } },

    }
)
