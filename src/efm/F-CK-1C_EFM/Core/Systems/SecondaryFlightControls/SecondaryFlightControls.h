#pragma once

#include "SecondaryFlightControlModel.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
class SecondaryFlightControls final : public System
{
public:
	explicit SecondaryFlightControls(StartMode start_mode);

	void setup(SystemSetup& setup) override;
	void step(
		const SystemStepContext& context,
		const AircraftDataView& aircraft,
		SystemResult& result) override;

	const SecondaryControlPosition& step(
		double true_airspeed_mps,
		double gear_position_normalized);
	void handle_command(const Command& command);
	const SecondaryControlPosition& position() const;

private:
	void register_commands(SystemSetup& setup);
	void refresh_position();

	::Systems::AirframeDeviceState devices_;
	SecondaryControlPosition position_;
};
}
}
