#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace DcsBridge
{
inline bool config_flag_is_true(const char* config_path, const char* flag_name)
{
	FILE* f = fopen(config_path, "rb");
	if (!f)
	{
		return false;
	}

	char content[8192];
	const size_t read = fread(content, 1, sizeof(content) - 1, f);
	fclose(f);
	content[read] = '\0';

	const char* p = strstr(content, flag_name);
	if (!p)
	{
		return false;
	}

	const char* line_end = strchr(p, '\n');
	const char* true_pos = strstr(p, "true");
	return true_pos && (!line_end || true_pos < line_end);
}

inline double config_number_or_default(const char* config_path, const char* key_name, double default_value)
{
	FILE* f = fopen(config_path, "rb");
	if (!f)
	{
		return default_value;
	}

	char content[8192];
	const size_t read = fread(content, 1, sizeof(content) - 1, f);
	fclose(f);
	content[read] = '\0';

	const char* p = strstr(content, key_name);
	if (!p)
	{
		return default_value;
	}

	const char* line_end = strchr(p, '\n');
	const char* equals = strchr(p, '=');
	if (!equals || (line_end && equals > line_end))
	{
		return default_value;
	}

	char* end = nullptr;
	const double value = strtod(equals + 1, &end);
	return (end != equals + 1) ? value : default_value;
}

inline void config_string_or_default(
	const char* config_path,
	const char* key_name,
	const char* default_value,
	char* out,
	size_t out_size)
{
	if (!out || out_size == 0)
	{
		return;
	}

	snprintf(out, out_size, "%s", default_value ? default_value : "");

	FILE* f = fopen(config_path, "rb");
	if (!f)
	{
		return;
	}

	char content[8192];
	const size_t read = fread(content, 1, sizeof(content) - 1, f);
	fclose(f);
	content[read] = '\0';

	const char* p = strstr(content, key_name);
	if (!p)
	{
		return;
	}

	const char* line_end = strchr(p, '\n');
	const char* first_quote = strchr(p, '"');
	if (!first_quote || (line_end && first_quote > line_end))
	{
		return;
	}

	const char* second_quote = strchr(first_quote + 1, '"');
	if (!second_quote || (line_end && second_quote > line_end))
	{
		return;
	}

	const size_t len = (size_t)(second_quote - first_quote - 1);
	const size_t copy_len = (len < out_size - 1) ? len : out_size - 1;
	memcpy(out, first_quote + 1, copy_len);
	out[copy_len] = '\0';
}
}
