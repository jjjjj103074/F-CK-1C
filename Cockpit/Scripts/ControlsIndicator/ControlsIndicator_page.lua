--[[
================================================================================
    F-CK-1C 操控指示器頁面定義（基於 F-16 版本）
    Controls Indicator Page Definition (Based on F-16)

    此檔案定義了螢幕上顯示的操控指示器，用於即時顯示玩家的：
    - 操縱桿位置（俯仰/滾轉）
    - 配平位置
    - 方向舵踏板位置
    - 油門位置
    - 左右輪煞車狀態
================================================================================
--]]

-- 載入 DCS 元素定義（包含創建顯示元素的基本函數）
dofile(LockOn_Options.common_script_path .. "elements_defs.lua")

-- 設置自定義縮放比例 (無單位)
-- 1.0 = 100% 原始大小
SetCustomScale(1.0)

--[[
    輔助函數：添加元素到顯示器
    確保所有元素都以螢幕空間模式顯示並啟用 Mipmap 過濾
--]]
function AddElement(object)
	object.screenspace = ScreenType.SCREENSPACE_TRUE -- 使用螢幕空間座標系統
	object.use_mipfilter = true                   -- 啟用 Mipmap 過濾（改善遠距離顯示品質)
	Add(object)
end

--[[
================================================================================
    材質定義區
    Material Definitions

    定義所有顯示元素使用的材質
    這些材質是必須的，否則元素不會顯示
================================================================================
--]]

-- 🔴 紅色材質（用於大部分線條和指示器）
-- 使用 DCS 共用的 arcade.tga 貼圖
local ARCADE = MakeMaterial("Bazar/Textures/AvionicsCommon/arcade.tga", { 255, 0, 0, 255 })

-- ⚪ 白色材質（用於實際煞車位置指示線）
local ARCADE_WHITE = MakeMaterial("Bazar/Textures/AvionicsCommon/arcade.tga", { 255, 255, 255, 255 })

-- 🎭 遮罩材質（用於煞車填充效果）
-- 注意：如果這個材質導致問題，可以用 ARCADE 替代
local MASK_MATERIAL = MakeMaterial(nil, { 255, 255, 255, 255 })

--[[
================================================================================
    操縱桿物理參數區
    Stick Physics Parameters

    這些參數基於真實 F-16 操縱桿的物理特性
================================================================================
--]]

-- 操縱桿前推最大行程 (度數)
-- F-16 操縱桿向前推（機頭向下）的最大角度
-- 修改建議: 調整此值會改變俯仰向下的最大範圍
local pitch_stick_fwd = 40.0

-- 操縱桿後拉最大行程 (度數)
-- F-16 操縱桿向後拉（機頭向上）的最大角度
-- 修改建議: 調整此值會改變俯仰向上的最大範圍
local pitch_stick_aft = 40.0

-- 操縱桿總行程 (度數)
-- 前後總行程 = 前推 + 後拉
local pitch_stick_range = pitch_stick_fwd + pitch_stick_aft

-- 操縱桿俯仰方向實際物理行程 (英吋)
-- F-16 操縱桿頂部前後移動的實際距離
local pitch_stick_real_range = 9.03125

-- 操縱桿前推部分所佔比例 (無單位，範圍 0.0-1.0)
-- 40/(40+40) = 0.5，表示前推佔總行程的 50%
local pitch_stick_part_positive = pitch_stick_fwd / pitch_stick_range

-- 操縱桿後拉部分所佔比例 (無單位，範圍 0.0-1.0)
-- 40/(40+40) = 0.5，表示後拉佔總行程的 50%
local pitch_stick_part_negative = pitch_stick_aft / pitch_stick_range

-- 操縱桿滾轉方向實際物理行程 (英吋)
-- F-16 操縱桿頂部左右移動的實際距離
local roll_stick_real_range = 11.875

-- 操縱桿左滾部分所佔比例 (無單位)
-- 0.5 表示左右對稱
local roll_stick_part_left = 0.5

-- 操縱桿右滾部分所佔比例 (無單位)
-- 0.5 表示左右對稱
local roll_stick_part_right = 0.5

