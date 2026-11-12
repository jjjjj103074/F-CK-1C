#include "RadarRequestQueue.h"

namespace Core::Systems::Mc
{
std::uint64_t RadarRequestQueue::submit(const RadarCommand& command)
{
	const auto sequence = actions_.reserve_radar_operation();
	requests_.push_back({ sequence, command });
	return sequence;
}

RadarRequests RadarRequestQueue::pending()
{
	while (!requests_.empty() && actions_.acknowledged(requests_.front().sequence))
		requests_.pop_front();
	const auto acknowledged = actions_.pending().acknowledged_through;
	return { acknowledged, { requests_.begin(), requests_.end() } };
}
}
