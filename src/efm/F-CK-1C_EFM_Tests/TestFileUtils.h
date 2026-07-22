#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <windows.h>

namespace TestFiles
{
class TemporaryDirectory final
{
public:
	explicit TemporaryDirectory(const char* prefix = "efm")
	{
		char temp_directory[MAX_PATH];
		char unique_path[MAX_PATH];
		if (GetTempPathA(MAX_PATH, temp_directory) == 0 ||
			GetTempFileNameA(temp_directory, prefix, 0, unique_path) == 0)
		{
			return;
		}
		DeleteFileA(unique_path);
		if (CreateDirectoryA(unique_path, nullptr) != 0)
		{
			path_ = unique_path;
		}
	}

	~TemporaryDirectory()
	{
		std::error_code error;
		std::filesystem::remove_all(path_, error);
	}

	const std::filesystem::path& path() const { return path_; }
	bool valid() const { return !path_.empty(); }

private:
	std::filesystem::path path_;
};

inline void write_text(const std::filesystem::path& path, const char* text)
{
	std::ofstream output(path, std::ios::binary);
	output << text;
}

inline std::string read_text_while_open(const std::filesystem::path& path)
{
	const HANDLE file = CreateFileA(
		path.string().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		return {};
	}
	LARGE_INTEGER size = {};
	if (!GetFileSizeEx(file, &size) || size.QuadPart < 0)
	{
		CloseHandle(file);
		return {};
	}
	std::string content(static_cast<std::size_t>(size.QuadPart), '\0');
	DWORD bytes_read = 0;
	const bool read = content.empty() || ReadFile(
		file,
		content.data(),
		static_cast<DWORD>(content.size()),
		&bytes_read,
		nullptr);
	CloseHandle(file);
	content.resize(read ? bytes_read : 0);
	return content;
}
}
