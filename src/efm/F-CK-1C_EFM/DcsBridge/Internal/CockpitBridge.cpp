#include "CockpitBridge.h"

#include "../../Common/Angles.h"
#include "../../Common/Units.h"
#include "../../DcsIds/CockpitParams.g.h"

#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>

namespace
{
constexpr double kEnabledThreshold = 0.5;
constexpr double kLegacyCockpitTemperatureOffset = 273.0;
constexpr double kMaximumExactInteger = 9007199254740991.0;
constexpr int kNoWeaponStation = -1;

bool is_enabled(double value)
{
	return value > kEnabledThreshold;
}

std::optional<int> to_integer(double value, int minimum)
{
	if (value < static_cast<double>(minimum) ||
		value > static_cast<double>((std::numeric_limits<int>::max)()) ||
		std::trunc(value) != value)
	{
		return std::nullopt;
	}
	return static_cast<int>(value);
}

std::optional<std::uint64_t> to_revision(double value)
{
	if (value < 0.0 ||
		value > kMaximumExactInteger ||
		std::trunc(value) != value)
	{
		return std::nullopt;
	}
	return static_cast<std::uint64_t>(value);
}

std::optional<Core::RadarMode> to_radar_mode(double value)
{
	const int maximum = static_cast<int>(
		Core::RadarMode::SingleTargetTrack);
	const std::optional<int> parsed = to_integer(value, 0);
	if (!parsed.has_value() || *parsed > maximum)
	{
		return std::nullopt;
	}
	return static_cast<Core::RadarMode>(*parsed);
}

Core::ObservationInvalidReason to_invalid_reason(double value)
{
	const int maximum = static_cast<int>(
		Core::ObservationInvalidReason::StationApiError);
	const std::optional<int> reason = to_integer(value, 0);
	if (!reason.has_value() || *reason > maximum)
	{
		return Core::ObservationInvalidReason::InvalidNumeric;
	}
	return static_cast<Core::ObservationInvalidReason>(*reason);
}

DcsBridge::Internal::CockpitParameterEvents with_endpoint_event(
	const DcsBridge::Internal::CockpitParameterEvents& events,
	const std::optional<DcsBridge::Internal::CockpitParameterEvent>& event)
{
	return event.has_value() ? events.with_event(*event) : events;
}

DcsBridge::Internal::CockpitParameterEvents events_from(
	std::initializer_list<
		DcsBridge::Internal::CockpitParameterReadResult> reads)
{
	DcsBridge::Internal::CockpitParameterEvents events;
	for (const auto& read : reads)
	{
		events = with_endpoint_event(events, read.event);
	}
	return events;
}

bool all_success(
	std::initializer_list<
		DcsBridge::Internal::CockpitParameterReadResult> reads)
{
	for (const auto& read : reads)
	{
		if (!read.success)
		{
			return false;
		}
	}
	return true;
}

bool any_invalid_numeric(
	std::initializer_list<
		DcsBridge::Internal::CockpitParameterReadResult> reads)
{
	for (const auto& read : reads)
	{
		if (read.invalid_numeric)
		{
			return true;
		}
	}
	return false;
}

std::optional<Core::ObservationInvalidReason> read_failure_reason(
	std::initializer_list<
		DcsBridge::Internal::CockpitParameterReadResult> reads)
{
	if (any_invalid_numeric(reads))
	{
		return Core::ObservationInvalidReason::InvalidNumeric;
	}
	if (!all_success(reads))
	{
		return Core::ObservationInvalidReason::ParameterUnavailable;
	}
	return std::nullopt;
}
}

