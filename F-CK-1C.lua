-- F-CK-1C.lua
-- 以 F-16C 範本為參考，建立 F-CK-1C 機體設定範本
-- 本檔案目的：提供完整但可修改的機體欄位 (可用於 AI/伺服器測試)
-- 註：某些 helper（pylon, gun_mount, makeAirplaneCanopyGeometry 等）與常數
-- (WSTYPE_PLACEHOLDER, MODULATION_AM 等) 由 DCS 執行環境提供，載入時需在 mod 環境下執行。


-- ===================== 基本說明區 (Identification) =====================
-- Name / DisplayName / shape_table_data 必要用於引擎辨識與載入
local F_CK_1C = {
    -- 機種內部識別名稱
    Name = 'F-CK-1C',
    -- 顯示名稱（可本地化）
    DisplayName = _('F-CK-1C'),

    Rate = 40, -- RewardPoint in Multiplayer

    Shape = "F-CK-1C",
    -- shape_table_data: 告訴引擎要載入的 3D 模型與毀損模型
    shape_table_data = { {
        file = "f-ck-1c", -- 3D 模型檔名（需放在 Shapes 資料夾）
        username = "F-CK-1C",
        index = WSTYPE_PLACEHOLDER,
        life = 20,                 -- 機體生命值（耐久/HP，整數，數值越高越難被摧毀）
        vis = 3,                   -- 可見性等級（LOD/視覺等級，整數）
        desrt = 'Fighter-2-crush', -- Name of destroyed object file name Alphajet-destr. This is a placeholder.
        fire = { 300, 2 },         -- 火焰效果設定：{持續時間秒, 強度}（秒, unitless）
        classname = "lLandPlane",
        positioning = "BYNORMAL"
    },
        -- {
        --     name = "F-CK-1C_destr",
        --     file = "f-ck-1c-oblomok",
        --     fire = {0, 1}
        -- }
    },

    -- 國家
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
    -- UI 顯示相關（可選，影響選單顯示） [OPTIONAL]
    Picture = "F-CK-1C.png", -- [OPTIONAL]
    mapclasskey = "P0091000024",
    WorldID = WSTYPE_PLACEHOLDER,
    attribute = { wsType_Air, wsType_Airplane, wsType_Fighter, WSTYPE_PLACEHOLDER, "Multirole fighters", "Refuelable" },
    Categories = { "{78EFB7A2-FD52-4b57-A6A6-3BF0E1D6555F}", "Interceptor" },

    -- ===================== 操控/機組資訊區 (Crew & Cockpit) =====================
    -- 操控/機組（分類：F-16/A-4E 風格）
    HumanCockpit = true,                                -- 啟用玩家座艙（必須為 true 才能處理玩家輸入）
    HumanCockpitPath = current_mod_path .. '/Cockpit/', -- 座艙腳本路徑

    crew_size = 1,                                      -- 機組人數（AI 會依此分配任務） [AI]
    crew_members = {
        [1] = {
            ejection_seat_name = 17, -- 使用DCS通用彈射座椅ID (參考A-4E)
            -- pilot_name 不是必須欄位
            -- drop_canopy_name = "F-CK-1C_canopy", -- TODO: 需要在模型中定義此名稱
            -- canopy_pos = {3.2, 0.674, 0}, -- 座艙罩位置
            pos = { 3.2, 0.27, 0 }, -- 駕駛座在機體座標的 (x, y, z)，單位：公尺 (m)
            g_suit = 1.02           -- G-suit 補償係數（unitless，通常 1.0 = 無補償）
        }
    },

    -- ===================== 性能參數區 (Mass, Geometry, Performance) =====================
    -- 質量（千克）與燃油
    M_empty = 6492,    -- 空機重量 (kg) | ✓(維基百科)
    M_nominal = 9072,  -- 作戰或典型質量 (kg) | ✓(維基百科)
    M_max = 12530,     -- 最大起飛重量 (kg) | ✓(維基百科)
    M_fuel_max = 2111, -- 最大內部燃油質量 (kg) | ✓(維基百科)

    -- 以下為 AI 相關性能參數（標註為 [AI]），部分取自 A-4E 範例
    CAS_min = 23.15, -- 最低校正空速 (Calibrated Airspeed)，AI 最低飛行速度 (m/s) [AI] | △(參考 A-4E-C，稍微降低)

    -- 幾何尺寸（公尺）
    length = 14.48,                         -- 機身總長 (m) | ✓(維基百科)
    height = 4.7,                           -- 機高 (m) | ✓(維基百科)
    wing_area = 24.26,                      -- 翼面積 (m^2) | ✓(維基百科)
    wing_span = 8.53,                       -- 翼展 (m) | ✓(維基百科)
    wing_tip_pos = { -2.3, 0.0006, 4.396 }, -- 翼尖相對座標 (x,y,z) (m) | ✓(自製模型)

    -- 飛行性能
    V_opt = 220,             -- 最適巡航速度 (m/s) | △(參考 F-16C模組)
    V_take_off = 75,         -- 起飛速度 (m/s) | △(通靈)
    V_land = 65,             -- 著陸速度 (m/s) | △(通靈)
    V_max_sea_level = 369.4, -- 海平面最大持續速度 (m/s) | ✓(維基百科)
    V_max_h = 612.5,         -- 高空最大速度 (m/s) | ✓(維基百科)
    Mach_max = 1.8,          -- 最大馬赫數 (Mach) | ✓(維基百科)
    Vy_max = 238.7,          -- 最大爬升率 (m/s) | △(Gemini 3 Pro AI 計算)
    Ny_min = -3,             -- 最小瞬時容許過載 (-G) | ✓(維基百科)
    Ny_max = 9,              -- 最大瞬時容許過載 (+G) | ✓(維基百科)
    bank_angle_max = 60,     -- 最大滾轉/橫滾角度 (度) | △(參考F-16C模組與A-4E-C模組)
    range = 2200,            -- 航程 (公里) [AI] | ✓(維基百科)

    -- 推力
    thrust_sum_max = 5506.47, -- 常規最大推力 (通常單位為 kgf 或 N，依專案慣例) | ✓(維基百科)
    thrust_sum_ab = 8524.83,  -- 加力時最大推力 (kgf 或 N) | ✓(維基百科)
    has_afterburner = true,   -- 是否配備加力燃燒器（布林） | ✓(維基百科)

    -- 阻力/雷達/紅外
    RCS = 3.6,                   -- 雷達散射截面 (m^2) | △(介於F-16C模組與A-4E-C模組之間)
    radar_can_see_ground = true, -- 雷達是否能偵測地面/海面目標（布林） | ✓(我們都有反艦導彈了)
    detection_range_max = 150,   -- 感測或雷達最大探測距離 (km) | △(AN/APG-67維基百科)
    IR_emission_coeff = 0.73,    -- 紅外發射係數（常態，無單位） | △(參考FA-18模組)
    IR_emission_coeff_ab = 4.0,  -- 紅外發射係數（加力時，無單位） | △(參考FA-18模組)

    -- 空中加油
    -- air_refuel_receptacle_pos = {-0.051, 0.911, 0.0}, -- 空中加油接收器位置 (x,y,z) (m)
    -- tanker_type = 1, -- 空中加油機分類（整數，DCS 定義）

    -- ===================== 起落架區 (Landing Gear) =====================
    tand_gear_max = 0.761,                                    -- 前輪轉向最大弧度 (rad) | △(模型估算)
    nose_gear_pos = { 4.12, -1.912, 0 },                      -- 前輪位置 (x,y,z) (m) | ✓(模型)
    nose_gear_amortizer_direct_stroke = 0.0,                  -- 減震器伸展量 (m) | ✓(模型)
    nose_gear_amortizer_reversal_stroke = 1.712 - 1.912,      -- 減震器收縮量 (m) | ✓(模型)
    nose_gear_amortizer_normal_weight_stroke = 1.812 - 1.912, -- 常重位置位移 (m) | ✓(模型)
    nose_gear_wheel_diameter = 0.4572,                        -- 前輪直徑 (m) | ✓(模型)

    main_gear_pos = { -1.185, -1.913, 0.7905 },               -- 主輪位置 (x,y,z) (m) | ✓(模型)
    main_gear_amortizer_direct_stroke = 0,                    -- 主輪減震伸展量 (m) | ✓(模型)
    main_gear_amortizer_reversal_stroke = 1.727 - 1.913,      -- 主輪減震收縮量 (m) | ✓(模型)
    main_gear_amortizer_normal_weight_stroke = 1.796 - 1.913, -- 主輪正常負重位移 (m) | ✓(模型)
    main_gear_wheel_diameter = 0.6096,                        -- 主輪直徑 (m) | ✓(模型)

    -- nose_gear_door_close_after_retract = false,
    -- main_gear_door_close_after_retract = false,

    -- ===================== 引擎區 (Engine) =====================
    engines_count = 2, -- 引擎數量（整數）
    engines_nozzles = {
        [1] = {
            pos = { -6.118, 0.0918, 0.4452 },                -- 噴嘴相對機體座標 (x,y,z) (m) | ✓(模型)
            elevation = 0,                                   -- 噴嘴仰角 (度)  | ✓(模型)
            diameter = 0.64,                                 -- 噴嘴直徑 (m)  | ✓(模型)
            exhaust_length_ab = 3.5,                         -- 加力噴焰視覺長度 (m) | △(模型估算)
            exhaust_length_ab_K = 0.77,                      -- 加力視覺縮放係數（無單位）| △(這個數字比較考看)
            smokiness_level = 0.05,                          -- 噴口冒煙程度（0-1 無單位） | △(參考F-16C模組與FA-18模組)
            afterburner_circles_count = 6,                   -- 加力環的數量（整數） | △(照片)
            -- afterburner_circles_pos = {0.2, 0.8}, -- 加力環位置比例（相對座標或比例值） | △(不知道填甚麼)
            afterburner_circles_scale = 1.0,                 -- 加力環縮放係數（無單位） | △(參考F16C)
            afterburner_effect_texture = "afterburner_f-16c" -- 加力噴焰效果紋理 | △(用F16C的)
        },
        [2] = {
            pos = { -6.118, 0.0918, -0.4452 },               -- 噴嘴相對機體座標 (x,y,z) (m) | ✓(模型)
            elevation = 0,                                   -- 噴嘴仰角 (度)  | ✓(模型)
            diameter = 0.64,                                 -- 噴嘴直徑 (m)  | ✓(模型)
            exhaust_length_ab = 3.5,                         -- 加力噴焰視覺長度 (m) | △(模型估算)
            exhaust_length_ab_K = 0.77,                      -- 加力視覺縮放係數（無單位）| △(這個數字比較考看)
            smokiness_level = 0.05,                          -- 噴口冒煙程度（0-1 無單位） | △(參考F-16C模組與FA-18模組)
            afterburner_circles_count = 6,                   -- 加力環的數量（整數） | △(照片)
            -- afterburner_circles_pos = {0.2, 0.8}, -- 加力環位置比例（相對座標或比例值） | △(不知道填甚麼)
            afterburner_circles_scale = 1.0,                 -- 加力環縮放係數（無單位） | △(參考F16C)
            afterburner_effect_texture = "afterburner_f-16c" -- 加力噴焰效果紋理 | △(用F16C的)
        }
    },

    -- ===================== 武裝區 (Guns & Pylons) =====================
    -- 武裝區（分類說明）
    -- - 參考 F-16 的程式化 pylon 定義方式，適合大量武裝選項與共用清單。此處保留簡化版。
    -- - 若要 AI 能正確掛載任務武器，Pylons 與 ammo_type/Stores 應完整，否則 AI 可能不會在任務中使用武裝。 [AI]
    -- 這裡採程式化定義，參考 F-16C 的 pylon 整理；可視需求縮減或展平為單純 CLSID 清單
    -- 以下範例保留基本 Gun 與示意 Pylons（若不需要武裝可把 Pylons 設為 {} 或只保留 <CLEAN>）
    -- TODO: 需自定義機砲函數或使用DCS內建機砲定義
    -- Guns = {gun_mount("M_61", {
    --     mixes = {{1}, {2}, {3}},
    --     count = 510
    -- }, {
    --     supply_position = {0.4, 0.55, 0.0},
    --     effects = {gatling_effect(351, 6), fire_effect(350), smoke_effect()}
    -- })},
    Guns = {},             -- [DISABLED - M_61機砲需自定義或驗證，暫時禁用避免載入錯誤]
    ammo_type_default = 1, -- 機砲彈藥類型預設索引（整數，對應 ammo_type 列表）
    ammo_type = { _("HEI-T High Explosive Incendiary-Tracer"), _("HEI High Explosive Incendiary"), _("AP Armor Piercing") },

    -- Pylons 範例（精簡版） [OPTIONAL]
    -- TODO: 需要在 F-CK-1C.edm 模型中定義 Pylon1, Pylon5 等連接器
    -- Pylons = { -- 每個 pylon 使用 pylon(index, ... ) helper
    -- pylon(1, 0, -2.2, 0.002, -4.739,
    -- {
    --     arg = 308,
    --     arg_value = 0,
    --     use_full_connector_position = true,
    --     connector = "Pylon1" -- 模型中需定義此連接器
    -- }, {
    --     { CLSID = "<CLEAN>", arg_value = 1 }, -- 空掛載點
    -- }),
    -- pylon(5, 0, -0.704, -1.173, 0.0, {
    --     arg = 312,
    --     arg_value = 0,
    --     use_full_connector_position = true,
    --     connector = "Pylon5", -- 模型中需定義此連接器
    --     mass = 78.9
    -- }, {
    --     { CLSID = "<CLEAN>", arg_value = 1 },
    -- }),
    -- },
    Pylons = {}, -- [DISABLED - 模型連接器未定義，暫時禁用避免載入錯誤]

    -- ===================== Countermeasures / Sensors 區 =====================
    passivCounterm = {
        CMDS_Edit = true,        -- 是否允許在界面編輯 CMDS（布林）
        SingleChargeTotal = 120, -- 總彈藥量（chaff + flare 總和）(整數)
        chaff = {
            default = 60,
            increment = 30,
            chargeSz = 1
        }, -- chaff: 預設數量、增量、單次釋放數
        flare = {
            default = 60,
            increment = 30,
            chargeSz = 1
        } -- flare: 預設數量、增量、單次釋放數
    },    --干擾措施投放設定 | △(參考F16C)

    Sensors = {
        RADAR = "AN/APG-67V", -- | ✓(維基百科)
        RWR = "Abstract RWR"  -- | △(參考F16C)
    },                        --雷達/電子戰設定


    EPLRS = true, --Enhanced Position Location Reporting System（增強型位置定位回報系統） | ✓(基本概念)

    -- ===================== 任務與分類 (AI Tasks) =====================
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
    DefaultTask = aircraft_task(CAP), -- | △(參考F16C)

    -- ===================== 損傷 (Damage) - AI/遊戲邏輯重要 =====================
    -- Damage 定義告訴模擬器各區塊被擊中時的行為與臨界傷害，對 AI 生存與傷害判定影響甚大。 [AI]

    Damage = verbose_to_dmg_properties({ -- 傷害分區配置（mapping 名稱->args/critical_damage）| ✖(暫時沒有)
        -- ["NOSE_CENTER"] = {
        --     args = {146},
        --     critical_damage = 3
        -- },
        -- ["COCKPIT"] = {
        --     args = {65},
        --     critical_damage = 6
        -- },
        -- ["WING_L_IN"] = {
        --     args = {225},
        --     critical_damage = 5
        -- },
        -- ["WING_R_IN"] = {
        --     args = {215},
        --     critical_damage = 5
        -- },
        -- ["ENGINE_C"] = {
        --     args = {160},
        --     critical_damage = 2
        -- },
        -- ["HOOK"] = {
        --     critical_damage = 2
        -- }
    }),

    -- TODO: 需要創建碎片模型 F-CK-1C_oblomok_wing_R.edm 和 F-CK-1C_oblomok_wing_L.edm
    -- DamageParts = {
    --     [1] = "F-CK-1C_oblomok_wing_R",
    --     [2] = "F-CK-1C_oblomok_wing_L"
    -- },
    -- 暫時禁用以避免引擎嘗試載入不存在的模型
    -- DamageParts = {}, -- 空陣列可能也會有問題，完全省略此欄位

    -- ===================== 飛行模型 (SFM_Data) - 簡化版 =====================
    -- 注意：完整 SFM_Data 很敏感，如要穩定飛行建議複製或微調現成機種數值
    SFM_Data = { -- |  △(複製於 F-16C)
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

    -- ===================== 燈光、效果、網路同步等 (Visual & Net) =====================
    lights_data = { -- | ✖(暫時沒有燈光)
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

    net_animation = { -- | ✖(暫時沒有網路同步)

    },

    -- 煙霧/火焰位置 (Fires Position)
    -- 格式：[編號] = {x, y, z}
    -- 座標軸：x 向右、y 向上、z 向前
    -- 複製於 A-4E-C.lua
    fires_pos =
    { -- | ✖(暫時沒有)
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
        -- [11] =     {-7.728,    0.039,    -0.5}, -- ?
    },

    -- effects_presets = {{
    --     effect = "OVERWING_VAPOR",
    --     file = current_mod_path .. "/Effects/F-CK-1C_overwingVapor.lua"
    -- }},

    -- chaff_flare_dispenser = {
    --     [1] = {
    --         dir = {0, -1, 0},
    --         pos = {-3.65, -0.5, -0.93}
    --     },
    --     [2] = {
    --         dir = {0, -1, 0},
    --         pos = {-3.91, -0.5, -0.93}
    --     }
    -- },

    -- 以下為從 A-4E 參考來的可選欄位，對 AI 或伺服器測試常有用：
    -- 若不需要可刪除或註解。
    -- stores_number = 9, -- 掛載點數量 (整數，供 AI/配置介面參考)
    -- average_fuel_consumption = 0.86, -- 平均油耗 (TSFC 或專案約定單位，unitless)
    -- is_tanker = false, -- 是否為加油機 (布林)
    -- launch_bar_connected_arg_value = 0.87, -- 發射吊架連接顯示參數 (arg value，用於載具 UI)
    -- sounderName = "Aircraft/Planes/F-CK-1C", -- 聲音資源路徑/名稱（字串，非必需但常見）
    -- -- 機艙視角限制 (AI 使用或視角約束)
    -- CanopyGeometry = {
    --     elevation = {-50.0, 90.0}
    -- },
    -- -- 通訊設定（常用常數 MODULATION_AM 由環境提供）
    -- HumanRadio = {
    --     modulation = MODULATION_AM
    -- },
    -- panelRadio = {}, -- 面板/無線電設定佔位 (可填具體表格)
    -- -- 跑道分類/起降分類 (可由地圖/AI 使用)
    -- LandRWCategories = {},
    -- TakeOffRWCategories = {},
    -- -- Failures 與簡單的 Countermeasures / ECM 入口
    -- Failures = {},
    -- Countermeasures = {
    --     ECM = "AN/ALQ-126"
    -- }, -- 簡單 ECM 欄位參考（字串）

    -- -- ===================== 其他可選屬性（UI、玩家專屬） =====================
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

    -- 如需 datalink，填寫 connectDatalinks / datalinks
    -- 本機版本未包含 datalinks，先註解以避免載入錯誤；若要啟用，請把下面兩行取消註解並確保檔案存在。
    -- connectDatalinks = {"Link16"},
    -- datalinks = { Link16 = "CoreMods\\aircraft\\F-CK-1C\\Datalinks\\Link16.lua" }

}

-- 註：add_aircraft 為 DCS 提供之函數，用於註冊機體
add_aircraft(F_CK_1C)
