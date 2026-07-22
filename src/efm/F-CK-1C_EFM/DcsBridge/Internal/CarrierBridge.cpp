#include "CarrierBridge.h"

namespace
{
constexpr double kLaunchThrottleThreshold = 0.99;
constexpr float kReadyEventPhase = 1.0F;
constexpr float kStartedEventPhase = 2.0F;
constexpr float kFinishedEventPhase = 3.0F;
constexpr float kLaunchStartDelayS = 2.0F;
constexpr float kLaunchAddedVelocityMps = 80.0F;
}

namespace DcsBridge
{
namespace Internal
{
CarrierBridge::CarrierBridge(const CarrierBridgeConfig& config)
	: reference_thrust_N_(config.reference_thrust_N)
{
}

void CarrierBridge::reset()
{
	std::lock_guard<std::mutex> lock(mutex_);
	phase_ = LaunchPhase::Idle;
}

bool CarrierBridge::pop_event(
	const CarrierLaunchInput& input,
	ed_fm_simulation_event& output)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (phase_ != LaunchPhase::Armed ||
		input.left_throttle_output <= kLaunchThrottleThreshold)
	{
		return false;
	}
	output.event_type = ED_FM_EVENT_CARRIER_CATAPULT;
	output.event_params[0] = kReadyEventPhase;
	output.event_params[1] = kLaunchStartDelayS;
	output.event_params[2] = kLaunchAddedVelocityMps;
	output.event_params[3] = static_cast<float>(reference_thrust_N_);
	phase_ = LaunchPhase::Issued;
	return true;
}

bool CarrierBridge::push_event(const ed_fm_simulation_event& input)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (input.event_type != ED_FM_EVENT_CARRIER_CATAPULT)
	{
		return false;
	}
	const float event_phase = input.event_params[0];
	if (event_phase == kReadyEventPhase)
	{
		phase_ = LaunchPhase::Armed;
	}
	else if (event_phase == kStartedEventPhase)
	{
		phase_ = LaunchPhase::Started;
	}
	else if (event_phase == kFinishedEventPhase)
	{
		phase_ = LaunchPhase::Idle;
	}
	return false;
}
}
}
