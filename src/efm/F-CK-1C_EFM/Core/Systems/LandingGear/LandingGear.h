#pragma once

#include "LandingGearModel.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"
#include "Systems/SuspensionSystem.h"

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
	LandingGear(
		StartMode start_mode,
		const ::Systems::SuspensionSystemConfig& suspension_config);

	void setup(SystemSetup& setup) override;
	void step(
		const AircraftDataSnapshot& snapshot,
		SystemResult& result) override;

	const LandingGearData& step(const LandingGearFrameInput& input);
	void apply_suspension_feedback(const FrameInput& input);
	void update_on_ground();
	void handle_command(const Command& command);

	const LandingGearData& data() const;
	const ::Systems::LandingGearSystemState& device_state() const;
	const ::Systems::SuspensionSystemState& suspension_state() const;

private:
	void register_commands(SystemSetup& setup);
	void refresh_data();

	::Systems::LandingGearSystemState landing_gear_;
	::Systems::SuspensionSystemState suspension_;
	std::array<double, kFrameSuspensionWheelCount> wheel_radius_;
	LandingGearData data_;
};
}
}
