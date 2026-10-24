#pragma once

#include "../Database/ExternalActionQueue.h"

#include <chrono>
#include <optional>

namespace Core::Systems::Mc
{
/// @brief A minimum hold starts when Lua has executed the leading edge, not when MC queued it.
class TimedAction final
{
public:
	void begin(std::uint64_t sequence)
	{
		start_sequence_ = sequence;
		started_at_.reset();
		release_pending_ = false;
	}

	void request_release() noexcept
	{
		release_pending_ = true;
	}

	void clear() noexcept
	{
		start_sequence_ = 0;
		started_at_.reset();
		release_pending_ = false;
	}

	bool active() const noexcept
	{
		return start_sequence_ != 0;
	}

	bool release_due(std::chrono::nanoseconds time, const ExternalActionQueue& queue)
	{
		if (active() && !started_at_ && queue.acknowledged(start_sequence_))
		{
			started_at_ = time;
		}
		return release_pending_ && started_at_ && time - *started_at_ >= std::chrono::milliseconds(80);
	}

private:
	std::uint64_t start_sequence_ = 0;
	std::optional<std::chrono::nanoseconds> started_at_;
	bool release_pending_ = false;
};
}
