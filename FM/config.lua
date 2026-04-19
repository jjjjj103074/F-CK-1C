-- BEGIN -- this part of the file is not intended for an end-user editing, but forget about that :) 
--[[ --------------------------------------------------------------- ]]--

-- damage_omega = 30.0, -- (deg?) speed threshold of jamming during impact of rotation limiter
-- state_angle_0 = 6.131341662, -- (deg?) designed angle of retracted gear with horizontal axis of plane
-- state_angle_1 = -2.995164152, -- (deg?) designed angle of released gear with vertical axis of plane
-- mount_pivot_x = -0.274, -- (m) X-coordinate of attachment to fuselage in body-axis system
-- mount_post_radius = 0.657, -- (m) distance from strut-axis to attachment point of piston to gear stand
-- mount_length = 0.604555117, -- (m) What is the difference between this and the post_radius? length of angle brace in retracted configuration
-- mount_angle_1 = -3.138548523, -- (deg?) length of Position (vector) from attachment point
-- post_length = 1.748, -- (m) distance from rotation-axis of strut to wheel-axis
-- wheel_axle_offset = 0.05, -- (m) displacement wheel axis relative to strut
-- self_attitude = false, -- true if gear is self-oriented (Alba or Yak-52 example)

-- amortizer_min_length = 0.0, -- (m) rate of (minimum spring lenght / minimum length of damper)
-- amortizer_max_length = 0.397, -- (m) same as previous but max length
-- amortizer_basic_length = 0.397, -- (m) rate of (spring length in free (without load) condition / damper length in free (without load) condition)
-- amortizer_spring_force_factor = 1.6e+13, -- (???) spring tension factor
-- amortizer_spring_force_factor_rate = 20.0, -- (???)
-- amortizer_static_force = 80000.0, -- (N?) static reaction force of damper
-- amortizer_reduce_length = 0.377, -- (m) total suspension compression distance in non-load condition
-- amortizer_direct_damper_force_factor = 45000.0, -- (???) damper of positive movement
-- amortizer_back_damper_force_factor = 15000.0, -- (???) damper of negative (reversed) movement

-- wheel_radius = 0.308, -- (m) Tire radius
-- wheel_static_friction_factor = 0.65 , -- (unitless) Static friction factor when wheel not moves
-- wheel_roll_friction_factor = 0.025, -- (unitless) Rolling friction factor when wheel not moves
-- wheel_damage_force_factor = 250.0, --wheel cover (tire) strength force (not sure)
-- wheel_brake_moment_max = 15000, -- (N-m) Max braking moment torque 

