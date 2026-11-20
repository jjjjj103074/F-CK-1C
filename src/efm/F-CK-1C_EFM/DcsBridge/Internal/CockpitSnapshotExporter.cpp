#include "CockpitSnapshotExporter.h"

#include "../../Common/Units.h"
#include "../../DcsIds/CockpitParams.g.h"

namespace
{
constexpr double kKnotsPerMeterPerSecond = 1.943844492440605;
}

namespace DcsBridge
{
namespace Internal
{
CockpitSnapshotExporter::CockpitSnapshotExporter(cockpit_param_api api)
    : api_(api),
      slots_({ { cockpit_parameter_writer(DcsIds::CockpitParams::CockpitSnapshotAvailable),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CockpitSnapshotRevision),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CockpitSnapshotTimeS),
                 cockpit_parameter_writer(DcsIds::CockpitParams::MaxPowerSwitch),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApMasterEngaged),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApVerticalMode),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApLateralMode),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApAutoThrottleEngaged),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApPitchAttitudeReference),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApVerticalSpeedReference),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApBankAngleReference),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApThrottleCommand),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApBypassActive),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApTargetAltitudeFt),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApTargetHeadingDeg),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApTargetSpeedKts),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApTargetPitchDeg),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApEngageRejectionReason),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApDisengageReason),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApAutoThrottleEngageRejectionReason),
                 cockpit_parameter_writer(DcsIds::CockpitParams::ApAutoThrottleDisengageReason),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmInventoryAvailable),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmFlareCount),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmChaffCount),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmAcceptedSequence),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmProgram),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmProgramState),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmCompletedRounds),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmExecutedSequence),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmFlareReturned),
                 cockpit_parameter_writer(DcsIds::CockpitParams::CmChaffReturned),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarScanOriginAzimuth),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarScanOriginElevation),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarScanVolumeAzimuth),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarScanVolumeElevation),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarScanMaximumRange),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarScanMinimumRange),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarPitchStabilization),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarBankStabilization),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarPitchBankStabilization),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarAcquisitionAzimuth),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarAcquisitionRange),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarTdcRange),
                 cockpit_parameter_writer(DcsIds::RawCockpitParams::RadarTdcAzimuth) } }),
      mission_computer_(api),
      radar_display_(api)
{
}

void CockpitSnapshotExporter::reset()
{
	std::lock_guard<std::mutex> lock(mutex_);
	mission_computer_.reset();
	radar_display_.reset();
	radar_scan_revision_ = 0;
	for (CockpitParameterEndpoint& slot : slots_)
	{
		slot.reset();
	}
}

CockpitParameterEvents CockpitSnapshotExporter::export_snapshot(const Core::CockpitSnapshot& snapshot,
                                                                const Core::MissionActionBatch& actions)
{
	std::lock_guard<std::mutex> lock(mutex_);
	CockpitParameterEvents events;
	events = events.merged(write_parameter(Parameter::Available, snapshot.status.available ? 1.0 : 0.0));
	events = events.merged(write_parameter(Parameter::Revision, static_cast<double>(snapshot.status.revision)));
	events = events.merged(write_parameter(Parameter::TimeS, snapshot.simulation_time_s));
	events = events.merged(
	    write_parameter(Parameter::MaxPowerSwitch, snapshot.propulsion_test_thrust_cut_requested ? 0.0 : 1.0));
	events = events.merged(export_automatic_flight_control(snapshot.automatic_flight_control));
	// 先寫入雷達掃描參數，再發布含捕獲命令的 MC 傳送視窗。
	events = events.merged(export_radar(snapshot.radar));
	events = events.merged(mission_computer_.export_snapshot(snapshot.combat_avionics, actions));
	events = events.merged(export_countermeasures(snapshot.countermeasures));
	events = events.merged(radar_display_.export_snapshot(snapshot.display.radar));
	return events;
}

CockpitParameterEvents CockpitSnapshotExporter::export_radar(const Core::RadarSnapshot& snapshot)
{
	CockpitParameterEvents events;
	const auto& scan = snapshot.scan;
	if (!scan.enabled)
		return events;
	bool complete = true;
	const auto write = [&](Parameter parameter, double value)
	{
		const auto result = slots_[static_cast<std::size_t>(parameter)].write_number(api_, value);
		if (result.event)
			events = events.with_event(*result.event);
		complete = result.success && complete;
	};
	// RWS 的天線中心於切入模式時初始化，後續沿用原生天線俯仰操作。
	if (!scan.attitude_stabilized || radar_scan_revision_ != scan.revision)
	{
		write(Parameter::RadarScanOriginAzimuth, scan.azimuth_center_rad);
		write(Parameter::RadarScanOriginElevation, scan.elevation_center_rad);
	}
	write(Parameter::RadarScanVolumeAzimuth, scan.azimuth_width_rad);
	write(Parameter::RadarScanVolumeElevation, scan.elevation_height_rad);
	write(Parameter::RadarScanMaximumRange, scan.maximum_search_range_m);
	write(Parameter::RadarScanMinimumRange, 0.0);
	write(Parameter::RadarPitchStabilization, scan.attitude_stabilized ? 1.0 : 0.0);
	write(Parameter::RadarBankStabilization, scan.attitude_stabilized ? 1.0 : 0.0);
	write(Parameter::RadarPitchBankStabilization, scan.attitude_stabilized ? 1.0 : 0.0);
	write(Parameter::RadarAcquisitionAzimuth, scan.acquisition_azimuth_width_rad);
	write(Parameter::RadarAcquisitionRange, scan.acquisition_range_window_m);
	if (scan.override_tdc)
	{
		write(Parameter::RadarTdcAzimuth, scan.tdc_azimuth_rad);
		write(Parameter::RadarTdcRange, scan.tdc_range_m);
	}
	if (complete)
		radar_scan_revision_ = scan.revision;
	return events;
}

