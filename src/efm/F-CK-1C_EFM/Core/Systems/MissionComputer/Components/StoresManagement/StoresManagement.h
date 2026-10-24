#pragma once

#include "../../Database/InputOutputData.h"
#include "../../Database/MissionData.h"

#include <chrono>

namespace Core::Systems::Mc
{
class StoresManagement final
{
public:
	StoresManagement(const MissionInputs& input, StoresData& stores);
	void step(std::chrono::nanoseconds time, double dt_s);
	void record_missile_release();

private:
	const MissionInputs& input_;
	StoresData& stores_;
};
}
