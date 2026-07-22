#pragma once

#include "../../Core/FrameContracts.h"

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
	std::optional<Core::FrameOutput> read() const;
	void mark_released();
	bool is_released() const;

private:
	mutable std::mutex mutex_;
	std::optional<Core::FrameOutput> latest_;
	bool released_ = false;
};
}
}
