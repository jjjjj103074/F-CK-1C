#include "PilotCommandLaw.h"

// Command-system implementation detail; use FlightControlCommandSystem.

#include "Common/Units.h"

namespace Systems
{
namespace
{
Core::Systems::LongitudinalManeuverReference make_longitudinal_command(
	const PilotCommandLawInput& input)
{
	const double pitch = input.signals.pilot_pitch_normalized;
	if (input.signals.landing_gear_handle_down)
	{
		return { Core::Systems::PitchRateCommand{
			pitch * input.longitudinal.landing_pitch_rate_limit_rad_s *
				input.gains.command_gain } };
	}
	const double positive_g =
		input.envelope.hard_protection.maximum_normal_acceleration_g;
	const double negative_g =
		input.envelope.hard_protection.minimum_normal_acceleration_g;
	const double target_g = pitch >= 0.0
		? 1.0 + pitch * (positive_g - 1.0)
		: 1.0 + pitch * (1.0 - negative_g);
	return { Core::Systems::NormalAccelerationCommand{ target_g } };
}
}

Core::Systems::CoordinatedManeuverReference make_pilot_maneuver_reference(
	const PilotCommandLawInput& input)
{
	return {
		make_longitudinal_command(input),
		{
			input.signals.pilot_roll_normalized *
				input.stores.envelope.maximum_roll_command_rad_s *
				input.gains.command_gain,
			0.0,
			input.signals.pilot_yaw_normalized *
				input.stores.envelope.maximum_yaw_command_rad_s *
				input.gains.command_gain
		},
		Core::Systems::AuthorityState::Manual,
		Core::Systems::AuthorityState::Manual,
		Core::Systems::AuthorityState::Manual
	};
}
}
