#pragma once

#include <cstdio>
#include <cstring>
#include <direct.h>

namespace Diagnostics
{
inline void append_line(const char* path, const char* msg)
{
	FILE* f = fopen(path, "a");
	if (!f)
	{
		return;
	}

	fprintf(f, "%s\n", msg);
	fclose(f);
}

inline void write_module_debug_log(const char* debug_dir, const char* msg)
{
	_mkdir(debug_dir);

	char path[1024];
	snprintf(path, sizeof(path), "%s\\fck1c_efm_dbg.txt", debug_dir);
	append_line(path, msg);
}

inline void write_suspension_probe_log(const char* log_dir, double fm_clock, const char* msg)
{
	_mkdir(log_dir);

	char path[1200];
	snprintf(path, sizeof(path), "%s\\fck_susp_debug.log", log_dir);

	FILE* f = fopen(path, "a");
	if (!f)
	{
		return;
	}

	fprintf(f, "%.3f %s\n", fm_clock, msg);
	fclose(f);
}

inline void append_tag(char* tags, size_t tags_size, const char* tag)
{
	if (!tags || !tag || tags_size == 0)
	{
		return;
	}

	const size_t used = strlen(tags);
	if (used >= tags_size - 1)
	{
		return;
	}

	snprintf(tags + used, tags_size - used, "%s%s", used > 0 ? "|" : "", tag);
}
}
