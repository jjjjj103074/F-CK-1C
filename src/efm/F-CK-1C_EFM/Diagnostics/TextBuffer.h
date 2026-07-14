#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace Diagnostics
{
class TextBufferWriter
{
public:
	TextBufferWriter(char* data, size_t capacity)
		: data_(data), capacity_(capacity)
	{
		data_[0] = '\0';
	}

	void append(const char* format, ...)
	{
		if (size_ >= capacity_ - 1)
		{
			return;
		}
		va_list args;
		va_start(args, format);
		const int count = vsnprintf_s(
			data_ + size_, capacity_ - size_, _TRUNCATE, format, args);
		va_end(args);
		size_ = count >= 0
			? size_ + static_cast<size_t>(count)
			: strnlen_s(data_, capacity_);
	}

	size_t size() const { return size_; }

private:
	char* data_;
	size_t capacity_;
	size_t size_ = 0;
};
}
