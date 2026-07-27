#include "Fuel.h"

#include "../SystemPipeline.h"

namespace Core
{
namespace Systems
{
Fuel::Fuel(const FlightFuelState& initial)
{
	set_internal_fuel(initial.internal_fuel);
	for (const ExternalFuelInput& external : initial.external_fuel)
	{
		set_external_fuel({
			external.station,
			external.fuel,
			external.position
		});
	}
	refresh_data();
}

void Fuel::setup(SystemSetup& setup)
{
	setup.read(AircraftDataKeys::kFrameInput);
	setup.read(AircraftDataKeys::kFuelDemand);
	setup.publish(AircraftDataKeys::kFuelData, data_);
	setup.register_fuel_management({
		[this]() { return management_state(); },
		[this]() { return data_; },
		[this](double fuel) { set_internal_fuel(fuel); },
		[this](const ExternalFuelInput& fuel)
		{
			set_external_fuel({ fuel.station, fuel.fuel, fuel.position });
		},
		[this]() { suppress_consumption(); }
	});
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

void Fuel::suppress_consumption()
{
	::Systems::suppress_fuel_consumption(fuel_);
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

const FuelData& Fuel::data() const
{
	return data_;
}

FlightFuelState Fuel::management_state() const
{
	FlightFuelState state;
	state.internal_fuel = fuel_.internal_fuel;
	state.external_fuel.reserve(fuel_.external_fuel_by_station.size());
	for (const auto& station : fuel_.external_fuel_by_station)
	{
		state.external_fuel.push_back({
			station.first,
			station.second.value,
			station.second.position
		});
	}
	return state;
}

void Fuel::refresh_data()
{
	data_ = {
		fuel_.internal_fuel,
		fuel_.external_fuel,
		fuel_.total_fuel_flow,
		fuel_.frame_consumed_mass
	};
}
}
}
