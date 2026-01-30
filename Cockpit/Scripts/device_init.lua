dofile(LockOn_Options.common_script_path .. "tools.lua")

dofile(LockOn_Options.common_script_path .. "KNEEBOARD/declare_kneeboard_device_left.lua")


indicators                  = {}
indicators[#indicators + 1] = {
    "ccControlsIndicatorBase",
    LockOn_Options.script_path .. "ControlsIndicator/ControlsIndicator.lua",
    nil
}
