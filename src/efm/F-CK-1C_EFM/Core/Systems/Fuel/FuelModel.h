#pragma once

#include "Common/Vec3.h"

#include <algorithm>
#include <map>

namespace Systems
{
struct FuelSystemConfig
{
	double consumption_rate = 0.0;
};

struct ExternalFuelState
{
	int station = 0;
	double value = 0.0;
	Common::Vec3 position;
};

struct FuelSystem
{
	double internal_fuel = 0.0;
	double external_fuel = 0.0;
	double total_fuel_flow = 0.0;
	double frame_consumed_mass = 0.0;
	std::map<int, ExternalFuelState> external_fuel_by_station;
};

inline double consume_external_fuel(FuelSystem& fuel, double requested)
{
	double remaining = requested;
	for (auto station = fuel.external_fuel_by_station.begin();
		station != fuel.external_fuel_by_station.end() && remaining > 0.0;)
	{
		const double consumed = (std::min)(remaining, station->second.value);
		station->second.value -= consumed;
		fuel.external_fuel -= consumed;
		remaining -= consumed;
		if (station->second.value <= 0.0)
		{
			station = fuel.external_fuel_by_station.erase(station);
		}
		else
		{
			++station;
		}
	}
	return requested - remaining;
}

inline double consume_internal_fuel(FuelSystem& fuel, double requested)
{
	const double consumed = (std::min)(requested, fuel.internal_fuel);
	fuel.internal_fuel -= consumed;
	return consumed;
}

inline void consume_fuel(
	FuelSystem& fuel,
	double flow_rate,
	double dt)
{
	fuel.total_fuel_flow = flow_rate;
	const double requested = flow_rate * dt;
	const double external_consumed = consume_external_fuel(fuel, requested);
	const double internal_consumed =
		consume_internal_fuel(fuel, requested - external_consumed);
	fuel.frame_consumed_mass =
		external_consumed + internal_consumed;
}

inline void apply_fuel_demand(
	FuelSystem& fuel,
	double flow_rate,
	double dt)
{
	consume_fuel(fuel, flow_rate, dt);
}

inline void suppress_fuel_consumption(FuelSystem& fuel)
{
	fuel.total_fuel_flow = 0.0;
	fuel.frame_consumed_mass = 0.0;
}

inline void set_internal_fuel(FuelSystem& fuel, double value)
{
	fuel.internal_fuel = value;
}

inline double get_internal_fuel(const FuelSystem& fuel)
{
	return fuel.internal_fuel;
}

inline void set_external_fuel(
	FuelSystem& fuel,
	const ExternalFuelState& external)
{
	if (external.value > 0.0)
	{
		fuel.external_fuel_by_station[external.station] = external;
	}
	else
	{
		fuel.external_fuel_by_station.erase(external.station);
	}
	fuel.external_fuel = 0.0;
	for (const auto& station : fuel.external_fuel_by_station)
	{
		fuel.external_fuel += station.second.value;
	}
}

inline double get_external_fuel(const FuelSystem& fuel)
{
	return fuel.external_fuel;
}

}
