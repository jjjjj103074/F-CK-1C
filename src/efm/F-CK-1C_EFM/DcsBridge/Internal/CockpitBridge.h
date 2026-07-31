#pragma once

#include "CockpitParameterEndpoint.h"
#include "../../Core/Contracts/FrameContracts.h"
#include "../../include/Cockpit/ccParametersAPI.h"

#include <array>
#include <cstddef>
#include <mutex>

namespace DcsBridge
{
namespace Internal
{
struct CockpitStepInput
{
	Core::CockpitObservation cockpit;
	CockpitParameterEvents events;
};

template<typename TValue>
struct CockpitValueResult
{
	TValue value;
	CockpitParameterEvents events;
};

class CockpitBridge final
{
public:
	explicit CockpitBridge(cockpit_param_api api);

	CockpitBridge(const CockpitBridge&) = delete;
	CockpitBridge& operator=(const CockpitBridge&) = delete;

	CockpitStepInput read_step_input();
	CockpitParameterEvents export_temperature(double dcs_temperature);

private:
	enum class Parameter : std::size_t
	{
		Temperature,
		RadarMode,
		RadarSttAzimuth,
		RadarSttElevation,
		RadarSttRange,
		RadarSttAzimuthStabilized,
		RadarSttElevationStabilized,
		RadarTdcAzimuth,
		RadarTdcRangeScaled,
		RadarGateRangeScaled,
		RadarContact01Azimuth,
		RadarContact01RangeScaled,
		WeaponTargetRange,
		IrDesiredAzimuth,
		IrDesiredElevation,
		IrLock,
		IrTargetAzimuth,
		IrTargetElevation,
		WeaponObservationAvailable,
		WeaponObservationRevision,
		WeaponObservationInvalidReason,
		WeaponObservationAim9Count,
		WeaponObservationSelectedStation,
		WeaponObservationScannedStationCount,
		Count
	};

	struct WeaponStationValues
	{
		bool readable = false;
		double available = 0.0;
		double revision = 0.0;
		double invalid_reason = 0.0;
		double aim9_count = 0.0;
		double selected_station = 0.0;
		double scanned_station_count = 0.0;
	};

	using ParameterSlots =
		std::array<
			CockpitParameterEndpoint,
			static_cast<std::size_t>(Parameter::Count)>;

	static ParameterSlots make_parameter_slots();
	CockpitParameterReadResult read_parameter(Parameter parameter);
	CockpitValueResult<Core::RadarObservation> read_radar();
	CockpitValueResult<Core::IrSeekerObservation> read_ir_seeker();
	CockpitValueResult<Core::WeaponStationObservation> read_weapon_stations();
	CockpitValueResult<WeaponStationValues> read_weapon_station_values();
	static Core::ObservationStatus validate_weapon_station_status(
		const WeaponStationValues& values);
	static Core::WeaponStationObservation build_weapon_station_observation(
		const WeaponStationValues& values);

	const cockpit_param_api api_;
	ParameterSlots slots_;
	std::uint64_t radar_revision_ = 0;
	std::uint64_t ir_seeker_revision_ = 0;
	std::mutex mutex_;
};
}
}
