#include "LogFileLifecycle.h"

#include "../../Common/PathUtils.h"

#include <cerrno>
#include <cstdio>
#include <direct.h>

namespace
{
constexpr const char* kLogDirectoryName = "log";
constexpr size_t kFileNameCapacity = 256;

DcsBridge::Internal::LogFilePreparation failure(
	const DcsBridge::Internal::LogFilePreparation& location,
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
	case LogFileOperation::None: return "none";
	case LogFileOperation::ResolvePath: return "resolve_path";
	case LogFileOperation::CreateDirectory: return "create_directory";
	case LogFileOperation::RemoveOld: return "remove_old";
	case LogFileOperation::RotateActive: return "rotate_active";
	}
	return "unknown";
}

LogFilePreparation prepare_rotating_log_file(
	const char* module_root,
	const char* active_file_name)
{
	LogFilePreparation result;
	if (!module_root || module_root[0] == '\0' ||
		!active_file_name || active_file_name[0] == '\0')
	{
		return failure(result, LogFileOperation::ResolvePath, EINVAL);
	}
	char log_directory[kLogFilePathCapacity];
	Common::build_path(
		{ log_directory, sizeof(log_directory) },
		{ module_root, kLogDirectoryName });
	if (_mkdir(log_directory) != 0 && errno != EEXIST)
	{
		return failure(result, LogFileOperation::CreateDirectory, errno);
	}
	Common::build_path(
		{ result.active_path, sizeof(result.active_path) },
		{ log_directory, active_file_name });
	char old_file_name[kFileNameCapacity];
	const int old_name_length = snprintf(
		old_file_name,
		sizeof(old_file_name),
		"%s.old",
		active_file_name);
	if (old_name_length < 0 || static_cast<size_t>(old_name_length) >= sizeof(old_file_name))
	{
		return failure(result, LogFileOperation::ResolvePath, ENAMETOOLONG);
	}
	char old_path[kLogFilePathCapacity];
	Common::build_path(
		{ old_path, sizeof(old_path) },
		{ log_directory, old_file_name });
	if (remove(old_path) != 0 && errno != ENOENT)
	{
		return failure(result, LogFileOperation::RemoveOld, errno);
	}
	if (rename(result.active_path, old_path) != 0 && errno != ENOENT)
	{
		return failure(result, LogFileOperation::RotateActive, errno);
	}
	result.ready = true;
	return result;
}
}
}
