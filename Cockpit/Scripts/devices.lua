-- Stable DCS cockpit device IDs. Never compact or reuse reserved slots.
devices = {
    -- Reserved after migration; do not register or reuse IDs 1, 2, or 9.
    Gear = 1,
    Actuators = 2,
    CMS = 3,
    WEAPON_SYSTEM = 4,
    HMCS = 5,
    AAM_AUDIO = 6,
    RADAR = 7,
    RADAR_STATE = 8,
    AUTOPILOT = 9,
}
