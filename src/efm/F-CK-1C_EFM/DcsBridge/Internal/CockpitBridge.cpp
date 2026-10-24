#include "CockpitBridge.h"

#include "../../Common/Angles.h"
#include "../../Common/Units.h"
#include "../../DcsIds/CockpitParams.g.h"

#include <charconv>
#include <cmath>
#include <string_view>
#include <initializer_list>
#include <limits>
#include <optional>

namespace
{
constexpr double kEnabledThreshold = 0.5;
constexpr double kCelsiusZeroInKelvin = 273.15;
constexpr double kMaximumExactInteger = 9007199254740991.0;
constexpr int kNoWeaponStation = -1;

bool is_enabled(double value)
{
	return value > kEnabledThreshold;
}

std::optional<int> to_integer(double value, int minimum)
{
	if (value < static_cast<double>(minimum) || value > static_cast<double>((std::numeric_limits<int>::max)()) ||
	    std::trunc(value) != value)
	{
		return std::nullopt;
	}
	return static_cast<int>(value);
}

std::optional<std::uint64_t> to_revision(double value)
{
	if (value < 0.0 || value > kMaximumExactInteger || std::trunc(value) != value)
	{
		return std::nullopt;
	}
	return static_cast<std::uint64_t>(value);
}

std::optional<Core::RadarMode> to_radar_mode(double value)
{
	const int maximum = static_cast<int>(Core::RadarMode::SingleTargetTrack);
	const std::optional<int> parsed = to_integer(value, 0);
	if (!parsed.has_value() || *parsed > maximum)
	{
		return std::nullopt;
	}
	return static_cast<Core::RadarMode>(*parsed);
}

Core::ObservationInvalidReason to_invalid_reason(double value)
{
	const int maximum = static_cast<int>(Core::ObservationInvalidReason::InvalidPayload);
	const std::optional<int> reason = to_integer(value, 0);
	if (!reason.has_value() || *reason > maximum)
	{
		return Core::ObservationInvalidReason::InvalidNumeric;
	}
	return static_cast<Core::ObservationInvalidReason>(*reason);
}

std::optional<Core::WeaponStationSample> parse_station_record(std::string_view text)
{
	std::array<int, 4> fields{};
	for (int& field : fields)
	{
		const auto delimiter = text.find(';');
		if (delimiter == std::string_view::npos)
			return std::nullopt;
		const auto token = text.substr(0, delimiter);
		const auto parsed = std::from_chars(token.data(), token.data() + token.size(), field);
		if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size() || field < 0)
			return std::nullopt;
		text.remove_prefix(delimiter + 1);
	}
	if (fields[0] > 1 || fields[1] > 9999)
		return std::nullopt;
	if (fields[0] == 0 && (fields[1] != 0 || fields[2] != 0 || fields[3] != 0 || !text.empty()))
		return std::nullopt;
	Core::WeaponStationSample sample;
	sample.present = fields[0] == 1;
	sample.count = fields[1];
	sample.category = fields[2] == 4 ? Core::WeaponCategory::Missile : Core::WeaponCategory::Other;
	sample.role = fields[3] == 7 ? Core::WeaponRole::AirToAir : Core::WeaponRole::Other;
	sample.weapon_identifier = std::string(text);
	return sample;
}

DcsBridge::Internal::CockpitParameterEvents with_endpoint_event(
    const DcsBridge::Internal::CockpitParameterEvents& events,
    const std::optional<DcsBridge::Internal::CockpitParameterEvent>& event)
{
	return event.has_value() ? events.with_event(*event) : events;
}

DcsBridge::Internal::CockpitParameterEvents events_from(
    std::initializer_list<DcsBridge::Internal::CockpitParameterReadResult> reads)
{
	DcsBridge::Internal::CockpitParameterEvents events;
	for (const auto& read : reads)
	{
		events = with_endpoint_event(events, read.event);
	}
	return events;
}

bool all_success(std::initializer_list<DcsBridge::Internal::CockpitParameterReadResult> reads)
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

