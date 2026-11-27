-- 共用清單與實際機型註冊共用同一組 CLSID 及掛載參數。
return function(expect)
    local data = dofile("Cockpit/Scripts/generated/WeaponCatalog.g.lua")
    local function equal(left, right)
        if type(left) ~= type(right) then return false end
        if type(left) ~= "table" then return left == right end
        for key, value in pairs(left) do if not equal(value, right[key]) then return false end end
        for key in pairs(right) do if left[key] == nil then return false end end
        return true
    end
    local identities, models, registered = {}, {}, {}
    -- 重放機型註冊環境，載入群組時只提供 DCS 的標準表格與載入介面。
    local sandbox = {}
    for key, value in pairs(_G) do sandbox[key] = value end
    sandbox.io, sandbox.select = nil, nil
    local host_loadfile = loadfile
    sandbox.dofile = function(path)
        local chunk = assert(host_loadfile(path))
        setfenv(chunk, sandbox)
        return chunk()
    end
    local groups = sandbox.dofile("Cockpit/Scripts/Stores/WeaponCatalog.lua").load(".").groups
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
                expect(equal(match[key], value), "native loadout parameters retain their catalog values")
            end
        end
    end
    local aircraft_file = assert(io.open("F-CK-1C.lua", "rb"))
    local source = aircraft_file:read("*a"); aircraft_file:close()
    local prefix = source:sub(1, assert(source:find("    %-%- Identification%.")) - 1)
    local env = setmetatable({ current_mod_path = "." }, { __index = sandbox })
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
