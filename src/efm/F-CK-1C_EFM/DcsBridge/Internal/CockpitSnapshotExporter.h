#pragma once

#include "CockpitParameterEndpoint.h"
#include "MissionComputerExporter.h"
#include "RadarDisplayExporter.h"
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
	CockpitParameterEvents export_snapshot(const Core::CockpitSnapshot& snapshot,
	                                       const Core::MissionActionBatch& actions = {});

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
		ApPitchAttitudeReference,
		ApVerticalSpeedReference,
		ApBankAngleReference,
		ApThrottleCommand,
		ApBypassActive,
		ApTargetAltitudeFt,
		ApTargetHeadingDeg,
		ApTargetSpeedKts,
		ApTargetPitchDeg,
		ApEngageRejectionReason,
		ApDisengageReason,
		ApAutoThrottleEngageRejectionReason,
		ApAutoThrottleDisengageReason,
		CmInventoryAvailable,
		CmFlareCount,
		CmChaffCount,
		CmAcceptedSequence,
		CmProgram,
		CmProgramState,
		CmCompletedRounds,
		CmExecutedSequence,
		CmFlareReturned,
		CmChaffReturned,
		RadarScanOriginAzimuth,
		RadarScanOriginElevation,
		RadarScanVolumeAzimuth,
		RadarScanVolumeElevation,
		RadarScanMaximumRange,
		RadarScanMinimumRange,
		RadarPitchStabilization,
		RadarBankStabilization,
		RadarPitchBankStabilization,
		RadarAcquisitionAzimuth,
		RadarAcquisitionRange,
		RadarTdcRange,
		RadarTdcAzimuth,
		Count
	};

	CockpitParameterEvents export_radar(const Core::RadarSnapshot& snapshot);
	CockpitParameterEvents export_countermeasures(const Core::CountermeasureSnapshot& snapshot);
	CockpitParameterEvents export_automatic_flight_control(const Core::AutomaticFlightControlSnapshot& snapshot);
	CockpitParameterEvents write_parameter(Parameter parameter, double value);
	const cockpit_param_api api_;
	std::array<CockpitParameterEndpoint, static_cast<std::size_t>(Parameter::Count)> slots_;
	MissionComputerExporter mission_computer_;
	RadarDisplayExporter radar_display_;
	std::uint64_t radar_scan_revision_ = 0;
	std::mutex mutex_;
};
}
}
