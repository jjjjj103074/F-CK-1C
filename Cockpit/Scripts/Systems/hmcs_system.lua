-- DCS observations and HMCS presentation. MC owns mission state, stores and weapon authorization.
local sensor_data = get_base_data()

dofile(LockOn_Options.script_path .. "generated/CockpitParams.g.lua")

-- Project-defined 64 Hz: no confirmed F-CK-1C heading-reference sampling
-- rate is public. This keeps the DCS-only magnetic observation aligned with
-- the current FCC reference rate until an aircraft source is available.
local HEADING_OBSERVATION_RATE_HZ = 64
local update_rate = 1.0 / HEADING_OBSERVATION_RATE_HZ
make_default_activity(update_rate)

local hmcs_ias_kts = get_param_handle("HMCS_IAS_KTS")
local hmcs_alt_ft = get_param_handle("HMCS_ALT_FT")
local pressure_altitude_available = get_param_handle(cockpit_params.PressureAltitudeAvailable)
local pressure_altitude_m = get_param_handle(cockpit_params.PressureAltitudeM)
local hmcs_hdg_deg = get_param_handle("HMCS_HDG_DEG")
local magnetic_heading_available = get_param_handle(cockpit_params.MagneticHeadingAvailable)
local magnetic_heading_rad = get_param_handle(cockpit_params.MagneticHeadingRad)
local hmcs_hdg_minor_offset = get_param_handle("HMCS_HDG_MINOR_OFFSET")
local hmcs_enabled = get_param_handle("HMCS_ENABLED")
local hmcs_display_mode = get_param_handle("HMCS_DISPLAY_MODE")

local hmcs_weapon_class = get_param_handle(cockpit_params.HmcsWeaponClass)
local hmcs_weapon_quantity = get_param_handle(cockpit_params.HmcsWeaponQuantity)
local mc_generation = get_param_handle(cockpit_params.McGeneration)
local mc_available = get_param_handle(cockpit_params.McAvailable)
local mc_fire_control_mode = get_param_handle(cockpit_params.HmcsFireControlMode)
local mc_uncage = get_param_handle(cockpit_params.Aim9UncageHeld)
local mc_contact = get_param_handle(cockpit_params.Aim9SeekerContact)
local mc_locked = get_param_handle(cockpit_params.Aim9SeekerLock)
local mc_readiness = get_param_handle(cockpit_params.Aim9MissileStatus)
local mc_gun_firing = get_param_handle(cockpit_params.HmcsGunFiring)
local mc_gun_count = get_param_handle(cockpit_params.McStoresGunCount)
local mc_missile_count = get_param_handle(cockpit_params.Aim9MissileCount)
local mc_trigger_held = get_param_handle(cockpit_params.McTriggerSecondStageHeld)
local mc_master_arm_press = get_param_handle(cockpit_params.McMasterArmPressSequence)
local mc_navigation_press = get_param_handle(cockpit_params.McNavigationPressSequence)
local mc_dogfight_press = get_param_handle(cockpit_params.McDogfightPressSequence)
local mc_missile_override_press = get_param_handle(cockpit_params.McMissileOverridePressSequence)
local mc_missile_uncage_press = get_param_handle(cockpit_params.McMissileUncagePressSequence)
local mc_weapon_release_press = get_param_handle(cockpit_params.McWeaponReleasePressSequence)

local WEAPON_CLASS_AAM = 1
local WEAPON_CLASS_GUN = 5

-- Read one committed MC snapshot. Only presentation choices are made here;
-- raw input observations never grant firing permission or change selected stations.
local function update_weapon_presentation()
    local before = mc_generation:get()
    if before < 2 or before > 9007199254740991 or before ~= math.floor(before)
        or before % 2 ~= 0 or mc_available:get() < 0.5 then return end

    local mode = mc_fire_control_mode:get()
    local uncage = mc_uncage:get() >= 0.5
    local contact = mc_contact:get() >= 0.5
    local locked = mc_locked:get() >= 0.5
    local readiness = mc_readiness:get()
    local gun_firing = mc_gun_firing:get() >= 0.5
    local gun_count = mc_gun_count:get()
    local missile_count = mc_missile_count:get()
    local trigger_held = mc_trigger_held:get() >= 0.5
    local gun_preference_sequence = math.max(mc_master_arm_press:get(), mc_navigation_press:get())
    local missile_preference_sequence = math.max(
        mc_dogfight_press:get(), mc_missile_override_press:get(),
        mc_missile_uncage_press:get(), mc_weapon_release_press:get()
    )
    if mc_generation:get() ~= before then return end

    -- Latest press sequences preserve the old sticky preference even if a pulse
    -- began and ended between HMCS updates, or its weapon operation was rejected.
    local preferred_class = missile_preference_sequence > gun_preference_sequence
        and WEAPON_CLASS_AAM or WEAPON_CLASS_GUN
    local weapon_class
    if mode ~= 0 or uncage or contact or locked or readiness ~= 0 then
        weapon_class = WEAPON_CLASS_AAM
    elseif gun_firing or trigger_held or preferred_class == WEAPON_CLASS_GUN then
        weapon_class = WEAPON_CLASS_GUN
    else
        weapon_class = preferred_class
    end
    hmcs_weapon_class:set(weapon_class)
    hmcs_weapon_quantity:set(weapon_class == WEAPON_CLASS_GUN and gun_count or missile_count)
