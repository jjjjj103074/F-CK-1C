#include "OutputStore.h"

namespace DcsBridge
{
namespace Internal
{
void OutputStore::publish(const Core::FrameOutput& output)
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_ = output;
	released_ = false;
}

std::optional<Core::FrameOutput> OutputStore::read() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return latest_;
}

void OutputStore::mark_released()
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.reset();
	released_ = true;
}

bool OutputStore::is_released() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return released_;
}
}
}
