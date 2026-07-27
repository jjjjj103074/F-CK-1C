#include "SecondaryFlightControls.h"

#include "../SystemPipeline.h"

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
}

namespace Core
{
namespace Systems
{
SecondaryFlightControls::SecondaryFlightControls(StartMode start_mode)
{
	if (start_mode == StartMode::HotGround)
	{
		::Systems::configure_hot_ground_start_devices(devices_);
	}
	refresh_position();
}

void SecondaryFlightControls::setup(SystemSetup& setup)
{
	setup.read(AircraftDataKeys::kAircraftObservation);
	setup.read(AircraftDataKeys::kLandingGearData);
	setup.publish(AircraftDataKeys::kSecondaryControlPosition, position_);
	register_commands(setup);
}

void SecondaryFlightControls::register_commands(SystemSetup& setup)
{
	const CommandId commands[] = {
		CommandId::ToggleAirbrake,
		CommandId::SetAirbrake,
		CommandId::ToggleFlaps,
		CommandId::SetFlapsUp,
		CommandId::SetFlapsAuto,
		CommandId::SetFlapsDown
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const Command& command) { handle_command(command); });
	}
}

void SecondaryFlightControls::step(
	const AircraftDataSnapshot& snapshot,
	SystemResult& result)
{
	const AircraftObservation& observation =
		snapshot.read(AircraftDataKeys::kAircraftObservation);
	const double gear =
		snapshot.read(AircraftDataKeys::kLandingGearData).position;
	result.publish(
		AircraftDataKeys::kSecondaryControlPosition,
		step(observation.speed_scalar, gear));
}

const SecondaryControlPosition& SecondaryFlightControls::step(
	double speed_scalar,
	double gear_position)
{
	::Systems::update_airframe_device_positions(
		devices_,
		{ speed_scalar, gear_position });
	refresh_position();
	return position_;
}

void SecondaryFlightControls::handle_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::ToggleAirbrake:
		::Systems::toggle_airbrake(devices_); break;
	case CommandId::SetAirbrake:
		::Systems::set_airbrake(
			devices_, command.value > kEnabledCommandThreshold); break;
	case CommandId::ToggleFlaps:
		::Systems::toggle_flap_mode(devices_); break;
	case CommandId::SetFlapsUp:
		::Systems::set_flap_mode(devices_, ::Systems::FLAP_MODE_UP); break;
	case CommandId::SetFlapsAuto:
		::Systems::set_flap_mode(devices_, ::Systems::FLAP_MODE_AUTO); break;
	case CommandId::SetFlapsDown:
		::Systems::set_flap_mode(devices_, ::Systems::FLAP_MODE_DOWN); break;
	default:
		break;
	}
}

void SecondaryFlightControls::refresh_position()
{
	position_ = {
		devices_.flaps_pos,
		devices_.slats_pos,
		devices_.airbrake_pos
	};
}

const SecondaryControlPosition& SecondaryFlightControls::position() const
{
	return position_;
}
}
}
