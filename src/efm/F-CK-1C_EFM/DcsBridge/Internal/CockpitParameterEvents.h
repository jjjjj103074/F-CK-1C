#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>

namespace DcsBridge
{
namespace Internal
{
enum class CockpitParameterEventType
{
	Error,
	Recovery
};

struct CockpitParameterEvent
{
	CockpitParameterEventType type = CockpitParameterEventType::Error;
	const char* parameter_name = nullptr;
	const char* reason = nullptr;
	double value = 0.0;
	bool has_value = false;
};

inline constexpr std::size_t kCockpitParameterEventCapacity = 64;

struct CockpitParameterEvents
{
	std::array<CockpitParameterEvent, kCockpitParameterEventCapacity> items = {};
	std::size_t count = 0;

	CockpitParameterEvents with_event(
		const CockpitParameterEvent& event) const
	{
		if (count >= items.size())
		{
			throw std::overflow_error(
				"Cockpit parameter event capacity exceeded.");
		}
		CockpitParameterEvents result = *this;
		result.items[result.count++] = event;
		return result;
	}

	CockpitParameterEvents merged(
		const CockpitParameterEvents& other) const
	{
		CockpitParameterEvents result = *this;
		for (std::size_t index = 0; index < other.count; ++index)
		{
			result = result.with_event(other.items[index]);
		}
		return result;
	}
};
}
}