bool any_invalid_numeric(std::initializer_list<DcsBridge::Internal::CockpitParameterReadResult> reads)
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
    std::initializer_list<DcsBridge::Internal::CockpitParameterReadResult> reads)
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
	return { { cockpit_parameter_writer(CockpitParams::TemperatureC),
		       cockpit_parameter_reader(CockpitParams::PressureAltitudeAvailable),
		       cockpit_parameter_reader(CockpitParams::PressureAltitudeM),
		       cockpit_parameter_reader(CockpitParams::MagneticHeadingAvailable),
		       cockpit_parameter_reader(CockpitParams::MagneticHeadingRad),
		       cockpit_parameter_reader(RawCockpitParams::RadarMode),
		       cockpit_parameter_reader(RawCockpitParams::RadarSttAzimuth),
		       cockpit_parameter_reader(RawCockpitParams::RadarSttElevation),
		       cockpit_parameter_reader(RawCockpitParams::RadarSttRange),
		       cockpit_parameter_reader(RawCockpitParams::RadarSttAzimuthStabilized),
		       cockpit_parameter_reader(RawCockpitParams::RadarSttElevationStabilized),
		       cockpit_parameter_reader(RawCockpitParams::RadarTdcAzimuth),
		       cockpit_parameter_reader(RawCockpitParams::RadarTdcRangeScaled),
		       cockpit_parameter_reader(RawCockpitParams::RadarGateRangeScaled),
		       cockpit_parameter_reader(RawCockpitParams::RadarContact01Azimuth),
		       cockpit_parameter_reader(RawCockpitParams::RadarContact01RangeScaled),
		       cockpit_parameter_reader(RawCockpitParams::WeaponTargetRange),
		       cockpit_parameter_reader(RawCockpitParams::IrDesiredAzimuth),
		       cockpit_parameter_reader(RawCockpitParams::IrDesiredElevation),
		       cockpit_parameter_reader(RawCockpitParams::IrLock),
		       cockpit_parameter_reader(RawCockpitParams::IrTargetAzimuth),
		       cockpit_parameter_reader(RawCockpitParams::IrTargetElevation),
		       cockpit_parameter_reader(CockpitParams::WeaponObservationAvailable),
		       cockpit_parameter_reader(CockpitParams::WeaponObservationRevision),
		       cockpit_parameter_reader(CockpitParams::WeaponObservationInvalidReason),
		       cockpit_parameter_reader(CockpitParams::WeaponObservationSelectedStation),
		       cockpit_parameter_reader(CockpitParams::WeaponObservationScannedStationCount),
		       cockpit_parameter_reader(CockpitParams::WeaponSelectionAckRevision),
		       cockpit_parameter_reader(CockpitParams::WeaponRearmRevision),
		       cockpit_parameter_reader(CockpitParams::McAdapterAvailable),
		       cockpit_parameter_reader(CockpitParams::McAcknowledgedSequence),
		       cockpit_parameter_reader(CockpitParams::RadarState),
		       cockpit_parameter_reader(CockpitParams::McMissileCountAvailable),
		       cockpit_parameter_reader(CockpitParams::McMissileCount),
		       cockpit_parameter_reader(CockpitParams::McGunCountAvailable),
		       cockpit_parameter_reader(CockpitParams::McGunCount) } };
}

CockpitBridge::CockpitBridge(cockpit_param_api api)
    : api_(api),
      slots_(make_parameter_slots()),
      station_slots_({ { cockpit_parameter_reader(DcsIds::CockpitParams::WeaponStation01),
                         cockpit_parameter_reader(DcsIds::CockpitParams::WeaponStation02),
                         cockpit_parameter_reader(DcsIds::CockpitParams::WeaponStation03),
                         cockpit_parameter_reader(DcsIds::CockpitParams::WeaponStation04),
                         cockpit_parameter_reader(DcsIds::CockpitParams::WeaponStation05),
                         cockpit_parameter_reader(DcsIds::CockpitParams::WeaponStation06),
                         cockpit_parameter_reader(DcsIds::CockpitParams::WeaponStation07) } })
{
}

CockpitStepInput CockpitBridge::read_step_input()
{
	std::lock_guard<std::mutex> lock(mutex_);
	CockpitStepInput result;
	read_into(&CockpitBridge::read_pressure_altitude, result.cockpit.pressure_altitude, result.events);
	read_into(&CockpitBridge::read_magnetic_heading, result.cockpit.magnetic_heading, result.events);
	read_into(&CockpitBridge::read_radar, result.cockpit.radar, result.events);
	read_into(&CockpitBridge::read_ir_seeker, result.cockpit.ir_seeker, result.events);
	read_into(&CockpitBridge::read_weapon_stations, result.cockpit.weapon_stations, result.events);
	read_into(&CockpitBridge::read_mission_adapter, result.cockpit.mission_adapter, result.events);
	return result;
}

