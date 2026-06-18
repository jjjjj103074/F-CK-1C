local self_ID = "F-CK-1C_Mod"
local logger = log
local function log_info(msg)
    if logger and logger.info then
        logger.info(msg)
    end
end
local function log_error(msg)
    if logger and logger.error then
        logger.error(msg)
    end
end

-- F-CK-1C module version metadata
local FCK1C_BUILD_VERSION = "v0.1.3-april-fools"
local FCK1C_BUILD_DATE = "2026-04-01"
local FCK1C_CHANGELOG = {
    "April Fools build: experimental ground-contact tuning",
    "Added EFM mode switch: baseline / efm_min / efm_full",
    "Integrated EFM DLL path and FM config handoff",
    "Added local design/progress report with version iteration section"
}
local FCK1C_VERSION_HISTORY = {
    { version = "v0.1.0", note = "Initial module load and base structure" },
    { version = "v0.1.1", note = "EFM integration and diagnostic EFM mode switch" },
    { version = "v0.1.2-dev", note = "Version metadata and iteration tracking" },
    { version = "v0.1.3-april-fools", note = "April Fools build with experimental ground-contact tuning" }
}

declare_plugin(self_ID,
    {
        -- =================基本模組信息=================
        image         = "F-CK-1C.png",                 -- 模組圖標文件: 顯示在DCS模組管理器中的圖標 [檔案名稱]
        installed     = true,                          -- 安裝狀態標記: true=已安裝可用, false=占位符或廣告 [布林值]
        dirName       = current_mod_path,              -- 模組目錄路徑: DCS自動提供的當前模組根目錄路徑 [字串路徑]
        displayName   = _("F-CK-1C Module"),           -- 顯示名稱: 用戶界面中顯示的模組名稱 [本地化字串]
        developerName = _("F-CK-1C Development Team"), -- 開發者名稱: 模組開發團隊或個人名稱 [本地化字串]

        -- ================= 更新與描述資訊 =================
        fileMenuName  = _("F-CK-1C"),                                                                                                   -- DCS 選單中的名稱
        update_id     = "F-CK-1C_Mod",                                                                                                  -- 更新系統使用的模組 ID
        version       = FCK1C_BUILD_VERSION,                                                                                            -- 模組版本號
        state         = "installed",                                                                                                    -- 模組狀態: installed / beta
        info          = _(
            "F-CK-1C multirole fighter module. Contains aircraft configuration, liveries and mission samples for testing and AI use."), -- 模組說明文字

        binaries      = { 'BasicEFM_template.dll', },

        -- ================= 外觀塗裝 =================
        Skins         = {
            {
                name = _("F-CK-1C Skins"), -- 塗裝分類名稱
                dir  = "Liveries"          -- 塗裝資料夾
            },
        },

        -- ================= 任務內容 =================
        Missions      = {
            {
                name  = _("F-CK-1C Training"), -- 任務包名稱: 顯示在任務選擇界面的名稱 [本地化字串]
                dir   = "Missions",            -- 任務檔案目錄: 存放.miz任務檔案的資料夾 [相對路徑]
                CLSID = "{F-CK-1C missions}",  -- 任務類別識別符: 用於任務分類的唯一ID [CLSID字串]
            },
        },

        -- ================= Logbook 類別 =================
        LogBook       = {
            {
                name = _("F-CK-1C Operations"), -- 飛行紀錄分類名稱
                type = "F-CK-1C_Mod",           -- 飛行紀錄類型 ID
            },
        },

        -- ================= 模組設定頁 =================
        Options       = {
            {
                name   = _("F-CK-1C Settings"), -- 設定選單名稱: 顯示在選項界面的名稱 [本地化字串]
                nameId = "F-CK-1C",             -- 設定識別符: 內部用於識別設定組的ID [ID字串]
                dir    = "Options",             -- 設定檔案目錄: 存放設定界面檔案的資料夾 [相對路徑]
                CLSID  = "{F-CK-1C options}"    -- 設定類別識別符: 用於設定分類的唯一ID [CLSID字串]
            },
        },

        -- ================= 輸入設定檔 =================
        InputProfiles = {
            ["F-CK-1C"] = current_mod_path .. '/Input/F-CK-1C/', -- 搖桿、鍵盤等輸入設定檔路徑
        },
    })