namespace DcsBridge
{
namespace Internal
{
CockpitBridge::ParameterSlots CockpitBridge::make_parameter_slots()
{
	using namespace DcsIds;
	return { {
		cockpit_parameter_writer(CockpitParams::TemperatureC),
		cockpit_parameter_reader(CockpitParams::PressureAltitudeAvailable),
		cockpit_parameter_reader(CockpitParams::PressureAltitudeM),
		cockpit_parameter_reader(CockpitParams::MagneticHeadingAvailable),
		cockpit_parameter_reader(CockpitParams::MagneticHeadingRad),
		cockpit_parameter_reader(RawCockpitParams::RadarMode),
		cockpit_parameter_reader(RawCockpitParams::RadarSttAzimuth),
		cockpit_parameter_reader(RawCockpitParams::RadarSttElevation),
		cockpit_parameter_reader(RawCockpitParams::RadarSttRange),
		cockpit_parameter_reader(
			RawCockpitParams::RadarSttAzimuthStabilized),
		cockpit_parameter_reader(
			RawCockpitParams::RadarSttElevationStabilized),
		cockpit_parameter_reader(RawCockpitParams::RadarTdcAzimuth),
		cockpit_parameter_reader(RawCockpitParams::RadarTdcRangeScaled),
		cockpit_parameter_reader(RawCockpitParams::RadarGateRangeScaled),
		cockpit_parameter_reader(RawCockpitParams::RadarContact01Azimuth),
		cockpit_parameter_reader(
			RawCockpitParams::RadarContact01RangeScaled),
		cockpit_parameter_reader(RawCockpitParams::WeaponTargetRange),
		cockpit_parameter_reader(RawCockpitParams::IrDesiredAzimuth),
		cockpit_parameter_reader(RawCockpitParams::IrDesiredElevation),
		cockpit_parameter_reader(RawCockpitParams::IrLock),
		cockpit_parameter_reader(RawCockpitParams::IrTargetAzimuth),
		cockpit_parameter_reader(RawCockpitParams::IrTargetElevation),
		cockpit_parameter_reader(
			CockpitParams::WeaponObservationAvailable),
		cockpit_parameter_reader(
			CockpitParams::WeaponObservationRevision),
		cockpit_parameter_reader(
			CockpitParams::WeaponObservationInvalidReason),
		cockpit_parameter_reader(
			CockpitParams::WeaponObservationAim9Count),
		cockpit_parameter_reader(
			CockpitParams::WeaponObservationSelectedStation),
		cockpit_parameter_reader(
			CockpitParams::WeaponObservationScannedStationCount)
	} };
}

CockpitBridge::CockpitBridge(cockpit_param_api api)
	: api_(api),
	slots_(make_parameter_slots())
{
}

CockpitStepInput CockpitBridge::read_step_input()
{
	std::lock_guard<std::mutex> lock(mutex_);
	const CockpitValueResult<Core::PressureAltitudeObservation> altitude =
		read_pressure_altitude();
	const CockpitValueResult<Core::MagneticHeadingObservation> heading =
		read_magnetic_heading();
	const CockpitValueResult<Core::RadarObservation> radar = read_radar();
	const CockpitValueResult<Core::IrSeekerObservation> ir = read_ir_seeker();
	const CockpitValueResult<Core::WeaponStationObservation> weapon =
		read_weapon_stations();
	CockpitStepInput result;
	result.cockpit = {
		heading.value,
		radar.value,
		ir.value,
		weapon.value,
		altitude.value
	};
	result.events = altitude.events
		.merged(heading.events)
		.merged(radar.events)
		.merged(ir.events)
		.merged(weapon.events);
	return result;
}

CockpitValueResult<Core::PressureAltitudeObservation>
CockpitBridge::read_pressure_altitude()
{
	const auto available =
		read_parameter(Parameter::PressureAltitudeAvailable);
	const auto altitude = read_parameter(Parameter::PressureAltitudeM);
	const auto reads = { available, altitude };
	Core::PressureAltitudeObservation result;
	result.status.revision = pressure_altitude_revision_;
	if (const auto failure = read_failure_reason(reads))
	{
		result.status.invalid_reason = *failure;
		return { result, events_from(reads) };
	}
	if (!is_enabled(available.value))
	{
		result.status.invalid_reason =
			Core::ObservationInvalidReason::NotProvided;
		return { result, events_from(reads) };
	}
	result.pressure_altitude_ft = Common::feet(altitude.value);
	result.status = { true, ++pressure_altitude_revision_,
		Core::ObservationInvalidReason::None };
	return { result, events_from(reads) };
}

CockpitParameterEvents CockpitBridge::export_temperature(double dcs_temperature)
{
	std::lock_guard<std::mutex> lock(mutex_);
	CockpitParameterEndpoint& slot =
		slots_[static_cast<std::size_t>(Parameter::Temperature)];
	const CockpitParameterWriteResult write = slot.write_number(
		api_,
		dcs_temperature + kLegacyCockpitTemperatureOffset);
	return with_endpoint_event({}, write.event);
}

CockpitParameterReadResult CockpitBridge::read_parameter(Parameter parameter)
{
	CockpitParameterEndpoint& slot = slots_[static_cast<std::size_t>(parameter)];
	return slot.read_number(api_);
}

CockpitValueResult<Core::MagneticHeadingObservation>
CockpitBridge::read_magnetic_heading()
{
	const auto available =
		read_parameter(Parameter::MagneticHeadingAvailable);
	const auto heading = read_parameter(Parameter::MagneticHeadingRad);
	const auto reads = { available, heading };
	Core::MagneticHeadingObservation result;
	result.status.revision = magnetic_heading_revision_;
	if (const auto failure = read_failure_reason(reads))
	{
		result.status.invalid_reason = *failure;
		return { result, events_from(reads) };
	}
	if (!is_enabled(available.value))
	{
		result.status.invalid_reason =
			Core::ObservationInvalidReason::NotProvided;
		return { result, events_from(reads) };
	}
	result.magnetic_heading_deg =
		Common::wrap_heading_deg(Common::deg(heading.value));
	result.status = { true, ++magnetic_heading_revision_,
		Core::ObservationInvalidReason::None };
	return { result, events_from(reads) };
}

CockpitValueResult<Core::RadarObservation> CockpitBridge::read_radar()
{
	const auto mode = read_parameter(Parameter::RadarMode);
	const auto stt_az = read_parameter(Parameter::RadarSttAzimuth);
	const auto stt_el = read_parameter(Parameter::RadarSttElevation);
	const auto stt_range = read_parameter(Parameter::RadarSttRange);
	const auto stt_az_stab =
		read_parameter(Parameter::RadarSttAzimuthStabilized);
	const auto stt_el_stab =
		read_parameter(Parameter::RadarSttElevationStabilized);
	const auto tdc_az = read_parameter(Parameter::RadarTdcAzimuth);
	const auto tdc_range = read_parameter(Parameter::RadarTdcRangeScaled);
	const auto gate_range = read_parameter(Parameter::RadarGateRangeScaled);
	const auto contact_az = read_parameter(Parameter::RadarContact01Azimuth);
	const auto contact_range =
		read_parameter(Parameter::RadarContact01RangeScaled);
	const auto reads = { mode, stt_az, stt_el, stt_range, stt_az_stab,
		stt_el_stab, tdc_az, tdc_range, gate_range, contact_az, contact_range };
	const CockpitParameterEvents events = events_from(reads);
	Core::RadarObservation result;
	result.status.revision = radar_revision_;
	if (const auto failure = read_failure_reason(reads))
	{
		result.status.invalid_reason = *failure;
		return { result, events };
	}
	const std::optional<Core::RadarMode> parsed_mode =
		to_radar_mode(mode.value);
	if (!parsed_mode.has_value())
	{
		result.status.invalid_reason =
			Core::ObservationInvalidReason::InvalidNumeric;
		return { result, events };
	}
	result.mode = *parsed_mode;
	result.stt_azimuth_rad = stt_az.value;
	result.stt_elevation_rad = stt_el.value;
	result.stt_range_m = stt_range.value;
	result.stt_azimuth_stabilized_rad = stt_az_stab.value;
	result.stt_elevation_stabilized_rad = stt_el_stab.value;
	result.tdc_azimuth_rad = tdc_az.value;
	result.tdc_range_normalized = tdc_range.value;
	result.gate_range_normalized = gate_range.value;
	result.contact_01_azimuth_rad = contact_az.value;
	result.contact_01_range_normalized = contact_range.value;
	result.status = { true, ++radar_revision_,
		Core::ObservationInvalidReason::None };
	return { result, events };
}

CockpitValueResult<Core::IrSeekerObservation> CockpitBridge::read_ir_seeker()
{
	const auto range = read_parameter(Parameter::WeaponTargetRange);
	const auto desired_az = read_parameter(Parameter::IrDesiredAzimuth);
	const auto desired_el = read_parameter(Parameter::IrDesiredElevation);
	const auto lock = read_parameter(Parameter::IrLock);
	const auto target_az = read_parameter(Parameter::IrTargetAzimuth);
	const auto target_el = read_parameter(Parameter::IrTargetElevation);
	const auto reads = {
		range, desired_az, desired_el, lock, target_az, target_el
	};
	const CockpitParameterEvents events = events_from(reads);
	Core::IrSeekerObservation result;
	result.status.revision = ir_seeker_revision_;
	if (const auto failure = read_failure_reason(reads))
	{
		result.status.invalid_reason = *failure;
		return { result, events };
	}
	result.locked = is_enabled(lock.value);
	result.target_range_m = range.value;
	result.desired_azimuth_rad = desired_az.value;
	result.desired_elevation_rad = desired_el.value;
	result.target_azimuth_rad = target_az.value;
	result.target_elevation_rad = target_el.value;
	result.status = { true, ++ir_seeker_revision_,
		Core::ObservationInvalidReason::None };
	return { result, events };
}

CockpitValueResult<Core::WeaponStationObservation>
CockpitBridge::read_weapon_stations()
{
	const CockpitValueResult<WeaponStationValues> read =
		read_weapon_station_values();
	return {
		build_weapon_station_observation(read.value),
		read.events
	};
}

Core::ObservationStatus CockpitBridge::validate_weapon_station_status(
	const WeaponStationValues& values)
{
	Core::ObservationStatus status;
	if (values.invalid_numeric)
	{
		status.invalid_reason =
			Core::ObservationInvalidReason::InvalidNumeric;
		return status;
	}
	if (!values.readable)
	{
		status.invalid_reason =
			Core::ObservationInvalidReason::ParameterUnavailable;
		return status;
	}
	const std::optional<std::uint64_t> revision =
		to_revision(values.revision);
	if (!revision.has_value())
	{
		status.invalid_reason =
			Core::ObservationInvalidReason::InvalidRevision;
		return status;
	}
	status.revision = *revision;
	const Core::ObservationInvalidReason invalid_reason =
		to_invalid_reason(values.invalid_reason);
	if (invalid_reason != Core::ObservationInvalidReason::None)
	{
		status.invalid_reason = invalid_reason;
		return status;
	}
	if (!is_enabled(values.available))
	{
		status.invalid_reason =
			Core::ObservationInvalidReason::NotProvided;
		return status;
	}
	status.available = true;
	status.invalid_reason = Core::ObservationInvalidReason::None;
	return status;
}

Core::WeaponStationObservation
CockpitBridge::build_weapon_station_observation(
	const WeaponStationValues& values)
{
	Core::WeaponStationObservation result;
	result.status = validate_weapon_station_status(values);
	if (!result.status.available)
	{
		return result;
	}
	const std::optional<int> aim9_count = to_integer(values.aim9_count, 0);
	const std::optional<int> station =
		to_integer(values.selected_station, kNoWeaponStation);
	const std::optional<int> scanned =
		to_integer(values.scanned_station_count, 0);
	if (!aim9_count.has_value() ||
		!station.has_value() ||
		!scanned.has_value())
	{
		result.status.invalid_reason =
			Core::ObservationInvalidReason::InvalidNumeric;
		result.status.available = false;
		return result;
	}
	result.aim9_count = *aim9_count;
	result.selected_station = *station == kNoWeaponStation
		? std::nullopt
		: std::optional<int>(*station);
	result.scanned_station_count = *scanned;
	return result;
}

CockpitValueResult<CockpitBridge::WeaponStationValues>
CockpitBridge::read_weapon_station_values()
{
	const auto available =
		read_parameter(Parameter::WeaponObservationAvailable);
	const auto revision =
		read_parameter(Parameter::WeaponObservationRevision);
	const auto reason =
		read_parameter(Parameter::WeaponObservationInvalidReason);
	const auto count =
		read_parameter(Parameter::WeaponObservationAim9Count);
	const auto station =
		read_parameter(Parameter::WeaponObservationSelectedStation);
	const auto scanned =
		read_parameter(Parameter::WeaponObservationScannedStationCount);
	const auto reads = {
		available, revision, reason, count, station, scanned
	};
	const WeaponStationValues values = {
		all_success(reads),
		any_invalid_numeric(reads),
		available.value,
		revision.value,
		reason.value,
		count.value,
		station.value,
		scanned.value
	};
	return { values, events_from(reads) };
}
}
}