CockpitValueResult<Core::MissionAdapterObservation> CockpitBridge::read_mission_adapter()
{
	const auto available = read_parameter(Parameter::McAdapterAvailable);
	const auto acknowledged = read_parameter(Parameter::McAcknowledgedSequence);
	const auto radar = read_parameter(Parameter::RadarPowerOn);
	const auto missile_available = read_parameter(Parameter::McMissileCountAvailable);
	const auto missiles = read_parameter(Parameter::McMissileCount);
	const auto gun_available = read_parameter(Parameter::McGunCountAvailable);
	const auto gun = read_parameter(Parameter::McGunCount);
	Core::MissionAdapterObservation result;
	const auto revision = acknowledged.success ? to_revision(acknowledged.value) : std::nullopt;
	result.available =
	    all_success({ available, acknowledged, radar }) && is_enabled(available.value) && revision.has_value();
	result.acknowledged_sequence = revision.value_or(0);
	result.radar_power_on = radar.success && is_enabled(radar.value);
	const auto count = missiles.success ? to_integer(missiles.value, 0) : std::nullopt;
	result.missile_count_available =
	    missile_available.success && is_enabled(missile_available.value) && count && *count <= 12;
	result.missile_count = result.missile_count_available ? *count : 0;
	result.gun_count_available = all_success({ gun_available, gun }) && is_enabled(gun_available.value) &&
	                             gun.value >= 0.0 && gun.value <= 9999.0;
	result.gun_count = result.gun_count_available ? gun.value : 0.0;
	return { result, events_from({ available, acknowledged, radar, missile_available, missiles, gun_available, gun }) };
}

CockpitValueResult<Core::PressureAltitudeObservation> CockpitBridge::read_pressure_altitude()
{
	const auto available = read_parameter(Parameter::PressureAltitudeAvailable);
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
		result.status.invalid_reason = Core::ObservationInvalidReason::NotProvided;
		return { result, events_from(reads) };
	}
	result.pressure_altitude_ft = Common::feet(altitude.value);
	result.status = { true, ++pressure_altitude_revision_, Core::ObservationInvalidReason::None };
	return { result, events_from(reads) };
}

CockpitParameterEvents CockpitBridge::export_temperature(double atmosphere_temperature_k)
{
	std::lock_guard<std::mutex> lock(mutex_);
	CockpitParameterEndpoint& slot = slots_[static_cast<std::size_t>(Parameter::Temperature)];
	const CockpitParameterWriteResult write = slot.write_number(api_, atmosphere_temperature_k - kCelsiusZeroInKelvin);
	return with_endpoint_event({}, write.event);
}

CockpitParameterReadResult CockpitBridge::read_parameter(Parameter parameter)
{
	CockpitParameterEndpoint& slot = slots_[static_cast<std::size_t>(parameter)];
	return slot.read_number(api_);
}

CockpitValueResult<Core::MagneticHeadingObservation> CockpitBridge::read_magnetic_heading()
{
	const auto available = read_parameter(Parameter::MagneticHeadingAvailable);
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
		result.status.invalid_reason = Core::ObservationInvalidReason::NotProvided;
		return { result, events_from(reads) };
	}
	result.magnetic_heading_deg = Common::wrap_heading_deg(Common::deg(heading.value));
	result.status = { true, ++magnetic_heading_revision_, Core::ObservationInvalidReason::None };
	return { result, events_from(reads) };
}

