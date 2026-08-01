#include "PilotCommandLaw.h"

#include "Common/Units.h"

namespace Systems
{
Core::Systems::CoordinatedManeuverReference make_pilot_maneuver_reference(
	const PilotCommandLawInput& input)
{
	const double pitch = input.flight.pilot_pitch_normalized;
	const double normal_acceleration_reference_g = pitch >= 0.0
		? 1.0 + pitch *
			(input.envelope.hard_protection.maximum_normal_acceleration_g - 1.0)
		: 1.0 + pitch *
			(1.0 - input.envelope.hard_protection.minimum_normal_acceleration_g);
	return {
		{
			normal_acceleration_reference_g,
			pitch * Common::rad(input.config.q_cmd_land_max_deg) *
				input.gains.cmd_gain
		},
		{
			input.flight.pilot_roll_normalized * input.cat.p_cmd_max *
				input.gains.cmd_gain,
			0.0,
			input.flight.pilot_yaw_normalized * input.cat.r_cmd_max *
				input.gains.cmd_gain
		},
		Core::Systems::AuthorityState::Manual,
		Core::Systems::AuthorityState::Manual,
		Core::Systems::AuthorityState::Manual
	};
}
}
