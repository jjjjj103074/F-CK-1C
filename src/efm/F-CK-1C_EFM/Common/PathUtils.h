#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <share.h>
#include <string>

namespace Common
{
struct PathParts
{
	const char* base = nullptr;
	const char* relative = nullptr;
};

inline void normalize_path_separators(std::string& path)
{
	for (char& character : path)
	{
		if (character == '/')
		{
			character = '\\';
		}
	}
}

inline bool is_path_separator(char c)
{
	return c == '\\' || c == '/';
}

inline bool path_file_exists(const std::string& path)
{
	FILE* f = _fsopen(path.c_str(), "rb", _SH_DENYNO);
	if (!f)
	{
		return false;
	}

	fclose(f);
	return true;
}

inline void path_dirname(std::string& path)
{
	if (path.empty())
	{
		return;
	}

	normalize_path_separators(path);

	while (path.size() > 1 && is_path_separator(path.back()))
	{
		path.pop_back();
	}

	const std::size_t last_sep = path.find_last_of('\\');
	if (last_sep == std::string::npos)
	{
		path = ".";
		return;
	}

	if (last_sep == 0)
	{
		path.resize(1);
		return;
	}

	if (path.size() > 1 && path[1] == ':' && last_sep == 2)
	{
		path.resize(3);
		return;
	}

	path.resize(last_sep);
}

inline bool path_has_component_suffix(const std::string& path, const char* component)
{
	if (!component)
	{
		return false;
	}

	const size_t path_len = path.size();
	const size_t component_len = strlen(component);
	if (path_len < component_len)
	{
		return false;
	}

	const char* suffix = path.c_str() + path_len - component_len;
	if (_stricmp(suffix, component) != 0)
	{
		return false;
	}

	return suffix == path.c_str() || is_path_separator(*(suffix - 1));
}

inline std::string build_path(const PathParts& parts)
{
	std::string path;
	if (parts.base && parts.base[0] != '\0' && strcmp(parts.base, ".") != 0)
	{
		path = parts.base;
		if (!is_path_separator(path.back()))
		{
			path += '\\';
		}
	}
	if (parts.relative)
	{
		path += parts.relative;
	}
	normalize_path_separators(path);
	return path;
}

inline std::unique_ptr<char, decltype(&std::free)> read_environment_variable(const char* name)
{
	char* value = nullptr;
	if (_dupenv_s(&value, nullptr, name) != 0)
	{
		return { nullptr, &std::free };
	}
	return { value, &std::free };
}

inline bool resolve_saved_games_logs_dir(std::string& out)
{
	const auto userprofile = read_environment_variable("USERPROFILE");
	if (!userprofile || userprofile.get()[0] == '\0')
	{
		const auto home_drive = read_environment_variable("HOMEDRIVE");
		const auto home_path = read_environment_variable("HOMEPATH");
		if (home_drive && home_drive.get()[0] != '\0' && home_path && home_path.get()[0] != '\0')
		{
			const std::string home = std::string(home_drive.get()) + home_path.get();
			out = build_path({ home.c_str(), "Saved Games\\DCS\\Logs" });
			return true;
		}

		return false;
	}

	out = build_path({ userprofile.get(), "Saved Games\\DCS\\Logs" });
	return true;
}
}