FM = {
    center_of_mass = {-0.6, 0, 0},
    moment_of_inertia = {38912, 254758, 223845, -705},
    -- Match the working Su-30 EFM setup for collision/suspension testing.
    zeroize_amortizers_before_collision_check = false,

    suspension = {
        { -- Nose
            mass = 100,
            -- Original FM contact point kept for rollback/reference.
            -- pos = {3.85, -1.12, 0.0},
            -- Geometry-aligned test value kept for rollback/reference.
            -- pos = {4.12, -1.912, 0.0},
            -- Match the working Su-30 structure: rely on collision_shell_name instead of explicit pos.
            damage_element = 0,
            self_attitude = true,
            wheel_axle_offset = 0.14,
            yaw_limit = math.rad(60.0),
            -- Original template-only damper coefficient kept for rollback/reference.
            -- damper_coeff = 400.0,
            allowable_hard_contact_length = 0.19,

            -- amortizer_max_length = 0.53,
            -- amortizer_basic_length = 0.53,
            -- amortizer_reduce_length = 0.53,

            -- amortizer_spring_force_factor = 990000.0, -- 彈簧剛度係數
            -- amortizer_spring_force_factor_rate = 1,  -- 彈簧非線性/倍率
            -- amortizer_static_force = 47500.0, -- 靜態支撐 / 預載力（影響落地時的靜態下沉）
            -- amortizer_direct_damper_force_factor = 50000,  -- 壓縮阻尼（compression），增大會抑制下壓震盪
            -- amortizer_back_damper_force_factor = 60000,  -- 回彈阻尼（rebound），增大會抑制回彈震動

            amortizer_max_length = 0.53,
            amortizer_basic_length = 0.53,
            amortizer_reduce_length = 0.53,

            amortizer_spring_force_factor = 450000.0,
            amortizer_spring_force_factor_rate = 1,
            amortizer_static_force = 47500.0,
            -- Adjust nose: soften spring and lower damping/rebound to reduce
            -- the initial harsh bounce while keeping convergence.
            amortizer_direct_damper_force_factor = 80000,
            amortizer_back_damper_force_factor = 50000,

            anti_skid_installed = true,

            -- Original FM wheel radius kept for rollback/reference.
            -- wheel_radius = 0.64,
            -- Match aircraft Lua gear geometry: nose_gear_wheel_diameter / 2
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
            -- Test: explicit contact position (from EDM nearest triplet)
            pos = { 4.12, -1.912, 0.0 },
            -- Use the actual collision shell segment names present in
            -- Shapes/F-CK-1C-box_new.edm (lineFG/lineLG/lineRG).
            -- Previously referenced WHEEL_F which does not exist in the
            -- active collision shell and causes fallback/single-point contact.
            -- collision_shell_name = "lineFG",
            arg_wheel_damage = 134,
        },

        { -- Left main
            mass = 200,
            -- Original FM contact point kept for rollback/reference.
            -- pos = {-1.51, -1.09, -1.35},
            -- Geometry-aligned test value kept for rollback/reference.
            -- pos = {-1.185, -1.913, -0.7905},
            -- Match the working Su-30 structure: rely on collision_shell_name instead of explicit pos.
            damage_element = 3,
            -- Original F-CK-1C offset kept for rollback/reference.
            -- wheel_axle_offset = 0.38,
            wheel_axle_offset = 0.0,
            self_attitude = false,
            yaw_limit = math.rad(0.0),
            -- Original template-only damper coefficient kept for rollback/reference.
            -- damper_coeff = 160.0,

            amortizer_max_length = 0.45,
            amortizer_basic_length = 0.45,
            amortizer_reduce_length = 0.63,

            amortizer_spring_force_factor = 480000.0,
            amortizer_spring_force_factor_rate = 3,
            -- Moderately reduce left-main spring and static preload; lower
            -- damping to soften initial contact while keeping damping for convergence.
            amortizer_static_force = 220000.0,
            amortizer_direct_damper_force_factor = 90000,
            amortizer_back_damper_force_factor = 70000,

            allowable_hard_contact_length = 0.25,
            anti_skid_installed = true,
            wheel_damage_speed = 180,
            wheel_moment_of_inertia = 0.6,

            -- Original FM wheel radius kept for rollback/reference.
            -- wheel_radius = 0.60,
            -- Match aircraft Lua gear geometry: main_gear_wheel_diameter / 2
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
            -- Map to EDM collision line for left main gear
            -- Use explicit contact pos derived from EDM triplet for testing
            pos = { -1.185, -1.913, -0.7905 },
            -- collision_shell_name = "lineLG",
            arg_wheel_damage = 136,
        },

        { -- Right main
            mass = 200,
            -- Original FM contact point kept for rollback/reference.
            -- pos = {-1.51, -1.09, 1.3},
            -- Geometry-aligned test value kept for rollback/reference.
            -- pos = {-1.185, -1.913, 0.7905},
            -- Match the working Su-30 structure: rely on collision_shell_name instead of explicit pos.
            damage_element = 5,
            -- Original F-CK-1C offset kept for rollback/reference.
            -- wheel_axle_offset = 0.38,
            wheel_axle_offset = 0.0,
            self_attitude = false,
            yaw_limit = math.rad(0.0),
            -- Original template-only damper coefficient kept for rollback/reference.
            -- damper_coeff = 160.0,

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

            -- Original FM wheel radius kept for rollback/reference.
            -- wheel_radius = 0.60,
            -- Match aircraft Lua gear geometry: main_gear_wheel_diameter / 2
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
            -- Map to EDM collision line for right main gear
            -- Use explicit contact pos derived from EDM triplet for testing
            pos = { -1.185, -1.913, 0.7905 },
            -- collision_shell_name = "lineRG",
        },
    },

    disable_built_in_oxygen_system = true,
    minor_shake_ampl = 0.21,
    major_shake_ampl = 0.5,
}
