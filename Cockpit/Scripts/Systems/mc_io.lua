-- Numeric transport only. Mission decisions and operation ordering belong to MC.
-- MissionActionKind values are defined in Core/Contracts/MissionActions.h.
local M = {}

local function integer(value, minimum, maximum)
    return type(value) == "number"
        and value == value
        and value >= minimum
        and value <= maximum
        and value == math.floor(value)
end

function M.new(execute)
    local generation = get_param_handle(cockpit_params.McGeneration)
    local available = get_param_handle(cockpit_params.McAvailable)
    local first = get_param_handle(cockpit_params.McActionFirstSequence)
    local count = get_param_handle(cockpit_params.McActionCount)
    local acknowledged = get_param_handle(cockpit_params.McAcknowledgedSequence)
    local slots = {
        get_param_handle(cockpit_params.McAction01),
        get_param_handle(cockpit_params.McAction02),
        get_param_handle(cockpit_params.McAction03),
        get_param_handle(cockpit_params.McAction04),
        get_param_handle(cockpit_params.McAction05),
        get_param_handle(cockpit_params.McAction06),
        get_param_handle(cockpit_params.McAction07),
        get_param_handle(cockpit_params.McAction08),
        get_param_handle(cockpit_params.McAction09),
        get_param_handle(cockpit_params.McAction10),
        get_param_handle(cockpit_params.McAction11),
        get_param_handle(cockpit_params.McAction12),
        get_param_handle(cockpit_params.McAction13),
        get_param_handle(cockpit_params.McAction14),
        get_param_handle(cockpit_params.McAction15),
        get_param_handle(cockpit_params.McAction16),
    }
    local last_sequence = 0
    acknowledged:set(0)

    local function update()
        local before = generation:get()
        if not integer(before, 2, 9007199254740991) or before % 2 ~= 0 or available:get() < 0.5 then return end
        local size = count:get()
        local start = first:get()
        if not integer(size, 0, #slots) then return end
        if size > 0 and not integer(start, 1, 9007199254740991 - size + 1) then return end
        local actions = {}
        for i = 1, size do
            actions[i] = slots[i]:get()
            if not integer(actions[i], 1, 20) then return end
        end
        if generation:get() ~= before then return end

        for i = 1, size do
            local sequence = start + i - 1
            if sequence > last_sequence then
                if sequence ~= last_sequence + 1 then return end
                local ok, done = pcall(execute, actions[i])
                if not ok or done ~= true then return end
                last_sequence = sequence
                acknowledged:set(last_sequence)
            end
        end
    end

    return { update = update }
end

return M