--[[
================================================================================
    油門參數區
    Throttle Parameters
================================================================================
--]]

-- 油門怠速位置 (無單位，範圍 0.0-1.0)
-- 0.1 表示怠速位置在總行程的 10%
-- 修改建議: 調整此值會改變怠速刻度線的位置
local throttleIdle = 0.1

-- 油門軍用推力位置 (無單位，範圍 0.0-1.0)
-- 0.775 表示軍推位置在總行程的 77.5%
-- 修改建議: 調整此值會改變軍推刻度線的位置
local throttleMil = 0.775

--[[
================================================================================
    顯示區域尺寸計算
    Display Area Size Calculations
================================================================================
--]]

-- 螢幕寬高比 (無單位，例如 16:9 ≈ 1.78)
local aspect = LockOn_Options.screen.aspect

-- 水平基礎大小 (螢幕座標單位，範圍 0.0-1.0)
-- 0.15 表示佔螢幕高度的 15%
-- 修改建議: 增大此值會讓整個指示器變大
local size_x = 0.15 -- +-size_x 表示左右各延伸 size_x

-- 垂直正向大小（向上延伸）(螢幕座標單位)
-- 根據物理行程比例計算，確保指示器比例符合真實操縱桿
-- 計算公式: 基礎大小 × 長寬比調整 × 物理行程比 × 前推比例
local size_y_positive = size_x * 2.2 * pitch_stick_real_range / roll_stick_real_range *
	pitch_stick_part_positive

-- 垂直負向大小（向下延伸）(螢幕座標單位)
-- 根據物理行程比例計算
local size_y_negative = size_x * 2.2 * pitch_stick_real_range / roll_stick_real_range *
	pitch_stick_part_negative

-- 貼圖縮放比例 (無單位)
-- 用於調整貼圖在元素上的顯示大小
local tex_scale = 0.25 / size_x

-- 線條寬度 (螢幕座標單位)
-- 4/512 是基於 512x512 貼圖的像素寬度，0.5 是額外的細化係數
-- 修改建議: 增大 4 或 0.5 倍數會讓線條變粗
local line_width = (4 / 512) / tex_scale * 0.5

--[[
    備用操縱桿移動範圍參數（用於兼容性）
    Alternative Stick Movement Parameters (for compatibility)
--]]

-- 操縱桿橫向（滾轉）移動量 (像素單位)
-- 140 像素是參考值
local roll_stick_movement = 140

-- 縮放係數，將像素單位轉換為螢幕座標
local k = size_x / roll_stick_movement

-- 操縱桿前推移動距離（備用計算方式）
-- 0.76 是縮放因子，0.625 是前推比例調整
local pitch_stick_movement_positive = 0.76 * roll_stick_movement * 0.625 * k

-- 操縱桿後拉移動距離（備用計算方式）
-- 0.375 是後拉比例調整
local pitch_stick_movement_negative = 0.76 * roll_stick_movement * 0.375 * k

--[[
    螢幕空間模式定義（註解僅供參考）
    CURR_SCREENSPACE_NONE = 0,          -- 無螢幕空間
    CURR_SCREENSPACE_HUD_ONLY_VIEW = 1, -- 僅 HUD 視角
    CURR_SCREENSPACE_ARCADE = 2,        -- 街機模式
--]]

--[[
================================================================================
    間距與偏移參數
    Spacing and Offset Parameters
================================================================================
--]]

-- 裝飾性間距 (螢幕座標單位)
-- 用於在各元素之間添加視覺間距
local ds = 0.05 * size_x

-- 方向舵區域偏移 (螢幕座標單位)
-- 2 * 0.1 = 0.2，控制方向舵顯示區域的寬度
-- 修改建議: 調整倍數可以改變方向舵區域的大小
local rud_shift = 2 * 0.1 * size_x

-- 方向舵基座偏移 (螢幕座標單位)
-- 控制方向舵整體位置，目前設為與 rud_shift 相同
local rud_base_shift = rud_shift

