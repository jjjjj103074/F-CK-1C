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
local ICMD_PLANE_DROP_SNAR_ONCE = 176
local ICMD_PLANE_DROP_SNAR_ONCE_OFF = 536
local ICMD_PLANE_DROP_FLARE_ONCE = 357
local ICMD_PLANE_DROP_CHAFF_ONCE = 358
local ICMD_PLANE_MODE_BORE = 108
local ICMD_PLANE_MODE_CANNON = 113

local MASTER_OFF = 0
local MASTER_SIM = 1
local MASTER_ON = 2

local cms_program_active = false
local cms_program_name = "idle"
local cms_program_steps_remaining = 0
local cms_program_timer = 0.0
local cms_program_interval = 1.0
local cms_program_flare_per_step = 0
local cms_program_chaff_per_step = 0
local master_arm_mode = MASTER_ON
local dogfight_active = false
local trigger_second_latched = false
local trigger_release_pending = false
local trigger_hold_timer = 0.0
local trigger_min_hold_time = 0.08
local cms_connected = true
local cms_pair_release_pending = 0
local cms_pair_release_latched = false
local cms_pair_release_hold_timer = 0.0
local cms_pair_release_hold_time = 0.04
local cms_pair_release_gap_timer = 0.0
local cms_pair_release_gap_time = 0.12
local debug_state_period = 0.20
local debug_state_timer = 0.0
local debug_state_last_line = ""
local stores_release_allowed
local cms_release_allowed
local hmcs_gun_firing = get_param_handle("HMCS_GUN_FIRING")

local function push_hmcs_gun_state()
    if trigger_second_latched and master_arm_mode == MASTER_ON then
        hmcs_gun_firing:set(1)
        return
    end
    hmcs_gun_firing:set(0)
end

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
        "DBG[%s] master=%s stores=%d cms=%d dogfight=%d trig=%d relpend=%d hold=%.2f cmsprog=%d mode=%s cmsrem=%d reason=%s",
        tostring(tag),
        master_mode_name(),
        bool_to_num(stores_ok),
        bool_to_num(cms_ok),
        bool_to_num(dogfight_active),
        bool_to_num(trigger_second_latched),
        bool_to_num(trigger_release_pending),
        trigger_hold_timer,
        bool_to_num(cms_program_active),
        tostring(cms_program_name),
        cms_program_steps_remaining,
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

local function chaff_once()
    if dispatch_action ~= nil then
        dlog("dispatch chaff once")
        local ok = dispatch_action(nil, ICMD_PLANE_DROP_CHAFF_ONCE)
        if ok ~= nil then
            dlog("dispatch chaff result=" .. tostring(ok))
        end
    end
end

local function pair_release_off()
    if not cms_pair_release_latched then
        return
    end

    if dispatch_action ~= nil then
        dlog("dispatch cm release OFF")
        local ok = dispatch_action(nil, ICMD_PLANE_DROP_SNAR_ONCE_OFF)
        if ok ~= nil then
            dlog("dispatch cm release OFF result=" .. tostring(ok))
        end
    end

    cms_pair_release_latched = false
    cms_pair_release_hold_timer = 0.0
end

local function clear_pair_release_queue(reason_tag)
    if cms_pair_release_latched or cms_pair_release_pending > 0 then
        dlog(
            "clear cm release queue -> "
                .. tostring(reason_tag)
                .. " pending="
                .. tostring(cms_pair_release_pending)
        )
    end

    pair_release_off()
    cms_pair_release_pending = 0
    cms_pair_release_gap_timer = 0.0
end

local function queue_pair_release(count)
    if count == nil or count <= 0 then
        return
    end

    cms_pair_release_pending = cms_pair_release_pending + count
    dlog("queue cm release x" .. tostring(count) .. " pending=" .. tostring(cms_pair_release_pending))
end

local function release_countermeasures(flare_count, chaff_count)
    if flare_count == nil then
        flare_count = 0
    end
    if chaff_count == nil then
        chaff_count = 0
    end

    if flare_count <= 0 and chaff_count <= 0 then
        return
    end

    if not cms_release_allowed() then
        dlog("countermeasure release blocked by arm/cms state")
        return
    end

    local paired_release_count = math.min(flare_count, chaff_count)
    if paired_release_count > 0 then
        queue_pair_release(paired_release_count)
        flare_count = flare_count - paired_release_count
        chaff_count = chaff_count - paired_release_count
    end

    local i = 0
    while i < flare_count do
        flare_once()
        i = i + 1
    end

    i = 0
    while i < chaff_count do
        chaff_once()
        i = i + 1
    end
end

local function stop_cms_program(reason_tag)
    if cms_program_active then
        dlog("cms program stop -> " .. tostring(reason_tag))
    end
    cms_program_active = false
    cms_program_name = "idle"
    cms_program_steps_remaining = 0
    cms_program_timer = 0.0
    cms_program_interval = 1.0
    cms_program_flare_per_step = 0
    cms_program_chaff_per_step = 0
    log_gate_snapshot(reason_tag)
