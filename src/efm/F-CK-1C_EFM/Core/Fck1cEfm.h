#pragma once

#include "Contracts/Commands.h"
#include "Contracts/Events.h"
#include "Contracts/FrameContracts.h"
#include "../Data/AircraftConfig.h"

#include <memory>

namespace Core
{
namespace Simulation
{
class AircraftSimulation;
}

class Fck1cEfm
{
public:
	explicit Fck1cEfm(const Data::AircraftConfig& config);
	~Fck1cEfm();

	Fck1cEfm(const Fck1cEfm&) = delete;
	Fck1cEfm& operator=(const Fck1cEfm&) = delete;

	FrameOutput start(StartMode mode);
	FrameOutput step(const FrameInput& input);
	double internal_fuel() const;
	double external_fuel() const;

	void handle_command(const Command& command);
	void set_internal_fuel(double fuel);
	void set_external_fuel(const ExternalFuelInput& input);
	void add_refueling_fuel(double fuel);
	void set_infinite_fuel(bool enabled);
	void set_easy_flight(bool enabled);
	void set_invincible(bool enabled);
	DamageApplyResult apply_damage(const DamageEvent& event);
	void release();
	void repair(const RepairEvent& event);

private:
	struct FlightPreparation;

	void synchronize_preparation();

	const Data::AircraftConfig config_;
	std::unique_ptr<FlightPreparation> preparation_;
	std::unique_ptr<Simulation::AircraftSimulation> simulation_;
};
}
