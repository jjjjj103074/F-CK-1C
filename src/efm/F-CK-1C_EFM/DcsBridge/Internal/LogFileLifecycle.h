#pragma once

#include <string>

namespace DcsBridge
{
namespace Internal
{
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
	std::string active_path;
	bool ready = false;
	int error_code = 0;
	LogFileOperation failed_operation = LogFileOperation::None;
};

LogFilePreparation prepare_rotating_log_file(const char* module_root, const char* active_file_name);
}
}
