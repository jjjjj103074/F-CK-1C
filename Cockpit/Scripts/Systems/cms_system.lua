local dev = GetSelf()

local update_rate = 0.02
make_default_activity(update_rate)

dofile(LockOn_Options.script_path .. "command_defs.lua")

local CMD_TRIGGER_FIRST_STAGE = device_commands.TriggerFirstStage
local CMD_CMS_FORWARD = device_commands.CMSForward
local CMD_CMS_AFT = device_commands.CMSAft
local CMD_CMS_LEFT = device_commands.CMSLeft
local CMD_CMS_RIGHT = device_commands.CMSRight
local CMD_CMS_PRESS = device_commands.CMSPress
local CMD_TRIGGER_SECOND_STAGE = device_commands.TriggerSecondStage
local CMD_MASTER_ARM_ON = device_commands.MasterArmOn
local CMD_MASTER_ARM_OFF = device_commands.MasterArmOff
local CMD_MASTER_ARM_SIM = device_commands.MasterArmSim
local CMD_DOGFIGHT_SWITCH = device_commands.DogfightSwitch

local ICMD_PLANE_FIRE = 84
local ICMD_PLANE_FIRE_OFF = 85
local ICMD_PLANE_DROP_FLARE_ONCE = 357
local ICMD_PLANE_MODE_BORE = 108
local ICMD_PLANE_MODE_CANNON = 113

local MASTER_OFF = 0
local MASTER_SIM = 1
local MASTER_ON = 2

local cms_program_active = false
local cms_program_remaining = 0
local cms_program_timer = 0.0
local master_arm_mode = MASTER_ON
local dogfight_active = false
local trigger_second_latched = false
local trigger_release_pending = false
local trigger_hold_timer = 0.0
local trigger_min_hold_time = 0.08
local cms_connected = true
local debug_state_period = 0.20
local debug_state_timer = 0.0
local debug_state_last_line = ""

local function dlog(msg)
    if log ~= nil and log.info ~= nil then
        log.info("FCK1C CMS: " .. tostring(msg))
    end
end

local function master_mode_name()
    if master_arm_mode == MASTER_ON then
        return "ON"
    end
    if master_arm_mode == MASTER_SIM then
        return "SIM"
    end
    return "OFF"
end

local function bool_to_num(v)
    if v then
        return 1
    end
    return 0
end

local function fire_gate_reason()
    if dispatch_action == nil then
        return "dispatch_nil"
    end
    if master_arm_mode ~= MASTER_ON then
        return "master_not_on"
    end
    return "ok"
end

local function log_gate_snapshot(tag)
    local stores_ok = (master_arm_mode == MASTER_ON)
    local cms_ok = (cms_connected and stores_ok)
    local reason = fire_gate_reason()
    local line = string.format(
        "DBG[%s] master=%s stores=%d cms=%d dogfight=%d trig=%d relpend=%d hold=%.2f cmsprog=%d cmsrem=%d reason=%s",
        tostring(tag),
        master_mode_name(),
        bool_to_num(stores_ok),
        bool_to_num(cms_ok),
        bool_to_num(dogfight_active),
        bool_to_num(trigger_second_latched),
        bool_to_num(trigger_release_pending),
        trigger_hold_timer,
        bool_to_num(cms_program_active),
        cms_program_remaining,
        reason
    )

    if line ~= debug_state_last_line then
        dlog(line)
        debug_state_last_line = line
    end
end

local function fire_on()
    if dispatch_action ~= nil then
        dlog("dispatch fire ON")
        local ok = dispatch_action(nil, ICMD_PLANE_FIRE)
        if ok ~= nil then
            dlog("dispatch fire ON result=" .. tostring(ok))
        end
    end
end

local function fire_off()
    if dispatch_action ~= nil then
        dlog("dispatch fire OFF")
        local ok = dispatch_action(nil, ICMD_PLANE_FIRE_OFF)
        if ok ~= nil then
            dlog("dispatch fire OFF result=" .. tostring(ok))
        end
    end
end

local function flare_once()
    if dispatch_action ~= nil then
        dlog("dispatch flare once")
        local ok = dispatch_action(nil, ICMD_PLANE_DROP_FLARE_ONCE)
        if ok ~= nil then
            dlog("dispatch flare result=" .. tostring(ok))
        end
    end
end

dev:listen_command(CMD_TRIGGER_FIRST_STAGE)
dev:listen_command(CMD_CMS_FORWARD)
dev:listen_command(CMD_CMS_AFT)
dev:listen_command(CMD_CMS_LEFT)
dev:listen_command(CMD_CMS_RIGHT)
dev:listen_command(CMD_CMS_PRESS)
dev:listen_command(CMD_TRIGGER_SECOND_STAGE)
dev:listen_command(CMD_MASTER_ARM_ON)
dev:listen_command(CMD_MASTER_ARM_OFF)
dev:listen_command(CMD_MASTER_ARM_SIM)
dev:listen_command(CMD_DOGFIGHT_SWITCH)

