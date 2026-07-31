#pragma once

#include "CockpitParameterEvents.h"
#include "../../include/Cockpit/ccParametersAPI.h"

#include <cmath>
#include <cstring>
#include <optional>

namespace DcsBridge
{
namespace Internal
{
enum class CockpitParameterAccess
{
	Read,
	Write
};

struct CockpitParameterReadResult
{
	bool success = false;
	double value = 0.0;
	std::optional<CockpitParameterEvent> event;
};

struct CockpitParameterWriteResult
{
	bool success = false;
	std::optional<CockpitParameterEvent> event;
};

class CockpitParameterEndpoint final
{
public:
	CockpitParameterEndpoint(
		const char* name = nullptr,
		CockpitParameterAccess access = CockpitParameterAccess::Read)
		: name_(name),
		access_(access)
	{
	}

	void reset()
	{
		availability_ = Availability::Unknown;
		failure_reason_ = nullptr;
	}

	CockpitParameterReadResult read_number(const cockpit_param_api& api)
	{
		if (access_ != CockpitParameterAccess::Read)
		{
			return { false, 0.0, record_failure({ "wrong_access" }) };
		}
		if (!ensure_handle(api))
		{
			return { false, 0.0, record_failure({ "missing_handle" }) };
		}
		if (api.pfn_ed_cockpit_parameter_value_to_number == nullptr)
		{
			return { false, 0.0, record_failure({ "api_unavailable" }) };
		}
		double value = 0.0;
		if (!api.pfn_ed_cockpit_parameter_value_to_number(
			handle_, value, false))
		{
			return { false, 0.0, record_failure({ "unreadable_value" }) };
		}
		if (!std::isfinite(value))
		{
			return {
				false,
				0.0,
				record_failure({ "invalid_numeric", value, true })
			};
		}
		return { true, value, record_available() };
	}

	CockpitParameterWriteResult write_number(
		const cockpit_param_api& api,
		double value)
	{
		if (access_ != CockpitParameterAccess::Write)
		{
			return { false, record_failure({ "wrong_access" }) };
		}
		if (!std::isfinite(value))
		{
			return {
				false,
				record_failure({ "invalid_numeric", value, true })
			};
		}
		if (!ensure_handle(api))
		{
			return { false, record_failure({ "missing_handle" }) };
		}
		if (api.pfn_ed_cockpit_update_parameter_with_number == nullptr)
		{
			return { false, record_failure({ "api_unavailable" }) };
		}
		api.pfn_ed_cockpit_update_parameter_with_number(handle_, value);
		return { true, record_available() };
	}

private:
	struct Failure
	{
		const char* reason = nullptr;
		double value = 0.0;
		bool has_value = false;
	};

	enum class Availability
	{
		Unknown,
		Available,
		Unavailable
	};

	bool ensure_handle(const cockpit_param_api& api)
	{
		if (handle_ != nullptr)
		{
			return true;
		}
		if (api.pfn_ed_cockpit_get_parameter_handle == nullptr)
		{
			return false;
		}
		handle_ = api.pfn_ed_cockpit_get_parameter_handle(name_);
		return handle_ != nullptr;
	}

	std::optional<CockpitParameterEvent> record_failure(Failure failure)
	{
		const bool repeated =
			availability_ == Availability::Unavailable &&
			failure_reason_ != nullptr &&
			std::strcmp(failure_reason_, failure.reason) == 0;
		availability_ = Availability::Unavailable;
		failure_reason_ = failure.reason;
		if (repeated)
		{
			return std::nullopt;
		}
		return CockpitParameterEvent{
			CockpitParameterEventType::Error,
			name_,
			failure.reason,
			failure.value,
			failure.has_value
		};
	}

	std::optional<CockpitParameterEvent> record_available()
	{
		const bool recovered = availability_ == Availability::Unavailable;
		availability_ = Availability::Available;
		failure_reason_ = nullptr;
		if (!recovered)
		{
			return std::nullopt;
		}
		return CockpitParameterEvent{
			CockpitParameterEventType::Recovery,
			name_
		};
	}

	const char* name_ = nullptr;
	void* handle_ = nullptr;
	CockpitParameterAccess access_ = CockpitParameterAccess::Read;
	Availability availability_ = Availability::Unknown;
	const char* failure_reason_ = nullptr;
};

inline CockpitParameterEndpoint cockpit_parameter_reader(const char* name)
{
	return { name, CockpitParameterAccess::Read };
}

inline CockpitParameterEndpoint cockpit_parameter_writer(const char* name)
{
	return { name, CockpitParameterAccess::Write };
}
}
}
