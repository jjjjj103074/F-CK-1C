#pragma once

#include <cstddef>

namespace DcsBridge
{
namespace Internal
{
inline bool legacy_debug_info_enabled() noexcept
{
	return false;
}

inline std::size_t clear_legacy_debug_watch_buffer(
	char* buffer,
	std::size_t max_length) noexcept
{
	if (buffer != nullptr && max_length > 0)
	{
		buffer[0] = '\0';
	}
	return 0;
}
}
}
