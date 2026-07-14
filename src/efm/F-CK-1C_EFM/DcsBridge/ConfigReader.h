#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace DcsBridge
{
constexpr size_t kConfigDocumentCapacity = 8192;

struct ConfigDocument
{
	char content[kConfigDocumentCapacity] = {};
};

struct ConfigStringInput
{
	const char* config_path = nullptr;
	const char* key_name = nullptr;
	const char* default_value = nullptr;
};

struct ConfigStringOutput
{
	char* data = nullptr;
	size_t capacity = 0;
};

struct ConfigStringValue
{
	const char* data = nullptr;
	size_t size = 0;
};

inline bool load_config_document(const char* path, ConfigDocument& document)
{
	FILE* file = fopen(path, "rb");
	if (!file)
	{
		return false;
	}
	const size_t read = fread(
		document.content, 1, sizeof(document.content) - 1, file);
	fclose(file);
	document.content[read] = '\0';
	return true;
}

inline bool config_flag_is_true(const char* config_path, const char* flag_name)
{
	ConfigDocument document;
	if (!load_config_document(config_path, document))
	{
		return false;
	}
	const char* p = strstr(document.content, flag_name);
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
	ConfigDocument document;
	if (!load_config_document(config_path, document))
	{
		return default_value;
	}
	const char* p = strstr(document.content, key_name);
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

inline ConfigStringValue find_config_string(
	const ConfigDocument& document,
	const char* key_name)
{
	const char* entry = strstr(document.content, key_name);
	if (!entry)
	{
		return {};
	}
	const char* line_end = strchr(entry, '\n');
	const char* first_quote = strchr(entry, '"');
	if (!first_quote || (line_end && first_quote > line_end))
	{
		return {};
	}
	const char* second_quote = strchr(first_quote + 1, '"');
	if (!second_quote || (line_end && second_quote > line_end))
	{
		return {};
	}
	return {
		first_quote + 1,
		static_cast<size_t>(second_quote - first_quote - 1)
	};
}

inline void config_string_or_default(
	const ConfigStringInput& input,
	const ConfigStringOutput& output)
{
	if (!output.data || output.capacity == 0)
	{
		return;
	}
	snprintf(
		output.data,
		output.capacity,
		"%s",
		input.default_value ? input.default_value : "");
	ConfigDocument document;
	if (!load_config_document(input.config_path, document))
	{
		return;
	}
	const ConfigStringValue value = find_config_string(document, input.key_name);
	if (!value.data)
	{
		return;
	}
	const size_t copy_len =
		(value.size < output.capacity - 1) ? value.size : output.capacity - 1;
	memcpy(output.data, value.data, copy_len);
	output.data[copy_len] = '\0';
}
}
