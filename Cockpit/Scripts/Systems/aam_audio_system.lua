local dev = GetSelf()

local update_rate = 0.05
make_default_activity(update_rate)

local AIM9_TONE_OFF = 0
local AIM9_TONE_SEEK = 1
local AIM9_TONE_ACQUIRE = 2
local AIM9_TONE_LOCK = 3
local SOUND_TEST_CYCLE_CMD = 3052

local tone_state = get_param_handle("AIM9_TONE_STATE")
local aim9_weapon_active = get_param_handle("AIM9_WEAPON_ACTIVE")

local sound_host = nil
local seek_sound = nil
local acquire_sound = nil
local lock_sound = nil
local active_tone = AIM9_TONE_OFF
local active_gain = 1.6
local test_gain = 1.4
local test_index = 0
local test_sound = nil
local test_sound_name = nil
local test_playlist = {
    { name = "AIM9 Seek", sdef = "Cockpit/Test/AIM9_Seek" },
    { name = "AIM9 Acquire LO", sdef = "Cockpit/Test/AIM9_AcquireLo" },
    { name = "AIM9 Tone HI", sdef = "Cockpit/Test/AIM9_ToneHi" },
    { name = "Radar Lock", sdef = "Cockpit/Test/Radar_LOCK" },
    { name = "Warning Altitude", sdef = "Cockpit/Test/Warning_ALTITUDE" },
    { name = "Warning Pull Up", sdef = "Cockpit/Test/Warning_PULL_UP" },
    { name = "Warning General", sdef = "Cockpit/Test/Warning_WARNING" },
    { name = "Text One", sdef = "Cockpit/Test/Text_ONE" },
    { name = "Text Two", sdef = "Cockpit/Test/Text_TWO" },
    { name = "Text Tree", sdef = "Cockpit/Test/Text_TREE" },
    { name = "Text Four", sdef = "Cockpit/Test/Text_FOUR" },
    { name = "Text Five", sdef = "Cockpit/Test/Text_FIVE" },
    { name = "Text Six", sdef = "Cockpit/Test/Text_SIX" },
    { name = "Text Seven", sdef = "Cockpit/Test/Text_SEVEN" },
    { name = "Text Eight", sdef = "Cockpit/Test/Text_EIGHT" },
    { name = "Text Niner", sdef = "Cockpit/Test/Text_NINER" },
    { name = "Text Dozen", sdef = "Cockpit/Test/Text_DOZEN" },
    { name = "Text Hundred", sdef = "Cockpit/Test/Text_HUNDRED" },
    { name = "Word Avionics", sdef = "Cockpit/Test/Word_Avionics" },
    { name = "Word Engine", sdef = "Cockpit/Test/Word_ENGINE" },
    { name = "Word Failure", sdef = "Cockpit/Test/Word_Failure" },
    { name = "Word FCR", sdef = "Cockpit/Test/Word_FCR" },
    { name = "Word Hydraulics", sdef = "Cockpit/Test/Word_Hydraulics" },
}

local function dlog(msg)
    if log ~= nil and log.info ~= nil then
        log.info("FCK1C AAM AUDIO: " .. tostring(msg))
    end
end

local function stop_sound(sound_obj)
    if sound_obj ~= nil and sound_obj.is_playing ~= nil and sound_obj:is_playing() then
        sound_obj:stop()
    end
end

local function update_playing_sound(sound_obj)
    if sound_obj == nil then
        return
    end

    if sound_obj.update ~= nil then
        sound_obj:update(nil, active_gain, nil)
    end
end

local function start_sound(sound_obj, label, prefer_continue)
    if sound_obj == nil then
        dlog("start requested but sound is nil: " .. tostring(label))
        return
    end

    if sound_obj.update ~= nil then
        sound_obj:update(nil, active_gain, nil)
    end

    if prefer_continue and sound_obj.play_continue ~= nil then
        sound_obj:play_continue()
    elseif sound_obj.play_once ~= nil then
        sound_obj:play_once()
    elseif sound_obj.play_continue ~= nil then
        sound_obj:play_continue()
    end
end

