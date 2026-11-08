-- 記錄原生觀測的欄位、型別與回傳值，內容變化時寫入 dcs.log。
local M = {}
local MAX_DEPTH = 4
local MAX_ENTRIES = 32
local MAX_STRING = 512

local function describe(value, depth)
    local kind = type(value)
    if kind == "string" then
        local text = string.format("%q", value:sub(1, MAX_STRING)):gsub("\n", "\\n"):gsub("\r", "\\r")
        return #value > MAX_STRING and text .. "<bytes=" .. #value .. ">" or text
    end
    if kind ~= "table" then return kind .. ":" .. tostring(value) end
    if depth >= MAX_DEPTH then return "<table-depth-limit>" end
    local keys = {}
    for key in pairs(value) do
        if type(key) == "string" or type(key) == "number" then keys[#keys + 1] = key end
    end
    table.sort(keys, function(a, b)
        return type(a) == type(b) and a < b or type(a) < type(b)
    end)
    local parts = {}
    for i = 1, math.min(#keys, MAX_ENTRIES) do
        local key = keys[i]
        parts[#parts + 1] = tostring(key) .. "=" .. describe(value[key], depth + 1)
    end
    if #keys > MAX_ENTRIES then parts[#parts + 1] = "<entries=" .. #keys .. ">" end
    return "{" .. table.concat(parts, ",") .. "}"
end

function M.new(scope)
    local previous = {}
    return function(key, value, context)
        if not log or type(log.info) ~= "function" then return end
        -- 診斷以受保護呼叫執行，記錄成功後保存最近內容。
        pcall(function()
            local text = describe(value, 0)
            if previous[key] == text then return end
            log.info("FCK1C " .. scope .. " " .. key .. (context and " " .. context or "") .. " " .. text)
            previous[key] = text
        end)
    end
end

return M
