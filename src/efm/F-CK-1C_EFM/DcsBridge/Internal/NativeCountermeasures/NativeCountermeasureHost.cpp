#include "NativeCountermeasureProbe.h"

#ifdef _WIN32
#include <windows.h>
namespace
{
using Digital = void(__cdecl*)(int);
using Analog = void(__cdecl*)(int, float);
bool dispatch_digital(Digital function, int command, std::uint32_t& exception) noexcept
{
	__try
	{
		function(command);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		exception = GetExceptionCode();
		return false;
	}
}
bool dispatch_analog(Analog function, int command, float value, std::uint32_t& exception) noexcept
{
	__try
	{
		function(command, value);
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
NativeCmHost make_native_countermeasure_host()
{
#ifdef _WIN32
	return { []()
		     {
		         const auto module = GetModuleHandleW(L"CockpitBase.dll");
		         if (!module)
			         return 0U;
		         return (GetProcAddress(module, "ed_cockpit_dispatch_action_digital") ? 1U : 0U) |
		                (GetProcAddress(module, "ed_cockpit_dispatch_action_analog") ? 2U : 0U);
		     },
		     [](const NativeCmCall& call)
		     {
		         NativeCmCallResult result;
		         const auto module = GetModuleHandleW(L"CockpitBase.dll");
		         if (!module)
			         return result;
		         if (call.method == NativeCmMethod::Digital)
		         {
			         const auto function =
			             reinterpret_cast<Digital>(GetProcAddress(module, "ed_cockpit_dispatch_action_digital"));
			         if (function)
				         result.returned = dispatch_digital(function, call.command, result.exception);
		         }
		         else
		         {
			         const auto function =
			             reinterpret_cast<Analog>(GetProcAddress(module, "ed_cockpit_dispatch_action_analog"));
			         if (function)
				         result.returned = dispatch_analog(function, call.command, call.value, result.exception);
		         }
		         return result;
		     },
		     []()
		     {
		         return static_cast<std::uint32_t>(GetCurrentThreadId());
		     } };
#else
	return {};
#endif
}
}