CockpitParameterEvents CockpitSnapshotExporter::export_countermeasures(const Core::CountermeasureSnapshot& s)
{
	CockpitParameterEvents events;
#define WRITE_CM(parameter, value) events = events.merged(write_parameter(Parameter::parameter, value))
	WRITE_CM(CmInventoryAvailable, s.observation.inventory.available ? 1.0 : 0.0);
	WRITE_CM(CmFlareCount, static_cast<double>(s.observation.inventory.flare));
	WRITE_CM(CmChaffCount, static_cast<double>(s.observation.inventory.chaff));
	WRITE_CM(CmAcceptedSequence, static_cast<double>(s.accepted_sequence));
	WRITE_CM(CmProgram, static_cast<double>(s.program));
	WRITE_CM(CmProgramState, static_cast<double>(s.state));
	WRITE_CM(CmCompletedRounds, static_cast<double>(s.completed_rounds));
	WRITE_CM(CmExecutedSequence, static_cast<double>(s.observation.execution.sequence));
	WRITE_CM(CmFlareReturned, s.observation.execution.flare_returned ? 1.0 : 0.0);
	WRITE_CM(CmChaffReturned, s.observation.execution.chaff_returned ? 1.0 : 0.0);
#undef WRITE_CM
	return events;
}

CockpitParameterEvents CockpitSnapshotExporter::export_automatic_flight_control(
    const Core::AutomaticFlightControlSnapshot& snapshot)
{
	CockpitParameterEvents events;
#define WRITE_AP(parameter, value) events = events.merged(write_parameter(Parameter::parameter, value))
	WRITE_AP(ApMasterEngaged, snapshot.master_engaged ? 1.0 : 0.0);
	WRITE_AP(ApVerticalMode, static_cast<double>(snapshot.vertical_mode));
	WRITE_AP(ApLateralMode, static_cast<double>(snapshot.lateral_mode));
	WRITE_AP(ApAutoThrottleEngaged, snapshot.auto_throttle_engaged ? 1.0 : 0.0);
	WRITE_AP(ApPitchAttitudeReference, snapshot.pitch_attitude_reference_rad);
	WRITE_AP(ApVerticalSpeedReference, Common::metres(snapshot.vertical_speed_reference_ft_s));
	WRITE_AP(ApBankAngleReference, snapshot.bank_angle_reference_rad);
	WRITE_AP(ApThrottleCommand, snapshot.throttle_command_normalized);
	WRITE_AP(ApBypassActive, snapshot.bypass_active ? 1.0 : 0.0);
	WRITE_AP(ApTargetAltitudeFt, snapshot.target_altitude_ft);
	WRITE_AP(ApTargetHeadingDeg, static_cast<double>(snapshot.target_heading_deg));
	WRITE_AP(ApTargetSpeedKts, snapshot.target_speed_mps * kKnotsPerMeterPerSecond);
	WRITE_AP(ApTargetPitchDeg, snapshot.target_pitch_rad * Common::kDegPerRad);
	WRITE_AP(ApEngageRejectionReason, static_cast<double>(snapshot.autopilot_engage_rejection_reason));
	WRITE_AP(ApDisengageReason, static_cast<double>(snapshot.autopilot_disengage_reason));
	WRITE_AP(ApAutoThrottleEngageRejectionReason, static_cast<double>(snapshot.auto_throttle_engage_rejection_reason));
	WRITE_AP(ApAutoThrottleDisengageReason, static_cast<double>(snapshot.auto_throttle_disengage_reason));
#undef WRITE_AP
	return events;
}

CockpitParameterEvents CockpitSnapshotExporter::write_parameter(Parameter parameter, double value)
{
	CockpitParameterEndpoint& slot = slots_[static_cast<std::size_t>(parameter)];
	const CockpitParameterWriteResult write = slot.write_number(api_, value);
	if (write.event.has_value())
	{
		return CockpitParameterEvents{}.with_event(*write.event);
	}
	return {};
}
}
}
