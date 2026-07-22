#include "DcsModule.h"

#include "Internal/BridgeContext.h"
#include "../Data/AircraftConfig.h"

namespace
{
const int kDcsModuleAddressAnchor = 0;

DcsBridge::Internal::BridgeContextOwner& context_owner()
{
	// Production context and its writer live until process termination. This
	// deliberately avoids destructor I/O or a thread join under loader lock.
	static auto* owner = new DcsBridge::Internal::BridgeContextOwner({
		ed_get_cockpit_param_api,
		Data::fck1c_aircraft_config(),
		&kDcsModuleAddressAnchor
	});
	return *owner;
}

DcsBridge::Internal::BridgeContext& module_state(
	const char* initial_config_path = nullptr)
{
	return context_owner().get(initial_config_path);
}
}

namespace DcsBridge
{
Core::Fck1cEfm& efm()
{
	return module_state().core();
}

DcsRuntime& runtime()
{
	return module_state().runtime();
}

Internal::EventLog& event_log()
{
	return module_state().event_log();
}

Internal::EfmEventReporter& event_reporter()
{
	return module_state().event_reporter();
}

Internal::CockpitBridge& cockpit_bridge()
{
	return module_state().cockpit_bridge();
}

Internal::CarrierBridge& carrier_bridge()
{
	return module_state().carrier_bridge();
}

Internal::FrameInputCollector& input_collector()
{
	return module_state().input_collector();
}

Internal::OutputStore& output_store()
{
	return module_state().output_store();
}

Internal::StateCsvWriter& state_csv_writer()
{
	return module_state().state_csv_writer();
}

std::mutex& execution_mutex()
{
	return module_state().execution_mutex();
}

void configure_module(const char* config_path)
{
	(void)module_state(config_path);
}
}
