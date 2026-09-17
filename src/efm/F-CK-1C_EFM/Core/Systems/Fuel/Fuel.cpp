#include "Fuel.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace Core
{
namespace Systems
{
Fuel::Fuel(const FlightFuelState& initial)
{
	set_internal_fuel(initial.internal_fuel_kg);
	for (const ExternalFuelInput& external : initial.external_fuel)
	{
		set_external_fuel({
			external.station,
			external.fuel_kg,
			external.position_body_m
		});
	}
	refresh_data();
}

void Fuel::setup(SystemSetup& setup)
{
	setup.update_rate_hz(kProjectDefinedFallbackUpdateRateHz);
	setup.read(AircraftDataKeys::kFuelDemand);
	setup.publish(AircraftDataKeys::kFuelData, data_);
	setup.register_fuel_management({
		[this]() { return management_state(); },
		[this]() { return data_; },
		[this](double fuel_kg) { set_internal_fuel(fuel_kg); },
		[this](const ExternalFuelInput& fuel)
		{
			set_external_fuel(
				{ fuel.station, fuel.fuel_kg, fuel.position_body_m });
		},
		[this](bool suppress_consumption)
		{
			begin_frame(suppress_consumption);
		}
	});
}

void Fuel::step(
	const SystemStepContext& context,
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	const FuelDemand& demand =
		aircraft.read(AircraftDataKeys::kFuelDemand);
	result.publish(
		AircraftDataKeys::kFuelData,
		update(demand, context.dt_s));
}

const FuelData& Fuel::step(const FuelDemand& demand, double dt_s)
{
	return update(demand, dt_s);
}

const FuelData& Fuel::update(const FuelDemand& demand, double dt_s)
{
	if (consumption_suppressed_)
	{
		::Systems::record_fuel_demand_without_consumption(
			fuel_,
			demand.flow_rate_kg_s);
	}
	else
	{
		::Systems::apply_fuel_demand(fuel_, demand.flow_rate_kg_s, dt_s);
	}
	refresh_data();
	return data_;
}

void Fuel::begin_frame(bool suppress_consumption)
{
	fuel_.frame_consumed_mass_kg = 0.0;
	consumption_suppressed_ = suppress_consumption;
	refresh_data();
}

void Fuel::set_internal_fuel(double fuel_kg)
{
	::Systems::set_internal_fuel(fuel_, fuel_kg);
	refresh_data();
}

void Fuel::set_external_fuel(
	const ::Systems::ExternalFuelState& fuel)
{
	::Systems::set_external_fuel(fuel_, fuel);
	refresh_data();
}

double Fuel::internal_fuel_kg() const
{
	return ::Systems::get_internal_fuel_kg(fuel_);
}

double Fuel::external_fuel_kg() const
{
	return ::Systems::get_external_fuel_kg(fuel_);
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
	state.internal_fuel_kg = fuel_.internal_fuel_kg;
	state.external_fuel.reserve(fuel_.external_fuel_by_station.size());
	for (const auto& station : fuel_.external_fuel_by_station)
	{
		state.external_fuel.push_back({
			station.first,
			station.second.fuel_kg,
			station.second.position_body_m
		});
	}
	return state;
}

void Fuel::refresh_data()
{
	data_ = {
		fuel_.internal_fuel_kg,
		fuel_.external_fuel_kg,
		fuel_.total_fuel_flow_kg_s,
		fuel_.frame_consumed_mass_kg
	};
}
}
}