-- 半透明橙色材質（背景用）
-- 參數: ("", {R, G, B, A})
-- A=50 表示高度透明，範圍 0-255
-- 修改建議: 調整 RGB 值改變顏色，調整 A 值改變透明度
local orange_mat = MakeMaterial("", { 255, 100, 0, 50 })

-- 方向舵顯示區域的垂直偏移 (螢幕座標單位)
-- 控制方向舵刻度與操縱桿區域的間距
local rudder_shift = 0.1 * size_x

--[[
================================================================================
    輔助函數：add_line
    Helper Function: add_line

    用於快速創建線條元素的輔助函數
    參數說明：
    - name: 元素名稱（字串）
    - pos: 位置 {x, y}（螢幕座標）
    - length: 線條長度（螢幕座標）
    - _width05: 線條半寬度（可選，默認使用 line_width）
    - rot: 旋轉角度（度數，可選）
    - parent: 父元素名稱（字串，可選）
    - controllers: 控制器陣列（可選）

    返回值：創建的元素

    修改建議：如果需要統一修改所有線條的外觀，在此函數中調整
================================================================================
--]]
local function add_line(name, pos, length, _width05, rot, parent, controllers)
	-- 線條半寬度（若未指定則使用默認值）
	local width05 = line_width
	if _width05 ~= nil then
		width05 = _width05
	end

	-- 創建矩形線條元素
	local elem = CreateElement "ceTexPoly"
	elem.name = name      -- 設定元素名稱
	elem.material = "ARCADE" -- 使用 ARCADE 貼圖材質

	-- 定義線條的四個頂點（形成一個矩形）
	-- 線條從原點 (0,0) 延伸到 (length, 0)，寬度為 2*width05
	elem.vertices = {
		{ 0, -width05 }, -- 起點下方
		{ 0, width05 }, -- 起點上方
		{ length, width05 }, -- 終點上方
		{ length, -width05 } -- 終點下方
	}

	elem.indices = default_box_indices -- 使用預設索引繪製矩形

	-- 貼圖 UV 座標設定
	-- {u起始, v起始, u縮放, v縮放}
	elem.tex_params = { (128 + 64) / 512, 176.5 / 512, 0.25 / tex_scale, 0.015625 / (width05 * 2) }

	-- 設定初始位置 {x, y, z}
	elem.init_pos = { pos[1], pos[2], 0 }

	-- 如果指定了旋轉角度，設定旋轉
	if rot ~= nil then
		elem.init_rot = { rot, 0, 0 } -- {繞X軸, 繞Y軸, 繞Z軸} (度數)
	end

	-- 如果指定了父元素，設定父子關係
	if parent ~= nil then
		elem.parent_element = parent
	end

	-- 如果指定了控制器，設定控制器
	if controllers ~= nil then
		elem.controllers = controllers
	end

	-- 將元素添加到顯示器
	AddElement(elem)

	-- 返回創建的元素（供其他元素引用）
	return elem
end

--[[
================================================================================
    背景元素區
    Background Element
================================================================================
--]]

-- BASE -----------------------------------------------------------------------
-- 創建主背景多邊形（半透明橙色背景）
base = CreateElement "ceMeshPoly"
base.name = "base"               -- 元素名稱，作為其他元素的父級
base.primitivetype = "triangles" -- 使用三角形繪製模式
base.material = orange_mat       -- 半透明橙色材質

-- 背景的四個頂點座標
-- 包含操縱桿區域、油門區域、方向舵區域的整個顯示範圍
-- 修改建議: 調整這些座標可以改變整個背景的大小
base.vertices = {
	{ -(size_x + rud_shift + rud_base_shift + 2 * ds), -(size_y_negative + 2 * ds + rudder_shift) }, -- 左下角
	{ -(size_x + rud_shift + rud_base_shift + 2 * ds), size_y_positive + ds },                    -- 左上角
	{ size_x + ds, size_y_positive + ds },                                                        -- 右上角
	{ size_x + ds, -(size_y_negative + 2 * ds + rudder_shift) }                                   -- 右下角
}

base.indices = default_box_indices -- 使用預設索引繪製矩形