-- log.info("FCK1C: entry.lua loaded, current_mod_path=" .. tostring(current_mod_path))

mount_vfs_model_path(current_mod_path .. "/Shapes")
mount_vfs_model_path(current_mod_path .. "/Cockpit/Shapes")
mount_vfs_liveries_path(current_mod_path .. "/Liveries")
mount_vfs_liveries_path(current_mod_path .. "/Cockpit/Liveries")
if mount_vfs_sound_path ~= nil then
    mount_vfs_sound_path(current_mod_path .. "/Sounds")
end
mount_vfs_texture_path(current_mod_path .. "/Textures")
mount_vfs_texture_path(current_mod_path .. "/Textures/F-CK-1C.zip")
-- mount_vfs_texture_path(current_mod_path .. "/Textures/F16C_bl50")
-- mount_vfs_texture_path(current_mod_path .. "/Textures/F16C_bl50_HAF")
-- mount_vfs_texture_path(current_mod_path .. "/Textures/F16C_bl50_IAF")
-- mount_vfs_texture_path(current_mod_path .. "/Textures/F16C_Pilot")
-- ---------------------------------------------------------
dofile(current_mod_path .. '/F-CK-1C.lua')

-- Diagnostic switch:
-- "baseline" = original non-EFM path (known good for collision)
-- "efm_min"  = EFM without FM/config.lua
-- "efm_full" = EFM with FM/config.lua
local EFM_MODE = "efm_full"
log_info("FCK1C: entry.lua loaded, version=" .. tostring(FCK1C_BUILD_VERSION) .. ", date=" .. tostring(FCK1C_BUILD_DATE) .. ", EFM_MODE=" .. tostring(EFM_MODE))

local cfg_path = current_mod_path .. "/FM/config.lua"
if EFM_MODE == "efm_min" or EFM_MODE == "efm_full" then
    log_info("FCK1C: EFM path active")
    if EFM_MODE == "efm_full" then
        dofile(cfg_path)
    end
    FM = FM or {}
    FM[1] = self_ID
    FM[2] = "BasicEFM_template.dll"
    if EFM_MODE == "efm_full" then
        FM.config_path = cfg_path
    else
        FM.config_path = nil
    end
    FM.old = nil
    make_flyable(
        "F-CK-1C",
        current_mod_path .. "/Cockpit/Scripts/",
        FM,
        current_mod_path .. "/comm.lua"
    )
else
    make_flyable(
        "F-CK-1C",
        current_mod_path .. "/Cockpit/Scripts/",
        nil,
        current_mod_path .. "/comm.lua"
    )
    local view_loaded = false
    local view_candidates = {
        current_mod_path .. "/Views.lua",
        current_mod_path .. "/baseline_original/F-CK-1C/Views.lua"
    }
    for _, view_path in ipairs(view_candidates) do
        local ok, err = pcall(dofile, view_path)
        if ok then
            view_loaded = true
            log_info("FCK1C: loaded view config: " .. tostring(view_path))
            break
        else
            log_info("FCK1C: view config not loaded from " .. tostring(view_path) .. " (" .. tostring(err) .. ")")
        end
    end

    if view_loaded and ViewSettings and SnapViews then
        make_view_settings("F-CK-1C", ViewSettings, SnapViews)
    else
        log_error("FCK1C: view settings missing, skip make_view_settings in baseline mode")
    end
end

plugin_done()

if EFM_MODE == "efm_full" then
    if io and io.open then
        local f = io.open(cfg_path, "r")
        if f then
            f:close()
            log_info("FCK1C: config.lua found: " .. cfg_path)
        else
            log_error("FCK1C: config.lua NOT FOUND at: " .. cfg_path)
        end
    else
        log_info("FCK1C: io library unavailable in sandbox; skip config.lua file check")
    end
end
