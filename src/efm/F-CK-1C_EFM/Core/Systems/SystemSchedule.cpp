#include "SystemSchedule.h"

#include <limits>
#include <stdexcept>

namespace
{
constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000ULL;

std::uint64_t checked_seconds_to_nanoseconds(std::uint64_t seconds)
{
	const std::uint64_t maximum =
		static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)());
	if (seconds > maximum / kNanosecondsPerSecond)
	{
		throw std::overflow_error("System schedule exceeded its time range.");
	}
	return seconds * kNanosecondsPerSecond;
}
}

namespace Core
{
namespace Systems
{
void SystemSchedule::initialize(
	const std::vector<SystemTiming>& timings)
{
	systems_.clear();
	due_.clear();
	systems_.reserve(timings.size());
	for (const SystemTiming& timing : timings)
	{
		const bool has_rate = timing.update_rate_hz != 0;
		const bool has_period = timing.update_period.count() > 0;
		if (has_rate == has_period)
		{
			throw std::logic_error(
				"System timing must contain one rate or period.");
		}
		systems_.push_back({ timing, 0 });
		schedule_next(systems_.size() - 1);
	}
}

bool SystemSchedule::has_due(SystemScheduledTime target_time) const
{
	return !due_.empty() && due_.begin()->first <= target_time;
}

ScheduledSystemBatch SystemSchedule::next_due() const
{
	if (due_.empty())
	{
		throw std::logic_error("System schedule has no pending work.");
	}
	return { due_.begin()->first, due_.begin()->second };
}

void SystemSchedule::complete_next_due()
{
	if (due_.empty())
	{
		throw std::logic_error("System schedule has no pending work.");
	}
	const std::vector<std::size_t> completed = due_.begin()->second;
	due_.erase(due_.begin());
	for (const std::size_t system_index : completed)
	{
		++systems_[system_index].completed_ticks;
		schedule_next(system_index);
	}
}

SystemScheduledTime SystemSchedule::tick_time(
	const PeriodicSystem& system,
	std::uint64_t tick)
{
	if (system.timing.update_rate_hz != 0)
	{
		return rate_tick_time(system.timing.update_rate_hz, tick);
	}
	return period_tick_time(system.timing.update_period, tick);
}

SystemScheduledTime SystemSchedule::rate_tick_time(
	std::uint32_t update_rate_hz,
	std::uint64_t tick)
{
	const std::uint64_t seconds = tick / update_rate_hz;
	const std::uint64_t remainder = tick % update_rate_hz;
	const std::uint64_t whole_nanoseconds =
		checked_seconds_to_nanoseconds(seconds);
	const std::uint64_t fractional_nanoseconds =
		(remainder * kNanosecondsPerSecond) / update_rate_hz;
	const std::uint64_t total = whole_nanoseconds + fractional_nanoseconds;
	const std::uint64_t maximum =
		static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)());
	if (total > maximum)
	{
		throw std::overflow_error("System schedule exceeded its time range.");
	}
	return SystemScheduledTime(static_cast<std::int64_t>(total));
}

SystemScheduledTime SystemSchedule::period_tick_time(
	SystemScheduledTime update_period,
	std::uint64_t tick)
{
	const std::uint64_t period =
		static_cast<std::uint64_t>(update_period.count());
	const std::uint64_t maximum =
		static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)());
	if (tick > maximum / period)
	{
		throw std::overflow_error("System schedule exceeded its time range.");
	}
	return SystemScheduledTime(
		static_cast<std::int64_t>(tick * period));
}

void SystemSchedule::schedule_next(std::size_t system_index)
{
	const PeriodicSystem& system = systems_[system_index];
	const SystemScheduledTime time =
		tick_time(system, system.completed_ticks + 1);
	due_[time].push_back(system_index);
}
}
}