-- 背景在螢幕上的初始位置
-- {x偏移, y偏移}
-- 修改建議: 調整這些值可以移動整個指示器在螢幕上的位置
base.init_pos = { 0, -(1 - 1.3 * size_x) }

-- 控制器設定
base.controllers = {
	{ "show" },                                            -- 控制顯示/隱藏
	{ "screenspace_position", 2, -(aspect - 2 * size_x), 0 }, -- 水平定位（根據螢幕比例調整）
	{ "screenspace_position", 1, 0, 0 }                    -- 垂直定位
}

-- 裁剪層級設定
base.h_clip_relation = h_clip_relations.REWRITE_LEVEL -- 重寫裁剪層級
base.level = DEFAULT_LEVEL                            -- 使用預設繪製層級

AddElement(base)

--[[
================================================================================
    操縱桿刻度線區
    Stick Scales
================================================================================
--]]

-- STICK SCALE-----------------------------------------------------------------
-- 俯仰刻度線（垂直線）
-- 位置從 -size_y_negative 到 size_y_positive，90度旋轉使其垂直
-- 修改建議: 要改變俯仰範圍的顯示，修改 size_y_negative 和 size_y_positive
local pitch_scale = add_line(
	"pitch_scale",                  -- 元素名稱
	{ 0, -size_y_negative },        -- 位置 {x, y}
	size_y_negative + size_y_positive, -- 線條長度
	nil,                            -- 寬度（使用默認）
	90,                             -- 旋轉 90 度（垂直）
	base.name                       -- 父元素
)

-- 滾轉刻度線（水平線）
-- 位置從 -size_x 到 size_x，水平放置
-- 修改建議: 要改變滾轉範圍的顯示，修改 size_x
local roll_scale = add_line(
	"roll_scale", -- 元素名稱
	{ -size_x, 0 }, -- 位置 {x, y}
	size_x * 2,  -- 線條長度（左右各 size_x）
	nil,         -- 寬度（使用默認）
	nil,         -- 不旋轉（水平）
	base.name    -- 父元素
)

--[[
================================================================================
    操縱桿位置指示器
    Stick Position Indicator
================================================================================
--]]

-- STICK ----------------------------------------------------------------------
-- 操縱桿圖標大小
-- 0.1 表示圖標佔主刻度的 10%
-- 修改建議: 增大此值會讓操縱桿圖標變大
local stick_index_size = 0.1 * size_x

-- 創建操縱桿位置指示器（顯示當前操縱桿位置）
stick_position = CreateElement "ceTexPoly"
stick_position.name = "stick_position"

-- 操縱桿指示器的四個頂點（正方形圖標）
stick_position.vertices = {
	{ -stick_index_size, -stick_index_size }, -- 左下
	{ -stick_index_size, stick_index_size }, -- 左上
	{ stick_index_size, stick_index_size }, -- 右上
	{ stick_index_size, -stick_index_size } -- 右下
}

stick_position.indices = default_box_indices
stick_position.material = "ARCADE" -- 使用 ARCADE 貼圖

-- 貼圖參數（指向操縱桿圖示的貼圖位置）
stick_position.tex_params = { 330 / 512, 365.5 / 512, 2 * tex_scale, 2 * tex_scale / 0.8 }

-- ⚠️ 操縱桿控制器（自定義 controller，需要額外定義）
-- "stick_pitch" - 控制垂直移動（俯仰），參數為最大移動範圍
-- "stick_roll" - 控制水平移動（滾轉），參數為最大移動範圍
stick_position.controllers = {
	{ "move_up_down_using_parameter", 0, 74, size_y_negative }, -- 俯仰控制
	{ "move_left_right_using_parameter", 0, 71, size_x }     -- 滾轉控制
}

stick_position.parent_element = base.name
AddElement(stick_position)

-- --[[
-- ================================================================================
--     配平位置指示器
--     Trimmer Position Indicator
-- ================================================================================
-- --]]

