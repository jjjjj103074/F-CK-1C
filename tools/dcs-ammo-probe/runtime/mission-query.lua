-- 在任務環境以本機識別選出玩家單位，讀取該單位的原生彈藥與掛載參數。
return function(request)
    local unit, matched_by
    local candidates = {}
    local scan_errors = {}
    for _, side in ipairs({ 0, 1, 2 }) do
        local ok, players = pcall(coalition.getPlayers, side)
        if not ok then scan_errors[tostring(side)] = tostring(players) end
        if type(players) ~= "table" then players = {} end
        for _, player in pairs(players) do
            if player:getTypeName() == "F-CK-1C" then candidates[#candidates + 1] = player end
        end
    end
    if type(request.object_id) == "number" then
        for _, candidate in ipairs(candidates) do
            if candidate:getID() == request.object_id then
                unit, matched_by = candidate, "object_id"
                break
            end
        end
    end
    if not unit and type(request.player_name) == "string" and request.player_name ~= "" then
        local matches = 0
        for _, candidate in ipairs(candidates) do
            if candidate:getPlayerName() == request.player_name then
                matches = matches + 1
                unit, matched_by = candidate, "player_name"
            end
        end
        if matches ~= 1 then unit = nil end
    end
    if not unit and request.single_player and world and type(world.getPlayer) == "function" then
        local player = world.getPlayer()
        if player and player:getTypeName() == "F-CK-1C" then
            unit, matched_by = player, "world.getPlayer"
        end
    end
    if not unit then return { status = "ownship_unmatched", candidates = #candidates, scan_errors = scan_errors } end
    local result = {
        status = "matched",
        matched_by = matched_by,
        unit_name = unit:getName(),
        unit_id = unit:getID(),
        aircraft = unit:getTypeName(),
        player_name = unit:getPlayerName(),
        time = timer.getTime(),
        scan_errors = scan_errors,
    }
    -- 保存本機在任務中的初始配置，對照機砲裝填比例與掛點選項。
    if env and env.mission and env.mission.coalition then
        for _, side in pairs(env.mission.coalition) do
            for _, country in pairs(side.country or {}) do
                for _, group in pairs(country.plane and country.plane.group or {}) do
                    for _, initial_unit in pairs(group.units or {}) do
                        if initial_unit.name == result.unit_name then result.initial_payload = initial_unit.payload end
                    end
                end
            end
        end
    end
    local ok, ammo = pcall(unit.getAmmo, unit)
    result.ammo = { call_ok = ok, result_type = type(ammo) }
    if ok and type(ammo) == "table" then
        local entries = {}
        for index, entry in pairs(ammo) do
            local desc = type(entry.desc) == "table" and entry.desc or {}
            entries[tostring(index)] = {
                count = entry.count,
                desc = { typeName = desc.typeName, displayName = desc.displayName, category = desc.category, caliber = desc.caliber, missileCategory = desc.missileCategory },
            }
        end
        result.ammo.entries = entries
    else
        result.ammo.value = ammo
    end
    local get_draw = unit.getDrawArgumentValue
    if type(get_draw) == "function" then
        result.draw_arguments = {}
        -- 依機型定義保存掛架外觀參數，供空掛載與 CLEAN 的狀態對照。
        for _, station in ipairs(Core.stations) do
            local success, value = pcall(get_draw, unit, station.draw)
            result.draw_arguments[station.name] = { argument = station.draw, call_ok = success, value = value }
        end
    end
    return result
end
