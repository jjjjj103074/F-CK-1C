#include "ProcessBridgeContext.h"

namespace DcsBridge
{
namespace Internal
{
ProcessBridgeContext::ProcessBridgeContext(
	CockpitApiProvider cockpit_api_provider,
	AircraftConfigProvider aircraft_config_provider,
	const void* module_address) noexcept
{
	try
	{
		owner_ = new BridgeContextOwner({
			cockpit_api_provider,
			aircraft_config_provider(),
			module_address
		});
	}
	catch (...)
	{
		owner_ = nullptr;
		initialization_error_ = std::current_exception();
	}
}

BridgeContext& ProcessBridgeContext::get(const char* initial_config_path)
{
	if (owner_ == nullptr)
	{
		std::rethrow_exception(initialization_error_);
	}
	return owner_->get(initial_config_path);
}

EventLog* ProcessBridgeContext::try_event_log() const noexcept
{
	BridgeContext* context = owner_ != nullptr ? owner_->try_get() : nullptr;
	return context != nullptr ? &context->event_log() : nullptr;
}
}
}
