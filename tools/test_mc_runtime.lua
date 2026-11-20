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
