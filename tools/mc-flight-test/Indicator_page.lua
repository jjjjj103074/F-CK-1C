-- 顯示飛行測試使用的既有 MC、掛點與感測來源參數。
dofile(LockOn_Options.common_script_path .. "elements_defs.lua")
SetCustomScale(1.0)
local right = LockOn_Options.screen.aspect - 0.025
local left = right - 0.70
local top = 0.94
local material = MakeMaterial("", { 0, 0, 0, 185 })
local panel = CreateElement("ceMeshPoly")
panel.name = "mc_flight_test_background"
panel.primitivetype = "triangles"
panel.material = material
panel.vertices = { { left - 0.015, top - 1.10 }, { left - 0.015, top + 0.03 },
    { right, top + 0.03 }, { right, top - 1.10 } }
panel.indices = default_box_indices
panel.screenspace = ScreenType.SCREENSPACE_TRUE
Add(panel)
local function text(name, value, x, y, parameter, format)
    local item = CreateElement("ceStringPoly")
    item.name = "mc_flight_test_" .. name
    item.material = "font_hmcs_small"
    item.alignment = "LeftTop"
    item.stringdefs = { 0.0024, 0.0024, 0, 0 }
    item.init_pos = { x, y, 0 }
    item.value = value
    item.screenspace = ScreenType.SCREENSPACE_TRUE
    if parameter then
        item.element_params = { parameter }
        item.controllers = { { "text_using_parameter", 0, 0 } }
        item.formats = { format }
    end
    Add(item)
end
text("title", "MC TEST / READ ONLY", left, top)
local rows = {
    { "NATIVE", "CPP_NATIVE_CM_ENABLED", "NATIVE %.0f", "API", "CPP_NATIVE_CM_API_MASK", "API %.0f" },
    { "NSTATUS", "CPP_NATIVE_CM_STATUS", "STAT %.0f", "NLAST", "CPP_NATIVE_CM_LAST_REQUEST", "LAST %.0f" },
    { "NREQUEST", "CPP_NATIVE_CM_REQUESTS", "REQ %.0f", "NCALLS", "CPP_NATIVE_CM_CALLS", "CALL %.0f" },
    { "CMOBS", "FCK1C_TEST_CM_OBSERVATION_AVAILABLE", "CMOBS %.0f", "FLARE", "FCK1C_TEST_CM_FLARE_COUNT", "FLR %.0f" },
    { "NMETHOD", "CPP_NATIVE_CM_METHOD", "METH %.0f", "CHAFF", "FCK1C_TEST_CM_CHAFF_COUNT", "CHF %.0f" },
    { "MC", "CPP_MC_AVAILABLE", "MC %.0f", "GEN", "CPP_MC_GENERATION", "GEN %.0f" },
    { "ARM", "HMCS_MASTER_MODE", "ARM %.0f", "MODE", "HMCS_FC_MODE", "MODE %.0f" },
    { "SUB", "HMCS_AAM_SUBMODE", "SUB %.0f", "UNCAGE", "CPP_MC_MISSILE_UNCAGE_HELD", "UNCAGE %.0f" },
    { "TRIGGER", "CPP_MC_TRIGGER_SECOND_STAGE_HELD", "TRIG %.0f", "FIRE", "HMCS_GUN_FIRING", "FIRE %.0f" },
    { "GUNVALID", "CPP_MC_STORES_GUN_COUNT_AVAILABLE", "GUN OK %.0f", "GUN", "CPP_MC_STORES_GUN_COUNT", "GUN %.0f" },
    { "AAMVALID", "CPP_MC_STORES_MISSILE_COUNT_AVAILABLE", "AAM OK %.0f", "AAM", "CPP_MC_STORES_AIR_TO_AIR_MISSILE_COUNT", "AAM %.0f" },
    { "STATION", "OBS_WEAPON_STATIONS_SELECTED", "SEL %.0f", "SACK", "OBS_WEAPON_SELECTION_ACK", "SACK %.0f" },
    { "RDR", "RADAR_MODE", "RDR %.0f", "RANGE", "RADAR_STT_RANGE", "STT %.0fm" },
    { "IRVALID", "CPP_MC_STORES_IR_SEEKER_AVAILABLE", "IR OK %.0f", "IRLOCK", "CPP_MC_STORES_IR_SEEKER_LOCK", "LOCK %.0f" },
    { "COUNT", "CPP_MC_ACTION_COUNT", "TX %.0f", "ACK", "OBS_MC_ACKNOWLEDGED_SEQUENCE", "ACK %.0f" },
}
for i, row in ipairs(rows) do
    local y = top - i * 0.055
    text(row[1], "", left, y, row[2], row[3])
    text(row[4], "", left + 0.34, y, row[5], row[6])
end
text("legend1", "ARM 0=SAFE 1=SIM 2=ON", left, top - 0.90)
text("legend2", "MODE 0=NAV 1=DGF 2=MSL", left, top - 0.95)
text("legend3", "SEL 0 RT/1 RO/2 RI/3 MM", left, top - 1.00)
text("legend4", "4 MF/5 MB/6 LI/7 LO/8 LT", left, top - 1.05)
