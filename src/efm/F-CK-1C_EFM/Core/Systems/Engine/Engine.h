#pragma once

#include "EngineModel.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"
#include "Common/SegmentedIntegrity.h"

namespace Core
{
namespace Systems
{
struct EngineFrameInput
{
	double dt = 0.0;
	EngineControlDemand demand;
	double internal_fuel = 0.0;
	double altitude_asl = 0.0;
};

class Engine final : public System
{
public:
	Engine(
		const ::Systems::EngineSystemConfig& config,
		double fuel_consumption_rate,
		StartMode start_mode);

	void setup(SystemSetup& setup) override;
	void step(
		const AircraftDataSnapshot& snapshot,
		SystemResult& result) override;

	const EngineData& step(const EngineFrameInput& input);
	void handle_command(const Command& command);
	void apply_damage(const DamageEvent& event);
	void repair(const RepairEvent& event);

	const EngineData& data() const;
	const FuelDemand& fuel_demand() const;
	const ::Systems::EngineSystemState& state() const;
	double left_integrity() const;
	double right_integrity() const;

private:
	void configure_start(StartMode start_mode);
	void register_handlers(SystemSetup& setup);
	void refresh_outputs();

	const ::Systems::EngineSystemConfig& config_;
	const double fuel_consumption_rate_;
	::Systems::EngineSystemState engines_;
	Common::SegmentedIntegrity<kEngineDamageSegmentCount> left_integrity_;
	Common::SegmentedIntegrity<kEngineDamageSegmentCount> right_integrity_;
	EngineData data_;
	FuelDemand fuel_demand_;
	bool thrust_inhibited_ = false;
};
}
}
