-- 在既有武器裝置內取樣實際反制存量，供測試面板及紀錄器讀取。
local M = {}
function M.new(device)
    local available = get_param_handle("FCK1C_TEST_CM_OBSERVATION_AVAILABLE")
    local flare = get_param_handle("FCK1C_TEST_CM_FLARE_COUNT")
    local chaff = get_param_handle("FCK1C_TEST_CM_CHAFF_COUNT")
    local function count(method)
        local ok, value = pcall(function() return device[method](device) end)
        if ok and type(value) == "number" and value >= 0 and value <= 9999 and value == math.floor(value) then
            return value
        end
    end
    return { update = function()
        local f, c = count("get_flare_count"), count("get_chaff_count")
        flare:set(f or -1)
        chaff:set(c or -1)
        available:set((f and 1 or 0) + (c and 2 or 0))
    end }
end
return M
