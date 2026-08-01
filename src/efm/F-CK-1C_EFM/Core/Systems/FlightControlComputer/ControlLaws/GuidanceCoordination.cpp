#include "GuidanceCoordination.h"

#include "Common/Clamp.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kGravityMps2 = 9.80665;
constexpr double kCosineFloor = 0.1;

double maximum_feasible_bank_rad(double nz_reference_g, double maximum_nz_g)
{
	const double ratio = std::fabs(nz_reference_g) / maximum_nz_g;
	return ratio >= 1.0 ? 0.0 : std::acos(ratio);
}

class CoordinationFrame
{
public:
	explicit CoordinationFrame(
		const Core::Systems::GuidanceCoordinationInput& input)
		: input_(input),
		  automatic_longitudinal_(std::holds_alternative<
			  Core::Systems::AutomaticLongitudinalFlightReference>(
				  input.selected.longitudinal)),
		  automatic_lateral_(std::holds_alternative<
			  Core::Systems::AutomaticLateralFlightReference>(
				  input.selected.lateral))
	{
		initialize_selected_payloads();
		result_.reference.longitudinal_authority =
			input_.selected.longitudinal_authority;
		result_.reference.lateral_authority =
			input_.selected.lateral_authority;
		result_.reference.directional_authority =
			input_.selected.directional_authority;
	}

	Core::Systems::GuidanceCoordinationResult run()
	{
		select_bank_reference();
		make_longitudinal_reference();
		constrain_vertical_reference();
		constrain_combined_reference();
		make_lateral_directional_reference();
		apply_bank_to_lift_compensation();
		return result_;
	}

private:
	void initialize_selected_payloads()
	{
		if (const auto* manual = std::get_if<
			Core::Systems::LongitudinalManeuverReference>(
				&input_.selected.longitudinal))
		{
			result_.reference.longitudinal = *manual;
		}
		if (const auto* manual = std::get_if<
			Core::Systems::ManualLateralFlightReference>(
				&input_.selected.lateral))
		{
			result_.reference.lateral_directional.roll_rate_reference_rad_s =
				manual->roll_rate_reference_rad_s;
		}
		result_.reference.lateral_directional.sideslip_reference_rad =
			input_.selected.directional.sideslip_reference_rad;
		result_.reference.lateral_directional.yaw_rate_feedforward_rad_s =
			input_.selected.directional.yaw_rate_feedforward_rad_s;
	}

	const Core::Systems::AutomaticLongitudinalFlightReference&
		automatic_longitudinal() const
	{
		return std::get<Core::Systems::AutomaticLongitudinalFlightReference>(
			input_.selected.longitudinal);
	}

	void select_bank_reference()
	{
		bank_reference_rad_ = input_.flight.roll_attitude_rad;
		if (!automatic_lateral_) return;
		const auto& automatic =
			std::get<Core::Systems::AutomaticLateralFlightReference>(
				input_.selected.lateral);
		bank_reference_rad_ = Common::limit(
			automatic.bank_angle_reference_rad,
			-input_.envelope.guidance.bank_limit_rad,
			input_.envelope.guidance.bank_limit_rad);
	}

	void make_longitudinal_reference()
	{
		vertical_nz_reference_g_ =
			result_.reference.longitudinal.normal_acceleration_reference_g;
		if (!automatic_longitudinal_) return;
		const auto type = automatic_longitudinal().type;
		if (type == Core::Systems::VerticalGuidanceReferenceType::PitchAttitude)
		{
			make_pitch_attitude_reference();
		}
		else if (type ==
			Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed)
		{
			make_vertical_speed_reference();
		}
	}

	void make_pitch_attitude_reference()
	{
		vertical_nz_reference_g_ = 1.0;
		result_.reference.longitudinal = {
			vertical_nz_reference_g_,
			input_.config.pitch_attitude_error_to_rate_gain *
				(automatic_longitudinal().pitch_attitude_reference_rad -
					input_.flight.pitch_attitude_rad)
		};
	}

	void make_vertical_speed_reference()
	{
		const double error_mps =
			automatic_longitudinal().vertical_speed_reference_mps -
			input_.flight.vertical_speed_mps;
		vertical_nz_reference_g_ = 1.0 +
			input_.config.vertical_speed_error_to_acceleration_gain *
				error_mps / kGravityMps2;
		result_.reference.longitudinal = {
			vertical_nz_reference_g_, 0.0
		};
	}

	void constrain_vertical_reference()
	{
		if (!automatic_longitudinal_) return;
		const auto& envelope = input_.envelope.guidance;
		const double limited = Common::limit(
			vertical_nz_reference_g_,
			envelope.minimum_normal_acceleration_g,
			envelope.maximum_normal_acceleration_g);
		if (limited == vertical_nz_reference_g_) return;
		vertical_nz_reference_g_ = limited;
		result_.constraint = {
			Core::Systems::ConstraintReason::VerticalReferenceUnmaintainable,
			true,
			false
		};
	}

	void constrain_combined_reference()
	{
		if (!automatic_longitudinal_ || !automatic_lateral_ ||
			result_.constraint.vertical_constrained) return;
		const double feasible_bank_rad = maximum_feasible_bank_rad(
			vertical_nz_reference_g_,
			input_.envelope.guidance.maximum_normal_acceleration_g);
		const double limited = Common::limit(
			bank_reference_rad_, -feasible_bank_rad, feasible_bank_rad);
		if (limited == bank_reference_rad_) return;
		bank_reference_rad_ = limited;
		result_.constraint = {
			Core::Systems::ConstraintReason::LateralConstrainedByVerticalAuthority,
			false,
			true
		};
	}

	void make_lateral_directional_reference()
	{
		if (!automatic_lateral_) return;
		auto& reference = result_.reference.lateral_directional;
		reference.roll_rate_reference_rad_s = Common::limit(
			input_.config.bank_angle_error_to_roll_rate_gain *
				(bank_reference_rad_ - input_.flight.roll_attitude_rad),
			-input_.envelope.guidance.roll_rate_limit_rad_s,
			input_.envelope.guidance.roll_rate_limit_rad_s);
		if (input_.flight.indicated_airspeed_mps <
			input_.config.coordinated_turn_minimum_speed_mps) return;
		reference.yaw_rate_feedforward_rad_s +=
			kGravityMps2 * std::tan(bank_reference_rad_) /
			input_.flight.indicated_airspeed_mps;
	}

	void apply_bank_to_lift_compensation()
	{
		const bool vertical_path = automatic_longitudinal_ &&
			automatic_longitudinal().type ==
				Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed;
		if (!vertical_path) return;
		const double lift_fraction = std::max(
			std::cos(bank_reference_rad_), kCosineFloor);
		result_.reference.longitudinal.normal_acceleration_reference_g =
			Common::limit(
				vertical_nz_reference_g_ / lift_fraction,
				input_.envelope.guidance.minimum_normal_acceleration_g,
				input_.envelope.guidance.maximum_normal_acceleration_g);
	}

	const Core::Systems::GuidanceCoordinationInput& input_;
	Core::Systems::GuidanceCoordinationResult result_;
	const bool automatic_longitudinal_;
	const bool automatic_lateral_;
	double bank_reference_rad_ = 0.0;
	double vertical_nz_reference_g_ = 1.0;
};
}

namespace Core
{
namespace Systems
{
GuidanceCoordinationResult coordinate_guidance(
	const GuidanceCoordinationInput& input)
{
	return CoordinationFrame(input).run();
}
}
}
