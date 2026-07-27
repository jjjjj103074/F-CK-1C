#include "Fuel.h"

#include "../SystemPipeline.h"

namespace Core
{
namespace Systems
{
void Fuel::setup(SystemSetup& setup)
{
	setup.read(AircraftDataKeys::kFrameInput);
	setup.read(AircraftDataKeys::kFuelDemand);
	setup.publish(AircraftDataKeys::kFuelData, data_);
}

void Fuel::step(
	const AircraftDataSnapshot& snapshot,
	SystemResult& result)
{
	const FrameInput& frame = snapshot.read(AircraftDataKeys::kFrameInput);
	const FuelDemand& demand =
		snapshot.read(AircraftDataKeys::kFuelDemand);
	result.publish(
		AircraftDataKeys::kFuelData,
		step(demand, frame.dt_s));
}

const FuelData& Fuel::step(const FuelDemand& demand, double dt)
{
	::Systems::apply_fuel_demand(fuel_, demand.flow_rate_kg_s, dt);
	refresh_data();
	return data_;
}

void Fuel::set_reported_flow(double flow_rate)
{
	::Systems::set_reported_fuel_flow(fuel_, flow_rate);
	refresh_data();
}

void Fuel::set_internal_fuel(double fuel)
{
	::Systems::set_internal_fuel(fuel_, fuel);
	refresh_data();
}

void Fuel::set_external_fuel(
	const ::Systems::ExternalFuelState& fuel)
{
	::Systems::set_external_fuel(fuel_, fuel);
	refresh_data();
}

double Fuel::internal_fuel() const
{
	return ::Systems::get_internal_fuel(fuel_);
}

double Fuel::external_fuel() const
{
	return ::Systems::get_external_fuel(fuel_);
}

const ::Systems::FuelSystem& Fuel::state() const
{
	return fuel_;
}

::Systems::FuelMassDeltaResult Fuel::take_mass_delta()
{
	return ::Systems::take_fuel_mass_delta(fuel_);
}

const FuelData& Fuel::data() const
{
	return data_;
}

void Fuel::refresh_data()
{
	data_ = {
		fuel_.internal_fuel,
		fuel_.external_fuel,
		fuel_.total_fuel_flow
	};
}
}
}
