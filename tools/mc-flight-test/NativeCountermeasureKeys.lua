-- 為測試包加入原生反制驗證按鍵。
return function(commands, ids)
    local function bind(key, modifiers, command, label)
        commands[#commands + 1] = {
            combos = { { key = key, reformers = modifiers } },
            down = command, up = command, value_down = 1.0, value_up = 0.0,
            name = _("[Flight Test] Native CM - " .. label), category = { _("Flight Test") }
        }
    end
    bind("Insert", { "LAlt", "LShift" }, ids.NativeCmFlareDirect, "Flare Direct - one round")
    bind("Delete", { "LAlt", "LShift" }, ids.NativeCmChaffDirect, "Chaff Direct - one round")
end
