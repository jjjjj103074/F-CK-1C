#pragma once

#include "CarrierBridge.h"
#include "CockpitBridge.h"
#include "EfmEventReporter.h"
#include "EventLog.h"
#include "FrameInputCollector.h"
#include "OutputStore.h"
#include "StateCsvWriter.h"
#include "../DcsRuntime.h"
#include "../ModulePaths.h"
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
	CockpitBridge& cockpit_bridge();
	CarrierBridge& carrier_bridge();
	DcsRuntime& runtime();
	std::mutex& execution_mutex();
	Core::Fck1cEfm& core();

private:
	ModulePaths module_paths_;
	EventLog event_log_;
	StateCsvWriter state_csv_writer_;
	FrameInputCollector input_collector_;
	OutputStore output_store_;
	EfmEventReporter event_reporter_;
	CockpitBridge cockpit_bridge_;
	CarrierBridge carrier_bridge_;
	DcsRuntime runtime_;
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