-- -- STICK TRIMMER --------------------------------------------------------------
-- -- 配平位置指示器（顯示配平設定的位置）
-- trimmer_position = CreateElement "ceTexPoly"
-- trimmer_position.name = "trimmer_position"

-- -- 配平指示器的頂點（與操縱桿指示器相同大小）
-- trimmer_position.vertices = {
-- 	{ -stick_index_size, -stick_index_size },
-- 	{ -stick_index_size, stick_index_size },
-- 	{ stick_index_size,  stick_index_size },
-- 	{ stick_index_size,  -stick_index_size }
-- }

-- trimmer_position.indices = default_box_indices
-- trimmer_position.material = "ARCADE"
-- trimmer_position.tex_params = { 330 / 512, 365.5 / 512, 2 * tex_scale, 2 * tex_scale / 0.8 }

-- -- ⚠️ 配平控制器（自定義 controller，需要額外定義）
-- -- "trimmer_stick_pitch" - 配平俯仰位置
-- -- "trimmer_stick_roll" - 配平滾轉位置
-- -- "scale" - 縮小到 70% 以區分主操縱桿和配平位置
-- trimmer_position.controllers = {
-- 	{ "trimmer_stick_pitch", size_y_negative },
-- 	{ "trimmer_stick_roll", size_x },
-- 	{ "scale", 0.7, 0.7 } -- 縮放為 70%
-- }

-- trimmer_position.parent_element = base.name
-- AddElement(trimmer_position)

-- -- 配平刻度標記（目前已停用）
-- -- 可用於顯示特定配平位置（例如 10 度）
-- trimmer10 = CreateElement "ceTexPoly"
-- trimmer10.name = "trimmer10"
-- trimmer10.vertices = {
-- 	{ -rud_shift * 0.5, -line_width },
-- 	{ -rud_shift * 0.5, line_width },
-- 	{ rud_shift * 0.5,  line_width },
-- 	{ rud_shift * 0.5,  -line_width }
-- }
-- trimmer10.indices = default_box_indices
-- trimmer10.material = "ARCADE"
-- trimmer10.tex_params = { 256 / 512, 176.5 / 512, tex_scale, 2 * tex_scale }

-- -- 配平標記的控制器
-- trimmer10.controllers = {
-- 	{ "move", -10.0 / 17.0 * size_y_negative }, -- 移動到 -10 度位置
-- 	{ "rotate", math.rad(90) },              -- 旋轉 90 度
-- 	{ "scale", 0.7, 1.0 }                    -- 縮放
-- }

-- trimmer10.parent_element = pitch_scale.name
-- --AddElement(trimmer10)  -- 已註解，如需啟用請移除註解

--[[
================================================================================
    方向舵區域
    Rudder (Pedals) Section
================================================================================
--]]

-- PEDALS ---------------------------------------------------------------------
-- 方向舵刻度線（水平線，位於操縱桿區域下方）
local rudder_scale = add_line(
	"rudder_scale",                              -- 元素名稱
	{ -size_x, -(size_y_negative + rudder_shift) }, -- 位置
	size_x * 2,                                  -- 線條長度
	nil,                                         -- 寬度（默認）
	nil,                                         -- 不旋轉
	base.name                                    -- 父元素
)

-- 方向舵位置指示器
-- ⚠️ 使用自定義 controller "rudder" 來控制水平移動
local rudder_index = add_line(
	"rudder_index",                            -- 元素名稱
	{ 0, -(size_y_negative + rudder_shift * 2) }, -- 位置
	rudder_shift * 2,                          -- 線條長度
	nil,                                       -- 寬度（默認）
	nil,                                       -- 不旋轉（由 controller 處理）
	base.name,                                 -- 父元素
	{                                          -- 控制器
		{ "rudder", size_x },                  -- 方向舵控制，最大移動距離為 size_x
		{ "rotate", math.rad(90) }             -- 旋轉 90 度使其垂直移動
	}
)

--[[
================================================================================
    油門區域
    Throttle Section
================================================================================
--]]

-- THROTTLE SCALE -------------------------------------------------------------
-- 油門刻度的水平位置（位於左側）
local throttle_px = -(size_x + rud_base_shift + ds)

