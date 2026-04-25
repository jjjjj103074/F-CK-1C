-- BEGIN -- this part of the file is not intended for an end-user editing, but forget about that :)
--[[ --------------------------------------------------------------- ]]--

-- Native suspension probe switches. Keep these focused on DCS wheel mapping
-- only; do not use them for brake, steering, thrust, or aero tuning.
local SUSP_TEST_MARK = "GEOM_TEST_100_050_ACTIVE"
local SUSP_USE_MODELVIEWER_WHEEL_NODES = false
local SUSP_GEOMETRY_TEST = true
-- Geometry probe amount. For aggressive tests, change this to 0.60 or 1.00.
local SUSP_GEOMETRY_TEST_RADIUS_ADD = 1.00
local SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET = -0.50
local SUSP_GEOMETRY_RADIUS_ADD = SUSP_GEOMETRY_TEST and SUSP_GEOMETRY_TEST_RADIUS_ADD or 0.0
local SUSP_GEOMETRY_WHEEL_Y_OFFSET = SUSP_GEOMETRY_TEST and SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET or 0.0

local SUSP_ORIGINAL_WHEEL_NODES = {
    "F-CK-1C-F_W",
    "F-CK-1C-LBW",
    "F-CK-1C-RBW",
}

local SUSP_MODELVIEWER_WHEEL_NODES = {
    "FG_W",
    "LG_W",
    "RG_W",
}

local SUSP_ACTIVE_WHEEL_NODES =
    SUSP_USE_MODELVIEWER_WHEEL_NODES and SUSP_MODELVIEWER_WHEEL_NODES or SUSP_ORIGINAL_WHEEL_NODES

local SUSP_SESSION_ID =
    SUSP_TEST_MARK .. "_" .. tostring(os and os.time and os.time() or "no_time")

local SUSP_BASE_WHEEL_POS = {
    { 4.12, -1.912, 0.0 },
    { -1.185, -1.913, -0.7905 },
    { -1.185, -1.913, 0.7905 },
}

local SUSP_FINAL_WHEEL_POS = {
    { SUSP_BASE_WHEEL_POS[1][1], SUSP_BASE_WHEEL_POS[1][2] + SUSP_GEOMETRY_WHEEL_Y_OFFSET, SUSP_BASE_WHEEL_POS[1][3] },
    { SUSP_BASE_WHEEL_POS[2][1], SUSP_BASE_WHEEL_POS[2][2] + SUSP_GEOMETRY_WHEEL_Y_OFFSET, SUSP_BASE_WHEEL_POS[2][3] },
    { SUSP_BASE_WHEEL_POS[3][1], SUSP_BASE_WHEEL_POS[3][2] + SUSP_GEOMETRY_WHEEL_Y_OFFSET, SUSP_BASE_WHEEL_POS[3][3] },
}

local function fck_susp_log(message)
    if not io or not io.open then
        return
    end

    local userprofile = os and os.getenv and os.getenv("USERPROFILE") or "C:\\Users\\Ragdoll"
    local path = userprofile .. "\\Saved Games\\DCS\\Logs\\fck_susp_debug.log"
    local f = io.open(path, "a")
    if not f then
        return
    end

    local stamp = os and os.date and os.date("!%Y-%m-%dT%H:%M:%SZ ") or ""
    f:write(stamp, tostring(message), "\n")
    f:close()
end

fck_susp_log(string.format(
    "FM/config.lua session_id=%s SUSP_TEST_MARK=%s SUSP_USE_MODELVIEWER_WHEEL_NODES=%s SUSP_GEOMETRY_TEST=%s SUSP_GEOMETRY_TEST_RADIUS_ADD=%.2f active_radius_add=%.2f SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET=%.2f active_wheel_y_offset=%.2f nodes=%s/%s/%s final_pos=(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f)",
    SUSP_SESSION_ID,
    SUSP_TEST_MARK,
    tostring(SUSP_USE_MODELVIEWER_WHEEL_NODES),
    tostring(SUSP_GEOMETRY_TEST),
    SUSP_GEOMETRY_TEST_RADIUS_ADD,
    SUSP_GEOMETRY_RADIUS_ADD,
    SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET,
    SUSP_GEOMETRY_WHEEL_Y_OFFSET,
    SUSP_ACTIVE_WHEEL_NODES[1],
    SUSP_ACTIVE_WHEEL_NODES[2],
    SUSP_ACTIVE_WHEEL_NODES[3],
    SUSP_FINAL_WHEEL_POS[1][1],
    SUSP_FINAL_WHEEL_POS[1][2],
    SUSP_FINAL_WHEEL_POS[1][3],
    SUSP_FINAL_WHEEL_POS[2][1],
    SUSP_FINAL_WHEEL_POS[2][2],
    SUSP_FINAL_WHEEL_POS[2][3],
    SUSP_FINAL_WHEEL_POS[3][1],
    SUSP_FINAL_WHEEL_POS[3][2],
    SUSP_FINAL_WHEEL_POS[3][3]
))

