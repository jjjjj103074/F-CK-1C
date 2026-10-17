#include "LogFileLifecycle.h"

#include "../../Common/PathUtils.h"

#include <cerrno>
#include <cstdio>
#include <direct.h>

namespace
{
constexpr const char* kLogDirectoryName = "log";
constexpr size_t kFileNameCapacity = 256;

DcsBridge::Internal::LogFilePreparation failure(const DcsBridge::Internal::LogFilePreparation& location,
                                                DcsBridge::Internal::LogFileOperation operation,
                                                int error_code)
{
	DcsBridge::Internal::LogFilePreparation result = location;
	result.failed_operation = operation;
	result.error_code = error_code;
	return result;
}
}

namespace DcsBridge
{
namespace Internal
{
const char* log_file_operation_name(LogFileOperation operation)
{
	switch (operation)
	{
	case LogFileOperation::None:
		return "none";
	case LogFileOperation::ResolvePath:
		return "resolve_path";
	case LogFileOperation::CreateDirectory:
		return "create_directory";
	case LogFileOperation::RemoveOld:
		return "remove_old";
	case LogFileOperation::RotateActive:
		return "rotate_active";
	}
	return "unknown";
}

LogFilePreparation prepare_rotating_log_file(const char* module_root, const char* active_file_name)
{
	LogFilePreparation result;
	if (!module_root || module_root[0] == '\0' || !active_file_name || active_file_name[0] == '\0')
	{
		return failure(result, LogFileOperation::ResolvePath, EINVAL);
	}
	const std::string log_directory = Common::build_path({ module_root, kLogDirectoryName });
	if (_mkdir(log_directory.c_str()) != 0 && errno != EEXIST)
	{
		return failure(result, LogFileOperation::CreateDirectory, errno);
	}
	result.active_path = Common::build_path({ log_directory.c_str(), active_file_name });
	const std::string old_file_name = std::string(active_file_name) + ".old";
	if (old_file_name.size() >= kFileNameCapacity)
	{
		return failure(result, LogFileOperation::ResolvePath, ENAMETOOLONG);
	}
	const std::string old_path = Common::build_path({ log_directory.c_str(), old_file_name.c_str() });
	if (remove(old_path.c_str()) != 0 && errno != ENOENT)
	{
		return failure(result, LogFileOperation::RemoveOld, errno);
	}
	if (rename(result.active_path.c_str(), old_path.c_str()) != 0 && errno != ENOENT)
	{
		return failure(result, LogFileOperation::RotateActive, errno);
	}
	result.ready = true;
	return result;
}
}
}
