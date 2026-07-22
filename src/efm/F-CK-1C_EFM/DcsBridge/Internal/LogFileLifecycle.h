#pragma once

#include <cstddef>

namespace DcsBridge
{
namespace Internal
{
inline constexpr std::size_t kLogFilePathCapacity = 1024;

struct LogFilePreparation
{
	char active_path[kLogFilePathCapacity] = {};
	bool ready = false;
	int error_code = 0;
	const char* failed_operation = nullptr;
};

LogFilePreparation prepare_rotating_log_file(
	const char* module_root,
	const char* active_file_name);
}
}
