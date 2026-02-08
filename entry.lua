local self_ID = "F-CK-1C_Mod"

declare_plugin(self_ID,
    {
        -- =================基本模組信息=================
        image         = "F-CK-1C.png",                 -- 模組圖標文件: 顯示在DCS模組管理器中的圖標 [檔案名稱]
        installed     = true,                          -- 安裝狀態標記: true=已安裝可用, false=占位符或廣告 [布林值]
        dirName       = current_mod_path,              -- 模組目錄路徑: DCS自動提供的當前模組根目錄路徑 [字串路徑]
        displayName   = _("F-CK-1C Module"),           -- 顯示名稱: 用戶界面中顯示的模組名稱 [本地化字串]
        developerName = _("F-CK-1C Development Team"), -- 開發者名稱: 模組開發團隊或個人名稱 [本地化字串]

        -- =================系統識別信息=================
        fileMenuName  = _("F-CK-1C"),                                                                                               -- 檔案選單名稱: 在DCS檔案選單中顯示的名稱 [本地化字串]
        update_id     = "F-CK-1C_Mod",                                                                                              -- 更新識別符: 用於自動更新檢查的唯一ID [字串]
        version       = "v0.1.1",                                                                                                   -- 模組版本號: 遵循語義化版本控制 [版本字串]
        state         = "installed",                                                                                                -- 模組狀態: "installed"=已安裝, "beta"=測試版 [狀態字串]
        info          = _(
            "F-CK-1C multirole fighter module. Contains aircraft configuration, liveries and mission samples for testing and AI use."), -- 模組描述: 詳細說明模組功能和特色 [本地化字串]

        -- =================視覺外觀資源=================
        Skins         = {
            {
                name = _("F-CK-1C Skins"), -- 外觀包名稱: 顯示在外觀選擇界面的名稱 [本地化字串]
                dir  = "Theme"             -- 外觀資源目錄: 存放外觀相關檔案的資料夾 [相對路徑]
            },
        },

        -- =================任務包資源=================
        Missions      = {
            {
                name  = _("F-CK-1C Training"), -- 任務包名稱: 顯示在任務選擇界面的名稱 [本地化字串]
                dir   = "Missions",            -- 任務檔案目錄: 存放.miz任務檔案的資料夾 [相對路徑]
                CLSID = "{F-CK-1C missions}",  -- 任務類別識別符: 用於任務分類的唯一ID [CLSID字串]
            },
        },

        -- =================飛行記錄配置=================
        LogBook       = {
            {
                name = _("F-CK-1C Operations"), -- 記錄簿分類名稱: 在飛行記錄中的分類標籤 [本地化字串]
                type = "F-CK-1C_Mod",           -- 記錄類型: 用於統計和篩選的類型標識 [類型字串]
            },
        },

        -- =================選項配置界面=================
        Options       = {
            {
                name   = _("F-CK-1C Settings"), -- 設定選單名稱: 顯示在選項界面的名稱 [本地化字串]
                nameId = "F-CK-1C",             -- 設定識別符: 內部用於識別設定組的ID [ID字串]
                dir    = "Options",             -- 設定檔案目錄: 存放設定界面檔案的資料夾 [相對路徑]
                CLSID  = "{F-CK-1C options}"    -- 設定類別識別符: 用於設定分類的唯一ID [CLSID字串]
            },
        },

        -- =================輸入設備配置=================
        InputProfiles = {
            ["F-CK-1C"] = current_mod_path .. '/Input/F-CK-1C/', -- 輸入配置檔路徑: 鍵盤/搖桿配置檔案的完整路徑 [絕對路徑]
        },
    })

mount_vfs_model_path(current_mod_path .. "/Shapes")
mount_vfs_model_path(current_mod_path .. "/Cockpit/Shapes")
mount_vfs_liveries_path(current_mod_path .. "/Liveries")
mount_vfs_liveries_path(current_mod_path .. "/Cockpit/Liveries")
mount_vfs_texture_path(current_mod_path .. "/Textures")
mount_vfs_texture_path(current_mod_path .. "/Textures/F-CK-1C")
-- mount_vfs_texture_path(current_mod_path .. "/Textures/F16C_bl50")
-- mount_vfs_texture_path(current_mod_path .. "/Textures/F16C_bl50_HAF")
-- mount_vfs_texture_path(current_mod_path .. "/Textures/F16C_bl50_IAF")
-- mount_vfs_texture_path(current_mod_path .. "/Textures/F16C_Pilot")
-- ---------------------------------------------------------
dofile(current_mod_path .. '/F-CK-1C.lua')

make_flyable('F-CK-1C', current_mod_path .. "/Cockpit/Scripts/", --[[{self_ID, 'Su-25T'}--]] nil,
    current_mod_path .. '/comm.lua')
-- ---------------------------------------------------------
dofile(current_mod_path .. "/Views.lua")
make_view_settings('F-CK-1C', ViewSettings, SnapViews)

-- ---------------------------------------------------------
plugin_done()
