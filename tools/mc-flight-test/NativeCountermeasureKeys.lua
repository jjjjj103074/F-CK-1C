-- 為測試包加入原生反制驗證與既有 debug 入口的快捷鍵。
return function(commands, ids)
    for _, binding in ipairs(commands) do
        if binding.down == ids.DebugIndicatorToggle and #(binding.combos or {}) == 0 then
            binding.combos = { { key = "F12", reformers = { "RCtrl", "RShift" } } }
        end
    end
    local function bind(key, modifiers, command, label)
        commands[#commands + 1] = {
            combos = { { key = key, reformers = modifiers } },
            down = command, up = command, value_down = 1.0, value_up = 0.0,
            name = _("[Flight Test] Native CM - " .. label), category = { _("Flight Test") }
        }
    end
    bind("Insert", { "LAlt", "LShift" }, ids.NativeCmFlareDigital, "Flare Digital")
    bind("Delete", { "LAlt", "LShift" }, ids.NativeCmChaffDigital, "Chaff Digital")
    bind("Insert", { "LCtrl", "LShift" }, ids.NativeCmFlareAnalog, "Flare Analog")
    bind("Delete", { "LCtrl", "LShift" }, ids.NativeCmChaffAnalog, "Chaff Analog")
end
