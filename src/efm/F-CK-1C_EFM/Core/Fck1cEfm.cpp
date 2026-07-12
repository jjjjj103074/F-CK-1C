#include "Fck1cEfm.h"

namespace Core
{
Fck1cEfm::Fck1cEfm(const Fck1cEfmConfig& config)
	: config_(config)
{
}

const Fck1cEfmConfig& Fck1cEfm::config() const
{
	return config_;
}

AircraftState& Fck1cEfm::aircraft_state()
{
	return aircraft_state_;
}

const AircraftState& Fck1cEfm::aircraft_state() const
{
	return aircraft_state_;
}

ForceMomentFrame& Fck1cEfm::force_moment()
{
	return force_moment_;
}

const ForceMomentFrame& Fck1cEfm::force_moment() const
{
	return force_moment_;
}

ControlSurfaceState& Fck1cEfm::control_surfaces()
{
	return control_surfaces_;
}

const ControlSurfaceState& Fck1cEfm::control_surfaces() const
{
	return control_surfaces_;
}

GameplayState& Fck1cEfm::gameplay()
{
	return gameplay_;
}

const GameplayState& Fck1cEfm::gameplay() const
{
	return gameplay_;
}

Fck1cEfmSystems& Fck1cEfm::systems()
{
	return systems_;
}

const Fck1cEfmSystems& Fck1cEfm::systems() const
{
	return systems_;
}
}
