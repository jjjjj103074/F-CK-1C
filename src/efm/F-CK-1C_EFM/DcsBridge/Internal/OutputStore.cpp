#include "OutputStore.h"

namespace DcsBridge
{
namespace Internal
{
void OutputStore::publish(const Core::FrameOutput& output)
{
	std::lock_guard<std::mutex> lock(mutex_);
	publish_locked(output);
}

void OutputStore::publish_start(const Core::FrameOutput& output)
{
	std::lock_guard<std::mutex> lock(mutex_);
	mass_delta_queue_.clear();
	publish_locked(output);
}

void OutputStore::publish_locked(const Core::FrameOutput& output)
{
	latest_ = output;
	if (output.mass_effect.available)
	{
		mass_delta_queue_.push_back(output.mass_effect.delta);
	}
	released_ = false;
}

std::optional<Core::FrameOutput> OutputStore::read() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return latest_;
}

Core::MassDeltaResult OutputStore::take_mass_delta()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (mass_delta_queue_.empty())
	{
		return {};
	}
	const Core::MassDelta delta = mass_delta_queue_.front();
	mass_delta_queue_.pop_front();
	return { true, delta };
}

void OutputStore::mark_released()
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.reset();
	mass_delta_queue_.clear();
	released_ = true;
}

bool OutputStore::is_released() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return released_;
}
}
}
