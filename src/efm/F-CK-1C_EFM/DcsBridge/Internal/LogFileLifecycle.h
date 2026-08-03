#pragma once

#include <cstddef>

namespace DcsBridge
{
namespace Internal
{
inline constexpr std::size_t kLogFilePathCapacity = 1024;

enum class LogFileOperation
{
	None,
	ResolvePath,
	CreateDirectory,
	RemoveOld,
	RotateActive
};

const char* log_file_operation_name(LogFileOperation operation);

struct LogFilePreparation
{
	char active_path[kLogFilePathCapacity] = {};
	bool ready = false;
	int error_code = 0;
	LogFileOperation failed_operation = LogFileOperation::None;
};

LogFilePreparation prepare_rotating_log_file(
	const char* module_root,
	const char* active_file_name);
}
}
