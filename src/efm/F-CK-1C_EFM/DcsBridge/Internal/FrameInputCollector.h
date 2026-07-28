#pragma once

#include "../../Core/Contracts/FrameContracts.h"

#include <mutex>

namespace DcsBridge
{
namespace Internal
{
class FrameInputCollector final
{
public:
	FrameInputCollector() = default;
	FrameInputCollector(const FrameInputCollector&) = delete;
	FrameInputCollector& operator=(const FrameInputCollector&) = delete;

	void reset();
	void publish_atmosphere(const Core::AtmosphereInput& sample);
	void publish_surface(const Core::SurfaceInput& sample);
	void publish_mass(const Core::MassStateInput& sample);
	void publish_world_kinematics(const Core::WorldKinematicsInput& sample);
	void publish_body_kinematics(const Core::BodyKinematicsInput& sample);
	bool publish_suspension(const Core::SuspensionFeedbackInput& sample);
	void publish_autopilot(const Core::AutopilotCommand& sample);
	void publish_max_power(const Core::MaxPowerCommand& sample);
	Core::FrameInput snapshot(double dt_s);

private:
	std::mutex mutex_;
	Core::FrameInput latest_;
};
}
}
