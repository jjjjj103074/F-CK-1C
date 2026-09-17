#pragma once

#include "Common/Vec3.h"

#include <algorithm>
#include <map>

namespace Systems
{
struct ExternalFuelState
{
	int station = 0;
	double fuel_kg = 0.0;
	Common::Vec3 position_body_m;
};

struct FuelSystem
{
	double internal_fuel_kg = 0.0;
	double external_fuel_kg = 0.0;
	double total_fuel_flow_kg_s = 0.0;
	double frame_consumed_mass_kg = 0.0;
	std::map<int, ExternalFuelState> external_fuel_by_station;
};

inline double consume_external_fuel(
	FuelSystem& fuel,
	double requested_kg)
{
	double remaining_kg = requested_kg;
	for (auto station = fuel.external_fuel_by_station.begin();
		station != fuel.external_fuel_by_station.end() && remaining_kg > 0.0;)
	{
		const double consumed_kg =
			(std::min)(remaining_kg, station->second.fuel_kg);
		station->second.fuel_kg -= consumed_kg;
		fuel.external_fuel_kg -= consumed_kg;
		remaining_kg -= consumed_kg;
		if (station->second.fuel_kg <= 0.0)
		{
			station = fuel.external_fuel_by_station.erase(station);
		}
		else
		{
			++station;
		}
	}
	return requested_kg - remaining_kg;
}

inline double consume_internal_fuel(
	FuelSystem& fuel,
	double requested_kg)
{
	const double consumed_kg =
		(std::min)(requested_kg, fuel.internal_fuel_kg);
	fuel.internal_fuel_kg -= consumed_kg;
	return consumed_kg;
}

inline void consume_fuel(
	FuelSystem& fuel,
	double flow_rate_kg_s,
	double dt_s)
{
	fuel.total_fuel_flow_kg_s = flow_rate_kg_s;
	const double requested_kg = flow_rate_kg_s * dt_s;
	const double external_consumed_kg =
		consume_external_fuel(fuel, requested_kg);
	const double internal_consumed_kg =
		consume_internal_fuel(fuel, requested_kg - external_consumed_kg);
	fuel.frame_consumed_mass_kg +=
		external_consumed_kg + internal_consumed_kg;
}

inline void apply_fuel_demand(
	FuelSystem& fuel,
	double flow_rate_kg_s,
	double dt_s)
{
	consume_fuel(fuel, flow_rate_kg_s, dt_s);
}

inline void record_fuel_demand_without_consumption(
	FuelSystem& fuel,
	double flow_rate_kg_s)
{
	fuel.total_fuel_flow_kg_s = flow_rate_kg_s;
}

inline void set_internal_fuel(FuelSystem& fuel, double fuel_kg)
{
	fuel.internal_fuel_kg = fuel_kg;
}

inline double get_internal_fuel_kg(const FuelSystem& fuel)
{
	return fuel.internal_fuel_kg;
}

inline void set_external_fuel(
	FuelSystem& fuel,
	const ExternalFuelState& external)
{
	if (external.fuel_kg > 0.0)
	{
		fuel.external_fuel_by_station[external.station] = external;
	}
	else
	{
		fuel.external_fuel_by_station.erase(external.station);
	}
	fuel.external_fuel_kg = 0.0;
	for (const auto& station : fuel.external_fuel_by_station)
	{
		fuel.external_fuel_kg += station.second.fuel_kg;
	}
}

inline double get_external_fuel_kg(const FuelSystem& fuel)
{
	return fuel.external_fuel_kg;
}

}
