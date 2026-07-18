#pragma once

#include <cstdio>
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

}
