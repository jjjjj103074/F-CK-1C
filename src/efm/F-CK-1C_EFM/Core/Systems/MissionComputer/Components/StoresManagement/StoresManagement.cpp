#include "StoresManagement.h"

#include <algorithm>

namespace Core::Systems::Mc
{
StoresManagement::StoresManagement(const MissionInputs& input, StoresData& stores)
    : input_(input),
      stores_(stores)
{
}

void StoresManagement::step(std::chrono::nanoseconds, double)
{
	const MissionAdapterObservation& observation = input_.cockpit.mission_adapter;
	stores_.live_missile_count = observation.missile_count_available;
	if (stores_.live_missile_count)
	{
		stores_.missile_count = (std::max)(0, observation.missile_count);
	}
}

void StoresManagement::record_missile_release()
{
	if (!stores_.live_missile_count)
	{
		stores_.missile_count = (std::max)(0, stores_.missile_count - 1);
	}
}
}