local function stores_release_allowed()
    return master_arm_mode == MASTER_ON
end

local function cms_release_allowed()
    return cms_connected and stores_release_allowed()
end

function post_initialize()
    dlog("loaded; CMS connected=" .. tostring(cms_connected) .. ", master=" .. tostring(master_arm_mode))
    log_gate_snapshot("init")
end

function SetCommand(command, value)
    dlog("SetCommand cmd=" .. tostring(command) .. " val=" .. string.format("%.2f", value))

    if command == CMD_TRIGGER_SECOND_STAGE then
        if value > 0.5 then
            log_gate_snapshot("trigger_down")
            if stores_release_allowed() then
                fire_on()
                trigger_second_latched = true
                trigger_release_pending = false
                trigger_hold_timer = trigger_min_hold_time
            else
                -- SIM/OFF: receive signal, but do not fire.
                dlog("trigger blocked by master arm mode=" .. tostring(master_arm_mode))
                trigger_second_latched = false
                trigger_release_pending = false
                trigger_hold_timer = 0.0
            end
        else
            log_gate_snapshot("trigger_up")
            if trigger_second_latched then
                if trigger_hold_timer <= 0.0 then
                    fire_off()
                    trigger_second_latched = false
                    trigger_release_pending = false
                else
                    trigger_release_pending = true
                end
            end
        end
        return
    end

    if value <= 0.5 then
        return
    end

    if command == CMD_MASTER_ARM_ON then
        master_arm_mode = MASTER_ON
        dlog("master arm -> ON")
        log_gate_snapshot("master_on")
        return
    end

    if command == CMD_MASTER_ARM_OFF then
        master_arm_mode = MASTER_OFF
        dlog("master arm -> OFF")
        if trigger_second_latched then
            fire_off()
            trigger_second_latched = false
            trigger_release_pending = false
            trigger_hold_timer = 0.0
        end
        log_gate_snapshot("master_off")
        return
    end

    if command == CMD_MASTER_ARM_SIM then
        master_arm_mode = MASTER_SIM
        dlog("master arm -> SIM")
        if trigger_second_latched then
            fire_off()
            trigger_second_latched = false
            trigger_release_pending = false
            trigger_hold_timer = 0.0
        end
        log_gate_snapshot("master_sim")
        return
    end

    if command == CMD_DOGFIGHT_SWITCH then
        dogfight_active = true
        master_arm_mode = MASTER_ON
        dlog("dogfight switch pressed; force master ON")
        log_gate_snapshot("dogfight")
        if dispatch_action ~= nil then
            -- Prefer short-range IR mode and keep cannon available.
            dispatch_action(nil, ICMD_PLANE_MODE_BORE)
            dispatch_action(nil, ICMD_PLANE_MODE_CANNON)
        end
        return
    end

    if command == CMD_CMS_FORWARD then
        if not cms_release_allowed() then
            dlog("cms forward blocked by arm/cms state")
            log_gate_snapshot("cms_forward_block")
            return
        end
        cms_program_active = true
        cms_program_remaining = 10
        cms_program_timer = 0.0
        flare_once()
        cms_program_remaining = cms_program_remaining - 1
        log_gate_snapshot("cms_forward_start")
        return
    end

    if command == CMD_CMS_LEFT then
        if cms_release_allowed() then
            flare_once()
            log_gate_snapshot("cms_left")
        else
            dlog("cms left blocked by arm/cms state")
            log_gate_snapshot("cms_left_block")
        end
        return
    end

    if command == CMD_CMS_AFT or command == CMD_CMS_RIGHT or command == CMD_CMS_PRESS then
        return
    end
end

function update()
    debug_state_timer = debug_state_timer + update_rate
    if debug_state_timer >= debug_state_period then
        debug_state_timer = debug_state_timer - debug_state_period
        log_gate_snapshot("tick")
    end

    if trigger_hold_timer > 0.0 then
        trigger_hold_timer = trigger_hold_timer - update_rate
        if trigger_hold_timer < 0.0 then
            trigger_hold_timer = 0.0
        end
    end

    if trigger_release_pending and trigger_second_latched and trigger_hold_timer <= 0.0 then
        fire_off()
        trigger_second_latched = false
        trigger_release_pending = false
        log_gate_snapshot("trigger_release")
    end

    if dogfight_active and master_arm_mode ~= MASTER_ON then
        dogfight_active = false
        log_gate_snapshot("dogfight_clear")
    end

    if not cms_program_active then
        return
    end

    if cms_program_remaining <= 0 then
        cms_program_active = false
        cms_program_timer = 0.0
        log_gate_snapshot("cms_done")
        return
    end

    cms_program_timer = cms_program_timer + update_rate
    if cms_program_timer >= 1.0 then
        cms_program_timer = cms_program_timer - 1.0
        if cms_release_allowed() then
            flare_once()
            log_gate_snapshot("cms_prog_drop")
        end
        cms_program_remaining = cms_program_remaining - 1
    end
end

need_to_be_closed = false
