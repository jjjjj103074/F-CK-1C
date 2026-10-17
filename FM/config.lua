-- Native suspension and collision-shell tuning.
-- Keep these switches focused on DCS wheel mapping only; do not use them for
-- brake, steering, thrust, or aerodynamic tuning.
local SUSP_TEST_MARK = "GEOM_TEST_100_050_ACTIVE"
local SUSP_USE_MODELVIEWER_WHEEL_NODES = false
local SUSP_GEOMETRY_TEST = false
-- Geometry probe amount. For aggressive tests, change this to 0.60 or 1.00.
local SUSP_GEOMETRY_TEST_RADIUS_ADD = 0.00
local SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET = 0.00
local SUSP_GEOMETRY_RADIUS_ADD = SUSP_GEOMETRY_TEST and SUSP_GEOMETRY_TEST_RADIUS_ADD or 0.0
local SUSP_GEOMETRY_WHEEL_Y_OFFSET = SUSP_GEOMETRY_TEST and SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET or 0.0

local SUSP_ORIGINAL_WHEEL_NODES = {
    "WHEEL_F",
    "WHEEL_L",
    "WHEEL_R",
}

local SUSP_MODELVIEWER_WHEEL_NODES = {
    "WHEEL_F",
    "WHEEL_L",
    "WHEEL_R",
}

local SUSP_ACTIVE_WHEEL_NODES = SUSP_USE_MODELVIEWER_WHEEL_NODES and SUSP_MODELVIEWER_WHEEL_NODES or SUSP_ORIGINAL_WHEEL_NODES

local SUSP_SESSION_ID = SUSP_TEST_MARK .. "_" .. tostring(os and os.time and os.time() or "no_time")

local SUSP_BASE_WHEEL_POS = {
    { 4.109, -1.685, 0.0 },
    { -1.19, -1.605, -0.899 },
    { -1.19, -1.605, 0.899 },
}

local SUSP_FINAL_WHEEL_POS = {
    { SUSP_BASE_WHEEL_POS[1][1], SUSP_BASE_WHEEL_POS[1][2] + SUSP_GEOMETRY_WHEEL_Y_OFFSET, SUSP_BASE_WHEEL_POS[1][3] },
    { SUSP_BASE_WHEEL_POS[2][1], SUSP_BASE_WHEEL_POS[2][2] + SUSP_GEOMETRY_WHEEL_Y_OFFSET, SUSP_BASE_WHEEL_POS[2][3] },
    { SUSP_BASE_WHEEL_POS[3][1], SUSP_BASE_WHEEL_POS[3][2] + SUSP_GEOMETRY_WHEEL_Y_OFFSET, SUSP_BASE_WHEEL_POS[3][3] },
}

local function fck_susp_log(message)
    if not io or not io.open then return end

    local userprofile = os and os.getenv and os.getenv("USERPROFILE") or nil
    local path = "fck_susp_debug.log"
    if userprofile and userprofile ~= "" then path = userprofile .. "\\Saved Games\\DCS\\Logs\\fck_susp_debug.log" end

    local f = io.open(path, "a")
    if not f then return end

    local stamp = os and os.date and os.date("!%Y-%m-%dT%H:%M:%SZ ") or ""
    f:write(stamp, tostring(message), "\n")
    f:close()
end

fck_susp_log(string.format("FM/config.lua session_id=%s SUSP_TEST_MARK=%s SUSP_USE_MODELVIEWER_WHEEL_NODES=%s SUSP_GEOMETRY_TEST=%s SUSP_GEOMETRY_TEST_RADIUS_ADD=%.2f active_radius_add=%.2f SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET=%.2f active_wheel_y_offset=%.2f nodes=%s/%s/%s final_pos=(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f)", SUSP_SESSION_ID, SUSP_TEST_MARK, tostring(SUSP_USE_MODELVIEWER_WHEEL_NODES), tostring(SUSP_GEOMETRY_TEST), SUSP_GEOMETRY_TEST_RADIUS_ADD, SUSP_GEOMETRY_RADIUS_ADD, SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET, SUSP_GEOMETRY_WHEEL_Y_OFFSET, SUSP_ACTIVE_WHEEL_NODES[1], SUSP_ACTIVE_WHEEL_NODES[2], SUSP_ACTIVE_WHEEL_NODES[3], SUSP_FINAL_WHEEL_POS[1][1], SUSP_FINAL_WHEEL_POS[1][2], SUSP_FINAL_WHEEL_POS[1][3], SUSP_FINAL_WHEEL_POS[2][1], SUSP_FINAL_WHEEL_POS[2][2], SUSP_FINAL_WHEEL_POS[2][3], SUSP_FINAL_WHEEL_POS[3][1], SUSP_FINAL_WHEEL_POS[3][2], SUSP_FINAL_WHEEL_POS[3][3]))

