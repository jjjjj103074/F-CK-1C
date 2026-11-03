#include "NativeGunProbe.h"

#include "NativeGunAccess.h"
#include "EventLog.h"
#include "../../ThirdParty/nlohmann/json.hpp"

#include <windows.h>
#include <array>
#include <filesystem>
#include <limits>

namespace
{
using namespace DcsBridge::Internal;

struct CallContext
{
	DWORD exception = 0;
};

bool read_process(void*, std::uintptr_t address, void* destination, std::size_t size)
{
	SIZE_T copied = 0;
	return address != 0 &&
	       ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), destination, size, &copied) &&
	       copied == size;
}

bool executable(std::uintptr_t address)
{
	MEMORY_BASIC_INFORMATION info{};
	if (address == 0 || VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) != sizeof(info))
		return false;
	const DWORD protection = info.Protect & 0xff;
	return info.State == MEM_COMMIT && (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) == 0 &&
	       (protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ || protection == PAGE_EXECUTE_READWRITE ||
	        protection == PAGE_EXECUTE_WRITECOPY);
}

// 原生呼叫以 SEH 邊界記錄例外代碼。
bool call_payload(void* context, std::uintptr_t function, std::uintptr_t& value)
{
	if (!executable(function))
		return false;
	__try
	{
		value = reinterpret_cast<std::uintptr_t>(reinterpret_cast<void*(__cdecl*)()>(function)());
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static_cast<CallContext*>(context)->exception = GetExceptionCode();
		return false;
	}
}

bool call_mount(void* context,
                std::uintptr_t function,
                std::uintptr_t payload,
                unsigned char index,
                std::uintptr_t& value)
{
	if (!executable(function))
		return false;
	__try
	{
		using Function = void*(__fastcall*)(void*, unsigned char, unsigned char);
		value = reinterpret_cast<std::uintptr_t>(
		    reinterpret_cast<Function>(function)(reinterpret_cast<void*>(payload), index, 255));
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static_cast<CallContext*>(context)->exception = GetExceptionCode();
		return false;
	}
}

bool call_count(void* context, std::uintptr_t function, std::uintptr_t gun, unsigned& value)
{
	if (!executable(function))
		return false;
	__try
	{
		using Function = unsigned(__fastcall*)(void*);
		value = reinterpret_cast<Function>(function)(reinterpret_cast<void*>(gun));
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static_cast<CallContext*>(context)->exception = GetExceptionCode();
		return false;
	}
}
}

namespace DcsBridge::Internal
{
struct NativeGunProbe::Implementation
{
	EventLog& log;
	bool enabled;
	bool resolved = false;
	bool pending = false;
	bool call_failed = false;
	double next_read = 0;
	NativeGunHost host;
	CallContext calls;
	NativeGunResult result;
	Core::GunAmmunitionObservation observation;
	std::string module_path;

	Implementation(const std::string& root, EventLog& event_log)
	    : log(event_log),
	      enabled(false)
	{
		std::error_code error;
		enabled = std::filesystem::is_regular_file(std::filesystem::path(root) / "FM/native-gun-probe.enabled", error);
	}

	void write(double time, nlohmann::json record)
	{
		record["probe"] = "native-gun-r1";
		const std::string text = "native_gun_probe " + record.dump();
		(void)log.write({ EventLevel::Info, time, text.c_str() });
	}

