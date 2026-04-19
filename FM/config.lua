-- BEGIN -- this part of the file is not intended for an end-user editing, but forget about that :)
--[[ --------------------------------------------------------------- ]]--

FM = {
    center_of_mass = {-0.6, 0, 0},
    moment_of_inertia = {38912, 254758, 223845, -705},

    -- Keep the fallback path disabled while validating the active collision
    -- shell contract and DCS-provided suspension feedback.
    zeroize_amortizers_before_collision_check = false,

    suspension = {
        { -- Nose
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

            wheel_radius = 0.2286,
            wheel_static_friction_factor = 0.75,
            wheel_roll_friction_factor = 0.08,
            wheel_glide_friction_factor = 0.65,
            wheel_damage_force_factor = 450.0,
            wheel_moment_of_inertia = 0.15,
            wheel_brake_moment_max = 50.0,

            arg_post = 0,
            arg_amortizer = 1,
            arg_wheel_yaw = 2,
            collision_shell_name = "F-CK-1C-F_W",
            arg_wheel_damage = 134,
        },

        { -- Left main
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

            wheel_radius = 0.3048,
            wheel_static_friction_factor = 0.75,
            wheel_side_friction_factor = 1.0,
            wheel_roll_friction_factor = 0.1,
            wheel_glide_friction_factor = 0.65,
            wheel_damage_force_factor = 450.0,
            wheel_brake_moment_max = 80000.0,

            arg_post = 5,
            arg_amortizer = 6,
            arg_wheel_yaw = -1,
            collision_shell_name = "F-CK-1C-LBW",
            arg_wheel_damage = 136,
        },

        { -- Right main
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

            wheel_radius = 0.3048,
            wheel_static_friction_factor = 0.75,
            wheel_side_friction_factor = 1.0,
            wheel_roll_friction_factor = 0.1,
            wheel_glide_friction_factor = 0.65,
            wheel_damage_force_factor = 450.0,
            wheel_brake_moment_max = 80000.0,

            arg_post = 3,
            arg_amortizer = 4,
            arg_wheel_yaw = -1,
            collision_shell_name = "F-CK-1C-RBW",
        },
    },

    disable_built_in_oxygen_system = true,
    minor_shake_ampl = 0.21,
    major_shake_ampl = 0.5,
}