-- Main gear suspension parameters (shared between left and right main wheels)
local mainGear = {
    mass = 200,
    wheel_axle_offset = 0.0,
    self_attitude = false,
    yaw_limit = math.rad(0.0),

    amortizer_max_length = 0.184,
    amortizer_basic_length = 0.184,
    amortizer_reduce_length = 0.01,

    amortizer_spring_force_factor = 5.0e+7,
    amortizer_spring_force_factor_rate = 3,
    amortizer_static_force = 5000.0,
    amortizer_direct_damper_force_factor = 25000,
    amortizer_back_damper_force_factor = 60000,
    damper_coeff = 300,

    allowable_hard_contact_length = 0.1,
    anti_skid_installed = true,
    wheel_damage_speed = 180,
    wheel_moment_of_inertia = 0.6,

    wheel_radius = 0.6055 / 2,
    wheel_static_friction_factor = 0.75,
    wheel_side_friction_factor = 1.0,
    wheel_roll_friction_factor = 0.08,
    wheel_glide_friction_factor = 0.35,
    wheel_damage_force_factor = 450.0,
    wheel_brake_moment_max = 7500.0,
}

FM = {
    center_of_mass = { -0.7, 0, 0 },
    moment_of_inertia = { 11000.0, 66000.0, 62000.0, -1000.0 },

    -- Keep the fallback path disabled while validating the active collision
    -- shell contract and DCS-provided suspension feedback.
    zeroize_amortizers_before_collision_check = false,

    suspension = {
        { -- Nose
            -- pos = SUSP_FINAL_WHEEL_POS[1],
            mass = 100,
            damage_element = 83,
            self_attitude = false,
            yaw_limit = math.rad(60.0),
            allowable_hard_contact_length = 0.1,

            amortizer_min_length = 0.0,
            amortizer_max_length = 0.2,
            amortizer_basic_length = 0.2,
            amortizer_reduce_length = 0.01,

            amortizer_spring_force_factor = 0.85e+7,
            amortizer_spring_force_factor_rate = 3,
            amortizer_static_force = 5000.0,
            amortizer_direct_damper_force_factor = 10000,
            amortizer_back_damper_force_factor = 30000,
            damper_coeff = 300,

            anti_skid_installed = false,

            wheel_radius = 0.4547 / 2,
            wheel_static_friction_factor = 0.75,
            wheel_side_friction_factor = 0.55,
            wheel_roll_friction_factor = 0.08,
            wheel_glide_friction_factor = 0.35,
            wheel_damage_force_factor = 450.0,
            wheel_damage_speed = 200,
            wheel_brake_moment_max = 0,
            wheel_moment_of_inertia = 0.15,

            arg_post = 0,
            arg_amortizer = 1,
            collision_shell_name = "WHEEL_F",
            arg_wheel_damage = 134,
        },

        { -- Left main
            -- pos                                  = SUSP_FINAL_WHEEL_POS[2],
            damage_element = 84,

            mass = mainGear.mass,
            wheel_axle_offset = mainGear.wheel_axle_offset,
            self_attitude = mainGear.self_attitude,
            yaw_limit = mainGear.yaw_limit,

            amortizer_min_length = 0.0,
            amortizer_max_length = mainGear.amortizer_max_length,
            amortizer_basic_length = mainGear.amortizer_basic_length,
            amortizer_reduce_length = mainGear.amortizer_reduce_length,

            amortizer_spring_force_factor = mainGear.amortizer_spring_force_factor,
            amortizer_spring_force_factor_rate = mainGear.amortizer_spring_force_factor_rate,
            amortizer_static_force = mainGear.amortizer_static_force,
            amortizer_direct_damper_force_factor = mainGear.amortizer_direct_damper_force_factor,
            amortizer_back_damper_force_factor = mainGear.amortizer_back_damper_force_factor,
            damper_coeff = mainGear.damper_coeff,

            allowable_hard_contact_length = mainGear.allowable_hard_contact_length,
            anti_skid_installed = mainGear.anti_skid_installed,
            wheel_damage_speed = mainGear.wheel_damage_speed,
            wheel_moment_of_inertia = mainGear.wheel_moment_of_inertia,

            wheel_radius = mainGear.wheel_radius,
            wheel_static_friction_factor = mainGear.wheel_static_friction_factor,
            wheel_side_friction_factor = mainGear.wheel_side_friction_factor,
            wheel_roll_friction_factor = mainGear.wheel_roll_friction_factor,
            wheel_glide_friction_factor = mainGear.wheel_glide_friction_factor,
            wheel_damage_force_factor = mainGear.wheel_damage_force_factor,
            wheel_brake_moment_max = mainGear.wheel_brake_moment_max,

            arg_post = 5,
            arg_amortizer = 6,
            collision_shell_name = "WHEEL_L",
            arg_wheel_damage = 136,
        },

        { -- Right main
            -- pos                                  = SUSP_FINAL_WHEEL_POS[3],
            damage_element = 85,

            mass = mainGear.mass,
            wheel_axle_offset = mainGear.wheel_axle_offset,
            self_attitude = mainGear.self_attitude,
            yaw_limit = mainGear.yaw_limit,

            amortizer_min_length = 0.0,
            amortizer_max_length = mainGear.amortizer_max_length,
            amortizer_basic_length = mainGear.amortizer_basic_length,
            amortizer_reduce_length = mainGear.amortizer_reduce_length,

            amortizer_spring_force_factor = mainGear.amortizer_spring_force_factor,
            amortizer_spring_force_factor_rate = mainGear.amortizer_spring_force_factor_rate,
            amortizer_static_force = mainGear.amortizer_static_force,
            amortizer_direct_damper_force_factor = mainGear.amortizer_direct_damper_force_factor,
            amortizer_back_damper_force_factor = mainGear.amortizer_back_damper_force_factor,
            damper_coeff = mainGear.damper_coeff,

            allowable_hard_contact_length = mainGear.allowable_hard_contact_length,
            anti_skid_installed = mainGear.anti_skid_installed,
            wheel_damage_speed = mainGear.wheel_damage_speed,
            wheel_moment_of_inertia = mainGear.wheel_moment_of_inertia,

            wheel_radius = mainGear.wheel_radius,
            wheel_static_friction_factor = mainGear.wheel_static_friction_factor,
            wheel_side_friction_factor = mainGear.wheel_side_friction_factor,
            wheel_roll_friction_factor = mainGear.wheel_roll_friction_factor,
            wheel_glide_friction_factor = mainGear.wheel_glide_friction_factor,
            wheel_damage_force_factor = mainGear.wheel_damage_force_factor,
            wheel_brake_moment_max = mainGear.wheel_brake_moment_max,

            arg_post = 3,
            arg_amortizer = 4,
            collision_shell_name = "WHEEL_R",
            arg_wheel_damage = 135,
        },
    },

    disable_built_in_oxygen_system = true,
    minor_shake_ampl = 0.21,
    major_shake_ampl = 0.5,
}

fck_susp_log(string.format(
    "FM/config.lua FINAL_SUSP_TABLE session_id=%s SUSP_TEST_MARK=%s SUSP_GEOMETRY_TEST=%s radius_add=%.2f wheel_y_offset=%.2f nodes=%s/%s/%s final_wheel_radius=%.4f/%.4f/%.4f",
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
    FM.suspension[3].wheel_radius
))
