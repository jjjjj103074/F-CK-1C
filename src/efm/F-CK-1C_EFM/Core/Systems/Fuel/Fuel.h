#pragma once

#include "FuelModel.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
class Fuel final : public System
{
public:
	explicit Fuel(const FlightFuelState& initial = {});

	void setup(SystemSetup& setup) override;
	void step(
		const AircraftDataSnapshot& snapshot,
		SystemResult& result) override;

	const FuelData& step(const FuelDemand& demand, double dt);
	void suppress_consumption();
	void set_internal_fuel(double fuel);
	void set_external_fuel(const ::Systems::ExternalFuelState& fuel);
	double internal_fuel() const;
	double external_fuel() const;
	const ::Systems::FuelSystem& state() const;
	const FuelData& data() const;

private:
	FlightFuelState management_state() const;
	void refresh_data();

	::Systems::FuelSystem fuel_;
	FuelData data_;
};
}
}
