#include "BridgeContext.h"

#include "../../Data/AircraftConfig.h"

namespace
{
constexpr double kCarrierLaunchReferenceMach = 0.1;

DcsBridge::ModulePaths make_module_paths(
	const DcsBridge::ModulePathSource& source)
{
	DcsBridge::ModulePaths paths = { "", "", false };
	DcsBridge::configure_module_paths(paths, source);
	return paths;
}

DcsBridge::Internal::CarrierBridgeConfig make_carrier_config(
	const Data::AircraftConfig& aircraft_config)
{
	return {
		Systems::max_dry_thrust(
			aircraft_config.engine,
			kCarrierLaunchReferenceMach)
	};
}
}

namespace DcsBridge
{
namespace Internal
{
BridgeContext::BridgeContext(const BridgeContextConfig& config)
	: module_paths_(make_module_paths(config.path_source)),
	event_log_(module_paths_.mod_root_path),
	state_csv_writer_(module_paths_.mod_root_path, event_log_),
	event_reporter_(event_log_, output_store_),
	cockpit_bridge_(config.cockpit_api_provider()),
	carrier_bridge_(make_carrier_config(config.aircraft_config)),
	core_(config.aircraft_config)
{
}

const ModulePaths& BridgeContext::module_paths() const
{
	return module_paths_;
}

EventLog& BridgeContext::event_log()
{
	return event_log_;
}

StateCsvWriter& BridgeContext::state_csv_writer()
{
	return state_csv_writer_;
}

FrameInputCollector& BridgeContext::input_collector()
{
	return input_collector_;
}

OutputStore& BridgeContext::output_store()
{
	return output_store_;
}

EfmEventReporter& BridgeContext::event_reporter()
{
	return event_reporter_;
}

CockpitBridge& BridgeContext::cockpit_bridge()
{
	return cockpit_bridge_;
}

CarrierBridge& BridgeContext::carrier_bridge()
{
	return carrier_bridge_;
}

DcsRuntime& BridgeContext::runtime()
{
	return runtime_;
}

std::mutex& BridgeContext::execution_mutex()
{
	return execution_mutex_;
}

Core::Fck1cEfm& BridgeContext::core()
{
	return core_;
}

BridgeContextOwner::BridgeContextOwner(
	const BridgeContextEnvironment& environment)
	: cockpit_api_provider_(environment.cockpit_api_provider),
	aircraft_config_(environment.aircraft_config),
	module_address_(environment.module_address)
{
}

BridgeContext& BridgeContextOwner::get(const char* initial_config_path)
{
	std::call_once(
		initialization_flag_,
		[this, initial_config_path]()
		{
			context_ = std::make_unique<BridgeContext>(BridgeContextConfig{
				{ initial_config_path, module_address_ },
				cockpit_api_provider_,
				aircraft_config_
			});
		});
	return *context_;
}
}
}
