#pragma once

#include "../TimedAction.h"
#include "../../Database/InputOutputData.h"
#include "../../Database/MissionData.h"
#include "../../../../Contracts/Commands.h"

namespace Core::Systems::Mc
{
class SensorCoordination final
{
public:
	SensorCoordination(const MissionInputs& input,
	                   const ModeData& mode,
	                   const StoresData& stores,
	                   SensorData& sensor,
	                   ExternalActionQueue& actions);
	void handle(const Command& command);
	void select_mode(bool cycle_weapon);
	void prepare_release();
	void unlock();
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	void ensure_radar_on();
	void apply_submode(AirToAirSubmode mode);
	void begin_lock(bool pulse);
	void finish_lock();
	const MissionInputs& input_;
	const ModeData& mode_;
	const StoresData& stores_;
	SensorData& sensor_;
	ExternalActionQueue& actions_;
	TimedAction lock_;
	bool radar_on_requested_ = false;
	std::chrono::nanoseconds next_auto_lock_ = {};
};
}
