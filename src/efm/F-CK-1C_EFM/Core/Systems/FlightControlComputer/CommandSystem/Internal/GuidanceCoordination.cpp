#include "GuidanceCoordination.h"

// Command-system implementation detail; use FlightControlCommandSystem.

#include "Common/Clamp.h"
#include "Common/Units.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

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
		if (const auto* command = std::get_if<
			Core::Systems::NormalAccelerationCommand>(
				&result_.reference.longitudinal.command))
		{
			vertical_nz_reference_g_ = command->target_g;
		}
		if (!automatic_longitudinal_) return;
		require_automatic_kinematics();
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

	void require_automatic_kinematics() const
	{
		if (input_.flight.flight_path_angle_available &&
			input_.flight.indicated_airspeed_mps > 0.0) return;
		throw std::logic_error(
			"Automatic longitudinal guidance requires valid air kinematics.");
	}

	void make_pitch_attitude_reference()
	{
		const double pitch_rate_rad_s =
			input_.config.pitch_error_to_rate_gain_s_inv *
			(automatic_longitudinal().pitch_attitude_reference_rad -
				input_.flight.pitch_attitude_rad);
		if (!input_.selected_gear_handle_down)
		{
			vertical_nz_reference_g_ = normal_acceleration_for_pitch_rate(
				pitch_rate_rad_s);
			result_.reference.longitudinal = {
				Core::Systems::NormalAccelerationCommand{
					vertical_nz_reference_g_ } };
			return;
		}
		result_.reference.longitudinal = {
			Core::Systems::PitchRateCommand{ pitch_rate_rad_s } };
	}

	void make_vertical_speed_reference()
	{
		const double error_ft_s =
			automatic_longitudinal().vertical_speed_reference_ft_s -
			input_.flight.vertical_speed_ft_s;
		const double vertical_acceleration_mps2 =
			input_.config.vertical_speed_error_to_acceleration_gain_s_inv *
			Common::metres(error_ft_s);
		if (input_.selected_gear_handle_down)
		{
			const double pitch_rate_rad_s = vertical_acceleration_mps2 /
				input_.flight.indicated_airspeed_mps;
			result_.reference.longitudinal = {
				Core::Systems::PitchRateCommand{ pitch_rate_rad_s } };
			return;
		}
		vertical_nz_reference_g_ =
			std::cos(input_.flight.flight_path_angle_rad) +
			vertical_acceleration_mps2 / kGravityMps2;
		result_.reference.longitudinal = {
			Core::Systems::NormalAccelerationCommand{
				vertical_nz_reference_g_ }
		};
	}

	double normal_acceleration_for_pitch_rate(double pitch_rate_rad_s) const
	{
		return std::cos(input_.flight.flight_path_angle_rad) +
			input_.flight.indicated_airspeed_mps * pitch_rate_rad_s /
			kGravityMps2;
	}

	void constrain_vertical_reference()
	{
		if (!automatic_longitudinal_) return;
		auto* command = std::get_if<Core::Systems::NormalAccelerationCommand>(
			&result_.reference.longitudinal.command);
		if (command == nullptr) return;
		const auto& envelope = input_.envelope.guidance;
		const double limited = Common::limit(
			command->target_g,
			envelope.minimum_normal_acceleration_g,
			envelope.maximum_normal_acceleration_g);
		if (limited == command->target_g) return;
		command->target_g = limited;
		vertical_nz_reference_g_ = command->target_g;
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
		if (!std::holds_alternative<Core::Systems::NormalAccelerationCommand>(
			result_.reference.longitudinal.command)) return;
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
			input_.config.bank_error_to_roll_rate_gain_s_inv *
				(bank_reference_rad_ - input_.flight.roll_attitude_rad),
			-input_.envelope.guidance.roll_rate_limit_rad_s,
			input_.envelope.guidance.roll_rate_limit_rad_s);
		if (input_.flight.indicated_airspeed_mps <
			input_.config.coordinated_turn_minimum_speed_mps) return;
		// DCS/Core local-body +y is nose-left, so a positive (right) bank
		// requires a negative coordinated yaw-rate reference.
		reference.yaw_rate_feedforward_rad_s -=
			kGravityMps2 * std::tan(bank_reference_rad_) /
			input_.flight.indicated_airspeed_mps;
	}

	void apply_bank_to_lift_compensation()
	{
		if (!automatic_longitudinal_) return;
		auto* command = std::get_if<Core::Systems::NormalAccelerationCommand>(
			&result_.reference.longitudinal.command);
		if (command == nullptr) return;
		const double lift_fraction = std::max(
			std::cos(bank_reference_rad_), kCosineFloor);
		command->target_g = Common::limit(
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
