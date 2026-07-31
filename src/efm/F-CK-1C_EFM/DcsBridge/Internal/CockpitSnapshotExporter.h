#pragma once

#include "CockpitParameterEndpoint.h"
#include "../../Core/Contracts/CockpitContracts.h"
#include "../../include/Cockpit/ccParametersAPI.h"

#include <array>
#include <cstddef>
#include <mutex>

namespace DcsBridge
{
namespace Internal
{
class CockpitSnapshotExporter final
{
public:
	explicit CockpitSnapshotExporter(cockpit_param_api api);

	CockpitSnapshotExporter(const CockpitSnapshotExporter&) = delete;
	CockpitSnapshotExporter& operator=(const CockpitSnapshotExporter&) = delete;

	void reset();
	CockpitParameterEvents export_snapshot(
		const Core::CockpitSnapshot& snapshot);

private:
	enum class Parameter : std::size_t
	{
		Available,
		Revision,
		TimeS,
		MaxPowerSwitch,
		ApMasterEngaged,
		ApVerticalMode,
		ApLateralMode,
		ApAutoThrottleEngaged,
		ApPitchCommand,
		ApRollCommand,
		ApThrottleCommand,
		ApBypassActive,
		ApTargetAltitudeFt,
		ApTargetHeadingDeg,
		ApTargetSpeedKts,
		ApTargetPitchDeg,
		ApTargetVerticalSpeedFpm,
		ApEngageRejectionReason,
		ApDisengageReason,
		ApAutoThrottleEngageRejectionReason,
		ApAutoThrottleDisengageReason,
		Count
	};

	CockpitParameterEvents export_automatic_flight_control(
		const Core::AutomaticFlightControlSnapshot& snapshot);
	CockpitParameterEvents write_parameter(
		Parameter parameter,
		double value);
	const cockpit_param_api api_;
	std::array<
		CockpitParameterEndpoint,
		static_cast<std::size_t>(Parameter::Count)> slots_;
	std::mutex mutex_;
};
}
}
