#include "FrameInputCollector.h"

#include <cstddef>

namespace DcsBridge
{
namespace Internal
{
void FrameInputCollector::reset()
{
	const Core::FrameInput initial;
	std::lock_guard<std::mutex> lock(mutex_);
	latest_ = initial;
}

void FrameInputCollector::publish_atmosphere(const Core::AtmosphereInput& sample)
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.atmosphere = sample;
	latest_.availability.atmosphere = true;
}

void FrameInputCollector::publish_surface(const Core::SurfaceInput& sample)
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.surface = sample;
	latest_.availability.surface = true;
}

void FrameInputCollector::publish_mass(const Core::MassStateInput& sample)
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.mass = sample;
	latest_.availability.mass = true;
}

void FrameInputCollector::publish_world_kinematics(
	const Core::WorldKinematicsInput& sample)
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.world_kinematics = sample;
	latest_.availability.world_kinematics = true;
}

void FrameInputCollector::publish_body_kinematics(
	const Core::BodyKinematicsInput& sample)
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.body_kinematics = sample;
	latest_.availability.body_kinematics = true;
}

bool FrameInputCollector::publish_suspension(const Core::SuspensionFeedbackInput& sample)
{
	if (sample.index < 0 ||
		static_cast<std::size_t>(sample.index) >= Core::kFrameSuspensionWheelCount)
	{
		return false;
	}

	const std::size_t index = static_cast<std::size_t>(sample.index);
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.suspension[index] = sample;
	latest_.availability.suspension[index] = true;
	return true;
}

void FrameInputCollector::publish_autopilot(const Core::AutopilotCommand& sample)
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.autopilot = sample;
}

void FrameInputCollector::publish_max_power(const Core::MaxPowerCommand& sample)
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_.max_power = sample;
}

Core::FrameInput FrameInputCollector::snapshot(double dt_s) const
{
	Core::FrameInput result;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		result = latest_;
	}
	result.dt_s = dt_s;
	return result;
}
}
}
