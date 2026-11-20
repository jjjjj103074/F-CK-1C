-- 核對持續更新裝置與頭盔顯示頁面的執行期契約。
return function(expect)
    local function execute(path, env)
        local chunk = assert(loadfile(path))
        setfenv(chunk, env)
        return chunk()
    end
    local function environment()
        local env = setmetatable({}, { __index = _G })
        env.LockOn_Options = { script_path = 'Cockpit/Scripts/', common_script_path = 'fixture/common/' }
        env.dofile = function(path)
            if path:match('^fixture/common/') then return end
            return execute(path, env)
        end
        return env
    end

    local radar = environment()
    local power_calls, rate = {}, nil
    radar.GetSelf = function()
        return { set_power = function(_, powered) power_calls[#power_calls + 1] = powered end }
    end
    radar.make_default_activity = function(value) rate = value end
    execute('Cockpit/Scripts/RADAR/FCK1C_Radar.lua', radar)
    if radar.post_initialize then radar.post_initialize() end
    if radar.need_to_be_closed == false then
        for _ = 1, 3 do radar.update() end
    end
    expect(#power_calls > 0, 'native radar must receive power through its live Lua callbacks')
    expect(rate == 0.02, 'native radar keeps its established update period')
    for _, value in ipairs(power_calls) do expect(value == true, 'native radar uses the powered flight configuration') end

    local config = environment()
    local configure_dofile = config.dofile
    config.dofile = function(path)
        if path:match("materials.lua$") then return end
        return configure_dofile(path)
    end
    execute('Cockpit/Scripts/device_init.lua', config)
    local native_supply = config.devices.ELECTRIC_SYSTEM and config.creators[config.devices.ELECTRIC_SYSTEM]
    expect(native_supply and native_supply[1] == 'avSimpleElectricSystem',
        'native radar has its required avSimpleElectricSystem supply')

    local observer = environment()
    local measured_power, radar_present = false, true
    local enabled, published = {}, {}
    local supply = {
        AC_Generator_1_on = function(_, state) enabled.ac1 = state end,
        AC_Generator_2_on = function(_, state) enabled.ac2 = state end,
        DC_Battery_on = function(_, state) enabled.battery = state end,
    }
    local native_radar = { get_power = function() return measured_power end }
    observer.GetDevice = function(id)
        if id == observer.devices.ELECTRIC_SYSTEM then return supply end
        if id == observer.devices.RADAR and radar_present then return native_radar end
    end
    observer.make_default_activity = function() end
    observer.get_param_handle = function(name)
        return { set = function(_, value) published[name] = value end }
    end
    execute('Cockpit/Scripts/Systems/radar_state_system.lua', observer)
    observer.post_initialize()
    local observed = observer.cockpit_params
    expect(published[observed.RadarState] == 0 and published[observed.RadarPowerState] == 0,
        'native radar power remains off until its getter reports power')
    expect(enabled.ac1 == true and enabled.ac2 == true and enabled.battery == true,
        'powered flight configuration enables both native generators and battery')
    measured_power = true
    observer.update()
    expect(published[observed.RadarState] == 1 and published[observed.RadarPowerState] == 1,
        'native powered state reaches both bridge observations')
    radar_present = false
    observer.update()
    expect(published[observed.RadarState] == 0 and published[observed.RadarPowerState] == 0,
        'an absent native radar publishes an unpowered state')

    local display = environment()
    display.indicator_types = { COLLIMATOR = 1 }
    display.render_purpose = { GENERAL = 1, HUD_ONLY_VIEW = 2 }
    display.MILLYRADIANS = 1
    display.SetScale = function() end
    display.GetScale = function() return 1 end
    display.MakeMaterial = function() return {} end
    display.CreateElement = function(kind) return { kind = kind } end
    display.h_clip_relations = { REWRITE_LEVEL = 1, INCREASE_IF_LEVEL = 2, COMPARE = 3 }
    display.default_box_indices = { 0, 1, 2, 0, 2, 3 }
    execute('Cockpit/Scripts/HMCS/HMCS_VR_init.lua', display)
    local all = {}
    for _, subset in ipairs(display.pages[display.init_pageID]) do
        local parents = {}
        display.Add = function(element)
            expect(element.parent_element == nil or parents[element.parent_element] ~= nil,
                'HMCS parent must be available in the same subset before child ' .. tostring(element.name))
            expect(not all[element.name], 'HMCS element names are unique: ' .. element.name)
            parents[element.name] = element
            all[element.name] = element
        end
        execute(display.page_subsets[subset], display)
    end
    expect(all.hmcs_vr_glass ~= nil and all.hmcs_vr_tfov ~= nil, 'HMCS retains both clipping masks')
    expect(all.hmcs_vr_root.parent_element == 'hmcs_vr_tfov', 'HMCS symbology retains its clipping parent')
    expect(all.hmcs_vr_reticle_h ~= nil and all.hmcs_vr_reticle_v ~= nil, 'HMCS retains its two reticle axes')
end
