-- 建立外部診斷的檔案與 dcs.log 雙重記錄，並保存每次執行的來源資訊。
local root = lfs.writedir() .. "Scripts/FCK1CAmmoProbe/"
local Core = dofile(root .. "AmmoProbe.lua")
local M = { Core = Core }

function M.logger(channel)
    local directory = lfs.writedir() .. "Logs/FCK1C-ammo/"
    pcall(lfs.mkdir, directory)
    local stamp = os.date("!%Y%m%dT%H%M%SZ")
    local file, file_error = io.open(directory .. channel .. "-" .. stamp .. ".jsonl", "a")
    local logger = Core.logger(channel, function(line)
        pcall(Core.log_line, line)
        if file then
            local ok, error_text = file:write(line .. "\n")
            if ok then
                file:flush()
            else
                pcall(Core.log_line, "file write failed: " .. tostring(error_text))
                pcall(file.close, file)
                file = nil
            end
        end
    end)
    logger.record("probe.start", { version = "ammo-probe-r1", file_open = file ~= nil, file_error = file_error }, Core.time(), true)
    logger.close = function()
        if file then
            pcall(file.close, file)
            file = nil
        end
    end
    return logger
end

-- 取得目前玩家機型與識別，讓紀錄只在 F-CK-1C 座艙使用期間取樣。
function M.identity()
    local export = Core.member(_G, "Export")
    local owner = type(Core.member(_G, "LoGetSelfData")) == "function" and _G or export
    local result = Core.call(owner, "LoGetSelfData", "dot")
    local value = result.status == "returned" and result.value or nil
    if type(value) ~= "table" or value.Name ~= "F-CK-1C" then return nil, result end
    local id = Core.call(owner, "LoGetPlayerPlaneId", "dot")
    local pilot = Core.call(owner, "LoGetPilotName", "dot")
    return { aircraft = value.Name, object_id = id.value, player_name = pilot.value, self_data = value }, result
end

return M