local function sustain_sound(sound_obj, label)
    if sound_obj == nil then
        return
    end

    update_playing_sound(sound_obj)

    if sound_obj.is_playing ~= nil and not sound_obj:is_playing() then
        start_sound(sound_obj, label, true)
        dlog("retrigger " .. tostring(label))
    end
end

local function stop_test_sound()
    if test_sound ~= nil then
        stop_sound(test_sound)
    end
    test_sound = nil
    test_sound_name = nil
end

local function play_test_sound(entry)
    stop_test_sound()

    if sound_host == nil or entry == nil then
        dlog("test play skipped")
        return
    end

    local next_sound = sound_host:create_sound(entry.sdef)
    if next_sound == nil then
        dlog("test sound create failed: " .. tostring(entry.sdef))
        return
    end

    if next_sound.update ~= nil then
        next_sound:update(nil, test_gain, nil)
    end

    if next_sound.play_once ~= nil then
        next_sound:play_once()
    elseif next_sound.play_continue ~= nil then
        next_sound:play_continue()
    end

    test_sound = next_sound
    test_sound_name = entry.name
    dlog("test -> " .. tostring(entry.name) .. " [" .. tostring(entry.sdef) .. "]")
end

local function set_active_tone(next_tone)
    if active_tone == next_tone then
        return
    end

    stop_sound(seek_sound)
    stop_sound(acquire_sound)
    stop_sound(lock_sound)

    if next_tone == AIM9_TONE_SEEK then
        start_sound(seek_sound, "seek", true)
    elseif next_tone == AIM9_TONE_ACQUIRE then
        start_sound(acquire_sound, "acquire", true)
    elseif next_tone == AIM9_TONE_LOCK then
        start_sound(lock_sound, "tone_hi", true)
    end

    active_tone = next_tone
    dlog("tone -> " .. tostring(next_tone))
end

function post_initialize()
    tone_state:set(AIM9_TONE_OFF)
    aim9_weapon_active:set(0)

    if create_sound_host == nil then
        dlog("create_sound_host unavailable")
        return
    end

    sound_host = create_sound_host("COCKPIT_RADAR_WARN", "HEADPHONES", 0, 0, 0)
    if sound_host == nil then
        sound_host = create_sound_host("COCKPIT", "HEADPHONES", 0, 0, 0)
        if sound_host ~= nil then
            dlog("fallback host -> COCKPIT/HEADPHONES")
        end
    else
        dlog("host -> COCKPIT_RADAR_WARN/HEADPHONES")
    end

    if sound_host == nil then
        dlog("sound host creation failed")
        return
    end

    seek_sound = sound_host:create_sound("Cockpit/AIM9/Seek")
    acquire_sound = sound_host:create_sound("Cockpit/AIM9/AcquireLo")
    lock_sound = sound_host:create_sound("Cockpit/AIM9/ToneHi")
    dlog("seek sound created=" .. tostring(seek_sound ~= nil))
    dlog("acquire sound created=" .. tostring(acquire_sound ~= nil))
    dlog("lock sound created=" .. tostring(lock_sound ~= nil))
    dlog("test playlist size=" .. tostring(#test_playlist))
    dlog("initialized")
end

function SetCommand(command, value)
    if command == SOUND_TEST_CYCLE_CMD and (value or 0) > 0.5 then
        if #test_playlist == 0 then
            dlog("test playlist empty")
            return
        end

        test_index = test_index + 1
        if test_index > #test_playlist then
            test_index = 1
        end

        play_test_sound(test_playlist[test_index])
    end
end

function update()
    local requested_tone = math.floor((tone_state:get() or 0) + 0.5)
    if requested_tone < AIM9_TONE_OFF or requested_tone > AIM9_TONE_LOCK then
        requested_tone = AIM9_TONE_OFF
    end

    set_active_tone(requested_tone)

    if active_tone == AIM9_TONE_SEEK then
        sustain_sound(seek_sound, "seek")
    elseif active_tone == AIM9_TONE_ACQUIRE then
        sustain_sound(acquire_sound, "acquire")
    elseif active_tone == AIM9_TONE_LOCK then
        sustain_sound(lock_sound, "tone_hi")
    end

    if test_sound ~= nil then
        update_playing_sound(test_sound)
    end
end

need_to_be_closed = false
