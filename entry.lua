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
local FCK1C_BUILD_VERSION = "v0.1.2-dev"
local FCK1C_BUILD_DATE = "2026-03-06"
local FCK1C_CHANGELOG = {
    "Added EFM mode switch: baseline / efm_min / efm_full",
    "Integrated EFM DLL path and FM config handoff",
    "Added local design/progress report with version iteration section"
}
local FCK1C_VERSION_HISTORY = {
    { version = "v0.1.0", note = "Initial module load and base structure" },
    { version = "v0.1.1", note = "EFM integration and diagnostic EFM mode switch" },
    { version = "v0.1.2-dev", note = "Version metadata and iteration tracking" }
}

declare_plugin(self_ID,
    {
        -- =================?蝞賃秧????賤蹓?================
        image         = "F-CK-1C.png",                -- ????謘???刻麾: ?輯???瘙S????????嗉??????[?澗?????]
        installed     = true,                         -- ?堆???????? true=??????? false=?蹎??????? [????憟?
        dirName       = current_mod_path,             -- ????獢???: DCS????????????荔?赯?獢??? [?殉??⊿???]
        displayName   = _("F-CK-1C Module"),          -- ?輯?????: ??踝???橫???嚗蝞??????? [??秘?謘??楞
        developerName = _("F-CK-1C Development Team"), -- ??赤????? ?????赤?謢??謘撢????? [??秘?謘??楞

        -- =================??蝯?謢塚??賤蹓?================
        fileMenuName  = _("F-CK-1C"),                                                                                                                -- ?澗???閰制????: ?瘙S?澗???閰制???嚗蝞???? [??秘?謘??楞
        update_id     = "F-CK-1C_Mod",                                                                                                               -- ?皝??朱??? ??踐?????皝??潘撓貔????ID [?殉??（
        version       = FCK1C_BUILD_VERSION,                                                                                                                    -- ?????秧?? ????止策??謘??蟡???[??秧?殉??（
        state         = "installed",                                                                                                                 -- ??????? "installed"=???? "beta"=??撗??[??????楞
        info          = _(
        "F-CK-1C multirole fighter module. Contains aircraft configuration, liveries and mission samples for testing and AI use."),                  -- ?????渲: ?啣??阡??????賹???潘??[??秘?謘??楞
        
        binaries = { 'BasicEFM_template.dll', },

        -- =================?秋▼甇餅謘???=================
        Skins         = {
            {
                name = _("F-CK-1C Skins"), -- ?叟▼?????? ?輯??????威?鞊???橫??????[??秘?謘??楞
                dir  = "Theme"             -- ?叟▼????獢?: ?殉朵?謘??鞈??澗??????謕? [?閰???]
            },
        },

        -- =================????????================
        Missions      = {
            {
                name  = _("F-CK-1C Training"), -- ???????? ?輯????園??謕?????嚗???? [??秘?謘??楞
                dir   = "Missions",           -- ????澗???獢?: ?殉朵??miz????澗??????謕? [?閰???]
                CLSID = "{F-CK-1C missions}", -- ????遴竣??謢塚??? ??踐???????????ID [CLSID?殉??（
            },
        },

        -- =================????殉死???=================
        LogBook       = {
            {
                name = _("F-CK-1C Operations"), -- ?殉死??芾號??遴竣??? ????蛛????????遴等???[??秘?謘??楞
                type = "F-CK-1C_Mod",           -- ?殉死??遴竣?: ??踐??舀０???潛??鞎??遴竣???? [?遴竣??殉??（
            },
        },

        -- =================?鞈?????橫?=================
        Options       = {
            {
                name   = _("F-CK-1C Settings"), -- ?桀???閰制????: ?輯????賃?????嚗???? [??秘?謘??楞
                nameId = "F-CK-1C",           -- ?桀???朱??? ????踐??朱????荔??ID [ID?殉??（
                dir    = "Options",           -- ?桀???澗???獢?: ?殉朵?????橫??澗??????謕? [?閰???]
                CLSID  = "{F-CK-1C options}"  -- ?桀???遴竣??謢塚??? ??踐??桀?????????ID [CLSID?殉??（
            },
        },

        -- =================?岳?舫????=================
        InputProfiles = {
            ["F-CK-1C"] = current_mod_path .. '/Input/F-CK-1C/', -- ?岳????澗?璆? ???/?謘賤???澗??????皜脫???[?荒????]
        },
    })


-- log.info("FCK1C: entry.lua loaded, current_mod_path=" .. tostring(current_mod_path))

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
    dofile(current_mod_path .. "/Views.lua")
    make_view_settings("F-CK-1C", ViewSettings, SnapViews)
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