end

local function abort_cms_program(reason_tag)
    stop_cms_program(reason_tag)
    clear_pair_release_queue(reason_tag)
end

local function start_cms_program(name, steps, interval, flare_per_step, chaff_per_step, immediate_release)
    if not cms_release_allowed() then
        dlog("cms program " .. tostring(name) .. " blocked by arm/cms state")
        log_gate_snapshot("cms_" .. tostring(name) .. "_block")
        return
    end

    cms_program_active = true
    cms_program_name = tostring(name)
    cms_program_steps_remaining = steps
    cms_program_timer = 0.0
    cms_program_interval = interval
    cms_program_flare_per_step = flare_per_step
    cms_program_chaff_per_step = chaff_per_step

    if immediate_release and cms_program_steps_remaining > 0 then
        release_countermeasures(cms_program_flare_per_step, cms_program_chaff_per_step)
        cms_program_steps_remaining = cms_program_steps_remaining - 1
    end

    log_gate_snapshot("cms_" .. tostring(name) .. "_start")
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

stores_release_allowed = function()
    return master_arm_mode == MASTER_ON
end

cms_release_allowed = function()
    return cms_connected and stores_release_allowed()
end

function post_initialize()
    dlog("loaded; CMS connected=" .. tostring(cms_connected) .. ", master=" .. tostring(master_arm_mode))
    push_hmcs_gun_state()
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
                push_hmcs_gun_state()
            else
                -- SIM/OFF: receive signal, but do not fire.
                dlog("trigger blocked by master arm mode=" .. tostring(master_arm_mode))
                trigger_second_latched = false
                trigger_release_pending = false
                trigger_hold_timer = 0.0
                push_hmcs_gun_state()
            end
        else
            log_gate_snapshot("trigger_up")
            if trigger_second_latched then
                if trigger_hold_timer <= 0.0 then
                    fire_off()
                    trigger_second_latched = false
                    trigger_release_pending = false
                    push_hmcs_gun_state()
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
            push_hmcs_gun_state()
        end
        abort_cms_program("master_off_abort")
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
            push_hmcs_gun_state()
        end
        abort_cms_program("master_sim_abort")
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
        start_cms_program("forward", 10, 1.0, 1, 1, true)
        return
    end

    if command == CMD_CMS_LEFT then
        start_cms_program("left", 2, 0.12, 1, 1, true)
        return
    end

    if command == CMD_CMS_AFT then
        start_cms_program("aft", 7, 0.5, 0, 1, true)
        return
    end

    if command == CMD_CMS_RIGHT then
        abort_cms_program("cms_right_abort")
        return
    end

    if command == CMD_CMS_PRESS then
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
        push_hmcs_gun_state()
        log_gate_snapshot("trigger_release")
    end

    push_hmcs_gun_state()

    if cms_pair_release_latched then
        cms_pair_release_hold_timer = cms_pair_release_hold_timer - update_rate
        if cms_pair_release_hold_timer <= 0.0 then
            pair_release_off()
            cms_pair_release_pending = cms_pair_release_pending - 1
            if cms_pair_release_pending < 0 then
                cms_pair_release_pending = 0
            end
            cms_pair_release_gap_timer = cms_pair_release_gap_time
        end
    elseif cms_pair_release_pending > 0 then
        if cms_pair_release_gap_timer > 0.0 then
            cms_pair_release_gap_timer = cms_pair_release_gap_timer - update_rate
            if cms_pair_release_gap_timer < 0.0 then
                cms_pair_release_gap_timer = 0.0
            end
        elseif cms_release_allowed() then
            if dispatch_action ~= nil then
                dlog("dispatch cm release ON")
                local ok = dispatch_action(nil, ICMD_PLANE_DROP_SNAR_ONCE)
                if ok ~= nil then
                    dlog("dispatch cm release ON result=" .. tostring(ok))
                end
                cms_pair_release_latched = true
                cms_pair_release_hold_timer = cms_pair_release_hold_time
            else
                clear_pair_release_queue("dispatch_nil")
            end
        else
            clear_pair_release_queue("cms_pair_block")
        end
    end

    if dogfight_active and master_arm_mode ~= MASTER_ON then
        dogfight_active = false
        log_gate_snapshot("dogfight_clear")
    end

    if not cms_program_active then
        return
    end

    if cms_program_steps_remaining <= 0 then
        stop_cms_program("cms_done")
        return
    end

    cms_program_timer = cms_program_timer + update_rate
    if cms_program_timer >= cms_program_interval then
        cms_program_timer = cms_program_timer - cms_program_interval
        if cms_release_allowed() then
            release_countermeasures(cms_program_flare_per_step, cms_program_chaff_per_step)
            cms_program_steps_remaining = cms_program_steps_remaining - 1
            log_gate_snapshot("cms_prog_drop")
        else
            abort_cms_program("cms_prog_block")
        end
    end
end

need_to_be_closed = false
