#pragma once

#include "LandingGearConfig.h"
#include "LandingGearModel.h"
#include "SuspensionFeedback.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
struct LandingGearFrameInput
{
	double speed_scalar = 0.0;
	double ground_speed = 0.0;
	double dt = 0.0;
	double altitude_agl = 0.0;
	double yaw_input = 0.0;
};

class LandingGear final : public System
{
public:
	LandingGear(StartMode start_mode, const LandingGearConfig& config);

	void setup(SystemSetup& setup) override;
	void step(
		const AircraftDataView& aircraft,
		SystemResult& result) override;

	const LandingGearData& step(const LandingGearFrameInput& input);
	void apply_suspension_feedback(const FrameInput& input);
	void update_on_ground();
	void handle_command(const Command& command);
	void apply_damage(const DamageEvent& event);
	void repair(const RepairEvent& event);

	const LandingGearData& data() const;
	const ::Systems::LandingGearSystemState& device_state() const;
	const SuspensionFeedbackState& suspension_state() const;

private:
	void register_handlers(SystemSetup& setup);
	void refresh_data();

	::Systems::LandingGearSystemState landing_gear_;
	SuspensionFeedbackState suspension_;
	std::array<double, kFrameSuspensionWheelCount> wheel_radius_;
	std::array<double, kLandingGearDamageSegmentCount> integrity_;
	LandingGearData data_;
};

SystemEntry make_landing_gear_system_entry(
	const LandingGearConfig& config);
}
}
