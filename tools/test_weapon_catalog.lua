-- 共用清單與實際機型註冊共用同一組 CLSID 及掛載參數。
return function(expect)
    local json = dofile("Cockpit/Scripts/ThirdParty/json/json.lua")
    local file = assert(io.open("FM/Weapons/WeaponCatalog.json", "rb"))
    local data = json.decode(file:read("*a")); file:close()
    local identities, models, registered = {}, {}, {}
    local groups = dofile("Cockpit/Scripts/Stores/WeaponCatalog.lua").load(".").groups
    for _, store in ipairs(data.stores) do
        expect(not models[store.id], "canonical weapon models are unique")
        models[store.id] = true
        for _, alias in ipairs(store.aliases) do
            expect(not identities[alias], "DCS aliases resolve to one model"); identities[alias] = store.id
        end
        for _, loadout in ipairs(store.loadouts) do
            expect(not identities[loadout.clsid], "registered CLSID resolves to one model")
            identities[loadout.clsid], registered[loadout.clsid] = store.id, loadout
            local match
            for _, entry in ipairs(groups[loadout.group]) do
                if entry.CLSID == loadout.clsid then match = entry end
            end
            expect(match ~= nil, "Lua registration includes each catalog loadout")
            for key, value in pairs(loadout.parameters) do
                expect(json.encode(match[key]) == json.encode(value), "native loadout parameters retain their catalog values")
            end
        end
    end
    local aircraft_file = assert(io.open("F-CK-1C.lua", "rb"))
    local source = aircraft_file:read("*a"); aircraft_file:close()
    local prefix = source:sub(1, assert(source:find("    %-%- Identification%.")) - 1)
    local env = setmetatable({ current_mod_path = "." }, { __index = _G })
    local chunk = assert(loadstring(prefix)); setfenv(chunk, env); chunk()
    for _, station in ipairs({ env.Tip, env.Outer, env.Inner, env.CenterlineM, env.CenterlineFB }) do
        local seen = {}
        expect(station[1].CLSID == "<CLEAN>", "each station retains CLEAN")
        for _, entry in ipairs(station) do
            expect(not seen[entry.CLSID], "station loadouts are unique"); seen[entry.CLSID] = true
            expect(entry.CLSID == "<CLEAN>" or registered[entry.CLSID], "station admits catalog registrations")
        end
    end
    for _, clsid in ipairs({ "{AIM-9P3}", "{AIM-9P5}", "{AIM-7P}", "{40EF17B7-F508-45de-8566-6FFECC0C1AB8}" }) do
        expect(identities[clsid] ~= nil and registered[clsid], "existing tested missile is registered")
    end
end
