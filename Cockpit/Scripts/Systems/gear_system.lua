local dev = GetSelf()

dofile(LockOn_Options.script_path .. "argument.lua")

local update_rate = 0.01
make_default_activity(update_rate)

function update()
    -- Nose wheel steering arg 2 is driven by the EFM.
end

function post_initialize()
    update()
end

need_to_be_closed = false
