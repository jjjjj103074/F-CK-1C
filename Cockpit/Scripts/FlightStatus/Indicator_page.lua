-- 名稱靠右排列，冒號與內容固定在左上角的同一欄。
dofile(LockOn_Options.common_script_path .. "elements_defs.lua")
dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")
SetCustomScale(1.0)

local rows = {
    { "IAS", cockpit_params.FlightStatusIas },
    { "ALT", cockpit_params.FlightStatusAltitude },
    { "MODE", cockpit_params.FlightStatusMasterMode },
    { "OVRD", cockpit_params.FlightStatusOverride },
    { "ARM", cockpit_params.FlightStatusArm },
    { "MISSILE", cockpit_params.FlightStatusMissile },
    { "STATION", cockpit_params.FlightStatusStation },
    { "GUN", cockpit_params.FlightStatusGun },
    { "FLARE", cockpit_params.FlightStatusFlare },
    { "CHAFF", cockpit_params.FlightStatusChaff },
    { "RADAR", cockpit_params.FlightStatusRadar },
    { "IR", cockpit_params.FlightStatusIr },
    { "UNCAGE", cockpit_params.FlightStatusUncage },
}
local left, top = -LockOn_Options.screen.aspect + 0.04, 0.94
local colon_x, row_height = left + 0.125, 0.026

local root = CreateElement("ceSimple")
root.name = "flight_status_root"
root.element_params = { cockpit_params.DebugIndicatorVisible }
root.controllers = { { "parameter_in_range", 0, -0.1, 0.1 } }
root.screenspace = ScreenType.SCREENSPACE_TRUE
Add(root)

local function text(name, value, x, y, alignment, parameter)
    local item = CreateElement("ceStringPoly")
    item.name = "flight_status_" .. name
    item.parent_element = root.name
    item.material = "font_hmcs_small"
    item.alignment = alignment
    item.stringdefs = { 0.0018, 0.0018, 0, 0 }
    item.init_pos = { x, y, 0 }
    item.value = value
    item.screenspace = ScreenType.SCREENSPACE_TRUE
    if parameter then
        item.element_params = { parameter }
        item.controllers = { { "text_using_parameter", 0, 0 } }
        item.formats = { "%s" }
    end
    Add(item)
end

for i, row in ipairs(rows) do
    local y = top - (i - 1) * row_height
    text(row[1] .. "_label", row[1], colon_x - 0.012, y, "RightTop")
    text(row[1] .. "_value", ": ---", colon_x, y, "LeftTop", row[2])
end
need_to_be_closed = true