end


local hmcs_slot_count = 13
local hmcs_slot_spacing = 0.048
local hmcs_heading_bias_deg = 0.0
local hmcs_heading_slot_tick = {}
local hmcs_heading_slot_label = {}
local HMCS_ARG_DEVICE = 509
local HMCS_ARG_DISPLAY_MODE = 510

for i = 1, hmcs_slot_count do
    hmcs_heading_slot_tick[i] = get_param_handle("HMCS_HDG_SLOT_TICK_" .. tostring(i))
    hmcs_heading_slot_label[i] = get_param_handle("HMCS_HDG_SLOT_LABEL_" .. tostring(i))
end

local function safe_sensor_call(method_name, default_value)
    local method = sensor_data and sensor_data[method_name]
    if type(method) ~= "function" then return default_value end

    local ok, value = pcall(method, sensor_data)
    if ok and type(value) == "number" then return value end

    return default_value
end

local function try_global_number_call(function_name, ...)
    local fn = _G[function_name]
    if type(fn) ~= "function" then return false, nil end

    local ok, value = pcall(fn, ...)
    if ok and type(value) == "number" then return true, value end

    return false, nil
end

local function read_draw_argument(argument_number, default_value)
    local ok, value = try_global_number_call("get_aircraft_draw_argument_value", argument_number)
    if ok then return value end
    return default_value
end

local function update_display_mode_params()
    local helmet_arg = read_draw_argument(HMCS_ARG_DEVICE, 0.0)
    local display_mode_arg = read_draw_argument(HMCS_ARG_DISPLAY_MODE, 0.0)
    local helmet_installed = math.abs(helmet_arg - 0.0) < 0.25
    local display_mode_value = 0

    if display_mode_arg >= 0.5 then display_mode_value = 1 end

    hmcs_enabled:set(helmet_installed and 1 or 0)
    hmcs_display_mode:set(display_mode_value)
end

local function try_sensor_call(method_name)
    local method = sensor_data and sensor_data[method_name]
    if type(method) ~= "function" then return false, nil end

    local ok, value = pcall(method, sensor_data)
    if ok and type(value) == "number" then return true, value end

    return false, nil
end

local function normalize_heading_rad(heading_rad)
    local normalized = heading_rad % (2.0 * math.pi)
    if normalized < 0.0 then normalized = normalized + 2.0 * math.pi end
    return normalized
end

local function read_magnetic_heading()
    local available, heading_rad = try_sensor_call("getMagneticHeading")
    magnetic_heading_available:set(available and 1 or 0)
    if not available then return nil end

    local normalized = normalize_heading_rad(heading_rad + math.rad(hmcs_heading_bias_deg))
    magnetic_heading_rad:set(normalized)
    return normalized
end

local function read_pressure_altitude()
    local available, altitude_m = try_sensor_call("getBarometricAltitude")
    pressure_altitude_available:set(available and 1 or 0)
    if not available then return nil end

    pressure_altitude_m:set(altitude_m)
    return altitude_m
end

local function heading_label_code(heading_deg)
    local normalized = heading_deg % 360
    if normalized % 30 ~= 0 then return 0 end
    if normalized == 0 then return 1 end
    if normalized == 30 then return 2 end
    if normalized == 60 then return 3 end
    if normalized == 90 then return 4 end
    if normalized == 120 then return 5 end
    if normalized == 150 then return 6 end
    if normalized == 180 then return 7 end
    if normalized == 210 then return 8 end
    if normalized == 240 then return 9 end
    if normalized == 270 then return 10 end
    if normalized == 300 then return 11 end
    return 12
end

local function heading_tick_code(heading_deg)
    local normalized = heading_deg % 360
    if normalized % 90 == 0 then return 3 end
    if normalized % 30 == 0 then return 2 end
    return 1
end

local function update_heading_slots(current_heading_deg)
    local base_heading = math.floor(current_heading_deg / 15.0) * 15.0
    local minor_deg = current_heading_deg - base_heading

    hmcs_hdg_minor_offset:set(-(minor_deg / 15.0) * hmcs_slot_spacing)

    local center_index = math.floor(hmcs_slot_count / 2)
    for i = 1, hmcs_slot_count do
        local offset_index = i - center_index - 1
        local slot_heading = (base_heading + offset_index * 15.0) % 360.0
        hmcs_heading_slot_tick[i]:set(heading_tick_code(slot_heading))
        hmcs_heading_slot_label[i]:set(heading_label_code(slot_heading))
    end
end

local function push_params()
    local ias_mps = safe_sensor_call("getIndicatedAirSpeed", 0.0)
    local baro_alt_m = read_pressure_altitude()
    local heading_rad = read_magnetic_heading()

    update_display_mode_params()
    update_weapon_presentation()
    hmcs_ias_kts:set(math.max(0.0, ias_mps * 1.943844))
    if baro_alt_m ~= nil then
        hmcs_alt_ft:set(math.max(0.0, baro_alt_m * 3.28084))
    end
    if heading_rad ~= nil then hmcs_hdg_deg:set(math.deg(heading_rad)) end
    if heading_rad ~= nil then update_heading_slots(math.deg(heading_rad)) end
end

function post_initialize()
    push_params()
end

function update()
    push_params()
end
