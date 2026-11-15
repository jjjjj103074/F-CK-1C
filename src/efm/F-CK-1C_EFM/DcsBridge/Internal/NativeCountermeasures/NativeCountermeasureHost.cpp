#include "NativeCountermeasureProbe.h"

#ifdef _WIN32
#include <windows.h>
#include <array>
#include <limits>

namespace
{
using namespace DcsBridge::Internal;
bool read_memory(std::uintptr_t address, void* target, std::size_t size)
{
	SIZE_T copied = 0;
	return address &&
	       ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), target, size, &copied) &&
	       copied == size;
}
bool read_pointer(std::uintptr_t object, std::size_t offset, std::uintptr_t& value)
{
	return object && object <= (std::numeric_limits<std::uintptr_t>::max)() - offset &&
	       read_memory(object + offset, &value, sizeof(value)) && value;
}
bool executable(std::uintptr_t address)
{
	MEMORY_BASIC_INFORMATION info{};
	if (!address || VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) != sizeof(info))
		return false;
	const DWORD protection = info.Protect & 0xff;
	return info.State == MEM_COMMIT && !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
	       (protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ || protection == PAGE_EXECUTE_READWRITE ||
	        protection == PAGE_EXECUTE_WRITECOPY);
}
bool virtual_method(std::uintptr_t object, std::size_t slot, std::uintptr_t& function)
{
	std::uintptr_t table = 0;
	return read_pointer(object, 0, table) && read_pointer(table, slot, function) && executable(function);
}
struct Resolved
{
	NativeCmSourceStatus status = NativeCmSourceStatus::WaitingForModule;
	std::uintptr_t contexts = 0, payload = 0;
};
Resolved resolve()
{
	Resolved result;
	const auto module = GetModuleHandleW(L"CockpitBase.dll");
	if (!module)
		return result;
	result.status = NativeCmSourceStatus::UnsupportedBuild;
	const auto base = reinterpret_cast<std::uintptr_t>(module);
	IMAGE_DOS_HEADER dos{};
	IMAGE_NT_HEADERS64 pe{};
	if (!read_memory(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0 ||
	    dos.e_lfanew > 1024 * 1024 || !read_memory(base + static_cast<unsigned>(dos.e_lfanew), &pe, sizeof(pe)) ||
	    pe.Signature != IMAGE_NT_SIGNATURE || pe.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
	    pe.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC || pe.FileHeader.TimeDateStamp != 0x6adbad09 ||
	    pe.OptionalHeader.SizeOfImage != 0x68e000)
		return result;
	// 以使用者提供的 DLL 核對 payload 與四個 Lua binding 的完整程式指紋。
	struct Fingerprint
	{
		const char* symbol;
		std::size_t size;
		std::uint64_t hash;
	};
	constexpr Fingerprint fingerprints[] = {
		{ "?c_payload@cockpit@@YAPEAVIwHumanPayload@@XZ", 29, 0xe4153ccf2d04e7d6ULL },
		{ "?l_drop_chaff@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 202, 0x46228e87c2536c9fULL },
		{ "?l_drop_flare@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 202, 0x18198bf9ab4a388aULL },
		{ "?l_get_chaff_count@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 199, 0x872314f529cfb306ULL },
		{ "?l_get_flare_count@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 199, 0x0f4663cb30bfdfa3ULL }
	};
	for (const auto& expected : fingerprints)
	{
		const auto function = reinterpret_cast<std::uintptr_t>(GetProcAddress(module, expected.symbol));
		std::array<unsigned char, 202> bytes{};
		if (!executable(function) || !read_memory(function, bytes.data(), expected.size))
			return result;
		std::uint64_t hash = 14695981039346656037ULL;
		for (std::size_t i = 0; i < expected.size; ++i)
			hash = (hash ^ bytes[i]) * 1099511628211ULL;
		if (hash != expected.hash)
			return result;
	}
	result.contexts =
	    reinterpret_cast<std::uintptr_t>(GetProcAddress(module, "?contexts_ptr@ccCockpitContext@cockpit@@0PAPEAV12@A"));
	result.payload =
	    reinterpret_cast<std::uintptr_t>(GetProcAddress(module, "?c_payload@cockpit@@YAPEAVIwHumanPayload@@XZ"));
	if (result.contexts && result.payload)
		result.status = NativeCmSourceStatus::Available;
	return result;
}
bool call_export(std::uintptr_t function, std::uintptr_t& result, std::uint32_t& exception)
{
	__try
	{
		result = reinterpret_cast<std::uintptr_t>(reinterpret_cast<void*(__cdecl*)()>(function)());
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool call_payload(std::uintptr_t function, std::uintptr_t human, std::uintptr_t& result, std::uint32_t& exception)
{
	__try
	{
		using Function = void*(__fastcall*)(void*);
		result = reinterpret_cast<std::uintptr_t>(reinterpret_cast<Function>(function)(reinterpret_cast<void*>(human)));
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool call_count(std::uintptr_t function, std::uintptr_t payload, int& count, std::uint32_t& exception)
{
	__try
	{
		using Function = int(__fastcall*)(void*, int);
		count = reinterpret_cast<Function>(function)(reinterpret_cast<void*>(payload), -1);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool call_drop(std::uintptr_t function, std::uintptr_t human, std::uint32_t& exception)
{
	__try
	{
		using Function = void(__fastcall*)(void*, int, unsigned char);
		reinterpret_cast<Function>(function)(reinterpret_cast<void*>(human), 1, 255);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
NativeCmInventory observe()
{
	NativeCmInventory result;
	const auto api = resolve();
	result.status = api.status;
	if (api.status != NativeCmSourceStatus::Available)
		return result;
	result.status = NativeCmSourceStatus::WaitingForContext;
	if (!read_pointer(api.contexts, 0, result.context))
		return result;
	result.status = NativeCmSourceStatus::WaitingForHuman;
	if (!read_pointer(result.context, 0x10, result.human))
		return result;
	result.status = NativeCmSourceStatus::UnreadableObject;
	std::uintptr_t get_payload = 0, get_flare = 0, get_chaff = 0;
	if (!virtual_method(result.human, 0x120, get_payload))
		return result;
	result.status = NativeCmSourceStatus::NativeCallFailed;
	if (!call_export(api.payload, result.payload, result.exception) ||
	    !call_payload(get_payload, result.human, result.human_payload, result.exception))
		return result;
	result.status = NativeCmSourceStatus::WaitingForPayload;
	if (!result.payload || !result.human_payload)
		return result;
	result.status = NativeCmSourceStatus::PayloadMismatch;
	result.payload_matches = result.payload == result.human_payload;
	if (!result.payload_matches)
		return result;
	result.status = NativeCmSourceStatus::UnreadableObject;
	if (!virtual_method(result.payload, 0x1a8, get_flare) || !virtual_method(result.payload, 0x1b0, get_chaff) ||
	    !virtual_method(result.human, 0x180, result.flare_function) ||
	    !virtual_method(result.human, 0x178, result.chaff_function))
		return result;
	result.status = NativeCmSourceStatus::NativeCallFailed;
	if (!call_count(get_flare, result.payload, result.flare, result.exception) ||
	    !call_count(get_chaff, result.payload, result.chaff, result.exception))
		return result;
	result.status = NativeCmSourceStatus::InvalidCount;
	if (result.flare < 0 || result.flare > 9999 || result.chaff < 0 || result.chaff > 9999)
		return result;
	result.status = NativeCmSourceStatus::Available;
	result.available = true;
	return result;
}
NativeCmCallResult dispatch(const NativeCmCall& call)
{
	NativeCmCallResult result;
	result.before = observe();
	result.after = result.before;
	if (!result.before.available)
	{
		result.exception = result.before.exception;
		return result;
	}
	const bool flare = call.kind == NativeCmKind::Flare;
	if (!flare && call.kind != NativeCmKind::Chaff)
		return result;
	if ((flare ? result.before.flare : result.before.chaff) == 0)
	{
		result.after.status = NativeCmSourceStatus::NoAmmunition;
		return result;
	}
	// 在這次施放前重新比對玩家物件與所選方法。
	const auto api = resolve();
	std::uintptr_t context = 0, human = 0, function = 0;
	if (api.status != NativeCmSourceStatus::Available || !read_pointer(api.contexts, 0, context) ||
	    context != result.before.context || !read_pointer(context, 0x10, human) || human != result.before.human ||
	    !virtual_method(human, flare ? 0x180 : 0x178, function) ||
	    function != (flare ? result.before.flare_function : result.before.chaff_function))
	{
		result.after.status = NativeCmSourceStatus::ContextChanged;
		result.after.available = false;
		return result;
	}
	result.returned = call_drop(function, human, result.exception);
	if (result.returned)
		result.after = observe();
	else
	{
		result.after.available = false;
		result.after.flare = result.after.chaff = -1;
		result.after.status = NativeCmSourceStatus::NativeCallFailed;
		result.after.exception = result.exception;
	}
	return result;
}
}
#endif
namespace DcsBridge::Internal
{
NativeCmHost make_native_countermeasure_host()
{
#ifdef _WIN32
	return { []
		     {
		         return resolve().status == NativeCmSourceStatus::Available ? 3U : 0U;
		     },
		     dispatch,
		     []
		     {
		         return static_cast<std::uint32_t>(GetCurrentThreadId());
		     },
		     observe };
#else
	return {};
#endif
}
}
