#include "GunAmmunitionSource.h"

#include "../EventLog.h"
#include <cstdio>

#include <windows.h>
#include <array>
#include <optional>
#include <utility>

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
struct GunAmmunitionSource::Implementation
{
	EventLog& log;
	Reader reader;
	bool resolved = false;
	bool call_failed = false;
	double next_read_s = 0;
	NativeGunHost host;
	CallContext calls;
	NativeGunResult result;
	Core::GunAmmunitionObservation observation;
	std::optional<NativeGunStatus> reported_status;

	Implementation(EventLog& event_log, Reader read)
	    : log(event_log),
	      reader(std::move(read))
	{
	}

	void write(double time_s, const char* message) noexcept
	try
	{
		(void)log.write({ EventLevel::Info, time_s, message });
	}
	catch (...)
	{
	}

	void resolve(double time_s)
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
			write(time_s, "gun_ammunition event=host_header_invalid");
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
		char message[256]{};
		std::snprintf(message,
		              sizeof(message),
		              "gun_ammunition event=host pe_timestamp=0x%08x image_size=0x%x "
		              "fingerprint=0x%016llx payload_export=%u matched=%u",
		              static_cast<unsigned>(host.timestamp),
		              static_cast<unsigned>(host.image_size),
		              static_cast<unsigned long long>(host.fingerprint),
		              host.payload_function != 0 ? 1u : 0u,
		              profile && profile->fingerprint == host.fingerprint ? 1u : 0u);
		write(time_s, message);
	}

	void report_status(double time_s)
	{
		if (reported_status == result.status)
			return;
		reported_status = result.status;
		char message[192]{};
		std::snprintf(message,
		              sizeof(message),
		              "gun_ammunition event=status status=%s exception_code=0x%08lx",
		              native_gun_status_name(result.status),
		              static_cast<unsigned long>(calls.exception));
		write(time_s, message);
	}
};

GunAmmunitionSource::GunAmmunitionSource(EventLog& log, Reader reader)
    : implementation_(std::make_unique<Implementation>(log, std::move(reader)))
{
}
GunAmmunitionSource::~GunAmmunitionSource() = default;

void GunAmmunitionSource::reset_flight()
{
	auto& state = *implementation_;
	state.next_read_s = 0;
	state.call_failed = false;
	state.calls = {};
	state.observation = {};
	state.result = {};
	state.resolved = false;
	state.host = {};
	state.reported_status.reset();
}

Core::GunAmmunitionObservation GunAmmunitionSource::observe(double time_s)
{
	auto& state = *implementation_;
	if (time_s < state.next_read_s)
		return state.observation;
	state.next_read_s = time_s + 0.1;
	try
	{
		if (!state.call_failed)
		{
			if (state.reader)
				state.result = state.reader();
			else
			{
				if (!state.resolved)
					state.resolve(time_s);
				if (state.resolved)
				{
					const NativeGunAccess access{ &state.calls, read_process, call_payload, call_mount, call_count };
					state.result = read_native_gun(state.host, access);
				}
			}
			state.call_failed = state.result.status == NativeGunStatus::CallFailed;
		}
	}
	catch (...)
	{
		state.call_failed = true;
		state.result.status = NativeGunStatus::CallFailed;
	}
	if (state.result.status == NativeGunStatus::Available && state.result.count > 9999)
		state.result.status = NativeGunStatus::InvalidCount;
	const bool available = state.result.status == NativeGunStatus::Available;
	const auto reason = available ? Core::ObservationInvalidReason::None
	                    : state.result.status == NativeGunStatus::InvalidCount
	                        ? Core::ObservationInvalidReason::InvalidNumeric
	                        : Core::ObservationInvalidReason::NotProvided;
	state.observation.status = { available, state.observation.status.revision + 1, reason };
	state.observation.count = available ? static_cast<int>(state.result.count) : 0;
	// 觀測先完成，再記錄來源狀態。
	state.report_status(time_s);
	return state.observation;
}
}
