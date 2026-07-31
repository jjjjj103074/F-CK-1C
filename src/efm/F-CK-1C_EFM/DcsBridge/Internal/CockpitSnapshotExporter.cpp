#include "CockpitSnapshotExporter.h"

#include "../../Common/Units.h"
#include "../../DcsIds/CockpitParams.g.h"

namespace
{
constexpr double kFeetPerMeter = 3.280839895013123;
constexpr double kKnotsPerMeterPerSecond = 1.943844492440605;
constexpr double kFeetPerMinutePerMeterPerSecond =
	kFeetPerMeter * 60.0;
}

namespace DcsBridge
{
namespace Internal
{
CockpitSnapshotExporter::CockpitSnapshotExporter(cockpit_param_api api)
	: api_(api),
	slots_({ {
		cockpit_parameter_writer(
			DcsIds::CockpitParams::CockpitSnapshotAvailable),
		cockpit_parameter_writer(
			DcsIds::CockpitParams::CockpitSnapshotRevision),
		cockpit_parameter_writer(
			DcsIds::CockpitParams::CockpitSnapshotTimeS),
		cockpit_parameter_writer(DcsIds::CockpitParams::MaxPowerSwitch),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApMasterEngaged),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApVerticalMode),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApLateralMode),
		cockpit_parameter_writer(
			DcsIds::CockpitParams::ApAutoThrottleEngaged),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApPitchCommand),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApRollCommand),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApThrottleCommand),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApBypassActive),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApTargetAltitudeFt),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApTargetHeadingDeg),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApTargetSpeedKts),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApTargetPitchDeg),
		cockpit_parameter_writer(
			DcsIds::CockpitParams::ApTargetVerticalSpeedFpm),
		cockpit_parameter_writer(
			DcsIds::CockpitParams::ApEngageRejectionReason),
		cockpit_parameter_writer(DcsIds::CockpitParams::ApDisengageReason),
		cockpit_parameter_writer(
			DcsIds::CockpitParams::ApAutoThrottleEngageRejectionReason),
		cockpit_parameter_writer(
			DcsIds::CockpitParams::ApAutoThrottleDisengageReason)
	} })
{
}

void CockpitSnapshotExporter::reset()
{
	std::lock_guard<std::mutex> lock(mutex_);
	for (CockpitParameterEndpoint& slot : slots_)
	{
		slot.reset();
	}
}

CockpitParameterEvents CockpitSnapshotExporter::export_snapshot(
	const Core::CockpitSnapshot& snapshot)
{
	std::lock_guard<std::mutex> lock(mutex_);
	CockpitParameterEvents events;
	events = events.merged(write_parameter(
		Parameter::Available,
		snapshot.status.available ? 1.0 : 0.0));
	events = events.merged(write_parameter(
		Parameter::Revision,
		static_cast<double>(snapshot.status.revision)));
	events = events.merged(write_parameter(
		Parameter::TimeS,
		snapshot.simulation_time_s));
	events = events.merged(write_parameter(
		Parameter::MaxPowerSwitch,
		snapshot.propulsion_test_thrust_cut_requested ? 0.0 : 1.0));
	events = events.merged(export_automatic_flight_control(
		snapshot.automatic_flight_control));
	return events;
}

CockpitParameterEvents
CockpitSnapshotExporter::export_automatic_flight_control(
	const Core::AutomaticFlightControlSnapshot& snapshot)
{
	CockpitParameterEvents events;
#define WRITE_AP(parameter, value) \
	events = events.merged(write_parameter(Parameter::parameter, value))
	WRITE_AP(ApMasterEngaged, snapshot.master_engaged ? 1.0 : 0.0);
	WRITE_AP(ApVerticalMode, static_cast<double>(snapshot.vertical_mode));
	WRITE_AP(ApLateralMode, static_cast<double>(snapshot.lateral_mode));
	WRITE_AP(ApAutoThrottleEngaged,
		snapshot.auto_throttle_engaged ? 1.0 : 0.0);
	WRITE_AP(ApPitchCommand, snapshot.pitch_command_normalized);
	WRITE_AP(ApRollCommand, snapshot.roll_command_normalized);
	WRITE_AP(ApThrottleCommand, snapshot.throttle_command_normalized);
	WRITE_AP(ApBypassActive, snapshot.bypass_active ? 1.0 : 0.0);
	WRITE_AP(ApTargetAltitudeFt,
		snapshot.target_altitude_m * kFeetPerMeter);
	WRITE_AP(ApTargetHeadingDeg,
		snapshot.target_heading_rad * Common::kDegPerRad);
	WRITE_AP(ApTargetSpeedKts,
		snapshot.target_speed_mps * kKnotsPerMeterPerSecond);
	WRITE_AP(ApTargetPitchDeg,
		snapshot.target_pitch_rad * Common::kDegPerRad);
	WRITE_AP(ApTargetVerticalSpeedFpm,
		snapshot.target_vertical_speed_mps *
			kFeetPerMinutePerMeterPerSecond);
	WRITE_AP(ApEngageRejectionReason,
		static_cast<double>(snapshot.autopilot_engage_rejection_reason));
	WRITE_AP(ApDisengageReason,
		static_cast<double>(snapshot.autopilot_disengage_reason));
	WRITE_AP(ApAutoThrottleEngageRejectionReason,
		static_cast<double>(
			snapshot.auto_throttle_engage_rejection_reason));
	WRITE_AP(ApAutoThrottleDisengageReason,
		static_cast<double>(snapshot.auto_throttle_disengage_reason));
#undef WRITE_AP
	return events;
}

CockpitParameterEvents CockpitSnapshotExporter::write_parameter(
	Parameter parameter,
	double value)
{
	CockpitParameterEndpoint& slot =
		slots_[static_cast<std::size_t>(parameter)];
	const CockpitParameterWriteResult write = slot.write_number(api_, value);
	if (write.event.has_value())
	{
		return CockpitParameterEvents{}.with_event(*write.event);
	}
	return {};
}
}
}
