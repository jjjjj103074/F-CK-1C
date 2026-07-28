#pragma once

#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
class PrimaryFlightControls final : public System
{
public:
	void setup(SystemSetup& setup) override;
	void step(
		const AircraftDataView& aircraft,
		SystemResult& result) override;

	const PrimaryControlPosition& step(const FlightControlDemand& demand);
	const PrimaryControlPosition& position() const;

private:
	PrimaryControlPosition position_;
};
}
}
