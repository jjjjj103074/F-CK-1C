#include "Fck1cEfm.h"

#include "ForceMoment.h"
#include "../Systems/FBWLifecycle.h"

namespace
{
constexpr double kColdStartThrottle = 0.0;
constexpr double kHotAirStartThrottle = 0.5;
}

namespace Core
{
Fck1cEfm::Fck1cEfm(const Fck1cEfmConfig& config, Fck1cEfmRuntime& runtime)
	: config_(config),
	runtime_(runtime)
{
}

const Fck1cEfmConfig& Fck1cEfm::config() const
{
	return config_;
}

AircraftState& Fck1cEfm::aircraft_state()
{
	return aircraft_state_;
}

const AircraftState& Fck1cEfm::aircraft_state() const
{
	return aircraft_state_;
}

ForceMomentFrame& Fck1cEfm::force_moment()
{
	return force_moment_;
}

const ForceMomentFrame& Fck1cEfm::force_moment() const
{
	return force_moment_;
}

ControlSurfaceState& Fck1cEfm::control_surfaces()
{
	return control_surfaces_;
}

const ControlSurfaceState& Fck1cEfm::control_surfaces() const
{
	return control_surfaces_;
}

GameplayState& Fck1cEfm::gameplay()
{
	return gameplay_;
}

const GameplayState& Fck1cEfm::gameplay() const
{
	return gameplay_;
}

Fck1cEfmSystems& Fck1cEfm::systems()
{
	return systems_;
}

const Fck1cEfmSystems& Fck1cEfm::systems() const
{
	return systems_;
}

void Fck1cEfm::simulate(double dt)
{
	begin_frame(dt);
	update_airframe(dt);
	Systems::update_primary_control_inputs(systems_.primary_controls);
	update_autopilot();
	update_fbw(dt);
	const Systems::AerodynamicsFrameInput aerodynamics_input = make_aerodynamics_input();
	update_primary_aerodynamics(aerodynamics_input);
	update_engines_and_fuel(dt);
	update_ground_and_suspension(dt, aerodynamics_input);
	finish_frame();
}

void Fck1cEfm::begin_frame(double dt)
{
	Systems::advance_simulation_time(systems_.startup, dt);
	reset_force_moment(force_moment_.force, force_moment_.moment);
	if (systems_.startup.first_frame_completed)
	{
		return;
	}

	Systems::initialize_aerodynamic_force_positions(
		systems_.aerodynamics,
		config_.aerodynamics,
		force_moment_.center_of_mass);
	runtime_.on_first_frame(*this);
}

void Fck1cEfm::update_airframe(double dt)
{
	Systems::update_gear_position(systems_.landing_gear);
	const Systems::AirframeDeviceUpdateInput device_input = {
		aircraft_state_.speed_scalar,
		systems_.landing_gear.position
	};
	Systems::update_airframe_device_positions(
		systems_.airframe_devices,
		device_input);
	Systems::update_nose_wheel_steering(
		systems_.landing_gear.wheels,
		nose_wheel_steering());
	update_airspeed(aircraft_state_);
	const Systems::WheelSpinInput wheel_input = {
		ground_speed(aircraft_state_),
		dt,
		aircraft_state_.altitude_agl,
		{
			config_.suspension.fallback_wheel_radius[0],
			config_.suspension.fallback_wheel_radius[1],
			config_.suspension.fallback_wheel_radius[2]
		}
	};
	Systems::update_wheel_spin(
		systems_.landing_gear.wheels,
		systems_.landing_gear.position,
		wheel_input);
	Systems::update_aerodynamic_conditions(
		systems_.aerodynamics,
		config_.aerodynamics,
		force_moment_.center_of_mass,
		aircraft_state_.atmosphere_density,
		aircraft_state_.speed_scalar,
		aircraft_state_.mach,
		aircraft_state_.alpha,
		aircraft_state_.beta,
		systems_.airframe_devices.slats_pos);
}

void Fck1cEfm::update_autopilot()
{
	const AutopilotCommand command = runtime_.read_autopilot();
	if (command.master && !command.bypass)
	{
		systems_.primary_controls.pitch.input = command.pitch_command;
		systems_.primary_controls.roll.input = command.roll_command;
	}

	if (command.auto_throttle_engaged)
	{
		systems_.fbw.throttle_cmd_left = command.throttle_command;
		systems_.fbw.throttle_cmd_right = command.throttle_command;
		systems_.fbw.throttle_blend = 1.0;
		systems_.fbw.throttle_override = false;
		return;
	}

	systems_.fbw.throttle_blend = 0.0;
}

void Fck1cEfm::update_fbw(double dt)
{
	const Systems::FBWControllerOutput output = Systems::update_fbw_controller(
		systems_.fbw,
		config_.fbw,
		make_fbw_input(dt));
	control_surfaces_.elevator_command = output.elevator_command;
	control_surfaces_.aileron_command = output.aileron_command;
	control_surfaces_.rudder_command = output.rudder_command;
}

Systems::FBWControllerInput Fck1cEfm::make_fbw_input(double dt) const
{
	Systems::FBWControllerInput input;
	input.dt = dt;
	input.qbar = systems_.aerodynamics.dynamic_pressure;
	input.alpha_limit_deg = systems_.aerodynamics.alpha_max_deg;
	input.roll = aircraft_state_.roll;
	input.pitch = aircraft_state_.pitch;
	input.roll_rate = aircraft_state_.roll_rate;
	input.pitch_rate = aircraft_state_.pitch_rate;
	input.yaw_rate = aircraft_state_.yaw_rate;
	input.alpha = aircraft_state_.alpha;
	input.beta = aircraft_state_.beta;
	input.speed_scalar = aircraft_state_.speed_scalar;
	input.mach = aircraft_state_.mach;
	input.g = aircraft_state_.g;
	input.roll_input = systems_.primary_controls.roll.input;
	input.roll_trim = systems_.primary_controls.roll.trim;
	input.pitch_input = systems_.primary_controls.pitch.input;
	input.pitch_trim = systems_.primary_controls.pitch.trim;
	input.yaw_input = systems_.primary_controls.yaw.input;
	input.yaw_trim = systems_.primary_controls.yaw.trim;
	input.gear_pos = systems_.landing_gear.position;
	input.wow = Systems::has_suspension_feedback(systems_.suspension) &&
		Systems::any_wow(systems_.suspension);
	input.elevator_command = control_surfaces_.elevator_command;
	input.aileron_command = control_surfaces_.aileron_command;
	input.rudder_command = control_surfaces_.rudder_command;
	return input;
}

Systems::AerodynamicsFrameInput Fck1cEfm::make_aerodynamics_input() const
{
	Systems::AerodynamicsFrameInput input;
	input.center_of_mass = force_moment_.center_of_mass;
	input.mach = aircraft_state_.mach;
	input.aoa = aircraft_state_.aoa;
	input.alpha_deg = aircraft_state_.alpha;
	input.aos = aircraft_state_.aos;
	input.roll = aircraft_state_.roll;
	input.pitch_rate = aircraft_state_.pitch_rate;
	input.roll_rate = aircraft_state_.roll_rate;
	input.yaw_rate = aircraft_state_.yaw_rate;
	input.elevator_command = control_surfaces_.elevator_command;
	input.aileron_command = control_surfaces_.aileron_command;
	input.rudder_command = control_surfaces_.rudder_command;
	input.airbrake_pos = systems_.airframe_devices.airbrake_pos;
	input.flaps_pos = systems_.airframe_devices.flaps_pos;
	input.gear_pos = systems_.landing_gear.position;
	input.left_wing_integrity = systems_.damage.left_wing_integrity;
	input.right_wing_integrity = systems_.damage.right_wing_integrity;
	input.tail_integrity = systems_.damage.tail_integrity;
	input.easy_flight = gameplay_.easy_flight;
	return input;
}

void Fck1cEfm::update_primary_aerodynamics(const Systems::AerodynamicsFrameInput& input)
{
	Systems::apply_primary_aerodynamics(
		systems_.aerodynamics,
		config_.aerodynamics,
		input,
		[this](const Common::Vec3& force, const Common::Vec3& position)
		{
			add_force(force, position);
		});
}

void Fck1cEfm::update_engines_and_fuel(double dt)
{
	const double dry_thrust = max_dry_thrust();
	Systems::update_pilot_throttle_cmds(systems_.throttle_inputs);
	Systems::apply_engine_throttle_commands(
		systems_.engines,
		Systems::compose_engine_throttle_cmd(
			systems_.throttle_inputs.left.pilot_cmd,
			systems_.fbw.throttle_cmd_left,
			systems_.fbw.throttle_override,
			systems_.fbw.throttle_blend),
		Systems::compose_engine_throttle_cmd(
			systems_.throttle_inputs.right.pilot_cmd,
			systems_.fbw.throttle_cmd_right,
			systems_.fbw.throttle_override,
			systems_.fbw.throttle_blend));
	update_engine_state(dt, dry_thrust);
	handle_engine_shutdown(dt);
	apply_thrust_and_observe();
	update_fuel(dt);
}

double Fck1cEfm::max_dry_thrust() const
{
	return Systems::max_dry_thrust(config_.engine, aircraft_state_.mach);
}

void Fck1cEfm::update_engine_state(double dt, double dry_thrust)
{
	Systems::clamp_engine_throttle_inputs(systems_.engines);
	Systems::update_dry_engine_channels(
		systems_.engines,
		config_.engine,
		dt);
	Systems::update_afterburners(systems_.engines, dt);
	Systems::update_nozzle_apertures(systems_.engines, dt);
	Systems::update_engine_thrust_outputs(
		systems_.engines,
		dry_thrust,
		aircraft_state_.engine_alt_effect,
		systems_.damage.left_engine_integrity,
		systems_.damage.right_engine_integrity);
	Systems::apply_engine_readout_integrity(
		systems_.engines,
		systems_.damage.left_engine_integrity,
		systems_.damage.right_engine_integrity);
}

void Fck1cEfm::handle_engine_shutdown(double dt)
{
	if (!Systems::should_shutdown_engines(systems_.fuel.internal_fuel, aircraft_state_.altitude_asl))
	{
		return;
	}

	runtime_.on_engine_shutdown(*this);
	Systems::shutdown_engines(systems_.engines, dt);
}

void Fck1cEfm::apply_thrust_and_observe()
{
	const MaxPowerCommand command = runtime_.read_max_power();
	Systems::apply_thrust_cut(systems_.engines, command.ready > 0.5 && command.value < 0.5);
	add_force(Common::Vec3(systems_.engines.left.thrust_force, 0.0, 0.0), config_.left_engine_position);
	add_force(Common::Vec3(systems_.engines.right.thrust_force, 0.0, 0.0), config_.right_engine_position);
	runtime_.on_thrust_updated(*this, command);
}

void Fck1cEfm::update_fuel(double dt)
{
	if (gameplay_.infinite_fuel)
	{
		return;
	}

	const Systems::FuelConsumptionInput input = {
		dt,
		systems_.engines.left.throttle_output,
		systems_.engines.right.throttle_output,
		systems_.engines.left.afterburner_ratio,
		systems_.engines.right.afterburner_ratio,
		systems_.engines.afterburner.fuel_factor
	};
	Systems::simulate_fuel_consumption(systems_.fuel, config_.fuel, input);
}

void Fck1cEfm::update_ground_and_suspension(
	double dt,
	const Systems::AerodynamicsFrameInput& input)
{
	Systems::apply_aerodynamic_limiters(
		systems_.aerodynamics,
		config_.aerodynamics,
		input,
		[this](const Common::Vec3& force, const Common::Vec3& position)
		{
			add_force(force, position);
		},
		[this](const Common::Vec3& moment)
		{
			add_moment(moment);
		});
	apply_fallback_ground_forces();
	runtime_.on_ground_diagnostics(*this, dt);
	Systems::update_on_ground(systems_.suspension, systems_.landing_gear.position);
	gameplay_.shake_amplitude = Systems::update_aerodynamic_shake(
		systems_.aerodynamics,
		config_.aerodynamics,
		input,
		systems_.suspension.on_ground,
		aircraft_state_.g);
}

void Fck1cEfm::apply_fallback_ground_forces()
{
	const Systems::SuspensionFallbackInput input = {
		aircraft_state_.altitude_agl,
		aircraft_state_.pitch,
		aircraft_state_.roll,
		aircraft_state_.velocity_world.y,
		aircraft_state_.velocity_body.x,
		systems_.landing_gear.position,
		aircraft_state_.current_mass,
		systems_.engines.left.throttle_input,
		systems_.engines.right.throttle_input,
		systems_.engines.left.thrust_force,
		systems_.engines.right.thrust_force,
		systems_.landing_gear.wheels.brake_left,
		systems_.landing_gear.wheels.brake_right
	};
	systems_.suspension.fallback_ground_force = Systems::apply_fallback_ground_forces(
		systems_.suspension,
		config_.suspension,
		input,
		[this](const Common::Vec3& force, const Common::Vec3& position)
		{
			add_force(force, position);
		});
}

double Fck1cEfm::nose_wheel_steering() const
{
	return Systems::compute_nose_wheel_steering(
		systems_.landing_gear,
		aircraft_state_.speed_scalar,
		systems_.primary_controls.yaw.input);
}

void Fck1cEfm::add_force(const Common::Vec3& force, const Common::Vec3& position)
{
	Core::add_local_force(
		force_moment_.force,
		force_moment_.moment,
		force_moment_.center_of_mass,
		force,
		position);
}

void Fck1cEfm::add_moment(const Common::Vec3& moment)
{
	Core::add_local_moment(force_moment_.moment, moment);
}

void Fck1cEfm::finish_frame()
{
	Systems::mark_first_frame_completed(systems_.startup);
}

void Fck1cEfm::reset_start_state(Systems::StartupMode mode)
{
	Systems::reset_damage_model(systems_.damage);
	Systems::reset_suspension_feedback_state(systems_.suspension);
	Systems::reset_fbw_state(
		systems_.fbw,
		aircraft_state_.roll,
		aircraft_state_.pitch,
		aircraft_state_.alpha,
		aircraft_state_.g);
	Systems::begin_startup(systems_.startup, mode);
}

void Fck1cEfm::cold_start()
{
	reset_start_state(Systems::STARTUP_MODE_COLD_GROUND);
	Systems::configure_ground_start_landing_gear(systems_.landing_gear);
	Systems::reset_throttle_inputs(
		systems_.throttle_inputs,
		kColdStartThrottle,
		kColdStartThrottle);
	Systems::configure_cold_start_engines(systems_.engines);
}

void Fck1cEfm::hot_ground_start()
{
	reset_start_state(Systems::STARTUP_MODE_HOT_GROUND);
	Systems::configure_ground_start_landing_gear(systems_.landing_gear);
	Systems::configure_hot_ground_start_devices(systems_.airframe_devices);
	Systems::reset_throttle_inputs(
		systems_.throttle_inputs,
		kColdStartThrottle,
		kColdStartThrottle);
	Systems::configure_hot_ground_start_engines(systems_.engines);
}

void Fck1cEfm::hot_air_start()
{
	reset_start_state(Systems::STARTUP_MODE_HOT_AIR);
	Systems::configure_air_start_landing_gear(systems_.landing_gear);
	Systems::reset_throttle_inputs(
		systems_.throttle_inputs,
		kHotAirStartThrottle,
		kHotAirStartThrottle);
	Systems::configure_hot_air_start_engines(systems_.engines);
}

void Fck1cEfm::reset_control_outputs()
{
	Systems::reset_primary_commands(systems_.primary_controls);
	control_surfaces_.elevator_command = 0.0;
	control_surfaces_.aileron_command = 0.0;
	control_surfaces_.rudder_command = 0.0;
}

void Fck1cEfm::release()
{
	Systems::reset_suspension_feedback_state(systems_.suspension);
	Systems::reset_fbw_state(
		systems_.fbw,
		aircraft_state_.roll,
		aircraft_state_.pitch,
		aircraft_state_.alpha,
		aircraft_state_.g);
	Systems::configure_release(systems_.startup);
	reset_control_outputs();
	Systems::configure_release_landing_gear(systems_.landing_gear);
	Systems::reset_throttle_inputs(systems_.throttle_inputs, 0.0, 0.0);
	Systems::reset_fbw_throttle_interface(systems_.fbw);
	Systems::reset_engine_release_state(systems_.engines);
	runtime_.on_release(*this);
	repair();
}

void Fck1cEfm::repair()
{
	Systems::reset_damage_model(systems_.damage);
}
}
