#pragma once

#include "BridgeContext.h"

#include <exception>

namespace DcsBridge
{
namespace Internal
{
class ProcessBridgeContext final
{
public:
	ProcessBridgeContext(
		CockpitApiProvider cockpit_api_provider,
		CoreFactory core_factory,
		const void* module_address) noexcept;

	ProcessBridgeContext(const ProcessBridgeContext&) = delete;
	ProcessBridgeContext& operator=(const ProcessBridgeContext&) = delete;

	BridgeContext& get(const char* initial_config_path);
	EventLog* try_event_log() const noexcept;

private:
	// DCS keeps the DLL loaded for the process lifetime. Intentionally avoid
	// destructor I/O and thread joins while the Windows loader lock is held.
	BridgeContextOwner* owner_ = nullptr;
	std::exception_ptr initialization_error_;
};
}
}
