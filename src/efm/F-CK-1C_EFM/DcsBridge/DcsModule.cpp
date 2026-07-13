#include "DcsModule.h"

#include "DcsRuntime.h"
#include "../Data/AircraftConfig.h"

namespace
{
class DcsModuleState
{
public:
	DcsModuleState()
		: efm_(Data::fck1c_aircraft_config(), runtime_)
	{
	}

	DcsBridge::DcsRuntime runtime_;
	Core::Fck1cEfm efm_;
};

DcsModuleState g_module;
}

namespace DcsBridge
{
Core::Fck1cEfm& efm()
{
	return g_module.efm_;
}

DcsRuntime& runtime()
{
	return g_module.runtime_;
}
}
