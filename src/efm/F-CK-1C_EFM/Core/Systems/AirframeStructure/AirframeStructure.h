#pragma once

#include "../System.h"
#include "../../Contracts/AircraftData.h"
#include "Common/SegmentedIntegrity.h"

namespace Core
{
namespace Systems
{
class AirframeStructure final : public System
{
public:
	void setup(SystemSetup& setup) override;
	void step(
		const AircraftDataView& aircraft,
		SystemResult& result) override;

	void apply_damage(const DamageEvent& event);
	void repair(const RepairEvent& event);
	const AirframeIntegrity& integrity() const;

private:
	void refresh_integrity();

	Common::SegmentedIntegrity<kWingDamageSegmentCount> left_wing_;
	Common::SegmentedIntegrity<kWingDamageSegmentCount> right_wing_;
	Common::SegmentedIntegrity<kTailDamageSegmentCount> tail_;
	AirframeIntegrity integrity_;
};
}
}
