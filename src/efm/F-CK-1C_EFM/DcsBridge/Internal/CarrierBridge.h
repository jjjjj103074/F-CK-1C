#pragma once

#include "../../include/FM/wHumanCustomPhysicsAPI.h"

#include <mutex>

namespace DcsBridge
{
namespace Internal
{
struct CarrierBridgeConfig
{
	double reference_thrust_N = 0.0;
};

struct CarrierLaunchInput
{
	double left_throttle_output = 0.0;
};

class CarrierBridge final
{
public:
	explicit CarrierBridge(const CarrierBridgeConfig& config);

	CarrierBridge(const CarrierBridge&) = delete;
	CarrierBridge& operator=(const CarrierBridge&) = delete;

	void reset();
	bool pop_event(
		const CarrierLaunchInput& input,
		ed_fm_simulation_event& output);
	bool push_event(const ed_fm_simulation_event& input);

private:
	enum class LaunchPhase
	{
		Idle,
		Armed,
		Issued,
		Started
	};

	const double reference_thrust_N_;
	std::mutex mutex_;
	LaunchPhase phase_ = LaunchPhase::Idle;
};
}
}
