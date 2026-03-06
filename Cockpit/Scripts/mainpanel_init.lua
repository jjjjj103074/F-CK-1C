-- 定義這是一個儀表板（即使是簡單的外部視角也需要這個）
shape_name = "default"
livery = "default"

is_3d = false -- 暫時使用 2D 模式（外部視角）

-- 對應到 device_init.lua 的定義
-- 這會創建控制器並連接玩家輸入到飛行系統
local controllers = LoRegisterPanelControls()

-- 這個標記告訴 DCS 儀表板邏輯已就緒
need_to_be_closed = true
