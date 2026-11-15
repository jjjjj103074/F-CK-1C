-- 並列 C++ 與座艙來源的反制庫存及測試執行狀態。
dofile(LockOn_Options.common_script_path .. "elements_defs.lua")
SetCustomScale(1.0)
local left = LockOn_Options.screen.aspect - 0.82
local top = 0.94
local panel = CreateElement("ceMeshPoly")
panel.name = "native_cm_test_background"
panel.primitivetype = "triangles"
panel.material = MakeMaterial("", { 0, 0, 0, 190 })
panel.vertices = { { left - 0.02, top - 0.69 }, { left - 0.02, top + 0.03 },
    { left + 0.79, top + 0.03 }, { left + 0.79, top - 0.69 } }
panel.indices = default_box_indices
panel.screenspace = ScreenType.SCREENSPACE_TRUE
Add(panel)
local function text(name, value, x, y, parameter, format)
    local item = CreateElement("ceStringPoly")
    item.name = "native_cm_test_" .. name
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
text("title", "C++ COUNTERMEASURE DIRECT TEST", left, top)
local rows = {
    { "CPP_F", "CPP_NATIVE_CM_FLARE_COUNT", "CPP FLR %.0f", "LUA_F", "FCK1C_TEST_CM_FLARE_COUNT", "LUA FLR %.0f" },
    { "CPP_C", "CPP_NATIVE_CM_CHAFF_COUNT", "CPP CHF %.0f", "LUA_C", "FCK1C_TEST_CM_CHAFF_COUNT", "LUA CHF %.0f" },
    { "READ", "CPP_NATIVE_CM_INVENTORY_AVAILABLE", "READ %.0f", "MATCH", "CPP_NATIVE_CM_PAYLOAD_MATCHES", "MATCH %.0f" },
    { "HOST", "CPP_NATIVE_CM_API_MASK", "HOST %.0f", "LUA", "FCK1C_TEST_CM_OBSERVATION_AVAILABLE", "LUA OK %.0f" },
    { "ARM", "HMCS_MASTER_MODE", "ARM %.0f", "STAT", "CPP_NATIVE_CM_STATUS", "STATUS %.0f" },
    { "REQ", "CPP_NATIVE_CM_REQUESTS", "REQ %.0f", "CALL", "CPP_NATIVE_CM_CALLS", "CALLS %.0f" },
    { "SAMPLES", "CPP_NATIVE_CM_SAMPLES", "SAMPLES %.0f", "SRC", "CPP_NATIVE_CM_SOURCE_STATUS", "SOURCE %.0f" },
    { "LAST", "CPP_NATIVE_CM_KIND", "LAST %.0f", "ERR", "CPP_NATIVE_CM_EXCEPTION", "ERROR %.0f" },
}
for i, row in ipairs(rows) do
    local y = top - i * 0.06
    text(row[1], "", left, y, row[2], row[3])
    text(row[4], "", left + 0.40, y, row[5], row[6])
end
text("legend1", "READY: READ 1 / MATCH 1 / HOST 3 / LUA OK 3", left, top - 0.55)
text("legend2", "ARM 2 = ON / STATUS 3 = CALL RETURNED", left, top - 0.60)
text("legend3", "LAST 1 = FLARE / LAST 2 = CHAFF", left, top - 0.65)
