-- 在 Export 與座艙之間編碼原生彈量，並共用接收時效。
local M = { maximum_count = 9999, timeout_s = 0.5 }
local scale = 10000

function M.encode(count)
    if type(count) ~= "number" or count ~= math.floor(count) or count < 0 or count > M.maximum_count then return -1 end
    return count / scale
end

function M.decode(value)
    if type(value) ~= "number" or value ~= value or value < 0 or value >= 1 then return nil end
    local count = math.floor(value * scale + 0.5)
    if count > M.maximum_count then return nil end
    return count
end

return M
