#pragma once

#include "../../Core/Contracts/FrameContracts.h"

#include <deque>
#include <mutex>
#include <optional>

namespace DcsBridge
{
namespace Internal
{
class OutputStore final
{
public:
	OutputStore() = default;
	OutputStore(const OutputStore&) = delete;
	OutputStore& operator=(const OutputStore&) = delete;

	void publish(const Core::FrameOutput& output);
	void publish_start(const Core::FrameOutput& output);
	std::optional<Core::FrameOutput> read() const;
	Core::MassDeltaResult take_mass_delta();
	void mark_released();
	bool is_released() const;

private:
	void publish_locked(const Core::FrameOutput& output);

	mutable std::mutex mutex_;
	std::optional<Core::FrameOutput> latest_;
	std::deque<Core::MassDelta> mass_delta_queue_;
	bool released_ = false;
};
}
}
