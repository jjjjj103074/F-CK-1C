#pragma once

#include "DcsIds/CockpitParams.g.h"
#include "include/Cockpit/ccParametersAPI.h"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace Tests
{
struct FakeCockpitParameter
{
	const char* name = nullptr;
	double value = 0.0;
	bool available = true;
};

class FakeCockpitParameters final
{
public:
	FakeCockpitParameters()
	{
		if (active_fixture() != nullptr)
		{
			throw std::logic_error(
				"Only one fake cockpit parameter fixture may be active.");
		}
		for (const DcsIds::CockpitParams::Entry& entry :
			DcsIds::CockpitParams::Catalog)
		{
			parameters_.push_back({ entry.name });
		}
		active_fixture() = this;
		for (const DcsIds::RawCockpitParams::Entry& entry :
			DcsIds::RawCockpitParams::Catalog)
		{
			parameters_.push_back({ entry.name });
		}
	}

	~FakeCockpitParameters()
	{
		active_fixture() = nullptr;
	}

	FakeCockpitParameters(const FakeCockpitParameters&) = delete;
	FakeCockpitParameters& operator=(const FakeCockpitParameters&) = delete;

	void set(const char* name, double value)
	{
		find(name)->value = value;
	}

	void set_available(const char* name, bool available)
	{
		find(name)->available = available;
	}

	double value(const char* name)
	{
		return find(name)->value;
	}

	void* handle(const char* name)
	{
		FakeCockpitParameter* parameter = find(name);
		return parameter != nullptr && parameter->available
			? parameter
			: nullptr;
	}

	cockpit_param_api api()
	{
		cockpit_param_api result = {};
		result.pfn_ed_cockpit_get_parameter_handle = get_parameter_handle;
		result.pfn_ed_cockpit_update_parameter_with_number = update_number;
		result.pfn_ed_cockpit_parameter_value_to_number = read_number;
		return result;
	}

private:
	static FakeCockpitParameters*& active_fixture()
	{
		static thread_local FakeCockpitParameters* fixture = nullptr;
		return fixture;
	}

	static void* get_parameter_handle(const char* name)
	{
		return active_fixture()->handle(name);
	}

	static void update_number(void* handle, double value)
	{
		static_cast<FakeCockpitParameter*>(handle)->value = value;
	}

	static bool read_number(
		const void* handle,
		double& result,
		bool interpolated)
	{
		(void)interpolated;
		result = static_cast<const FakeCockpitParameter*>(handle)->value;
		return true;
	}

	FakeCockpitParameter* find(const char* name)
	{
		for (FakeCockpitParameter& parameter : parameters_)
		{
			if (std::strcmp(parameter.name, name) == 0)
			{
				return &parameter;
			}
		}
		return nullptr;
	}

	std::vector<FakeCockpitParameter> parameters_;
};
}
