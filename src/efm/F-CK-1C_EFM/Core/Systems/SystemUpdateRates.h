#pragma once

#include <cstdint>

namespace Core
{
namespace Systems
{
// Reference: NASA F-16XL DFLCS control laws ran at 64 cycles per second.
// This is F-16XL evidence, not a confirmed F-CK-1C device update rate.
inline constexpr std::uint32_t kF16XlDflcsReferenceUpdateRateHz = 64;

// Project-defined fallback: use 64 Hz where no reliable F-16 or F-CK-1C
// device-specific update rate has been identified.
inline constexpr std::uint32_t kProjectDefinedFallbackUpdateRateHz = 64;
}
}
