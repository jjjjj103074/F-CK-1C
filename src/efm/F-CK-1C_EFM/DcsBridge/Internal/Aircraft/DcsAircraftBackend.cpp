#include "DcsAircraftBackend.h"
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <limits>
namespace
{
using namespace DcsBridge::Internal;
bool read(std::uintptr_t address, void* destination, std::size_t size)
{
	SIZE_T copied = 0;
	return address && size && address <= (std::numeric_limits<std::uintptr_t>::max)() - (size - 1) &&
	       ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), destination, size, &copied) &&
	       copied == size;
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
AircraftModule module()
{
	AircraftModule result;
	result.base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(L"CockpitBase.dll"));
	if (!result.base)
		return result;
	IMAGE_DOS_HEADER dos{};
	IMAGE_NT_HEADERS64 pe{};
	if (!read(result.base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0 ||
	    dos.e_lfanew > 1024 * 1024 || !read(result.base + static_cast<unsigned>(dos.e_lfanew), &pe, sizeof(pe)) ||
	    pe.Signature != IMAGE_NT_SIGNATURE || pe.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
	    pe.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		return result;
	result.valid = true;
	result.timestamp = pe.FileHeader.TimeDateStamp;
	result.image_size = pe.OptionalHeader.SizeOfImage;
	return result;
}
bool payload(std::uintptr_t function, std::uintptr_t& result, std::uint32_t& exception)
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
bool human_payload(std::uintptr_t function, std::uintptr_t human, std::uintptr_t& result, std::uint32_t& exception)
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
bool countermeasure_count(std::uintptr_t function, std::uintptr_t object, int& result, std::uint32_t& exception)
{
	__try
	{
		using Function = int(__fastcall*)(void*, int);
		result = reinterpret_cast<Function>(function)(reinterpret_cast<void*>(object), -1);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool dispense(std::uintptr_t function, std::uintptr_t human, std::uint32_t& exception)
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
bool device(std::uintptr_t function,
            std::uintptr_t object,
            unsigned id,
            std::uintptr_t& result,
            std::uint32_t& exception)
{
	__try
	{
		using Function = void*(__fastcall*)(void*, unsigned);
		result =
		    reinterpret_cast<std::uintptr_t>(reinterpret_cast<Function>(function)(reinterpret_cast<void*>(object), id));
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool object_action(std::uintptr_t function, std::uintptr_t object, std::uint32_t& exception)
{
	__try
	{
		reinterpret_cast<void(__fastcall*)(void*)>(function)(reinterpret_cast<void*>(object));
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool set_float(std::uintptr_t function, std::uintptr_t object, float value, std::uint32_t& exception)
{
	__try
	{
		reinterpret_cast<void(__fastcall*)(void*, float)>(function)(reinterpret_cast<void*>(object), value);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool direction(std::uintptr_t function,
               std::uintptr_t object,
               const std::array<double, 3>& value,
               std::uint32_t& exception)
{
	__try
	{
		reinterpret_cast<void(__fastcall*)(void*, const std::array<double, 3>&)>(
		    function)(reinterpret_cast<void*>(object), value);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool gun_mount(std::uintptr_t function,
               std::uintptr_t object,
               unsigned char index,
               std::uintptr_t& result,
               std::uint32_t& exception)
{
	__try
	{
		using Function = void*(__fastcall*)(void*, unsigned char, unsigned char);
		result = reinterpret_cast<std::uintptr_t>(
		    reinterpret_cast<Function>(function)(reinterpret_cast<void*>(object), index, 255));
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool gun_count(std::uintptr_t function, std::uintptr_t object, unsigned& result, std::uint32_t& exception)
{
	__try
	{
		using Function = unsigned(__fastcall*)(void*);
		result = reinterpret_cast<Function>(function)(reinterpret_cast<void*>(object));
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
}
#endif
namespace DcsBridge::Internal
{
std::uint64_t aircraft_code_fingerprint(const unsigned char* code, std::size_t size)
{
	if (!code || !size)
		return 0;
	std::uint64_t hash = 14695981039346656037ULL;
	for (std::size_t i = 0; i < size; ++i)
		hash = (hash ^ code[i]) * 1099511628211ULL;
	return hash;
}
DcsAircraftBackend make_dcs_aircraft_backend()
{
#ifdef _WIN32
	DcsAircraftBackend result;
	result.device = device;
	result.object_action = object_action;
	result.set_float = set_float;
	result.direction = direction;
	result.module = module;
	result.symbol = [](std::uintptr_t base, const char* name)
	{
		return reinterpret_cast<std::uintptr_t>(GetProcAddress(reinterpret_cast<HMODULE>(base), name));
	};
	result.read = read;
	result.executable = executable;
	result.fingerprint = [](std::uintptr_t address, std::size_t size)
	{
		std::vector<unsigned char> code(size);
		return read(address, code.data(), code.size()) ? aircraft_code_fingerprint(code.data(), code.size()) : 0;
	};
	result.payload = payload;
	result.human_payload = human_payload;
	result.countermeasure_count = countermeasure_count;
	result.dispense = dispense;
	result.gun_mount = gun_mount;
	result.gun_count = gun_count;
	return result;
#else
	return {};
#endif
}
}
