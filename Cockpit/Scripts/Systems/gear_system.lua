local dev = GetSelf() -- 獲取當前設備指標

dofile(LockOn_Options.script_path .. "argument.lua")

-- 設定更新頻率
local update_rate = 0.01
make_default_activity(update_rate)

-- 1. 建立資料源連接 (這就是資料流的源頭)
local sensor_data = get_base_data()


function update()
    -- 2. 擷取控制 (Capture Input)
    -- getRudderPosition() 回傳值是角度
    local raw_rudder = sensor_data:getRudderPosition()

    -- 3. 實際控制 (Output Control)
    -- 直接把讀到的數值，塞給動畫參數
    -- Nose wheel steering arg 2 is driven by the EFM.

    -- -- 除錯用：印出數值到螢幕上 (測試完可以刪掉)
    -- print_message_to_user("Rudder Input: " .. tostring(raw_rudder))
end

function post_initialize()
    update() -- 初始化時先執行一次，歸零位置
end

need_to_be_closed = false
