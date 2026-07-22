#include "DcsModule.h"

#include "DcsRuntime.h"
#include "ModulePaths.h"
#include "../Data/AircraftConfig.h"

namespace
{
const int kDcsModuleAddressAnchor = 0;

DcsBridge::ModulePaths make_module_paths(const char* config_path)
{
	DcsBridge::ModulePaths paths = { "", "", false };
	DcsBridge::configure_module_paths(
		paths,
		{ config_path, &kDcsModuleAddressAnchor });
	return paths;
}

class DcsModuleState
{
public:
	explicit DcsModuleState(const char* config_path)
		: module_paths_(make_module_paths(config_path)),
		event_log_(module_paths_.mod_root_path),
		state_csv_writer_(module_paths_.mod_root_path, event_log_),
		event_reporter_(event_log_, output_store_),
		efm_(Data::fck1c_aircraft_config())
	{
	}

	DcsBridge::ModulePaths module_paths_;
	DcsBridge::Internal::EventLog event_log_;
	DcsBridge::Internal::StateCsvWriter state_csv_writer_;
	DcsBridge::Internal::FrameInputCollector input_collector_;
	DcsBridge::Internal::OutputStore output_store_;
	DcsBridge::Internal::EfmEventReporter event_reporter_;
	DcsBridge::DcsRuntime runtime_;
	std::mutex execution_mutex_;
	Core::Fck1cEfm efm_;
};

DcsModuleState& module_state(const char* initial_config_path = nullptr)
{
	// DCS keeps the EFM DLL loaded for the process lifetime. Avoid destructor I/O
	// under the Windows loader lock when the process exits.
	static DcsModuleState* state = new DcsModuleState(initial_config_path);
	return *state;
}
}

namespace DcsBridge
{
Core::Fck1cEfm& efm()
{
	return module_state().efm_;
}

DcsRuntime& runtime()
{
	return module_state().runtime_;
}

Internal::EventLog& event_log()
{
	return module_state().event_log_;
}

Internal::EfmEventReporter& event_reporter()
{
	return module_state().event_reporter_;
}

Internal::FrameInputCollector& input_collector()
{
	return module_state().input_collector_;
}

Internal::OutputStore& output_store()
{
	return module_state().output_store_;
}

Internal::StateCsvWriter& state_csv_writer()
{
	return module_state().state_csv_writer_;
}

std::mutex& execution_mutex()
{
	return module_state().execution_mutex_;
}

void configure_module(const char* config_path)
{
	(void)module_state(config_path);
}
}
