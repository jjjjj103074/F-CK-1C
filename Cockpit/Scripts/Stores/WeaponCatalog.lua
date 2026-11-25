-- 載入共用武器資料，建立機型註冊所需的 DCS 掛載群組。
local Catalog = {}

function Catalog.load(module_root)
    local json = dofile(module_root .. "/Cockpit/Scripts/ThirdParty/json/json.lua")
    local filename = module_root .. "/FM/Weapons/WeaponCatalog.json"
    local file = assert(io.open(filename, "rb"), "Weapon catalog cannot be opened: " .. filename)
    local source = file:read("*a")
    file:close()
    local data = json.decode(source)
    assert(data.schema_version == 1, "Unsupported weapon catalog schema")
    local groups, identities, models = {}, {}, {}
    for _, group in ipairs(data.groups) do
        assert(type(group.id) == "string" and groups[group.id] == nil, "Duplicate weapon group")
        assert(type(group.air_to_air) == "boolean", "Invalid weapon group role")
        groups[group.id] = { _isAAM = group.air_to_air }
    end
    for _, store in ipairs(data.stores) do
        assert(type(store.id) == "string" and store.id ~= "" and not models[store.id], "Invalid or duplicate weapon identity")
        assert(type(store.name) == "string" and store.name ~= "", "Invalid weapon name")
        models[store.id] = true
        for _, alias in ipairs(store.aliases) do
            assert(type(alias) == "string" and alias ~= "" and not identities[alias], "Duplicate DCS weapon identifier")
            identities[alias] = true
        end
        for _, loadout in ipairs(store.loadouts) do
            assert(type(loadout.clsid) == "string" and not identities[loadout.clsid], "Duplicate loadout CLSID")
            identities[loadout.clsid] = true
            local group = assert(groups[loadout.group], "Unknown loadout group")
            assert(type(loadout.parameters) == "table", "Invalid loadout parameters")
            local entry = { CLSID = loadout.clsid }
            for key, value in pairs(loadout.parameters) do
                assert(key ~= "CLSID", "CLSID belongs to the loadout identity")
                entry[key] = value
            end
            group[#group + 1] = entry
        end
    end
    return { groups = groups }
end

return Catalog