-- 油門刻度的垂直起始位置
local throttle_py = -(size_y_negative + rudder_shift * 2)

-- 油門刻度線的總長度
-- 從最下方延伸到最上方，涵蓋整個油門行程範圍
local throttle_scale_length = size_y_negative + size_y_positive + rudder_shift * 2

-- 主油門刻度線（垂直線）
local throttle_scale = add_line(
	"throttle_scale",
	{ throttle_px, throttle_py },
	throttle_scale_length,
	nil,
	90, -- 旋轉 90 度（垂直）
	base.name
)

-- 怠速位置標記線（水平短線）
-- 位置在總行程的 10% (throttleIdle = 0.1)
local idle_scale = add_line(
	"idle_scale",
	{ throttle_px - rud_shift * 0.5, throttle_py + throttle_scale_length * throttleIdle },
	rud_shift, -- 線條長度
	nil,
	nil,    -- 水平
	base.name
)

-- 軍用推力位置標記線（水平短線）
-- 位置在總行程的 77.5% (throttleMil = 0.775)
local ab_scale = add_line(
	"ab_scale",
	{ throttle_px - rud_shift * 0.5, throttle_py + throttle_scale_length * throttleMil },
	rud_shift, -- 線條長度
	nil,
	nil,    -- 水平
	base.name
)

-- THROTTLE
-- 油門位置指示器（較粗的水平線，顯示當前油門位置）
-- ⚠️ 使用自定義 controller "throttle" 來控制垂直移動
local throttle_index = add_line(
	"throttle_index",
	{ throttle_px - rud_shift * 1.5 * 0.5, throttle_py }, -- 位置
	rud_shift * 1.5,                                   -- 線條長度（較長）
	line_width * 2,                                    -- 線條寬度（較粗）
	nil,                                               -- 不旋轉
	base.name,                                         -- 父元素
	{ { "throttle", throttle_scale_length } }          -- 油門控制器
)

--[[
================================================================================
    煞車指示器區域
    Wheel Brakes Section
================================================================================
--]]

-- WHEEL BRAKES ---------------------------------------------------------------

-- 煞車指示器的位置（位於方向舵刻度的左右兩側）
local brakes_pos = { size_x, rudder_scale.init_pos[2] }

-- 煞車指示器的大小（高度）
-- 2.0 倍數讓煞車指示器比操縱桿區域更高
local sz_wheel_brake = 2.0 * pitch_stick_movement_negative

