#pragma once

#include "System.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace Core
{
namespace Systems
{
struct SystemTiming
{
	std::uint32_t update_rate_hz = 0;
	SystemScheduledTime update_period = {};
};

struct ScheduledSystemBatch
{
	SystemScheduledTime scheduled_time = {};
	std::vector<std::size_t> system_indices;
};

class SystemSchedule final
{
public:
	void initialize(const std::vector<SystemTiming>& timings);
	bool has_due(SystemScheduledTime target_time) const;
	ScheduledSystemBatch next_due() const;
	void complete_next_due();

private:
	struct PeriodicSystem
	{
		SystemTiming timing;
		std::uint64_t completed_ticks = 0;
	};

	static SystemScheduledTime tick_time(
		const PeriodicSystem& system,
		std::uint64_t tick);
	static SystemScheduledTime rate_tick_time(
		std::uint32_t update_rate_hz,
		std::uint64_t tick);
	static SystemScheduledTime period_tick_time(
		SystemScheduledTime update_period,
		std::uint64_t tick);
	void schedule_next(std::size_t system_index);

	std::vector<PeriodicSystem> systems_;
	std::map<SystemScheduledTime, std::vector<std::size_t>> due_;
};
}
}
