-- Route firing/release aliases through MC and filter inherited direct mission controls.
return function(bindings, commands, native_commands)
    local routes = {
        [native_commands.iCommandPlaneFire] = {
            command = commands.TriggerSecondStage,
            value = 1,
            release = native_commands.iCommandPlaneFireOff,
        },
        [native_commands.iCommandPlaneFireOff] = { command = commands.TriggerSecondStage, value = 0 },
        [native_commands.iCommandPlanePickleOn] = {
            command = commands.WeaponRelease,
            value = 1,
            release = native_commands.iCommandPlanePickleOff,
        },
        [native_commands.iCommandPlanePickleOff] = { command = commands.WeaponRelease, value = 0 },
    }
    -- MC and RadarSystem own these mission operations; this profile exposes their semantic inputs.
    local internal_actions = {
        "iCommandPlaneChangeWeapon", "iCommandPlaneModeBVR", "iCommandPlaneModeVS",
        "iCommandPlaneModeBore", "iCommandPlaneModeHelmet", "iCommandPlaneModeFI0",
        "iCommandPlaneModeCannon", "iCommandPlaneLaunchPermissionOverride",
        "iCommandPlaneDropSnarOnce", "iCommandPlaneDropSnarOnceOff",
        "iCommandPlaneDropFlareOnce", "iCommandPlaneDropChaffOnce",
        "iCommandPlaneLockOnStart", "iCommandPlaneLockOnFinish",
        "iCommandPlaneChangeLock", "iCommandPlaneChangeLockUp",
    }
    local internal = {}
    for _, name in ipairs(internal_actions) do
        local command = native_commands[name]
        if command then internal[command] = true end
    end
    for index = #bindings, 1, -1 do
        local binding = bindings[index]
        if internal[binding.down] or internal[binding.up] or internal[binding.pressed] then
            table.remove(bindings, index)
        end
    end
    local canonical = {}
    for _, binding in ipairs(bindings) do
        if
            (binding.down == commands.TriggerSecondStage or binding.down == commands.WeaponRelease)
            and binding.up == binding.down
            and binding.value_down == 1
            and binding.value_up == 0
            and binding.cockpit_device_id == nil
        then
            canonical[binding.down] = binding
        end
    end
    for _, binding in ipairs(bindings) do
        local held = routes[binding.pressed]
        if held and held.value == 1 then
            -- MC retains the accepted operation; a held input needs one leading edge.
            binding.down = binding.pressed
            binding.up = held.release
            binding.pressed = nil
            binding.value_pressed = nil
        end
        local mapped = false
        for _, edge in ipairs({ "down", "up", "pressed" }) do
            local route = routes[binding[edge]]
            if route then
                binding[edge] = route.command
                binding["value_" .. edge] = route.value
                mapped = true
            end
        end
        if mapped then binding.cockpit_device_id = nil end
    end
    -- DCS identifies commands by their edges and values. Merge aliases into the
    -- existing MC entry so one action has one menu entry and retains every key.
    local function same_combo(lhs, rhs)
        if lhs.key ~= rhs.key then return false end
        local left, right = lhs.reformers or {}, rhs.reformers or {}
        if #left ~= #right then return false end
        for i = 1, #left do
            if left[i] ~= right[i] then return false end
        end
        return true
    end
    for i = #bindings, 1, -1 do
        local binding = bindings[i]
        local target = canonical[binding.down]
        if
            target
            and target ~= binding
            and binding.up == binding.down
            and binding.value_down == 1
            and binding.value_up == 0
            and binding.pressed == nil
        then
            for _, combo in ipairs(binding.combos or {}) do
                target.combos = target.combos or {}
                local duplicate = false
                for _, existing in ipairs(target.combos) do
                    if same_combo(combo, existing) then
                        duplicate = true
                        break
                    end
                end
                if not duplicate then target.combos[#target.combos + 1] = combo end
            end
            table.remove(bindings, i)
        end
    end

    -- 共用設定中相同的預設組合交給 HOTAS 與解籠控制，其他組合沿用原動作。
    local owned_inputs = {}
    for _, name in ipairs({ "TMSUp", "TMSDown", "TMSLeft", "TMSRight", "DMSUp", "DMSDown", "DMSLeft", "DMSRight", "RadarCursorUp", "RadarCursorDown", "RadarCursorLeft", "RadarCursorRight", "CMSForward", "CMSAft", "CMSLeft", "CMSRight", "MissileUncage" }) do
        owned_inputs[commands[name]] = true
    end
    local function combo_key(combo)
        local modifiers = {}
        for _, modifier in ipairs(combo.reformers or {}) do modifiers[#modifiers + 1] = modifier end
        table.sort(modifiers)
        return table.concat(modifiers, "+") .. ":" .. combo.key
    end
    local owned_combos = {}
    for _, binding in ipairs(bindings) do
        if owned_inputs[binding.down] then
            for _, combo in ipairs(binding.combos or {}) do owned_combos[combo_key(combo)] = true end
        end
    end
    for _, binding in ipairs(bindings) do
        if not owned_inputs[binding.down] then
            for index = #(binding.combos or {}), 1, -1 do
                if owned_combos[combo_key(binding.combos[index])] then table.remove(binding.combos, index) end
            end
        end
    end
end
