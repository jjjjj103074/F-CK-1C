#include "TestHarness.h"

#include "Core/Fck1cEfm.h"
#include "DcsBridge/DcsSnapshots.h"

#include <stdexcept>

namespace
{
constexpr double kTolerance = 1e-9;

Data::AircraftConfig make_test_config()
{
	Data::AircraftConfig config;
	config.aerodynamics.wing_area = 24.26;
	config.aerodynamics.wingspan = 8.53;
	config.aerodynamics.length = 14.48;
	config.aerodynamics.height = 4.7;
	config.aerodynamics.mach_max = 1.5;
	config.aerodynamics.mach_table = { 0.0, 1.0 };
	config.aerodynamics.cx_zero_table = { 0.025, 0.030 };
	config.aerodynamics.cy_alpha_table = { 0.05, 0.04 };
	config.aerodynamics.roll_rate_max_table = { 3.0, 2.0 };
	config.aerodynamics.alpha_max_table = { 20.0, 18.0 };
	config.aerodynamics.cy_max_table = { 1.2, 1.0 };
	config.engine.start_time = 5.0;
	config.engine.spool_up_tau = 1.0;
	config.engine.spool_down_tau = 1.0;
	config.engine.mach_table = { 0.0, 1.0 };
	config.engine.max_thrust_table = { 54000.0, 50000.0 };
	config.engine.throttle_input_table = { 0.0, 1.0 };
	config.engine.power_table = { 0.1, 1.0 };
	config.left_engine_position = Common::Vec3(-3.793, -0.391, -0.716);
	config.right_engine_position = Common::Vec3(-3.793, -0.391, 0.716);
	return config;
}

void send_command(
	Core::Fck1cEfm& efm,
	const Core::EfmCommand& command)
{
	efm.handle_command(command);
}

Core::FrameDataAvailability all_frame_data_available()
{
	return { true, true, true, true, true, { true, true, true } };
}

Core::FrameInput make_frame_input()
{
	Core::FrameInput input;
	input.dt_s = 0.02;
	input.availability = all_frame_data_available();
	input.atmosphere = {
		1200.0, 281.0, 330.0, 1.1, 88000.0, Common::Vec3(5.0, 1.0, -2.0)
	};
	input.surface = { 200.0, 203.0, 4, Common::Vec3(0.0, 1.0, 0.0) };
	input.mass = {
		9400.0, Common::Vec3(0.2, -0.1, 0.3), Common::Vec3(11.0, 12.0, 13.0)
	};
	input.world_kinematics = {
		Common::Vec3(0.1, 0.2, 0.3), Common::Vec3(150.0, 4.0, 2.0),
		Common::Vec3(10.0, 20.0, 1200.0), Common::Vec3(0.01, 0.02, 0.03),
		Common::Vec3(0.1, 0.2, 0.3), { 0.0, 0.0, 0.0, 1.0 }
	};
	input.body_kinematics = {
		Common::Vec3(0.0, 9.81, 0.0), Common::Vec3(140.0, 3.0, 1.0),
		Common::Vec3(4.0, 0.5, -1.0), Common::Vec3(0.02, 0.03, 0.04),
		Common::Vec3(0.05, 0.06, 0.07), 0.3, 0.1, -0.2, 0.15, -0.04
	};
	input.suspension = {
		Core::SuspensionFeedbackInput{ 0, Common::Vec3(3.0, 4.0, 0.0), Common::Vec3(1.0, 2.0, 3.0), 0.9, 0.10, 12.0 },
		Core::SuspensionFeedbackInput{ 1, Common::Vec3(0.0, 80.0, 0.0), Common::Vec3(4.0, 5.0, 6.0), 0.8, 0.20, 13.0 },
		Core::SuspensionFeedbackInput{ 2, Common::Vec3(0.0, 90.0, 0.0), Common::Vec3(7.0, 8.0, 9.0), 0.7, 0.30, 14.0 }
	};
	input.autopilot = { true, false, true, 0.2, -0.3, 0.4 };
	input.max_power = { 1.0, 1.0 };
	return input;
}

void apply_legacy_frame_input(
	Core::Fck1cEfm& efm,
	const Core::FrameInput& input)
{
	efm.set_atmosphere(input.atmosphere);
	efm.set_surface(input.surface);
	efm.set_mass_state(input.mass);
	efm.set_world_kinematics(input.world_kinematics);
	efm.set_body_kinematics(input.body_kinematics);
	for (const Core::SuspensionFeedbackInput& wheel : input.suspension)
	{
		efm.update_suspension_feedback(wheel);
	}
	efm.simulate(input.dt_s, input.autopilot, input.max_power);
}

void expect_vec3(
	Tests::Context& context,
	const Common::Vec3& actual,
	const Common::Vec3& expected)
{
	TEST_EXPECT_NEAR(context, actual.x, expected.x, kTolerance);
	TEST_EXPECT_NEAR(context, actual.y, expected.y, kTolerance);
	TEST_EXPECT_NEAR(context, actual.z, expected.z, kTolerance);
}

void expect_engine_output(
	Tests::Context& context,
	const Core::EngineOutput& actual,
	const Systems::EngineChannelState& expected)
{
	TEST_EXPECT(context, actual.switch_on == expected.switch_on);
	TEST_EXPECT_NEAR(context, actual.throttle_input, expected.throttle_input, kTolerance);
	TEST_EXPECT_NEAR(context, actual.throttle_output, expected.throttle_output, kTolerance);
	TEST_EXPECT_NEAR(context, actual.power_readout, expected.power_readout, kTolerance);
	TEST_EXPECT_NEAR(context, actual.thrust_force, expected.thrust_force, kTolerance);
	TEST_EXPECT_NEAR(context, actual.afterburner_ratio, expected.afterburner_ratio, kTolerance);
	TEST_EXPECT(context, actual.afterburner_lit == expected.afterburner_lit);
	TEST_EXPECT_NEAR(context, actual.nozzle_aperture, expected.nozzle_aperture, kTolerance);
}

void expect_flight_output(
	Tests::Context& context,
	const Core::FlightOutput& actual,
	const Core::AircraftState& expected)
{
	TEST_EXPECT_NEAR(context, actual.altitude_asl_m, expected.altitude_asl, kTolerance);
	TEST_EXPECT_NEAR(context, actual.altitude_agl_m, expected.altitude_agl, kTolerance);
	TEST_EXPECT_NEAR(context, actual.position_world_z_m, expected.position_world_z, kTolerance);
	TEST_EXPECT_NEAR(context, actual.mach, expected.mach, kTolerance);
	TEST_EXPECT_NEAR(context, actual.g_load, expected.g, kTolerance);
	TEST_EXPECT_NEAR(context, actual.angle_of_attack_deg, expected.alpha, kTolerance);
	TEST_EXPECT_NEAR(context, actual.angle_of_slide_deg, expected.beta, kTolerance);
	TEST_EXPECT_NEAR(context, actual.atmosphere_temperature_k, expected.atmosphere_temperature, kTolerance);
}

void expect_controls_output(
	Tests::Context& context,
	const Core::ControlOutput& actual,
	const Core::Fck1cEfmSnapshot& expected)
{
	TEST_EXPECT_NEAR(context, actual.pitch_input, expected.systems.primary_controls.pitch.input, kTolerance);
	TEST_EXPECT_NEAR(context, actual.roll_input, expected.systems.primary_controls.roll.input, kTolerance);
	TEST_EXPECT_NEAR(context, actual.yaw_input, expected.systems.primary_controls.yaw.input, kTolerance);
	TEST_EXPECT_NEAR(context, actual.elevator_command, expected.control_surfaces.elevator_command, kTolerance);
	TEST_EXPECT_NEAR(context, actual.aileron_command, expected.control_surfaces.aileron_command, kTolerance);
	TEST_EXPECT_NEAR(context, actual.rudder_command, expected.control_surfaces.rudder_command, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flaps_position, expected.systems.airframe_devices.flaps_pos, kTolerance);
	TEST_EXPECT_NEAR(context, actual.slats_position, expected.systems.airframe_devices.slats_pos, kTolerance);
	TEST_EXPECT_NEAR(context, actual.airbrake_position, expected.systems.airframe_devices.airbrake_pos, kTolerance);
}

void expect_landing_gear_output(
	Tests::Context& context,
	const Core::LandingGearOutput& actual,
	const Systems::LandingGearSystemState& expected)
{
	TEST_EXPECT_NEAR(context, actual.gear_position, expected.position, kTolerance);
	TEST_EXPECT_NEAR(context, actual.nose_wheel_steering, expected.wheels.nose_steering, kTolerance);
	TEST_EXPECT_NEAR(context, actual.brake_left, expected.wheels.brake_left, kTolerance);
	TEST_EXPECT_NEAR(context, actual.brake_right, expected.wheels.brake_right, kTolerance);
	for (std::size_t index = 0; index < actual.wheel_spin.size(); ++index)
	{
		TEST_EXPECT_NEAR(context, actual.wheel_spin[index], expected.wheels.spin[index], kTolerance);
	}
}

void expect_suspension_output(
	Tests::Context& context,
	const Core::SuspensionOutput& actual,
	const Systems::SuspensionSystemState& expected)
{
	for (std::size_t index = 0; index < actual.wheels.size(); ++index)
	{
		expect_vec3(context, actual.wheels[index].acting_force, expected.force_vec[index]);
		TEST_EXPECT_NEAR(context, actual.wheels[index].compression, expected.compression[index], kTolerance);
		TEST_EXPECT_NEAR(context, actual.wheels[index].force_magnitude, expected.force_mag[index], kTolerance);
		TEST_EXPECT(context, actual.wheels[index].weight_on_wheel == expected.wow[index]);
	}
	TEST_EXPECT(context, actual.any_weight_on_wheels == Systems::any_wow(expected));
	TEST_EXPECT(context, actual.on_ground == expected.on_ground);
}

void expect_frame_matches_snapshot(
	Tests::Context& context,
	const Core::FrameOutput& actual,
	const Core::Fck1cEfmSnapshot& expected)
{
	TEST_EXPECT_NEAR(context, actual.simulation_time_s, expected.systems.startup.simulation_time, kTolerance);
	expect_flight_output(context, actual.flight, expected.aircraft);
	expect_vec3(context, actual.force_moment.force, expected.force_moment.force);
	expect_vec3(context, actual.force_moment.moment, expected.force_moment.moment);
	expect_vec3(context, actual.force_moment.center_of_mass, expected.force_moment.center_of_mass);
	expect_engine_output(context, actual.engines[0], expected.systems.engines.left);
	expect_engine_output(context, actual.engines[1], expected.systems.engines.right);
	expect_controls_output(context, actual.controls, expected);
	expect_landing_gear_output(context, actual.landing_gear, expected.systems.landing_gear);
	expect_suspension_output(context, actual.suspension, expected.systems.suspension);
	TEST_EXPECT_NEAR(context, actual.fuel.internal_fuel, expected.systems.fuel.internal_fuel, kTolerance);
	TEST_EXPECT_NEAR(context, actual.fuel.external_fuel, expected.systems.fuel.external_fuel, kTolerance);
	TEST_EXPECT_NEAR(context, actual.fuel.total_fuel, expected.systems.fuel.total_fuel, kTolerance);
	TEST_EXPECT_NEAR(context, actual.shake_amplitude, expected.gameplay.shake_amplitude, kTolerance);
}

void expect_availability(
	Tests::Context& context,
	const Core::FrameDataAvailability& actual,
	const Core::FrameDataAvailability& expected)
{
	TEST_EXPECT(context, actual.atmosphere == expected.atmosphere);
	TEST_EXPECT(context, actual.surface == expected.surface);
	TEST_EXPECT(context, actual.mass == expected.mass);
	TEST_EXPECT(context, actual.world_kinematics == expected.world_kinematics);
	TEST_EXPECT(context, actual.body_kinematics == expected.body_kinematics);
	for (std::size_t index = 0; index < actual.suspension.size(); ++index)
	{
		TEST_EXPECT(context, actual.suspension[index] == expected.suspension[index]);
	}
}

void expect_existing_draw_output(
	Tests::Context& context,
	const Core::FrameOutput& actual,
	const Core::Fck1cEfmSnapshot& snapshot)
{
	const DcsBridge::DrawArgState expected = DcsBridge::make_draw_arg_state(snapshot);
	TEST_EXPECT_NEAR(context, actual.landing_gear.gear_position, expected.gear_pos, kTolerance);
	TEST_EXPECT_NEAR(context, actual.landing_gear.nose_wheel_steering, expected.nose_wheel_steering, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.elevator_command, expected.elevator_command, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.flaps_position, expected.flaps_pos, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.aileron_command, expected.aileron_command, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.rudder_command, expected.rudder_command, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.airbrake_position, expected.airbrake_pos, kTolerance);
	TEST_EXPECT_NEAR(context, actual.engines[0].afterburner_ratio, expected.left_afterburner_ratio, kTolerance);
	TEST_EXPECT_NEAR(context, actual.engines[1].afterburner_ratio, expected.right_afterburner_ratio, kTolerance);
	TEST_EXPECT_NEAR(context, actual.engines[0].nozzle_aperture, expected.left_nozzle_aperture, kTolerance);
	TEST_EXPECT_NEAR(context, actual.engines[1].nozzle_aperture, expected.right_nozzle_aperture, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.slats_position, expected.slats_pos, kTolerance);
	for (std::size_t index = 0; index < actual.landing_gear.wheel_spin.size(); ++index)
	{
		TEST_EXPECT_NEAR(context, actual.landing_gear.wheel_spin[index], expected.wheel_spin[index], kTolerance);
	}
}

void expect_existing_param_output(
	Tests::Context& context,
	const Core::FrameOutput& actual,
	const Core::Fck1cEfmSnapshot& snapshot)
{
	const DcsBridge::ParamExportState expected = DcsBridge::make_param_export_state(snapshot);
	TEST_EXPECT(context, actual.suspension.any_weight_on_wheels == expected.any_weight_on_wheels);
	TEST_EXPECT_NEAR(context, actual.landing_gear.brake_left, expected.wheel_brake_left, kTolerance);
	TEST_EXPECT_NEAR(context, actual.landing_gear.brake_right, expected.wheel_brake_right, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.pitch_input, expected.pitch_input, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.roll_input, expected.roll_input, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.yaw_input, expected.yaw_input, kTolerance);
	TEST_EXPECT(context, actual.engines[0].switch_on == expected.left_engine_switch);
	TEST_EXPECT(context, actual.engines[1].switch_on == expected.right_engine_switch);
	TEST_EXPECT_NEAR(context, actual.engines[0].throttle_input, expected.left_throttle_input, kTolerance);
	TEST_EXPECT_NEAR(context, actual.engines[1].throttle_input, expected.right_throttle_input, kTolerance);
	TEST_EXPECT_NEAR(context, actual.engines[0].power_readout, expected.left_engine_power_readout, kTolerance);
	TEST_EXPECT_NEAR(context, actual.engines[1].power_readout, expected.right_engine_power_readout, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flight.atmosphere_temperature_k, expected.atmosphere_temperature, kTolerance);
	TEST_EXPECT_NEAR(context, actual.fuel.internal_fuel, expected.internal_fuel, kTolerance);
	TEST_EXPECT_NEAR(context, actual.fuel.total_fuel, expected.total_fuel, kTolerance);
}

void start_legacy(Core::Fck1cEfm& efm, Core::StartMode mode)
{
	switch (mode)
	{
	case Core::StartMode::ColdGround:
		efm.cold_start();
		return;
	case Core::StartMode::HotGround:
		efm.hot_ground_start();
		return;
	case Core::StartMode::HotAir:
		efm.hot_air_start();
		return;
	}
	throw std::invalid_argument("Unknown Core::StartMode.");
}

void expect_start_specific_output(
	Tests::Context& context,
	const Core::FrameOutput& output,
	Core::StartMode mode)
{
	const bool hot = mode != Core::StartMode::ColdGround;
	const bool airborne = mode == Core::StartMode::HotAir;
	TEST_EXPECT_NEAR(context, output.simulation_time_s, 0.0, kTolerance);
	TEST_EXPECT(context, output.engines[0].switch_on == hot);
	TEST_EXPECT(context, output.engines[1].switch_on == hot);
	TEST_EXPECT_NEAR(context, output.landing_gear.gear_position, airborne ? 0.0 : 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[0].throttle_input, airborne ? 0.5 : 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[1].throttle_input, airborne ? 0.5 : 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[0].throttle_output, hot ? 0.5 : 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[1].throttle_output, hot ? 0.5 : 0.0, kTolerance);
}

void test_complete_frame_input_contract(Tests::Context& context)
{
	const Core::FrameInput input = make_frame_input();
	TEST_EXPECT_NEAR(context, input.dt_s, 0.02, kTolerance);
	TEST_EXPECT_NEAR(context, input.atmosphere.pressure, 88000.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.surface.normal.y, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.mass.moment_of_inertia.z, 13.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.world_kinematics.acceleration.x, 0.1, kTolerance);
	TEST_EXPECT_NEAR(context, input.world_kinematics.position.z, 1200.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.world_kinematics.angular_acceleration.z, 0.03, kTolerance);
	TEST_EXPECT_NEAR(context, input.world_kinematics.orientation.w, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.body_kinematics.wind_velocity.x, 4.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.body_kinematics.angular_acceleration.y, 0.03, kTolerance);
	TEST_EXPECT_NEAR(context, input.suspension[2].acting_force_point.z, 9.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.suspension[2].integrity_factor, 0.7, kTolerance);
	TEST_EXPECT_NEAR(context, input.suspension[2].wheel_speed_x, 14.0, kTolerance);
	TEST_EXPECT(context, input.autopilot.master);
	TEST_EXPECT_NEAR(context, input.max_power.value, 1.0, kTolerance);
}

void test_all_start_mode_frame_outputs(Tests::Context& context)
{
	const Core::StartMode modes[] = {
		Core::StartMode::ColdGround,
		Core::StartMode::HotGround,
		Core::StartMode::HotAir
	};
	for (Core::StartMode mode : modes)
	{
		Core::Fck1cEfm efm(make_test_config());
		start_legacy(efm, mode);
		const Core::FrameDataAvailability unavailable;
		const Core::FrameOutput output = efm.frame_output(unavailable);
		expect_availability(context, output.availability, unavailable);
		expect_frame_matches_snapshot(context, output, efm.snapshot());
		expect_start_specific_output(context, output, mode);
	}
}

void test_frame_output_matches_existing_outputs(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	efm.hot_ground_start();
	efm.set_internal_fuel(500.0);
	efm.set_external_fuel({ 1, 120.0, Common::Vec3(0.5, -0.2, 0.1) });
	send_command(efm, { Core::CommandGroup::LandingGear, Core::CommandAction::SetLeftBrake, 0.4 });
	send_command(efm, { Core::CommandGroup::LandingGear, Core::CommandAction::SetRightBrake, 0.6 });
	const Core::FrameInput input = make_frame_input();
	apply_legacy_frame_input(efm, input);
	const Core::FrameOutput output = efm.frame_output(input.availability);
	const Core::Fck1cEfmSnapshot snapshot = efm.snapshot();
	expect_availability(context, output.availability, input.availability);
	expect_frame_matches_snapshot(context, output, snapshot);
	expect_existing_draw_output(context, output, snapshot);
	expect_existing_param_output(context, output, snapshot);
	TEST_EXPECT_NEAR(context, output.simulation_time_s, input.dt_s, kTolerance);
}

void test_config_ownership(Tests::Context& context)
{
	Data::AircraftConfig source = make_test_config();
	Core::Fck1cEfm efm(source);
	source.aerodynamics.mach_table[0] = 99.0;
	source.engine.max_thrust_table[0] = 0.0;
	TEST_EXPECT_NEAR(context, efm.config().aerodynamics.wing_area, 24.26, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().aerodynamics.mach_table[0], 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().engine.max_thrust_table[0], 54000.0, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().left_engine_position.z, -0.716, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().right_engine_position.z, 0.716, kTolerance);
}

void test_invalid_config_rejected(Tests::Context& context)
{
	bool rejected = false;
	try
	{
		Core::Fck1cEfm efm(Data::AircraftConfig{});
	}
	catch (const std::invalid_argument&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
}

void test_snapshot_isolation(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	efm.set_mass_state({ 9100.0, Common::Vec3(1.0, 2.0, 3.0) });
	efm.set_internal_fuel(1200.0);
	efm.set_easy_flight(true);
	Core::Fck1cEfmSnapshot copy = efm.snapshot();
	copy.aircraft.current_mass = 1.0;
	copy.systems.fuel.internal_fuel = 2.0;
	copy.gameplay.easy_flight = false;
	const Core::Fck1cEfmSnapshot current = efm.snapshot();
	TEST_EXPECT_NEAR(context, current.aircraft.current_mass, 9100.0, kTolerance);
	TEST_EXPECT_NEAR(context, current.systems.fuel.internal_fuel, 1200.0, kTolerance);
	TEST_EXPECT(context, current.gameplay.easy_flight);
}

void test_simulation_pipeline(Tests::Context& context)
{
	Core::AutopilotCommand autopilot;
	autopilot.master = true;
	autopilot.pitch_command = 0.2;
	autopilot.roll_command = -0.3;
	autopilot.auto_throttle_engaged = true;
	autopilot.throttle_command = 0.4;
	const Core::MaxPowerCommand max_power;
	Core::Fck1cEfm efm(make_test_config());
	efm.set_internal_fuel(100.0);
	efm.simulate(0.01, autopilot, max_power);
	Core::Fck1cEfmSnapshot state = efm.snapshot();
	TEST_EXPECT(context, state.systems.startup.first_frame_completed);
	TEST_EXPECT_NEAR(context, state.systems.startup.simulation_time, 0.01, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.primary_controls.pitch.input, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.primary_controls.roll.input, -0.3, kTolerance);
	efm.simulate(0.01, autopilot, max_power);
	state = efm.snapshot();
	TEST_EXPECT_NEAR(context, state.systems.startup.simulation_time, 0.02, kTolerance);
}

void test_cockpit_inputs_drive_core_outputs(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	efm.hot_ground_start();
	efm.set_internal_fuel(100.0);
	const Core::AutopilotCommand autopilot = {
		true, false, true, 0.25, -0.35, 0.6
	};
	const Core::MaxPowerCommand normal_power = { 1.0, 1.0 };
	efm.simulate(0.01, autopilot, normal_power);
	Core::Fck1cEfmSnapshot state = efm.snapshot();
	TEST_EXPECT_NEAR(context, state.systems.primary_controls.pitch.input, 0.25, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.primary_controls.roll.input, -0.35, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.engines.left.throttle_input, 0.6, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.engines.right.throttle_input, 0.6, kTolerance);
	TEST_EXPECT(context, state.systems.engines.left.power_readout > 0.0);
	TEST_EXPECT(context, state.systems.engines.right.power_readout > 0.0);
	TEST_EXPECT(context, state.systems.engines.left.thrust_force > 0.0);
	TEST_EXPECT(context, state.systems.engines.right.thrust_force > 0.0);

	const Core::MaxPowerCommand cut_power = { 1.0, 0.0 };
	efm.simulate(0.01, autopilot, cut_power);
	state = efm.snapshot();
	TEST_EXPECT_NEAR(context, state.systems.engines.left.thrust_force, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.engines.right.thrust_force, 0.0, kTolerance);
}

void test_neutral_cockpit_inputs_complete_step(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	efm.hot_ground_start();
	efm.set_internal_fuel(100.0);
	send_command(efm,
		{ Core::CommandGroup::PitchRoll, Core::CommandAction::SetPitchAxis, 0.3 });
	efm.simulate(0.01, {}, {});
	const Core::Fck1cEfmSnapshot state = efm.snapshot();
	TEST_EXPECT_NEAR(context, state.systems.startup.simulation_time, 0.01, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.primary_controls.pitch.input, 0.3, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.fbw.throttle_blend, 0.0, kTolerance);
	TEST_EXPECT(context, state.systems.engines.left.thrust_force > 0.0);
	TEST_EXPECT(context, state.systems.engines.right.thrust_force > 0.0);
}

void test_ground_start_lifecycle(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	efm.apply_damage({ Core::DamageArea::LeftWing, 0, 0.2 });
	efm.cold_start();
	Core::Fck1cEfmSnapshot state = efm.snapshot();
	TEST_EXPECT(context, state.systems.startup.mode == Systems::STARTUP_MODE_COLD_GROUND);
	TEST_EXPECT_NEAR(context, state.systems.damage.left_wing_integrity, 1.0, kTolerance);
	TEST_EXPECT(context, state.systems.landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, state.systems.landing_gear.position, 1.0, kTolerance);
	TEST_EXPECT(context, !state.systems.engines.left.switch_on);
	efm.hot_ground_start();
	state = efm.snapshot();
	TEST_EXPECT(context, state.systems.startup.mode == Systems::STARTUP_MODE_HOT_GROUND);
	TEST_EXPECT(context, state.systems.airframe_devices.flap_mode == Systems::FLAP_MODE_DOWN);
	TEST_EXPECT_NEAR(context, state.systems.airframe_devices.flaps_pos, 1.0, kTolerance);
	TEST_EXPECT(context, state.systems.engines.left.switch_on);
	TEST_EXPECT_NEAR(context, state.systems.engines.left.throttle_output, 0.5, kTolerance);
}

void test_air_start_lifecycle(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	efm.hot_air_start();
	const Core::Fck1cEfmSystems systems = efm.snapshot().systems;
	TEST_EXPECT(context, systems.startup.mode == Systems::STARTUP_MODE_HOT_AIR);
	TEST_EXPECT(context, !systems.landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, systems.landing_gear.position, 0.0, kTolerance);
	TEST_EXPECT(context, !systems.landing_gear.wheels.nose_turn_enabled);
	TEST_EXPECT_NEAR(context, systems.throttle_inputs.left.pilot_cmd, 0.5, kTolerance);
	TEST_EXPECT(context, systems.engines.left.switch_on);
	TEST_EXPECT_NEAR(context, systems.engines.left.throttle_input, 0.5, kTolerance);
}

void test_release_lifecycle(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	efm.hot_ground_start();
	efm.simulate(0.01, {}, {});
	efm.apply_damage({ Core::DamageArea::LeftEngine, 0, 0.2 });
	send_command(efm,
		{ Core::CommandGroup::PitchRoll, Core::CommandAction::SetPitchAxis, 0.5 });
	send_command(efm,
		{ Core::CommandGroup::LandingGear, Core::CommandAction::SetLeftBrake, 1.0 });
	efm.release();
	const Core::Fck1cEfmSnapshot state = efm.snapshot();
	TEST_EXPECT(context, state.systems.startup.mode == Systems::STARTUP_MODE_RELEASED);
	TEST_EXPECT_NEAR(context, state.systems.startup.simulation_time, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.primary_controls.pitch.input, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.control_surfaces.elevator_command, 0.0, kTolerance);
	TEST_EXPECT(context, !state.systems.landing_gear.wheels.nose_turn_enabled);
	TEST_EXPECT_NEAR(context, state.systems.landing_gear.wheels.brake_left, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.engines.left.nozzle_aperture, 0.8, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.damage.left_engine_integrity, 1.0, kTolerance);
}
}

void run_fck1c_efm_tests(Tests::Context& context)
{
	test_complete_frame_input_contract(context);
	test_all_start_mode_frame_outputs(context);
	test_frame_output_matches_existing_outputs(context);
	test_config_ownership(context);
	test_invalid_config_rejected(context);
	test_snapshot_isolation(context);
	test_simulation_pipeline(context);
	test_cockpit_inputs_drive_core_outputs(context);
	test_neutral_cockpit_inputs_complete_step(context);
	test_ground_start_lifecycle(context);
	test_air_start_lifecycle(context);
	test_release_lifecycle(context);
}
