local Actuator = {}
Actuator.__index = Actuator

function Actuator:new(model_arg, arg_range, input_arg, input_range)
    local self = setmetatable({}, Actuator)
    self.model_arg = model_arg              -- argument(int)
    self.arg_range = arg_range or { -1, 1 } -- {min, max} 輸出範圍，默認 [-1, 1]
    self.input = input_arg                  -- function
    self.input_range = input_range          -- {min, max} 輸入範圍

    return self
end

function Actuator:update()
    -- 1. 從資料源讀取原始輸入值
    local input_value = self.input()

    -- 2. 將輸入值映射到輸出範圍
    -- 公式: output = (input - input_min) / (input_max - input_min) * (output_max - output_min) + output_min
    local input_min = self.input_range[1]
    local input_max = self.input_range[2]
    local output_min = self.arg_range[1]
    local output_max = self.arg_range[2]

    -- 先標準化到 [0, 1]，再映射到輸出範圍
    local normalized = (input_value - input_min) / (input_max - input_min)
    local output_value = normalized * (output_max - output_min) + output_min

    -- 3. 設置飛機模型的繪圖參數
    set_aircraft_draw_argument_value(self.model_arg, output_value)
end

return Actuator