FM = {
    center_of_mass = {-0.6, 0, 0},
    moment_of_inertia = {38912, 254758, 223845, -705},

    -- Keep the fallback path disabled while validating the active collision
    -- shell contract and DCS-provided suspension feedback.
    zeroize_amortizers_before_collision_check = false,

    suspension = {
        { -- Nose
            pos = SUSP_FINAL_WHEEL_POS[1],
            mass = 100,
            damage_element = 0,
            self_attitude = true,
            wheel_axle_offset = 0.14,
            yaw_limit = math.rad(60.0),
            allowable_hard_contact_length = 0.19,

            amortizer_max_length = 0.53,
            amortizer_basic_length = 0.53,
            amortizer_reduce_length = 0.53,

            amortizer_spring_force_factor = 450000.0,
            amortizer_spring_force_factor_rate = 1,
            amortizer_static_force = 47500.0,
            amortizer_direct_damper_force_factor = 80000,
            amortizer_back_damper_force_factor = 50000,

            anti_skid_installed = true,

            wheel_radius = 0.2286 + SUSP_GEOMETRY_RADIUS_ADD,
            wheel_static_friction_factor = 0.75,
            wheel_roll_friction_factor = 0.018,
            wheel_glide_friction_factor = 0.35,
            wheel_damage_force_factor = 450.0,
            wheel_moment_of_inertia = 0.15,
            wheel_brake_moment_max = 50.0,

            arg_post = 0,
            arg_amortizer = 1,
            arg_wheel_yaw = 2,
            collision_shell_name = SUSP_ACTIVE_WHEEL_NODES[1],
            arg_wheel_damage = 134,
        },

        { -- Left main
            pos = SUSP_FINAL_WHEEL_POS[2],
            mass = 200,
            damage_element = 3,
            wheel_axle_offset = 0.0,
            self_attitude = false,
            yaw_limit = math.rad(0.0),

            amortizer_max_length = 0.45,
            amortizer_basic_length = 0.45,
            amortizer_reduce_length = 0.63,

            amortizer_spring_force_factor = 480000.0,
            amortizer_spring_force_factor_rate = 3,
            amortizer_static_force = 220000.0,
            amortizer_direct_damper_force_factor = 90000,
            amortizer_back_damper_force_factor = 70000,

            allowable_hard_contact_length = 0.25,
            anti_skid_installed = true,
            wheel_damage_speed = 180,
            wheel_moment_of_inertia = 0.6,

            wheel_radius = 0.3048 + SUSP_GEOMETRY_RADIUS_ADD,
            wheel_static_friction_factor = 0.75,
            wheel_side_friction_factor = 1.0,
            wheel_roll_friction_factor = 0.022,
            wheel_glide_friction_factor = 0.35,
            wheel_damage_force_factor = 450.0,
            wheel_brake_moment_max = 25000.0,

            arg_post = 5,
            arg_amortizer = 6,
            arg_wheel_yaw = -1,
            collision_shell_name = SUSP_ACTIVE_WHEEL_NODES[2],
            arg_wheel_damage = 136,
        },

        { -- Right main
            pos = SUSP_FINAL_WHEEL_POS[3],
            mass = 200,
            damage_element = 5,
            wheel_axle_offset = 0.0,
            self_attitude = false,
            yaw_limit = math.rad(0.0),

            amortizer_max_length = 0.45,
            amortizer_basic_length = 0.45,
            amortizer_reduce_length = 0.63,

            amortizer_spring_force_factor = 480000.0,
            amortizer_spring_force_factor_rate = 3,
            amortizer_static_force = 202394.0,
            amortizer_direct_damper_force_factor = 50000,
            amortizer_back_damper_force_factor = 60000,

            allowable_hard_contact_length = 0.25,
            anti_skid_installed = true,
            wheel_damage_speed = 180,
            wheel_moment_of_inertia = 0.6,

            wheel_radius = 0.3048 + SUSP_GEOMETRY_RADIUS_ADD,
            wheel_static_friction_factor = 0.75,
            wheel_side_friction_factor = 1.0,
            wheel_roll_friction_factor = 0.022,
            wheel_glide_friction_factor = 0.35,
            wheel_damage_force_factor = 450.0,
            wheel_brake_moment_max = 25000.0,

            arg_post = 3,
            arg_amortizer = 4,
            arg_wheel_yaw = -1,
            collision_shell_name = SUSP_ACTIVE_WHEEL_NODES[3],
        },
    },

    disable_built_in_oxygen_system = true,
    minor_shake_ampl = 0.21,
    major_shake_ampl = 0.5,
}

fck_susp_log(string.format(
    "FM/config.lua FINAL_SUSP_TABLE session_id=%s SUSP_TEST_MARK=%s SUSP_GEOMETRY_TEST=%s radius_add=%.2f wheel_y_offset=%.2f nodes=%s/%s/%s final_wheel_radius=%.4f/%.4f/%.4f final_wheel_pos=(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f)",
    SUSP_SESSION_ID,
    SUSP_TEST_MARK,
    tostring(SUSP_GEOMETRY_TEST),
    SUSP_GEOMETRY_RADIUS_ADD,
    SUSP_GEOMETRY_WHEEL_Y_OFFSET,
    FM.suspension[1].collision_shell_name,
    FM.suspension[2].collision_shell_name,
    FM.suspension[3].collision_shell_name,
    FM.suspension[1].wheel_radius,
    FM.suspension[2].wheel_radius,
    FM.suspension[3].wheel_radius,
    FM.suspension[1].pos[1],
    FM.suspension[1].pos[2],
    FM.suspension[1].pos[3],
    FM.suspension[2].pos[1],
    FM.suspension[2].pos[2],
    FM.suspension[2].pos[3],
    FM.suspension[3].pos[1],
    FM.suspension[3].pos[2],
    FM.suspension[3].pos[3]
))
