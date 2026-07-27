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
	void setup(SystemSetup& setup) override;
	void step(
		const AircraftDataSnapshot& snapshot,
		SystemResult& result) override;

	const FuelData& step(const FuelDemand& demand, double dt);
	void set_reported_flow(double flow_rate);
	void set_internal_fuel(double fuel);
	void set_external_fuel(const ::Systems::ExternalFuelState& fuel);
	double internal_fuel() const;
	double external_fuel() const;
	const ::Systems::FuelSystem& state() const;
	::Systems::FuelMassDeltaResult take_mass_delta();
	const FuelData& data() const;

private:
	void refresh_data();

	::Systems::FuelSystem fuel_;
	FuelData data_;
};
}
}
