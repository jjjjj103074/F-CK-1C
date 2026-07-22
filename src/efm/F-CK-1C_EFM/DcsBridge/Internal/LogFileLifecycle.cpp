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
	const char* operation,
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
LogFilePreparation prepare_rotating_log_file(
	const char* module_root,
	const char* active_file_name)
{
	LogFilePreparation result;
	if (!module_root || module_root[0] == '\0' ||
		!active_file_name || active_file_name[0] == '\0')
	{
		return failure(result, "resolve_path", EINVAL);
	}
	char log_directory[kLogFilePathCapacity];
	Common::build_path(
		{ log_directory, sizeof(log_directory) },
		{ module_root, kLogDirectoryName });
	if (_mkdir(log_directory) != 0 && errno != EEXIST)
	{
		return failure(result, "create_directory", errno);
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
		return failure(result, "resolve_path", ENAMETOOLONG);
	}
	char old_path[kLogFilePathCapacity];
	Common::build_path(
		{ old_path, sizeof(old_path) },
		{ log_directory, old_file_name });
	if (remove(old_path) != 0 && errno != ENOENT)
	{
		return failure(result, "remove_old", errno);
	}
	if (rename(result.active_path, old_path) != 0 && errno != ENOENT)
	{
		return failure(result, "rotate_active", errno);
	}
	result.ready = true;
	return result;
}
}
}
