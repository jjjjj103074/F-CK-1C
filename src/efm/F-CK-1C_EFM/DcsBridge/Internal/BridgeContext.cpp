#include "BridgeContext.h"

#include <stdexcept>

namespace
{
DcsBridge::ModulePaths make_module_paths(
	const DcsBridge::ModulePathSource& source)
{
	DcsBridge::ModulePaths paths = { "", "", false };
	DcsBridge::configure_module_paths(paths, source);
	return paths;
}

DcsBridge::Internal::CarrierBridgeConfig make_carrier_config()
{
	return { Core::carrier_launch_reference_thrust() };
}

cockpit_param_api make_cockpit_api(
	DcsBridge::Internal::CockpitApiProvider provider)
{
	if (provider == nullptr)
	{
		throw std::invalid_argument(
			"BridgeContext requires a cockpit API provider.");
	}
	return provider();
}

std::unique_ptr<Core::Fck1cEfm> make_core(
	const DcsBridge::Internal::CoreFactory& factory)
{
	if (!factory)
	{
		throw std::invalid_argument("BridgeContext requires a Core factory.");
	}
	std::unique_ptr<Core::Fck1cEfm> core = factory();
	if (!core)
	{
		throw std::invalid_argument(
			"BridgeContext core factory returned no Core.");
	}
	return core;
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
	param_exporter_(event_reporter_),
	cockpit_bridge_(make_cockpit_api(config.cockpit_api_provider)),
	carrier_bridge_(make_carrier_config()),
	core_(make_core(config.core_factory))
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

ParamExporter& BridgeContext::param_exporter()
{
	return param_exporter_;
}

CockpitBridge& BridgeContext::cockpit_bridge()
{
	return cockpit_bridge_;
}

CarrierBridge& BridgeContext::carrier_bridge()
{
	return carrier_bridge_;
}

std::mutex& BridgeContext::execution_mutex()
{
	return execution_mutex_;
}

Core::Fck1cEfm& BridgeContext::core()
{
	return *core_;
}

Core::FrameOutput BridgeContext::start_flight(Core::StartMode mode)
{
	const std::lock_guard<std::mutex> lock(execution_mutex_);
	if (output_store_.read())
	{
		event_reporter_.log_repeated_start(mode);
	}
	param_exporter_.reset();
	carrier_bridge_.reset();
	const Core::FrameOutput output = core_->start(mode);
	output_store_.publish_start(output);
	param_exporter_.observe(output);
	state_csv_writer_.publish_start(output);
	return output;
}

Core::MassDeltaResult BridgeContext::take_flight_mass_delta()
{
	const std::lock_guard<std::mutex> lock(execution_mutex_);
	return output_store_.take_mass_delta();
}

BridgeContextOwner::BridgeContextOwner(
	const BridgeContextEnvironment& environment)
	: cockpit_api_provider_(environment.cockpit_api_provider),
	core_factory_(environment.core_factory),
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
				core_factory_
			});
			published_context_.store(context_.get(), std::memory_order_release);
		});
	return *context_;
}

BridgeContext* BridgeContextOwner::try_get() const noexcept
{
	return published_context_.load(std::memory_order_acquire);
}
}
}
