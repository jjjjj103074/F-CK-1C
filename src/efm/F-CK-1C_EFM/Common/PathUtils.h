#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Common
{
inline void copy_path(char* out, size_t out_size, const char* value)
{
	if (!out || out_size == 0)
	{
		return;
	}

	snprintf(out, out_size, "%s", value ? value : "");
}

inline void normalize_path_separators(char* path)
{
	if (!path)
	{
		return;
	}

	for (char* p = path; *p; ++p)
	{
		if (*p == '/')
		{
			*p = '\\';
		}
	}
}

inline bool is_path_separator(char c)
{
	return c == '\\' || c == '/';
}

inline bool path_file_exists(const char* path)
{
	FILE* f = fopen(path, "rb");
	if (!f)
	{
		return false;
	}

	fclose(f);
	return true;
}

inline void path_dirname(char* path, size_t path_size)
{
	if (!path || path[0] == '\0')
	{
		return;
	}

	normalize_path_separators(path);

	size_t len = strlen(path);
	while (len > 1 && is_path_separator(path[len - 1]))
	{
		path[--len] = '\0';
	}

	char* last_sep = strrchr(path, '\\');
	if (!last_sep)
	{
		copy_path(path, path_size, ".");
		return;
	}

	if (last_sep == path)
	{
		path[1] = '\0';
		return;
	}

	if (last_sep > path && path[1] == ':' && last_sep == path + 2)
	{
		path[3] = '\0';
		return;
	}

	*last_sep = '\0';
}

inline bool path_has_component_suffix(const char* path, const char* component)
{
	if (!path || !component)
	{
		return false;
	}

	const size_t path_len = strlen(path);
	const size_t component_len = strlen(component);
	if (path_len < component_len)
	{
		return false;
	}

	const char* suffix = path + path_len - component_len;
	if (_stricmp(suffix, component) != 0)
	{
		return false;
	}

	return suffix == path || is_path_separator(*(suffix - 1));
}

inline void build_path(char* out, size_t out_size, const char* base, const char* relative)
{
	if (!out || out_size == 0)
	{
		return;
	}

	if (!base || base[0] == '\0' || strcmp(base, ".") == 0)
	{
		snprintf(out, out_size, "%s", relative ? relative : "");
	}
	else
	{
		const size_t base_len = strlen(base);
		const bool needs_sep = base_len > 0 && !is_path_separator(base[base_len - 1]);
		snprintf(out, out_size, "%s%s%s", base, needs_sep ? "\\" : "", relative ? relative : "");
	}

	normalize_path_separators(out);
}

inline bool resolve_saved_games_logs_dir(char* out, size_t out_size)
{
	const char* userprofile = getenv("USERPROFILE");
	if (!userprofile || userprofile[0] == '\0')
	{
		const char* home_drive = getenv("HOMEDRIVE");
		const char* home_path = getenv("HOMEPATH");
		if (home_drive && home_drive[0] != '\0' && home_path && home_path[0] != '\0')
		{
			char home[1024];
			snprintf(home, sizeof(home), "%s%s", home_drive, home_path);
			build_path(out, out_size, home, "Saved Games\\DCS\\Logs");
			return true;
		}

		return false;
	}

	build_path(out, out_size, userprofile, "Saved Games\\DCS\\Logs");
	return true;
}
}