CockpitValueResult<Core::RadarObservation> CockpitBridge::read_radar()
{
	const auto mode = read_parameter(Parameter::RadarMode);
	const auto stt_az = read_parameter(Parameter::RadarSttAzimuth);
	const auto stt_el = read_parameter(Parameter::RadarSttElevation);
	const auto stt_range = read_parameter(Parameter::RadarSttRange);
	const auto stt_az_stab = read_parameter(Parameter::RadarSttAzimuthStabilized);
	const auto stt_el_stab = read_parameter(Parameter::RadarSttElevationStabilized);
	const auto tdc_az = read_parameter(Parameter::RadarTdcAzimuth);
	const auto tdc_range = read_parameter(Parameter::RadarTdcRangeScaled);
	const auto gate_range = read_parameter(Parameter::RadarGateRangeScaled);
	const auto contact_az = read_parameter(Parameter::RadarContact01Azimuth);
	const auto contact_range = read_parameter(Parameter::RadarContact01RangeScaled);
	const auto reads = { mode,   stt_az,    stt_el,     stt_range,  stt_az_stab,  stt_el_stab,
		                 tdc_az, tdc_range, gate_range, contact_az, contact_range };
	const CockpitParameterEvents events = events_from(reads);
	Core::RadarObservation result;
	result.status.revision = radar_revision_;
	if (const auto failure = read_failure_reason(reads))
	{
		result.status.invalid_reason = *failure;
		return { result, events };
	}
	const std::optional<Core::RadarMode> parsed_mode = to_radar_mode(mode.value);
	if (!parsed_mode.has_value())
	{
		result.status.invalid_reason = Core::ObservationInvalidReason::InvalidNumeric;
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
	result.status = { true, ++radar_revision_, Core::ObservationInvalidReason::None };
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
	const auto reads = { range, desired_az, desired_el, lock, target_az, target_el };
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
	result.status = { true, ++ir_seeker_revision_, Core::ObservationInvalidReason::None };
	return { result, events };
}

CockpitValueResult<Core::WeaponStationObservation> CockpitBridge::read_weapon_stations()
{
	CockpitValueResult<Core::WeaponStationObservation> result;
	auto& observation = result.value;
	const auto before = read_parameter(Parameter::WeaponObservationRevision);
	const auto available = read_parameter(Parameter::WeaponObservationAvailable);
	const auto reason = read_parameter(Parameter::WeaponObservationInvalidReason);
	const auto station = read_parameter(Parameter::WeaponObservationSelectedStation);
	const auto scanned = read_parameter(Parameter::WeaponObservationScannedStationCount);
	const auto ack = read_parameter(Parameter::WeaponSelectionAckRevision);
	const auto rearm = read_parameter(Parameter::WeaponRearmRevision);
	const auto reads = { before, available, reason, station, scanned, ack, rearm };
	result.events = events_from(reads);
	if (const auto failure = read_failure_reason(reads))
	{
		observation.status.invalid_reason = *failure;
		return result;
	}
	const auto revision = to_revision(before.value);
	if (!revision || *revision == 0 || *revision % 2 != 0)
	{
		observation.status.invalid_reason = Core::ObservationInvalidReason::InvalidRevision;
		return result;
	}
	observation.status.revision = *revision;
	observation.status.invalid_reason = to_invalid_reason(reason.value);
	if (observation.status.invalid_reason != Core::ObservationInvalidReason::None)
		return result;
	if (!is_enabled(available.value))
	{
		observation.status.invalid_reason = Core::ObservationInvalidReason::NotProvided;
		return result;
	}
	const auto selected = to_integer(station.value, kNoWeaponStation);
	const auto acknowledged = to_revision(ack.value);
	const auto rearmed = to_revision(rearm.value);
	if (!selected || *selected >= static_cast<int>(Core::kWeaponStationCount) || !acknowledged || !rearmed ||
	    scanned.value != static_cast<double>(Core::kWeaponStationCount))
	{
		observation.status.invalid_reason = Core::ObservationInvalidReason::InvalidNumeric;
		return result;
	}
	for (std::size_t i = 0; i < station_slots_.size(); ++i)
	{
		const auto read = station_slots_[i].read_string(api_);
		result.events = with_endpoint_event(result.events, read.event);
		if (!read.success)
		{
			observation.status.invalid_reason = Core::ObservationInvalidReason::ParameterUnavailable;
			return result;
		}
		const auto sample = parse_station_record(read.value);
		if (!sample)
		{
			observation.status.invalid_reason = Core::ObservationInvalidReason::InvalidPayload;
			return result;
		}
		observation.stations[i] = *sample;
	}
	const auto after = read_parameter(Parameter::WeaponObservationRevision);
	result.events = with_endpoint_event(result.events, after.event);
	if (!after.success || after.value != before.value)
	{
		observation.status.invalid_reason = Core::ObservationInvalidReason::InvalidRevision;
		return result;
	}
	observation.selected_station = *selected == kNoWeaponStation ? std::nullopt : selected;
	observation.selection_ack_revision = *acknowledged;
	observation.rearm_revision = *rearmed;
	observation.status.available = true;
	return result;
}
}
}
