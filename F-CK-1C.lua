-- F-CK-1C.lua
-- 以 F-16C 為基礎調整的 F-CK-1C 初版 SFM 定義。
-- 目前以可載入、可飛行、可供 AI 使用為優先，數值仍會持續修正。
-- 檔案內使用 DCS 既有 helper，例如 pylon、gun_mount、makeAirplaneCanopyGeometry。
-- 若某些常數尚未在目前環境定義，需依 DCS 版本或其他模組補齊。


-- ===================== 基本識別資料 (Identification) =====================
-- Name / DisplayName / shape_table_data 會對應到 DCS 的單位與模型註冊資料。
local F_CK_1C = {
    -- 內部單位名稱
    Name = 'F-CK-1C',
    -- UI 顯示名稱
    DisplayName = _('F-CK-1C'),

    Rate = 40, -- RewardPoint in Multiplayer

    Shape = "F-CK-1C",
    -- shape_table_data: 將單位名稱綁定到 3D 模型與損毀模型設定。
    shape_table_data = { {
        name = "F-CK-1C",
        file = "F-CK-1C", -- 3D 模型檔名，對應 Shapes 資料夾
        username = "F-CK-1C",
        index = WSTYPE_PLACEHOLDER,
        life = 20,                 -- 單位生命值 / 耐久度
        vis = 3,                   -- 可見度等級，影響 LOD 與目視辨識
        desrt = 'Fighter-2-crush', -- Name of destroyed object file name Alphajet-destr. This is a placeholder.
        fire = { 300, 2 },         -- 受損起火效果參數
        classname = "lLandPlane",
        positioning = "BYNORMAL"
    },
        -- {
        --     name = "F-CK-1C_destr",
        --     file = "f-ck-1c-oblomok",
        --     fire = {0, 1}
        -- }
    },

    -- 可用國家
    Countries = {
        "Abkhazia",
        "Algeria",
        "Argentina",
        "Australia",
        "Austria",
        "Belarus",
        "Belgium",
        "Brazil",
        "Bulgaria",
        "Canada",
        "China",
        "Chile",
        "Croatia",
        "Cuba",
        "Czech Republic",
        "Cyprus",
        "Denmark",
        "Egypt",
        "Finland",
        "France",
        "Georgia",
        "Germany",
        "Ghana",
        "Greece",
        "Honduras",
        "Hungary",
        "India",
        "Indonesia",
        "Insurgents",
        "Iran",
        "Iraq",
        "Israel",
        "Italy",
        "Japan",
        "Jordan",
        "Kazakhstan",
        "Lebanon",
        "Libya",
        "Malaysia",
        "Mexico",
        "Morocco",
        "The Netherlands",
        "Nigeria",
        "North Korea",
        "Norway",
        "Oman",
        "Pakistan",
        "Peru",
        "Phllipines",
        "Poland",
        "Qatar",
        "Romania",
        "Russia",
        "Saudi Arabia",
        "Serbia",
        "Slovakia",
        "Slovenia",
        "South Africa",
        "South Korea",
        "South Ossetia",
        "Spain",
        "Sudan",
        "Sweden",
        "Switzerland",
        "Syria",
        "Thailand",
        "Tunisia",
        "Turkey",
        "UK",
        "Ukraine",
        "United Arab Emirates",
        "United Nations Peacekeepers",
        "USA",
        "USAF Aggressors",
        "Venezuela",
        "Vietnam",
        "Yemen"
    },

    -- UI 與分類
    -- 這些欄位控制任務編輯器與選單中的顯示方式。
    Picture = "F-CK-1C.png", -- [OPTIONAL]
    mapclasskey = "P0091000024",
    WorldID = WSTYPE_PLACEHOLDER,
    attribute = { wsType_Air, wsType_Airplane, wsType_Fighter, WSTYPE_PLACEHOLDER, "Multirole fighters", "Refuelable" },
    Categories = { "{78EFB7A2-FD52-4b57-A6A6-3BF0E1D6555F}", "Interceptor" },

    -- ===================== 座艙與組員 (Crew & Cockpit) =====================
    -- 先沿用既有座艙結構，之後再依實機需求細化。
    HumanCockpit = true,                                -- 允許玩家進入座艙飛行
    HumanCockpitPath = current_mod_path .. '/Cockpit/', -- 座艙腳本路徑

    crew_size = 1,                                      -- 機組員數量
    crew_members = {
        [1] = {
            ejection_seat_name = 17, -- DCS 內建彈射椅 ID，暫沿用既有設定
            -- pilot_name 可於後續補上專用飛行員模型名稱
            -- drop_canopy_name = "F-CK-1C_canopy", -- TODO: 補上可拋棄艙罩模型名稱
            -- canopy_pos = {3.2, 0.674, 0}, -- 艙罩參考位置
            pos = { 3.2, 0.27, 0 }, -- 飛行員座位位置 (x, y, z)，單位公尺
            g_suit = 1.02           -- G-suit 係數，1.0 為標準值
        }
    },

    -- ===================== 重量、幾何與性能 (Mass, Geometry, Performance) =====================
    -- 幾何、重量與性能數值。
    M_empty = 6492,    -- 空重 (kg)
    M_nominal = 9072,  -- 典型作戰重量 (kg)
    M_max = 12530,     -- 最大起飛重量 (kg)
    M_fuel_max = 2111, -- 最大內載燃油 (kg)

    -- AI 使用的最低校正空速，暫參考 A-4E-C 類型設定。
    CAS_min = 23.15, -- 最低校正空速 (m/s) [AI]

    -- 外形尺寸
    length = 14.48,                         -- 機長 (m)
    height = 4.7,                           -- 機高 (m)
    wing_area = 24.26,                      -- 翼面積 (m^2)
    wing_span = 8.53,                       -- 翼展 (m)
    wing_tip_pos = { -2.3, 0.0006, 4.396 }, -- 翼尖參考位置 (x, y, z)，單位公尺

    -- 速度與飛行包線
    V_opt = 220,             -- 最佳巡航速度 (m/s)
    V_take_off = 75,         -- 起飛速度 (m/s)
    V_land = 65,             -- 著陸速度 (m/s)
    V_max_sea_level = 369.4, -- 海平面最大速度 (m/s)
    V_max_h = 612.5,         -- 高空最大速度 (m/s)
    Mach_max = 1.8,          -- 最大馬赫數
    Vy_max = 238.7,          -- 最大爬升率 (m/s)
    Ny_min = -3,             -- 最小過載限制 (-G)
    Ny_max = 9,              -- 最大過載限制 (+G)
    bank_angle_max = 60,     -- 最大持續傾斜角 (deg)
    range = 2200,            -- 航程 (km) [AI]

    -- 推力
    thrust_sum_max = 5506.47, -- 軍用推力總和
    thrust_sum_ab = 8524.83,  -- 後燃器推力總和
    has_afterburner = true,   -- 具備後燃器

    -- 雷達與紅外特徵
    RCS = 3.6,                   -- 雷達截面積估計值 (m^2)
    radar_can_see_ground = true, -- 雷達可進行對地探測
    detection_range_max = 150,   -- 最大偵測距離 (km)
    IR_emission_coeff = 0.73,    -- 軍用推力紅外特徵係數
    IR_emission_coeff_ab = 4.0,  -- 後燃器紅外特徵係數

    -- 空中加油相關
    -- air_refuel_receptacle_pos = {-0.051, 0.911, 0.0}, -- 受油口位置 (x, y, z)，單位公尺
    -- tanker_type = 1, -- 若要充當加油機，可在此指定類型

    -- ===================== 起落架 (Landing Gear) =====================
    tand_gear_max = 0.761,                                    -- 起落架可承受的最大俯仰角參考值 (rad)
    nose_gear_pos = { 4.12, -1.912, 0 },                      -- 前輪位置 (x, y, z) (m)
    nose_gear_amortizer_direct_stroke = 0.0,                  -- 前輪減震器壓縮行程 (m)
    nose_gear_amortizer_reversal_stroke = 1.712 - 1.912,      -- 前輪減震器回彈行程 (m)
    nose_gear_amortizer_normal_weight_stroke = 1.812 - 1.912, -- 前輪在正常重量下的壓縮量 (m)
    nose_gear_wheel_diameter = 0.4572,                        -- 前輪輪徑 (m)

    main_gear_pos = { -1.185, -1.913, 0.7905 },               -- 主輪位置 (x, y, z) (m)
    main_gear_amortizer_direct_stroke = 0,                    -- 主輪減震器壓縮行程 (m)
    main_gear_amortizer_reversal_stroke = 1.727 - 1.913,      -- 主輪減震器回彈行程 (m)
    main_gear_amortizer_normal_weight_stroke = 1.796 - 1.913, -- 主輪在正常重量下的壓縮量 (m)
    main_gear_wheel_diameter = 0.6096,                        -- 主輪輪徑 (m)

    -- nose_gear_door_close_after_retract = false,
    -- main_gear_door_close_after_retract = false,

    -- ===================== 發動機 (Engine) =====================
    engines_count = 2, -- 發動機數量
    engines_nozzles = {
        [1] = {
            pos = { -6.118, 0.0918, 0.4452 },                -- 噴口位置 (x, y, z) (m)
            elevation = 0,                                   -- 噴口仰角 (deg)
            diameter = 0.64,                                 -- 噴口直徑 (m)
            exhaust_length_ab = 3.5,                         -- 後燃尾焰長度 (m)
            exhaust_length_ab_K = 0.77,                      -- 後燃尾焰視覺縮放係數
            smokiness_level = 0.05,                          -- 燃燒煙霧等級
            afterburner_circles_count = 6,                   -- 後燃圓環效果數量
            -- afterburner_circles_pos = {0.2, 0.8}, -- 後燃圓環在噴口內的相對位置
            afterburner_circles_scale = 1.0,                 -- 後燃圓環尺寸縮放
            afterburner_effect_texture = "afterburner_f-16c" -- 後燃效果貼圖
        },
        [2] = {
            pos = { -6.118, 0.0918, -0.4452 },               -- 噴口位置 (x, y, z) (m)
            elevation = 0,                                   -- 噴口仰角 (deg)
            diameter = 0.64,                                 -- 噴口直徑 (m)
            exhaust_length_ab = 3.5,                         -- 後燃尾焰長度 (m)
            exhaust_length_ab_K = 0.77,                      -- 後燃尾焰視覺縮放係數
            smokiness_level = 0.05,                          -- 燃燒煙霧等級
            afterburner_circles_count = 6,                   -- 後燃圓環效果數量
            -- afterburner_circles_pos = {0.2, 0.8}, -- 後燃圓環在噴口內的相對位置
            afterburner_circles_scale = 1.0,                 -- 後燃圓環尺寸縮放
            afterburner_effect_texture = "afterburner_f-16c" -- 後燃效果貼圖
        }
    },

    -- ===================== 機砲與掛點 (Guns & Pylons) =====================
    -- 機砲與掛點先以 DCS 既有資料結構占位。
    -- - 參考 F-16 / A-4E 的寫法，但目前先不啟用外掛點，避免模型 connector 錯誤。
    -- - AI 對掛載資料較敏感，之後若開啟 Pylons 需同步補齊 ammo_type / Stores。
    -- - 若模型內已有 Pylon1、Pylon5 等 connector，可再逐步啟用對應 CLSID。
    -- 目前保留機砲，掛點維持空表以避免 connector 不完整造成載入問題。
    -- M61A2 (using M_61 gun descriptor for compatibility in current mod environment)
    Guns = {
        gun_mount("M_61",
            {
                mixes = {
                    { 1 },    -- XM242 HEI-T
                    { 2 },    -- M56 HEI
                    { 3 },    -- M53 API
                    { 4, 5 }, -- M55 + M220 TP
                    { 6 },    -- PGU-28/B SAPHEI
                    { 7, 8 }, -- PGU-27/B TP with tracers
                },
                count = 523
            },
            {
                supply_position = { 0.4, 0.55, 0.0 },
                muzzle_pos = { 3.85, 0.56, 2.02 },
                ejector_pos = { 2.95, 0.42, 2.00 },
                effects = { gatling_effect(351, 6), fire_effect(350), smoke_effect() },
            })
    },
    ammo_type_default = 5,
    ammo_type = {
        _("HEI-T High Explosive Incendiary-Tracer"),
        _("HEI High Explosive Incendiary"),
        _("AP Armor Piercing"),
        _("TP Target Practice-Tracer"),
        _("SAPHEI High Explosive Armor Piercing PGU"),
        _("TP Target Practice-Tracer PGU"),
    },


    Pylons = {
        pylon(1, 0, 0, -1, 2,
            {
                use_full_connector_position = true, connector = "Pylon1", arg = 308, arg_value = 1,
            },
            {
                { CLSID = "<CLEAN>",  arg_value = -1 },  -- Remove pylon
                { CLSID = "{AIM-9L}", arg_value = 0.2 }, -- AIM-9L
                { CLSID = "CATM-9M",  arg_value = 0.2 }, -- CATM-9M
            },
            1
        )
    },

    -- ===================== 反制系統與感測器 =====================
    passivCounterm = {
        CMDS_Edit = true,        -- 允許在任務中編輯 CMDS 配置
        SingleChargeTotal = 180, -- 箔條與熱焰彈總數
        chaff = {
            default = 90,
            increment = 30,
            chargeSz = 1
        }, -- chaff: 箔條配置
        flare = {
            default = 90,
            increment = 30,
            chargeSz = 1
        } -- flare: 熱焰彈配置
    },    -- 參考 F-16C 的基本配比

    Sensors = {
        RWR = "Abstract RWR"
    }, -- 感測器設定


    EPLRS = true, -- 啟用 EPLRS / 資料鏈定位能力

    -- ===================== AI 任務 (AI Tasks) =====================
    Tasks = {
        aircraft_task(CAP),
        aircraft_task(Escort),
        aircraft_task(FighterSweep),
        aircraft_task(Intercept),
        aircraft_task(PinpointStrike),
        aircraft_task(CAS),
        aircraft_task(GroundAttack),
        aircraft_task(RunwayAttack),
        aircraft_task(SEAD),
        aircraft_task(AFAC),
        aircraft_task(AntishipStrike),
        aircraft_task(Reconnaissance),
    },                                -- end of Tasks
    DefaultTask = aircraft_task(CAP), -- 預設 AI 任務

    -- ===================== 損傷 (Damage) =====================
    -- Damage 區塊可把命名損傷區映射到模型 arg 與 critical_damage。

    Damage = verbose_to_dmg_properties({
        ["Blap"]       = { critical_damage = 3 },
        ["body"]       = { critical_damage = 10 },
        ["Brap"]       = { critical_damage = 3 },
        ["F W"]        = { critical_damage = 4 },
        ["FGG"]        = { critical_damage = 3 },
        ["Flap"]       = { critical_damage = 2 },
        ["Flap.001"]   = { critical_damage = 2 },
        ["LBW"]        = { critical_damage = 4 },
        ["LC"]         = { critical_damage = 4 },
        ["LGG"]        = { critical_damage = 3 },
        ["M wing"]     = { critical_damage = 5 },
        ["M wing.001"] = { critical_damage = 5 },
        ["RBW"]        = { critical_damage = 4 },
        ["RC"]         = { critical_damage = 4 },
        ["RGG"]        = { critical_damage = 3 },
        ["Tail"]       = { critical_damage = 6 },
        ["Wayt"]       = { critical_damage = 3 },
    }),

    -- TODO: 後續補上機翼殘骸模型，例如 F-CK-1C_oblomok_wing_R/L.edm
    -- DamageParts = {
    --     [1] = "F-CK-1C_oblomok_wing_R",
    --     [2] = "F-CK-1C_oblomok_wing_L"
    -- },
    -- 若暫時沒有殘骸模型，可先保持註解狀態。
    -- DamageParts = {}, -- 無殘骸模型時可維持空表

    -- ===================== 簡化飛行模型 (SFM_Data) =====================
    -- SFM_Data 先沿用接近 F-16C 的數值型態，後續再依 EFM / SFM 需求調整。
    SFM_Data = { -- 目前主要提供 AI 與基礎飛行行為
        aerodynamics =
        {
            Cy0 = 0,
            Mzalfa = 4.355,
            Mzalfadt = 0.8,
            kjx = 2.75,
            kjz = 0.00125,
            Czbe = -0.016,
            cx_gear = 0.0268,
            cx_flap = 0.05,
            cy_flap = 0.52,
            cx_brk = 0.06,
            table_data =
            {
                [1] = { 0, 0.0165, 0.07, 0.132, 0.025, 0.5, 30, 1.45 },
                [2] = { 0.2, 0.0165, 0.07, 0.132, 0.025, 1.5, 30, 1.45 },
                [3] = { 0.4, 0.0165, 0.07, 0.133, 0.028, 2.5, 30, 1.45 },
                [4] = { 0.6, 0.0196, 0.073, 0.133, 0.032, 3.5, 30, 1.45 },
                [5] = { 0.7, 0.0228, 0.076, 0.134, 0.034, 3.5, 28.666666666667, 1.45 },
                [6] = { 0.8, 0.0314, 0.079, 0.137, 0.036, 3.5, 27.333333333333, 1.35 },
                [7] = { 0.9, 0.0542, 0.083, 0.1327, 0.042, 3.5, 26, 1.3 },
                [8] = { 1, 0.0707, 0.085, 0.1634, 0.1, 3.5, 24.666666666667, 1.12 },
                [9] = { 1.05, 0.07, 0.0855, 0.1975, 0.095, 3.5, 24, 1.11 },
                [10] = { 1.1, 0.0699, 0.086, 0.215, 0.09, 3.15, 18, 1.1 },
                [11] = { 1.2, 0.0652, 0.083, 0.228, 0.12, 2.45, 17, 1.05 },
                [12] = { 1.3, 0.0605, 0.077, 0.237, 0.17, 1.75, 16, 1 },
                [13] = { 1.49, 0.05, 0.062, 0.241, 0.2, 1.5125, 13.15, 0.905 },
                [14] = { 1.5, 0.0542, 0.061483870967742, 0.241, 0.2058064516129, 1.5, 13, 0.9 },
                [15] = { 1.7, 0.05, 0.051161290322581, 0.24354838709677, 0.32193548387097, 0.9, 12, 0.7 },
                [16] = { 1.8, 0.05, 0.046, 0.24, 0.38, 0.86, 11.4, 0.64 },
                [17] = { 2, 0.0471, 0.039, 0.222, 2.5, 0.78, 10.2, 0.52 },
                [18] = { 2.2, 0.0455, 0.034, 0.227, 3.2, 0.7, 9, 0.4 },
                [19] = { 2.5, 0.039, 0.033, 0.25, 4.5, 0.7, 9, 0.4 },
                [20] = { 3.9, 0.035, 0.033, 0.35, 6, 0.7, 9, 0.4 },
            }, -- end of table_data
        },     -- end of aerodynamics
        engine =
        {
            type              = "TurboFan",
            Nmg               = 67.5,
            Nominal_RPM       = 14710.0,
            Nominal_Fan_RPM   = 8215.0,
            Startup_Prework   = 10.0,
            Startup_Duration  = 35.0,
            Shutdown_Duration = 19.0,
            MinRUD            = 0,
            MaxRUD            = 1,
            MaksRUD           = 0.85,
            ForsRUD           = 0.91,
            hMaxEng           = 19,
            dcx_eng           = 0.0144,
            cemax             = 1.24,
            cefor             = 2.56,
            dpdh_m            = 6200,
            dpdh_f            = 13000,
            table_data        =
            {
                [1] = { 0, 77000, 108313.6 },
                [2] = { 0.2, 74000, 109850 },
                [3] = { 0.4, 74000, 122000 },
                [4] = { 0.6, 82000, 142000 },
                [5] = { 0.7, 85000, 156000 },
                [6] = { 0.8, 92000, 177000 },
                [7] = { 0.9, 100000, 202000 },
                [8] = { 1, 109000, 218000 },
                [9] = { 1.096, 99000, 222000 },
                [10] = { 1.2, 86000, 228000 },
                [11] = { 1.3, 68000, 231000 },
                [12] = { 1.4, 55000, 230000 },
                [13] = { 1.6, 56000, 229000 },
                [14] = { 1.8, 56000, 227000 },
                [15] = { 2.2, 52000, 234000 },
                [16] = { 2.35, 43000, 224000 },
                [17] = { 3.9, 25000, 120636.4 },
            }, -- end of table_data
        },     -- end of engine
    },

    -- ===================== 視覺與網路 (Visual & Net) =====================
    lights_data = { -- 燈光集合
        typename = "collection",
        lights = {
            -- [WOLALIGHT_STROBES] = {
            --     typename = "collection",
            --     lights = {}
            -- },
            -- [WOLALIGHT_LANDING_LIGHTS] = {
            --     typename = "collection",
            --     lights = {}
            -- }
        }
    },

    net_animation = { -- 網路同步動畫參數

    },

    -- 起火與煙霧效果位置 (Fires Position)
    -- 每個項目格式: [index] = {x, y, z}
    -- x 為前後、y 為上下、z 為左右方向
    -- 位置可參考 A-4E-C.lua 的配置方式
    fires_pos =
    { -- 預留火焰 / 煙霧發生點
        -- [1] =     {-0.232,    1.262,    0},     -- Fuselage
        -- [2] =     {-0.2,    -0.5,    0.84},     -- wing (inner?) right, WING_R_IN
        -- [3] =     {-0.75,    -0.5,    -0.8},    -- wing (inner?) left, WING_L_IN
        -- [4] =     {-0.32,    0.265,    1.774},  -- Wing center Right? {-0.82,    0.265,    2.774},
        -- [5] =     {-0.32,    0.265,    -1.774}, -- Wing center Left?  {-0.82,    0.265,    -2.774},
        -- [6] =     {-1.0,    -0.5,    4.0},      -- Wing outer Right? {-0.82,    0.255,    4.274}, probably WING_R_OUT
        -- [7] =     {-1.0,    -0.5,    -4.0},     -- Wing outer Left?  {-0.82,    0.255,    -4.274}, probably WING_L_OUT
        -- [8] =     {-5.6,    0.185,    0},       -- High Altitude Contrails
        -- [9] =     {-5.5,    0.2,    0},         -- left engine
        -- [10] =     {-7.728,    0.039,    0.5},  -- Right Engine? {0.304,    -0.748,    0.442},
        -- [11] =     {-7.728,    0.039,    -0.5}, -- Left Engine?
    },

    -- effects_presets = {{
    --     effect = "OVERWING_VAPOR",
    --     file = current_mod_path .. "/Effects/F-CK-1C_overwingVapor.lua"
    -- }},

    chaff_flare_dispenser = {
        [1] = {
            dir = { 0, -1, 0 },
            pos = { -3.78, -0.45, -0.78 } -- Left side of speedbrake
        },
        [2] = {
            dir = { 0, -1, 0 },
            pos = { -3.78, -0.45, 0.78 } -- Right side of speedbrake
        }
    },

    -- 以下欄位多參考 A-4E 的可選設定，目前先保留註解。
    -- 後續若要補完整 AI / 無線電 / 航艦操作能力，可再逐項開啟。
    -- stores_number = 9, -- 掛載站位總數
    -- average_fuel_consumption = 0.86, -- 平均燃油消耗係數
    -- is_tanker = false, -- 是否為空中加油機
    -- launch_bar_connected_arg_value = 0.87, -- 航艦彈射桿動畫參數
    -- sounderName = "Aircraft/Planes/F-CK-1C", -- 音效資源路徑
    -- -- 艙罩運動限制 (AI / 動畫用)
    -- CanopyGeometry = {
    --     elevation = {-50.0, 90.0}
    -- },
    -- -- 無線電調變設定，通常戰鬥機使用 AM
    -- HumanRadio = {
    --     modulation = MODULATION_AM
    -- },
    -- panelRadio = {}, -- 座艙面板無線電定義
    -- -- 跑道分類限制 (玩家 / AI 起降用)
    -- LandRWCategories = {},
    -- TakeOffRWCategories = {},
    -- -- Failures / Countermeasures / ECM 可於後續補齊
    -- Failures = {},
    -- Countermeasures = {
    --     ECM = "AN/ALQ-126"
    -- }, -- ECM 範例設定

    -- -- ===================== 額外 UI 屬性 =====================
    -- AddPropAircraft = {{
    --     id = "HelmetMountedDevice",
    --     control = 'comboList',
    --     label = _('Helmet Mounted Device'),
    --     values = {{
    --         id = 0,
    --         dispName = _('Not installed'),
    --         value = 0.5
    --     }, {
    --         id = 1,
    --         dispName = _('JHMCS'),
    --         value = 0.0
    --     }, {
    --         id = 2,
    --         dispName = _('NVG'),
    --         value = 1.0
    --     }},
    --     defValue = 1,
    --     wCtrl = 150,
    --     playerOnly = true,
    --     arg = 509
    -- }}

    -- 若之後要加入 datalink，可使用 connectDatalinks / datalinks。
    -- 目前版本先不啟用，避免引用不存在的腳本或資料表。
    -- connectDatalinks = {"Link16"},
    -- datalinks = { Link16 = "CoreMods\\aircraft\\F-CK-1C\\Datalinks\\Link16.lua" }

}

-- 將此機體註冊到 DCS
add_aircraft(F_CK_1C)
