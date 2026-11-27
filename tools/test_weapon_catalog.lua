-- 執行機型完整註冊流程，核對掛點資料與模組初始化結果。
return function(expect)
    local env = {}
    for key, value in pairs(_G) do env[key] = value end
    env.io, env.select = nil, nil
    env.current_mod_path = "."
    local aircraft, flyable, done, messages = nil, nil, 0, {}
    env.log = { INFO = 3, ERROR = 1, write = function(_, level, message)
        expect(level == 3, "aircraft entry logs initialization without errors")
        messages[#messages + 1] = message
    end }
    local function descriptor(...) return {...} end
    for _, name in ipairs({ "gun_mount", "gatling_effect", "fire_effect", "smoke_effect", "pylon", "aircraft_task" }) do
        env[name] = descriptor
    end
    env.verbose_to_dmg_properties = function(value) return value end
    env._ = function(value) return value end
    env.WSTYPE_PLACEHOLDER, env.wsType_Air, env.wsType_Airplane, env.wsType_Fighter = 0, 1, 1, 1
    for _, name in ipairs({ "mount_vfs_model_path", "mount_vfs_liveries_path", "mount_vfs_sound_path", "mount_vfs_texture_path", "declare_plugin" }) do
        env[name] = function() end
    end
    env.add_aircraft = function(value) aircraft = value end
    env.make_flyable = function(name, cockpit, fm) flyable = { name = name, cockpit = cockpit, fm = fm } end
    env.plugin_done = function() done = done + 1 end
    local host_loadfile = loadfile
    env.dofile = function(path)
        local chunk = assert(host_loadfile(path))
        setfenv(chunk, env)
        return chunk()
    end
    env.dofile("entry.lua")
    expect(aircraft and aircraft.Name == "F-CK-1C", "complete aircraft definition reaches add_aircraft")
    expect(flyable and flyable.name == aircraft.Name, "the registered aircraft receives its cockpit")
    expect(flyable.fm[2] == "F-CK-1C_EFM.dll", "flyable registration retains the validated native DLL")
    expect(done == 1 and #messages >= 3, "entry completes and reports initialization using DCS log.write")
    expect(#aircraft.Pylons == 9, "all nine pylons reach aircraft registration")
    local registered = {}
    for index, pylon in ipairs(aircraft.Pylons) do
        expect(pylon[1] == index, "pylon numbers follow station definitions")
        local launchers = pylon[7]
        expect(launchers[1].CLSID == "<CLEAN>", "each station retains CLEAN")
        local seen = {}
        for _, entry in ipairs(launchers) do
            expect(type(entry.CLSID) == "string" and not seen[entry.CLSID], "station loadout identities are unique")
            seen[entry.CLSID], registered[entry.CLSID] = true, true
            expect(entry.stations == nil and entry.deny == nil and entry.extra == nil, "native station entries contain resolved pylon parameters")
        end
    end
    for _, clsid in ipairs({ "{AIM-9P3}", "{AIM-9P5}", "{AIM-7P}", "{40EF17B7-F508-45de-8566-6FFECC0C1AB8}" }) do
        expect(registered[clsid], "existing tested missile reaches native registration")
    end
end
