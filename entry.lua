local self_ID = "F-CK-1C_Mod"
local logger = log
local function log_info(msg)
    if logger and logger.info then logger.info(msg) end
end
local function log_error(msg)
    if logger and logger.error then logger.error(msg) end
end

-- Module version metadata.
local FCK1C_BUILD_VERSION = "v0.1.3-april-fools"
local FCK1C_BUILD_DATE = "2026-04-01"
local FCK1C_CHANGELOG = {
    "April Fools build: experimental ground-contact tuning",
    "Added EFM mode switch: baseline / efm_min / efm_full",
    "Integrated EFM DLL path and FM config handoff",
    "Added local design/progress report with version iteration section",
}
local FCK1C_VERSION_HISTORY = {
    { version = "v0.1.0", note = "Initial module load and base structure" },
    { version = "v0.1.1", note = "EFM integration and diagnostic EFM mode switch" },
    { version = "v0.1.2-dev", note = "Version metadata and iteration tracking" },
    { version = "v0.1.3-april-fools", note = "April Fools build with experimental ground-contact tuning" },
}

declare_plugin(self_ID, {
    -- Module identity.
    image = "F-CK-1C.png", -- DCS module manager icon.
    installed = true,
    dirName = current_mod_path,
    displayName = _("F-CK-1C Module"),
    developerName = _("F-CK-1C Development Team"),

    -- Version and menu metadata.
    fileMenuName = _("F-CK-1C"),
    update_id = "F-CK-1C_Mod",
    version = FCK1C_BUILD_VERSION,
    state = "installed",
    info = _("F-CK-1C multirole fighter module. Contains aircraft configuration, liveries and mission samples for testing and AI use."),

    binaries = { "F-CK-1C_EFM.dll" },

    -- External liveries.
    Skins = {
        {
            name = _("F-CK-1C Skins"),
            dir = "Liveries",
        },
    },

    -- Mission package.
    Missions = {
        {
            name = _("F-CK-1C Training"),
            dir = "Missions",
            CLSID = "{F-CK-1C missions}",
        },
    },

    -- Logbook category.
    LogBook = {
        {
            name = _("F-CK-1C Operations"),
            type = "F-CK-1C_Mod",
        },
    },

    -- Options page.
    Options = {
        {
            name = _("F-CK-1C Settings"),
            nameId = "F-CK-1C",
            dir = "Options",
            CLSID = "{F-CK-1C options}",
        },
    },

    -- Input profile root.
    InputProfiles = {
        ["F-CK-1C"] = current_mod_path .. "/Input/F-CK-1C/",
    },
})

mount_vfs_model_path(current_mod_path .. "/Shapes")
mount_vfs_model_path(current_mod_path .. "/Cockpit/Shapes")
mount_vfs_liveries_path(current_mod_path .. "/Liveries")
mount_vfs_liveries_path(current_mod_path .. "/Cockpit/Liveries")
if mount_vfs_sound_path ~= nil then mount_vfs_sound_path(current_mod_path .. "/Sounds") end
mount_vfs_texture_path(current_mod_path .. "/Textures")
mount_vfs_texture_path(current_mod_path .. "/Textures/F-CK-1C.zip")
dofile(current_mod_path .. "/F-CK-1C.lua")

-- EFM mode selector:
-- baseline: non-EFM fallback path.
-- efm_min : EFM without FM/config.lua.
-- efm_full: EFM with FM/config.lua.
local EFM_MODE = "efm_full"
log_info("FCK1C: entry.lua loaded, version=" .. tostring(FCK1C_BUILD_VERSION) .. ", date=" .. tostring(FCK1C_BUILD_DATE) .. ", EFM_MODE=" .. tostring(EFM_MODE))

local cfg_path = current_mod_path .. "/FM/config.lua"
if EFM_MODE == "efm_min" or EFM_MODE == "efm_full" then
    log_info("FCK1C: EFM path active")
    if EFM_MODE == "efm_full" then dofile(cfg_path) end
    FM = FM or {}
    FM[1] = self_ID
    FM[2] = "F-CK-1C_EFM.dll"
    if EFM_MODE == "efm_full" then
        FM.config_path = cfg_path
    else
        FM.config_path = nil
    end
    FM.old = nil
    make_flyable("F-CK-1C", current_mod_path .. "/Cockpit/Scripts/", FM, current_mod_path .. "/comm.lua")
else
    make_flyable("F-CK-1C", current_mod_path .. "/Cockpit/Scripts/", nil, current_mod_path .. "/comm.lua")
    local view_loaded = false
    local view_candidates = {
        current_mod_path .. "/Views.lua",
        current_mod_path .. "/baseline_original/F-CK-1C/Views.lua",
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
