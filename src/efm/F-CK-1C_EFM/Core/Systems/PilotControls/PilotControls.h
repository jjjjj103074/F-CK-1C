#pragma once

#include "InputModel.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
class PilotControls final : public System
{
public:
	explicit PilotControls(const ThrottleLeverSignal& initial_throttle_levers);

	void setup(SystemSetup& setup) override;
	void step(
		const SystemStepContext& context,
		const AircraftDataView& aircraft,
		SystemResult& result) override;
	void handle_command(const Command& command);

	const PilotControlSignal& signal() const;
	const ThrottleLeverSignal& throttle_levers() const;

private:
	void register_commands(SystemSetup& setup);
	void handle_pitch_roll_command(const Command& command);
	void handle_yaw_command(const Command& command);
	void handle_throttle_command(const Command& command);
	void refresh_signal();

	::Systems::PrimaryControlState primary_controls_;
	::Systems::ThrottleInputState throttle_inputs_;
	PilotControlSignal signal_;
	ThrottleLeverSignal throttle_levers_;
};
}
}
