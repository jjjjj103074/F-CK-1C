local count = 0
local function counter()
    count = count + 1
    return count
end
-------DEVICE ID-------
devices = {}

devices["Gear"] = counter()
devices["Actuators"] = counter()
devices["CMS"] = counter()
devices["WEAPON_SYSTEM"] = counter()
devices["HMCS"] = counter()
devices["AAM_AUDIO"] = counter()
devices["RADAR"] = counter()
devices["RADAR_STATE"] = counter()
devices["AUTOPILOT"] = counter()
