#include "IrSeekerAccess.h"
#include "AircraftBuildProfiles.h"
#include "../../../DcsIds/CockpitDevices.h"
#include <cmath>
#include <limits>
namespace DcsBridge::Internal
{
void IrSeekerAccess::reset()
{
	*this = {};
}
bool IrSeekerAccess::resolve(const DcsAircraftBackend& backend, const AircraftModule& module)
{
	if (resolved_)
		return status_ == AircraftAccessStatus::Available;
	resolved_ = true;
	status_ = AircraftAccessStatus::UnsupportedBuild;
	const auto* profile = aircraft_build_profile(module.timestamp, module.image_size);
	if (!module.valid || !profile || !backend.symbol || !backend.fingerprint || !backend.executable)
		return false;
	for (std::size_t index = 0; index < kIrSeekerFunctions.size(); ++index)
	{
		const auto& check = kIrSeekerFunctions[index];
		const auto function = backend.symbol(module.base, check.symbol);
		if (!function || !backend.executable(function) ||
		    backend.fingerprint(function, check.byte_count) != profile->infrared_hashes[index])
			return false;
	}
	const auto symbol = [&](const char* name)
	{
		return backend.symbol(module.base, name);
	};
	contexts_ = symbol("?contexts_ptr@ccCockpitContext@cockpit@@0PAPEAV12@A");
	device_ = symbol("?get_wide_device@ccCockpitContext@cockpit@@QEAAPEAVavDevice@2@I@Z");
	// get_wide_device 回傳裝置主物件，其主虛擬表由 ccContextRelatedObject 提供。
	weapon_table_ = symbol("??_7avSimpleWeaponSystem@cockpit@@6BccContextRelatedObject@1@@");
	sensor_table_ = symbol("??_7avSidewinderSeeker@cockpit@@6BavPlatform@1@@");
	cage_ = symbol("?cage@avSidewinderSeeker@cockpit@@MEAAXXZ");
	uncage_ = symbol("?uncage@avIRSensor@cockpit@@IEAAXXZ");
	consent_ = symbol("?setConsentToTrack@avSidewinderSeeker@cockpit@@QEAAXM@Z");
	slave_ = symbol("?toSlave@avSidewinderSeeker@cockpit@@IEAAXXZ");
	direction_ = symbol("?slaveToDirection@avSidewinderSeeker@cockpit@@UEAAXAEBVVec3d@osg@@@Z");
	status_ =
	    contexts_ && device_ && weapon_table_ && sensor_table_ && cage_ && uncage_ && consent_ && slave_ && direction_
	        ? AircraftAccessStatus::Available
	        : AircraftAccessStatus::MissingInterface;
	return status_ == AircraftAccessStatus::Available;
}
AircraftIrReading IrSeekerAccess::apply(const DcsAircraftBackend& b,
                                        const AircraftModule& module,
                                        bool uncaged,
                                        double azimuth,
                                        double elevation,
                                        bool reset_selection)
{
	AircraftIrReading result;
	const auto complete = [&]()
	{
		if (result.exception)
		{
			status_ = AircraftAccessStatus::CallFailed;
			fault_ = result.exception;
		}
		return result;
	};
	if (!std::isfinite(azimuth) || !std::isfinite(elevation) || std::abs(azimuth) > 3.141592653589793 ||
	    std::abs(elevation) > 1.570796326794897)
	{
		result.status = AircraftAccessStatus::InvalidRequest;
		return complete();
	}
	if (!resolve(b, module))
	{
		result.status = status_;
		result.exception = fault_;
		return complete();
	}
	const auto read = [&](std::uintptr_t object, std::size_t offset, void* destination, std::size_t size)
	{
		return object && offset <= (std::numeric_limits<std::uintptr_t>::max)() - object && b.read &&
		       b.read(object + offset, destination, size);
	};
	std::uintptr_t context = 0, weapon = 0, table = 0, sensor = 0;
	result.stage = "context";
	result.status = AircraftAccessStatus::WaitingForContext;
	if (!read(contexts_, 0, &context, sizeof(context)) || !context)
		return complete();
	result.status = AircraftAccessStatus::CallFailed;
	// 設備 4 對應 devices.lua 的穩定 WEAPON_SYSTEM 位置。
	result.stage = "weapon_device";
	if (!b.device || !b.device(device_, context, DcsIds::Devices::WeaponSystem, weapon, result.exception))
		return complete();
	result.status = AircraftAccessStatus::WaitingForPayload;
	if (!weapon)
		return complete();
	result.status = AircraftAccessStatus::UnreadableObject;
	result.stage = "weapon_identity";
	if (!read(weapon, 0, &table, sizeof(table)) || table != weapon_table_)
		return complete();
	result.stage = "seeker_pointer";
	if (!read(weapon, 0x158, &sensor, sizeof(sensor)))
		return complete();
	if (!sensor)
	{
		result.status = AircraftAccessStatus::WaitingForPayload;
		return complete();
	}
	result.stage = "seeker_identity";
	if (!read(sensor, 0, &table, sizeof(table)) || table != sensor_table_)
		return complete();
	result.stage = "native_mode";
	if (!read(sensor, 0x284, &result.native_mode, sizeof(result.native_mode)) || result.native_mode < 0 ||
	    result.native_mode > 5)
		return complete();
	result.stage = "context_confirmation";
	std::uintptr_t current = 0;
	if (!read(contexts_, 0, &current, sizeof(current)) || current != context)
	{
		result.status = AircraftAccessStatus::ContextChanged;
		return complete();
	}
	result.status = AircraftAccessStatus::CallFailed;
	const bool transition = reset_selection || sensor != last_sensor_ || uncaged != last_uncaged_;
	result.stage = "consent_write";
	if (!b.set_float || !b.object_action || !b.direction)
		return complete();
	// 原生 consent 許可持續作用於 update_angles，保存自行追蹤與飛機指向的區別。
	if (!b.set_float(consent_, sensor, uncaged ? 1.0F : 0.0F, result.exception))
		return complete();
	result.stage = "tracking_transition";
	if (transition && !uncaged)
	{
		if (!b.object_action(cage_, sensor, result.exception) || !b.object_action(slave_, sensor, result.exception))
			return complete();
	}
	if (transition && uncaged)
	{
		unsigned target = 0;
		if (!read(sensor, 0x2c8, &target, sizeof(target)))
			return complete();
		// 已捕獲時保留原生目標；空視線時啟動尋標器搜尋。
		if (!target && !b.object_action(uncage_, sensor, result.exception))
			return complete();
	}
	if (!uncaged)
	{
		result.stage = "direction_write";
		const std::array<double, 3> los{ std::cos(elevation) * std::cos(azimuth),
			                             std::sin(elevation),
			                             -std::cos(elevation) * std::sin(azimuth) };
		if (!b.direction(direction_, sensor, los, result.exception))
			return complete();
	}
	result.stage = "readback";
	unsigned char consent = 0;
	if (!read(sensor, 0x42d, &consent, sizeof(consent)) ||
	    !read(sensor, 0x284, &result.native_mode, sizeof(result.native_mode)))
		return complete();
	result.self_track_enabled = consent != 0;
	if (result.self_track_enabled != uncaged)
		return complete();
	last_sensor_ = sensor;
	last_uncaged_ = uncaged;
	result.status = AircraftAccessStatus::Available;
	result.stage = "ready";
	return complete();
}
}
