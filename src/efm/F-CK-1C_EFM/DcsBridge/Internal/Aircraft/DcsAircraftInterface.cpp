#include "DcsAircraftInterface.h"
#include "AircraftBuildProfiles.h"
#include "DcsAircraftBackend.h"
#include "IrSeekerAccess.h"
#include "../GunAmmunition/NativeGunAccess.h"
#include <limits>
#include <mutex>
#include <utility>

namespace DcsBridge::Internal
{
namespace
{
constexpr auto kPayloadSymbol = "?c_payload@cockpit@@YAPEAVIwHumanPayload@@XZ";
constexpr auto kContextsSymbol = "?contexts_ptr@ccCockpitContext@cockpit@@0PAPEAV12@A";
struct AircraftObjects
{
	AircraftCountermeasureReading reading;
	std::uintptr_t context = 0, human = 0, payload = 0, flare_function = 0, chaff_function = 0;
};
}
struct DcsAircraftInterface::Implementation
{
	DcsAircraftBackend backend;
	std::mutex mutex;
	AircraftModule module;
	NativeGunHost gun;
	IrSeekerAccess seeker;
	bool gun_resolved = false, cm_resolved = false;
	AircraftAccessStatus cm_status = AircraftAccessStatus::WaitingForModule;
	std::uintptr_t contexts = 0, payload_function = 0;
	std::uint32_t exception = 0;
	explicit Implementation(DcsAircraftBackend source)
	    : backend(std::move(source))
	{
	}
	void clear()
	{
		module = {};
		gun = {};
		seeker.reset();
		gun_resolved = cm_resolved = false;
		contexts = payload_function = 0;
		exception = 0;
		cm_status = AircraftAccessStatus::WaitingForModule;
	}
	bool refresh()
	{
		const auto current = backend.module ? backend.module() : AircraftModule{};
		if (current.base != module.base || current.valid != module.valid || current.timestamp != module.timestamp ||
		    current.image_size != module.image_size)
		{
			clear();
			module = current;
		}
		return module.base != 0;
	}
	std::uintptr_t symbol(const char* name)
	{
		return backend.symbol ? backend.symbol(module.base, name) : 0;
	}
	bool executable(std::uintptr_t function)
	{
		return function && backend.executable && backend.executable(function);
	}
	bool read(std::uintptr_t address, void* target, std::size_t size)
	{
		return address && size && address <= (std::numeric_limits<std::uintptr_t>::max)() - (size - 1) &&
		       backend.read && backend.read(address, target, size);
	}
	bool pointer(std::uintptr_t object, std::size_t offset, std::uintptr_t& value)
	{
		return object && object <= (std::numeric_limits<std::uintptr_t>::max)() - offset &&
		       read(object + offset, &value, sizeof(value));
	}
	bool method(std::uintptr_t object, std::size_t offset, std::uintptr_t& function)
	{
		std::uintptr_t table = 0;
		return pointer(object, 0, table) && pointer(table, offset, function) && executable(function);
	}
	void resolve_gun()
	{
		gun_resolved = true;
		if (!module.valid)
			return;
		gun.timestamp = module.timestamp;
		gun.image_size = module.image_size;
		gun.payload_function = symbol(kPayloadSymbol);
		const auto* profile = native_gun_profile(gun.timestamp, gun.image_size);
		if (!profile || profile->fingerprint_rva > gun.image_size ||
		    kNativeGunFingerprintSize > gun.image_size - profile->fingerprint_rva)
			return;
		if (backend.fingerprint)
			gun.fingerprint = backend.fingerprint(module.base + profile->fingerprint_rva, kNativeGunFingerprintSize);
	}
	bool resolve_countermeasures()
	{
		if (!refresh())
		{
			cm_status = AircraftAccessStatus::WaitingForModule;
			return false;
		}
		if (cm_resolved)
			return cm_status == AircraftAccessStatus::Available;
		cm_resolved = true;
		cm_status = AircraftAccessStatus::UnsupportedBuild;
		const auto* profile = aircraft_build_profile(module.timestamp, module.image_size);
		if (!module.valid || !profile)
			return false;
		for (std::size_t index = 0; index < kCountermeasureFunctions.size(); ++index)
		{
			const auto& entry = kCountermeasureFunctions[index];
			const auto function = symbol(entry.symbol);
			if (!function)
			{
				cm_status = AircraftAccessStatus::MissingInterface;
				return false;
			}
			if (!executable(function) || !backend.fingerprint ||
			    backend.fingerprint(function, entry.byte_count) != profile->countermeasure_hashes[index])
				return false;
		}
		contexts = symbol(kContextsSymbol);
		payload_function = symbol(kPayloadSymbol);
		cm_status =
		    contexts && payload_function ? AircraftAccessStatus::Available : AircraftAccessStatus::MissingInterface;
		return cm_status == AircraftAccessStatus::Available;
	}
	AircraftGunReading read_gun()
	{
		AircraftGunReading result;
		if (!refresh())
			return result;
		if (!gun_resolved)
			resolve_gun();
		result.build = { true,
			             gun.timestamp,
			             gun.image_size,
			             gun.fingerprint,
			             gun.payload_function != 0,
			             native_gun_profile(gun.timestamp, gun.image_size) &&
			                 native_gun_profile(gun.timestamp, gun.image_size)->fingerprint == gun.fingerprint,
			             module.valid };
		exception = 0;
		const NativeGunAccess access{
			this,
			[](void* p, std::uintptr_t a, void* t, std::size_t n)
			{
			    return static_cast<Implementation*>(p)->read(a, t, n);
			},
			[](void* p, std::uintptr_t f, std::uintptr_t& v)
			{
			    auto& s = *static_cast<Implementation*>(p);
			    return s.executable(f) && s.backend.payload && s.backend.payload(f, v, s.exception);
			},
			[](void* p, std::uintptr_t f, std::uintptr_t o, unsigned char i, std::uintptr_t& v)
			{
			    auto& s = *static_cast<Implementation*>(p);
			    return s.executable(f) && s.backend.gun_mount && s.backend.gun_mount(f, o, i, v, s.exception);
			},
			[](void* p, std::uintptr_t f, std::uintptr_t o, unsigned& v)
			{
			    auto& s = *static_cast<Implementation*>(p);
			    return s.executable(f) && s.backend.gun_count && s.backend.gun_count(f, o, v, s.exception);
			}
		};
		result.ammunition = read_native_gun(gun, access);
		result.exception = exception;
		return result;
	}
	AircraftObjects observe_countermeasures()
	{
		AircraftObjects object;
		auto& result = object.reading;
		if (!resolve_countermeasures())
		{
			result.status = cm_status;
			return object;
		}
		result.status = AircraftAccessStatus::WaitingForContext;
		if (!pointer(contexts, 0, object.context) || !object.context)
			return object;
		result.status = AircraftAccessStatus::WaitingForHuman;
		if (!pointer(object.context, 0x10, object.human) || !object.human)
			return object;
		result.status = AircraftAccessStatus::UnreadableObject;
		std::uintptr_t get_payload = 0, get_flare = 0, get_chaff = 0, human_payload = 0;
		if (!method(object.human, 0x120, get_payload))
			return object;
		result.status = AircraftAccessStatus::CallFailed;
		if (!backend.payload || !backend.human_payload ||
		    !backend.payload(payload_function, object.payload, result.exception) ||
		    !backend.human_payload(get_payload, object.human, human_payload, result.exception))
			return object;
		result.status = AircraftAccessStatus::WaitingForPayload;
		if (!object.payload || !human_payload)
			return object;
		result.status = AircraftAccessStatus::PayloadMismatch;
		if (object.payload != human_payload)
			return object;
		result.status = AircraftAccessStatus::UnreadableObject;
		if (!method(object.payload, 0x1a8, get_flare) || !method(object.payload, 0x1b0, get_chaff) ||
		    !method(object.human, 0x180, object.flare_function) || !method(object.human, 0x178, object.chaff_function))
			return object;
		result.status = AircraftAccessStatus::CallFailed;
		if (!backend.countermeasure_count ||
		    !backend.countermeasure_count(get_flare, object.payload, result.flare, result.exception) ||
		    !backend.countermeasure_count(get_chaff, object.payload, result.chaff, result.exception))
		{
			result.flare = result.chaff = -1;
			return object;
		}
		result.status = result.flare >= 0 && result.flare <= 9999 && result.chaff >= 0 && result.chaff <= 9999
		                    ? AircraftAccessStatus::Available
		                    : AircraftAccessStatus::InvalidCount;
		return object;
	}
	AircraftCountermeasureResult dispense(AircraftCountermeasure kind)
	{
		AircraftCountermeasureResult result;
		if (kind != AircraftCountermeasure::Flare && kind != AircraftCountermeasure::Chaff)
		{
			result.after.status = AircraftAccessStatus::InvalidRequest;
			return result;
		}
		const auto before = observe_countermeasures();
		result.before = result.after = before.reading;
		if (!result.before.available())
			return result;
		const bool flare = kind == AircraftCountermeasure::Flare;
		if ((flare ? result.before.flare : result.before.chaff) == 0)
		{
			result.after.status = AircraftAccessStatus::NoAmmunition;
			return result;
		}
		const auto previous_module = module;
		if (!refresh() || module.base != previous_module.base || module.timestamp != previous_module.timestamp ||
		    module.image_size != previous_module.image_size || !module.valid)
		{
			result.after.status = AircraftAccessStatus::ContextChanged;
			return result;
		}
		std::uintptr_t context = 0, human = 0, function = 0;
		if (!pointer(contexts, 0, context) || context != before.context || !pointer(context, 0x10, human) ||
		    human != before.human || !method(human, flare ? 0x180 : 0x178, function) ||
		    function != (flare ? before.flare_function : before.chaff_function))
		{
			result.after.status = AircraftAccessStatus::ContextChanged;
			return result;
		}
		result.returned = backend.dispense && backend.dispense(function, human, result.after.exception);
		if (result.returned)
		{
			const auto after = observe_countermeasures();
			result.after = after.reading;
			if (after.context != before.context || after.human != before.human || after.payload != before.payload)
			{
				result.after.status = AircraftAccessStatus::ContextChanged;
				result.after.flare = result.after.chaff = -1;
			}
		}
		else
		{
			result.after.status = AircraftAccessStatus::CallFailed;
			result.after.flare = result.after.chaff = -1;
		}
		return result;
	}
};
DcsAircraftInterface::DcsAircraftInterface()
    : DcsAircraftInterface(make_dcs_aircraft_backend())
{
}
DcsAircraftInterface::DcsAircraftInterface(DcsAircraftBackend backend)
    : implementation_(std::make_unique<Implementation>(std::move(backend)))
{
}
DcsAircraftInterface::~DcsAircraftInterface() = default;
void DcsAircraftInterface::reset_flight()
{
	const std::lock_guard<std::mutex> lock(implementation_->mutex);
	implementation_->clear();
}
AircraftGunReading DcsAircraftInterface::read_gun_ammunition()
{
	const std::lock_guard<std::mutex> lock(implementation_->mutex);
	return implementation_->read_gun();
}
AircraftCountermeasureReading DcsAircraftInterface::read_countermeasures()
{
	const std::lock_guard<std::mutex> lock(implementation_->mutex);
	return implementation_->observe_countermeasures().reading;
}
AircraftCountermeasureResult DcsAircraftInterface::dispense_countermeasure(AircraftCountermeasure kind)
{
	const std::lock_guard<std::mutex> lock(implementation_->mutex);
	return implementation_->dispense(kind);
}
AircraftIrReading DcsAircraftInterface::control_ir_seeker(bool uncaged,
                                                          double azimuth_rad,
                                                          double elevation_rad,
                                                          bool reset)
{
	const std::lock_guard<std::mutex> lock(implementation_->mutex);
	if (!implementation_->refresh())
		return {};
	return implementation_->seeker
	    .apply(implementation_->backend, implementation_->module, uncaged, azimuth_rad, elevation_rad, reset);
}

}