	void resolve(double time)
	{
		const HMODULE module = GetModuleHandleW(L"CockpitBase.dll");
		if (module == nullptr)
			return;
		resolved = true;
		const auto base = reinterpret_cast<std::uintptr_t>(module);
		IMAGE_DOS_HEADER dos{};
		IMAGE_NT_HEADERS64 pe{};
		if (!read_process(nullptr, base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0 ||
		    dos.e_lfanew > 1024 * 1024 ||
		    !read_process(nullptr, base + static_cast<unsigned>(dos.e_lfanew), &pe, sizeof(pe)) ||
		    pe.Signature != IMAGE_NT_SIGNATURE || pe.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
		    pe.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		{
			result.status = NativeGunStatus::UnsupportedBuild;
			write(time, { { "event", "host_header_invalid" } });
			return;
		}
		host.timestamp = pe.FileHeader.TimeDateStamp;
		host.image_size = pe.OptionalHeader.SizeOfImage;
		host.payload_function =
		    reinterpret_cast<std::uintptr_t>(GetProcAddress(module, "?c_payload@cockpit@@YAPEAVIwHumanPayload@@XZ"));
		const auto* profile = native_gun_profile(host.timestamp, host.image_size);
		if (profile && profile->fingerprint_rva <= host.image_size &&
		    kNativeGunFingerprintSize <= host.image_size - profile->fingerprint_rva)
		{
			std::array<unsigned char, kNativeGunFingerprintSize> code{};
			if (read_process(nullptr, base + profile->fingerprint_rva, code.data(), code.size()))
				host.fingerprint = native_gun_fingerprint(code.data(), code.size());
		}
		std::wstring path(32768, L'\0');
		const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
		if (length > 0 && length < path.size())
		{
			const int size = WideCharToMultiByte(CP_UTF8,
			                                     WC_ERR_INVALID_CHARS,
			                                     path.data(),
			                                     static_cast<int>(length),
			                                     nullptr,
			                                     0,
			                                     nullptr,
			                                     nullptr);
			if (size > 0)
			{
				module_path.resize(static_cast<std::size_t>(size));
				if (WideCharToMultiByte(CP_UTF8,
				                        WC_ERR_INVALID_CHARS,
				                        path.data(),
				                        static_cast<int>(length),
				                        module_path.data(),
				                        size,
				                        nullptr,
				                        nullptr) != size)
					module_path.clear();
			}
		}
		write(time,
		      { { "event", "host" },
		        { "module_path", module_path },
		        { "pe_timestamp", host.timestamp },
		        { "image_size", host.image_size },
		        { "fingerprint", host.fingerprint },
		        { "payload_export", host.payload_function != 0 },
		        { "matched", profile && profile->fingerprint == host.fingerprint } });
	}
};

NativeGunProbe::NativeGunProbe(const std::string& module_root, EventLog& log)
    : implementation_(std::make_unique<Implementation>(module_root, log))
{
}
NativeGunProbe::~NativeGunProbe() = default;

bool NativeGunProbe::enabled() const noexcept
{
	return implementation_->enabled;
}

void NativeGunProbe::reset_flight()
{
	auto& state = *implementation_;
	state.next_read = 0;
	state.pending = false;
	state.call_failed = false;
	state.calls = {};
	state.observation = {};
	state.result = {};
	state.resolved = false;
	state.host = {};
}

Core::GunAmmunitionObservation NativeGunProbe::observe(double time_s)
try
{
	auto& state = *implementation_;
	if (!state.enabled || time_s < state.next_read)
		return state.observation;
	state.next_read = time_s + 0.1;
	state.pending = true;
	if (!state.resolved)
		state.resolve(time_s);
	if (state.resolved && !state.call_failed)
	{
		const NativeGunAccess access{ &state.calls, read_process, call_payload, call_mount, call_count };
		state.result = read_native_gun(state.host, access);
		state.call_failed = state.result.status == NativeGunStatus::CallFailed;
	}
	state.observation.status = { state.result.status == NativeGunStatus::Available,
		                         state.observation.status.revision + 2,
		                         state.result.status == NativeGunStatus::Available
		                             ? Core::ObservationInvalidReason::None
		                             : Core::ObservationInvalidReason::NotProvided };
	state.observation.count = state.observation.status.available ? static_cast<int>(state.result.count) : 0;
	return state.observation;
}

catch (...)
{
	auto& state = *implementation_;
	state.call_failed = true;
	state.result.status = NativeGunStatus::CallFailed;
	state.observation.status.available = false;
	state.observation.status.invalid_reason = Core::ObservationInvalidReason::NotProvided;
	(void)state.log.write({ EventLevel::Error, time_s, "native_gun_probe diagnostic_exception" });
	return state.observation;
}

void NativeGunProbe::record_mc(double time_s, const Core::CombatAvionicsSnapshot& snapshot)
try
{
	auto& state = *implementation_;
	if (!state.enabled || !state.pending)
		return;
	state.pending = false;
	const auto& gun = snapshot.stores.gun_ammunition;
	state.write(time_s,
	            { { "event", "sample" },
	              { "status", native_gun_status_name(state.result.status) },
	              { "time_s", time_s },
	              { "revision", state.observation.status.revision },
	              { "raw_available", state.observation.status.available },
	              { "raw_count",
	                state.observation.status.available ? nlohmann::json(state.result.count) : nlohmann::json(nullptr) },
	              { "mounts", state.result.mounts },
	              { "exception_code", state.calls.exception },
	              { "mc_available", gun.observation.available },
	              { "mc_count", gun.count ? nlohmann::json(*gun.count) : nlohmann::json(nullptr) } });
}
catch (...)
{
	(void)implementation_->log.write({ EventLevel::Error, time_s, "native_gun_probe publication_exception" });
}
}
