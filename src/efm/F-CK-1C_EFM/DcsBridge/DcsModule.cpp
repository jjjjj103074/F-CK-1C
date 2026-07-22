#include "DcsModule.h"

#include "DcsRuntime.h"
#include "../Data/AircraftConfig.h"

namespace
{
class DcsModuleState
{
public:
	DcsModuleState()
		: efm_(Data::fck1c_aircraft_config())
	{
	}

	DcsBridge::DcsRuntime runtime_;
	DcsBridge::Internal::FrameInputCollector input_collector_;
	DcsBridge::Internal::OutputStore output_store_;
	std::mutex execution_mutex_;
	Core::Fck1cEfm efm_;
};

DcsModuleState& module_state()
{
	static DcsModuleState state;
	return state;
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

Internal::FrameInputCollector& input_collector()
{
	return module_state().input_collector_;
}

Internal::OutputStore& output_store()
{
	return module_state().output_store_;
}

std::mutex& execution_mutex()
{
	return module_state().execution_mutex_;
}
}
