local dev = GetSelf()
local sensor_data = get_base_data()

dofile(LockOn_Options.script_path .. "command_defs.lua")

local update_rate = 0.05
make_default_activity(update_rate)

local CMD_MASTER_ARM_ON = device_commands.MasterArmOn
local CMD_MASTER_ARM_OFF = device_commands.MasterArmOff
local CMD_MASTER_ARM_SIM = device_commands.MasterArmSim
local CMD_DOGFIGHT_SWITCH = device_commands.DogfightSwitch
local CMD_TRIGGER_SECOND_STAGE = device_commands.TriggerSecondStage

local MASTER_OFF = 0
local MASTER_SIM = 1
local MASTER_ON = 2

local WEAPON_AAM = 1
local WEAPON_AG = 2
local WEAPON_BOMB = 3
local WEAPON_RKT = 4
local WEAPON_GUN = 5

local master_mode = MASTER_ON
local weapon_class = WEAPON_GUN
local weapon_quantity = 523
local submode_dogfight = 0

local hmcs_ias_kts = get_param_handle("HMCS_IAS_KTS")
local hmcs_alt_ft = get_param_handle("HMCS_ALT_FT")
local hmcs_hdg_deg = get_param_handle("HMCS_HDG_DEG")
local hmcs_hdg_minor_offset = get_param_handle("HMCS_HDG_MINOR_OFFSET")
local hmcs_master_mode = get_param_handle("HMCS_MASTER_MODE")
local hmcs_weapon_class = get_param_handle("HMCS_WEAPON_CLASS")
local hmcs_weapon_qty = get_param_handle("HMCS_WEAPON_QTY")
local hmcs_dogfight_mode = get_param_handle("HMCS_DOGFIGHT_MODE")
local hmcs_gun_firing = get_param_handle("HMCS_GUN_FIRING")

local hmcs_slot_count = 13
local hmcs_slot_spacing = 0.048
local hmcs_gun_rounds_per_second = 100.0
local hmcs_trigger_down = false
local hmcs_gun_qty_sim = weapon_quantity
local hmcs_heading_bias_deg = 0.0
local hmcs_heading_slot_tick = {}
local hmcs_heading_slot_label = {}

for i = 1, hmcs_slot_count do
    hmcs_heading_slot_tick[i] = get_param_handle("HMCS_HDG_SLOT_TICK_" .. tostring(i))
    hmcs_heading_slot_label[i] = get_param_handle("HMCS_HDG_SLOT_LABEL_" .. tostring(i))
end

local function safe_sensor_call(method_name, default_value)
    local method = sensor_data and sensor_data[method_name]
    if type(method) ~= "function" then
        return default_value
    end

    local ok, value = pcall(method, sensor_data)
    if ok and type(value) == "number" then
        return value
    end

    return default_value
end

local function try_sensor_call(method_name)
    local method = sensor_data and sensor_data[method_name]
    if type(method) ~= "function" then
        return false, nil
    end

    local ok, value = pcall(method, sensor_data)
    if ok and type(value) == "number" then
        return true, value
    end

    return false, nil
end

local function normalize_heading_deg(heading_rad)
    local heading_deg = math.deg(heading_rad or 0.0)
    heading_deg = heading_deg % 360.0
    if heading_deg < 0.0 then
        heading_deg = heading_deg + 360.0
    end
    return heading_deg
end

local function safe_heading_deg()
    local has_magnetic_heading, magnetic_heading = try_sensor_call("getMagneticHeading")
    if has_magnetic_heading then
        return normalize_heading_deg(magnetic_heading + math.rad(hmcs_heading_bias_deg))
    end

    local heading = safe_sensor_call("getHeading", 0.0)
    return normalize_heading_deg(-heading + math.rad(hmcs_heading_bias_deg))
end

local function heading_label_code(heading_deg)
    local normalized = heading_deg % 360
    if normalized % 30 ~= 0 then
        return 0
    end
    if normalized == 0 then
        return 1
    end
    if normalized == 30 then
        return 2
    end
    if normalized == 60 then
        return 3
    end
    if normalized == 90 then
        return 4
    end
    if normalized == 120 then
        return 5
    end
    if normalized == 150 then
        return 6
    end
    if normalized == 180 then
        return 7
    end
    if normalized == 210 then
        return 8
    end
    if normalized == 240 then
        return 9
    end
    if normalized == 270 then
        return 10
    end
    if normalized == 300 then
        return 11
    end
    return 12
end

local function heading_tick_code(heading_deg)
    local normalized = heading_deg % 360
    if normalized % 90 == 0 then
        return 3
    end
    if normalized % 30 == 0 then
        return 2
    end
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

local function safe_gun_quantity()
    local candidates = {
        "getGunAmmoCount",
        "getCannonsAmmoCount",
        "getAmmoCount",
    }

    for _, method_name in ipairs(candidates) do
        local value = safe_sensor_call(method_name, -1.0)
        if value >= 0.0 then
            return value
        end
    end

    return -1.0
end

local function push_params()
    local ias_mps = safe_sensor_call("getIndicatedAirSpeed", 0.0)
    local baro_alt_m = safe_sensor_call("getBarometricAltitude", 0.0)
    local heading_deg = safe_heading_deg()
    local gun_quantity_live = safe_gun_quantity()

    local gun_firing = hmcs_gun_firing:get() > 0.5 or hmcs_trigger_down

    if gun_quantity_live >= 0.0 then
        hmcs_gun_qty_sim = gun_quantity_live
    elseif gun_firing and master_mode == MASTER_ON and hmcs_gun_qty_sim > 0.0 then
        hmcs_gun_qty_sim = math.max(0.0, hmcs_gun_qty_sim - hmcs_gun_rounds_per_second * update_rate)
    end

    hmcs_ias_kts:set(math.max(0.0, ias_mps * 1.943844))
    hmcs_alt_ft:set(math.max(0.0, baro_alt_m * 3.28084))
    hmcs_hdg_deg:set(heading_deg)
    hmcs_master_mode:set(master_mode)
    hmcs_weapon_class:set(weapon_class)
    hmcs_weapon_qty:set(hmcs_gun_qty_sim)
    hmcs_dogfight_mode:set(submode_dogfight)
    update_heading_slots(heading_deg)
end

dev:listen_command(CMD_MASTER_ARM_ON)
dev:listen_command(CMD_MASTER_ARM_OFF)
dev:listen_command(CMD_MASTER_ARM_SIM)
dev:listen_command(CMD_DOGFIGHT_SWITCH)
dev:listen_command(CMD_TRIGGER_SECOND_STAGE)

function post_initialize()
    push_params()
end

function SetCommand(command, value)
    if command == CMD_TRIGGER_SECOND_STAGE then
        hmcs_trigger_down = value > 0.5
        push_params()
        return
    end

    if value <= 0.5 then
        return
    end

    if command == CMD_MASTER_ARM_ON then
        master_mode = MASTER_ON
        push_params()
        return
    end

    if command == CMD_MASTER_ARM_OFF then
        master_mode = MASTER_OFF
        push_params()
        return
    end

    if command == CMD_MASTER_ARM_SIM then
        master_mode = MASTER_SIM
        push_params()
        return
    end

    if command == CMD_DOGFIGHT_SWITCH then
        master_mode = MASTER_ON
        submode_dogfight = 1
        push_params()
        return
    end

end

function update()
    push_params()
end