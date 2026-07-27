#include "Fck1cEfm.h"

#include "Simulation/AircraftSimulation.h"

#include <stdexcept>
#include <utility>

namespace Core
{
struct Fck1cEfm::FlightPreparation
{
	Simulation::FlightFuelLoad fuel;
	Simulation::SimulationOptions options;
};

Fck1cEfm::Fck1cEfm(const Data::AircraftConfig& config)
	: config_(config),
	preparation_(std::make_unique<FlightPreparation>())
{
	Simulation::validate_aircraft_config(config_);
}

Fck1cEfm::~Fck1cEfm() = default;

FrameOutput Fck1cEfm::start(StartMode mode)
{
	synchronize_preparation();
	const Simulation::FlightSetupContext setup = {
		mode,
		preparation_->fuel,
		preparation_->options
	};
	auto simulation =
		std::make_unique<Simulation::AircraftSimulation>(config_, setup);
	const FrameOutput output = simulation->initial_output();
	simulation_ = std::move(simulation);
	return output;
}

FrameOutput Fck1cEfm::step(const FrameInput& input)
{
	if (!simulation_)
	{
		throw std::logic_error("Fck1cEfm::step requires an active flight.");
	}
	const FrameOutput output = simulation_->step(input);
	preparation_->fuel = simulation_->fuel_load();
	return output;
}

double Fck1cEfm::internal_fuel() const
{
	return simulation_
		? simulation_->internal_fuel()
		: preparation_->fuel.internal_fuel;
}

double Fck1cEfm::external_fuel() const
{
	if (simulation_)
	{
		return simulation_->external_fuel();
	}
	double total = 0.0;
	for (const auto& station : preparation_->fuel.external_fuel_by_station)
	{
		total += station.second.fuel;
	}
	return total;
}

void Fck1cEfm::handle_command(const Command& command)
{
	if (simulation_)
	{
		simulation_->handle_command(command);
	}
}

MassDeltaResult Fck1cEfm::take_mass_delta()
{
	return simulation_ ? simulation_->take_mass_delta() : MassDeltaResult{};
}

void Fck1cEfm::set_internal_fuel(double fuel)
{
	preparation_->fuel.internal_fuel = fuel;
	if (simulation_)
	{
		simulation_->set_internal_fuel(fuel);
	}
}

void Fck1cEfm::set_external_fuel(const ExternalFuelInput& input)
{
	const Simulation::ExternalFuelLoad load = {
		input.fuel,
		input.position
	};
	if (input.fuel > 0.0)
	{
		preparation_->fuel.external_fuel_by_station[input.station] = load;
	}
	else
	{
		preparation_->fuel.external_fuel_by_station.erase(input.station);
	}
	if (simulation_)
	{
		simulation_->set_external_fuel(input.station, load);
	}
}

void Fck1cEfm::add_refueling_fuel(double fuel)
{
	(void)fuel;
}

void Fck1cEfm::set_infinite_fuel(bool enabled)
{
	preparation_->options.infinite_fuel = enabled;
	if (simulation_)
	{
		simulation_->set_infinite_fuel(enabled);
	}
}

void Fck1cEfm::set_easy_flight(bool enabled)
{
	preparation_->options.easy_flight = enabled;
	if (simulation_)
	{
		simulation_->set_easy_flight(enabled);
	}
}

void Fck1cEfm::set_invincible(bool enabled)
{
	preparation_->options.invincible = enabled;
	if (simulation_)
	{
		simulation_->set_invincible(enabled);
	}
}

DamageApplyResult Fck1cEfm::apply_damage(const DamageEvent& event)
{
	return simulation_
		? simulation_->apply_damage(event)
		: DamageApplyResult{ preparation_->options.invincible };
}

void Fck1cEfm::synchronize_preparation()
{
	if (simulation_)
	{
		preparation_->fuel = simulation_->fuel_load();
	}
}

void Fck1cEfm::release()
{
	synchronize_preparation();
	simulation_.reset();
}

void Fck1cEfm::repair(const RepairEvent& event)
{
	if (simulation_)
	{
		simulation_->repair(event);
	}
}
}
