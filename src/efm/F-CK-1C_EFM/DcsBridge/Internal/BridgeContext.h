#pragma once

#include "CarrierBridge.h"
#include "CockpitBridge.h"
#include "EfmEventReporter.h"
#include "EventLog.h"
#include "FrameInputCollector.h"
#include "OutputStore.h"
#include "ParamExporter.h"
#include "StateCsvWriter.h"
#include "ModulePaths.h"
#include "../../Core/Fck1cEfm.h"

#include <memory>
#include <mutex>

namespace DcsBridge
{
namespace Internal
{
using CockpitApiProvider = cockpit_param_api (*)();

struct BridgeContextConfig
{
	ModulePathSource path_source;
	CockpitApiProvider cockpit_api_provider;
	const Data::AircraftConfig& aircraft_config;
};

class BridgeContext final
{
public:
	explicit BridgeContext(const BridgeContextConfig& config);

	BridgeContext(const BridgeContext&) = delete;
	BridgeContext& operator=(const BridgeContext&) = delete;

	const ModulePaths& module_paths() const;
	EventLog& event_log();
	StateCsvWriter& state_csv_writer();
	FrameInputCollector& input_collector();
	OutputStore& output_store();
	EfmEventReporter& event_reporter();
	ParamExporter& param_exporter();
	CockpitBridge& cockpit_bridge();
	CarrierBridge& carrier_bridge();
	std::mutex& execution_mutex();
	Core::Fck1cEfm& core();

	template <typename Action>
	bool perform_flight_action(
		const CallbackContext& context,
		const Action& action)
	{
		bool released = false;
		{
			const std::lock_guard<std::mutex> lock(execution_mutex_);
			released = output_store_.is_released();
			if (!released)
			{
				action();
			}
		}
		if (released)
		{
			event_reporter_.log_callback_lifecycle_error(context, "released");
		}
		return !released;
	}

	template <typename Action>
	bool perform_core_action(
		const CallbackContext& context,
		const Action& action)
	{
		return perform_flight_action(
			context,
			[this, &action]() { action(core_); });
	}

	template <typename Action>
	void perform_core_preparation(const Action& action)
	{
		const std::lock_guard<std::mutex> lock(execution_mutex_);
		action(core_);
	}

	template <typename Query>
	auto query_core_preparation(const Query& query)
	{
		const std::lock_guard<std::mutex> lock(execution_mutex_);
		return query(core_);
	}

	Core::MassDeltaResult take_flight_mass_delta();

private:
	ModulePaths module_paths_;
	EventLog event_log_;
	StateCsvWriter state_csv_writer_;
	FrameInputCollector input_collector_;
	OutputStore output_store_;
	EfmEventReporter event_reporter_;
	ParamExporter param_exporter_;
	CockpitBridge cockpit_bridge_;
	CarrierBridge carrier_bridge_;
	std::mutex execution_mutex_;
	Core::Fck1cEfm core_;
};

struct BridgeContextEnvironment
{
	CockpitApiProvider cockpit_api_provider;
	const Data::AircraftConfig& aircraft_config;
	const void* module_address;
};

class BridgeContextOwner final
{
public:
	explicit BridgeContextOwner(const BridgeContextEnvironment& environment);

	BridgeContextOwner(const BridgeContextOwner&) = delete;
	BridgeContextOwner& operator=(const BridgeContextOwner&) = delete;

	BridgeContext& get(const char* initial_config_path);

private:
	const CockpitApiProvider cockpit_api_provider_;
	const Data::AircraftConfig& aircraft_config_;
	const void* const module_address_;
	std::once_flag initialization_flag_;
	std::unique_ptr<BridgeContext> context_;
};
}
}
