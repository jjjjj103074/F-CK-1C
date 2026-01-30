-- 1. 告訴 DCS 這些腳本具有遊戲介面功能
-- attributes = {
--     "cockpit_game_interface",
--     "check_simplification",
-- }

-- -- 2. 定義視角原點 (你的眼睛位置)
-- -- 你之後可以微調這些數字 (前後, 上下, 左右)
-- layout = {
--     {0.0, 0.0}, -- 忽略
--     {
--         cockpit_local_point = { -1.0, 0.6, 0.0 }, -- 眼睛在機身中心線, 稍微靠後, 稍微高一點
--     },
-- }

-- -- 3. 核心初始化 (最重要！沒有這個就是 AI 接管)
-- -- 這會告訴 DCS 去讀取 mainpanel_init.lua
-- creators = {}
-- creators[LockOn_Options.main_panel_source_id] = {"avMainPanel", LockOn_Options.script_path.."mainpanel_init.lua"}

-- -- 4. 載入基本指示器 (可選，防止報錯)
-- indicators = {}

dofile(LockOn_Options.common_script_path.."KNEEBOARD/declare_kneeboard_device_left.lua")