--[[
    迴圈創建左右兩個煞車指示器
    i = 1: 左輪煞車
    i = 2: 右輪煞車
--]]
for i = 1, 2 do
	-- 符號陣列：左側為負(-1)，右側為正(1)
	-- 用於控制三角形指向（左輪向左，右輪向右）
	local signum = { -1, 1 }

	--[[
		煞車遮罩（用於實現漸進填充效果）
		Brake Mask for Progressive Fill Effect
	--]]
	local wheel_brake_mask = CreateElement "ceMeshPoly"
	wheel_brake_mask.name = "wheel_brake_mask_" .. tostring(i) -- wheel_brake_mask_1 或 _2
	wheel_brake_mask.primitivetype = "triangles"

	-- 遮罩的四個頂點（梯形）
	-- 根據煞車踏板壓力，遮罩會從底部向上延伸
	wheel_brake_mask.vertices = {
		{ 0, 0 },                                        -- 底部內側
		{ 0, sz_wheel_brake },                           -- 頂部內側
		{ -signum[i] * 0.3 * sz_wheel_brake, sz_wheel_brake }, -- 頂部外側（寬度 30%）
		{ -signum[i] * 0.3 * sz_wheel_brake, 0 }         -- 底部外側
	}

	wheel_brake_mask.indices = { 0, 1, 2, 0, 2, 3 } -- 兩個三角形組成矩形
	wheel_brake_mask.material = "MASK_MATERIAL"  -- 遮罩材質
	wheel_brake_mask.init_pos = { signum[i] * brakes_pos[1], brakes_pos[2] }
	wheel_brake_mask.parent_element = base.name

	-- ⚠️ 煞車數值控制器（自定義 controller）
	-- "brakes_value" - 根據煞車踏板壓力控制遮罩高度
	-- i - 煞車索引（1=左，2=右）
	-- sz_wheel_brake - 最大高度
	wheel_brake_mask.controllers = { { "brakes_value", i, sz_wheel_brake } }

	wheel_brake_mask.h_clip_relation = h_clip_relations.INCREASE_LEVEL
	wheel_brake_mask.isvisible = false -- 遮罩本身不可見

	AddElement(wheel_brake_mask)

	--[[
		煞車填充指示器（顯示煞車壓力的三角形）
		Brake Fill Indicator (Triangle showing brake pressure)
	--]]
	local wheel_brake = CreateElement "ceMeshPoly"
	wheel_brake.name = "wheel_brake_" .. tostring(i)
	wheel_brake.primitivetype = "triangles"

	-- 三角形的三個頂點（從底部向上延伸）
	wheel_brake.vertices = {
		{ 0, 0 },                                       -- 底部
		{ 0, sz_wheel_brake },                          -- 頂部內側
		{ -signum[i] * 0.3 * sz_wheel_brake, sz_wheel_brake } -- 頂部外側
	}

	wheel_brake.indices = { 0, 1, 2 }                   -- 單一三角形
	wheel_brake.material = "ARCADE"                     -- 使用 ARCADE 材質（紅色）
	wheel_brake.init_pos = wheel_brake_mask.init_pos    -- 與遮罩相同位置
	wheel_brake.parent_element = base.name
	wheel_brake.h_clip_relation = h_clip_relations.COMPARE -- 與遮罩配合裁剪
	wheel_brake.level = DEFAULT_LEVEL

	AddElement(wheel_brake)

	--[[
		實際煞車位置指示器（白色橫線）
		Actual Brake Position Indicator (White horizontal line)
		顯示當前煞車踏板的實際位置
	--]]
	local wheel_brake_actual = CreateElement "ceMeshPoly"
	wheel_brake_actual.name = "wheel_brake_actual_" .. tostring(i)

	-- 細橫線的四個頂點（高度為總高度的 10%）
	-- 修改建議: 調整 0.05 值可以改變線條粗細
	wheel_brake_actual.vertices = {
		{ 0, -0.05 * sz_wheel_brake },                          -- 左下
		{ 0, 0.05 * sz_wheel_brake },                           -- 左上
		{ -signum[i] * 0.3 * sz_wheel_brake, 0.05 * sz_wheel_brake }, -- 右上
		{ -signum[i] * 0.3 * sz_wheel_brake, -0.05 * sz_wheel_brake } -- 右下
	}

	wheel_brake_actual.indices = default_box_indices
	wheel_brake_actual.material = "ARCADE_WHITE" -- 白色材質
	wheel_brake_actual.init_pos = { signum[i] * brakes_pos[1], brakes_pos[2] }
	wheel_brake_actual.parent_element = base.name

	-- ⚠️ 實際煞車位置控制器（自定義 controller）
	-- "brakes_value_actual" - 根據實際煞車踏板位置移動白線
	wheel_brake_actual.controllers = { { "brakes_value_actual", i, sz_wheel_brake } }

	AddElement(wheel_brake_actual)
end

--[[
================================================================================
    檔案結束
    End of File

    ⚠️ 重要提醒：
    此檔案使用了多個自定義 controller（基於 F-16 實作），包括：

    1. stick_pitch, stick_roll - 操縱桿位置控制
    2. trimmer_stick_pitch, trimmer_stick_roll - 配平位置控制
    3. rudder - 方向舵踏板位置控制
    4. throttle - 油門位置控制
    5. brakes_value, brakes_value_actual - 輪煞車控制

    這些 controller 需要在其他檔案中定義，通常在 F-16 的
    ControlsIndicator 目錄中會有對應的定義檔案。

    如果指示器無法正常運作，請確認已從 F-16 模組複製了完整的
    ControlsIndicator 目錄及相關定義檔案。
================================================================================
--]]
