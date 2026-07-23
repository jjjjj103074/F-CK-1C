#pragma once

#include "../Common/Vec3.h"

#include <algorithm>
#include <map>

namespace Systems
{
struct FuelSystemConfig
{
	double consumption_rate = 0.0;
};

struct FuelConsumptionInput
{
	double dt = 0.0;
	double left_throttle_output = 0.0;
	double right_throttle_output = 0.0;
	double left_afterburner_ratio = 0.0;
	double right_afterburner_ratio = 0.0;
	double afterburner_fuel_factor = 0.0;
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
	double fuel_consumption_since_last_time = 0.0;
	std::map<int, ExternalFuelState> external_fuel_by_station;
};

struct FuelMassDelta
{
	double mass = 0.0;
	Common::Vec3 position;
	Common::Vec3 moment_of_inertia;
};

struct FuelMassDeltaResult
{
	bool available = false;
	FuelMassDelta delta;
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

inline void simulate_fuel_consumption(
	FuelSystem& fuel,
	const FuelSystemConfig& config,
	const FuelConsumptionInput& input)
{
	const double ab_avg = 0.5 * (input.left_afterburner_ratio + input.right_afterburner_ratio);
	const double ab_fuel_mult = 1.0 + ab_avg * (input.afterburner_fuel_factor - 1.0);

	// Fuel drain at full throttle in Kg/s.
	fuel.total_fuel_flow =
		config.consumption_rate *
		((input.left_throttle_output + input.right_throttle_output + 1) / 3) *
		ab_fuel_mult;
	const double requested = fuel.total_fuel_flow * input.dt;

	const double external_consumed = consume_external_fuel(fuel, requested);
	const double internal_consumed =
		consume_internal_fuel(fuel, requested - external_consumed);
	fuel.fuel_consumption_since_last_time =
		external_consumed + internal_consumed;
}

inline FuelMassDeltaResult take_fuel_mass_delta(FuelSystem& fuel)
{
	FuelMassDeltaResult result;
	if (fuel.fuel_consumption_since_last_time <= 0.0)
	{
		return result;
	}

	result.available = true;
	result.delta.mass = fuel.fuel_consumption_since_last_time;
	result.delta.position = Common::Vec3(-1.0, 1.0, 0.0);
	fuel.fuel_consumption_since_last_time = 0.0;
	return result;
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

inline void reset_fuel_transient_state(FuelSystem& fuel)
{
	fuel.total_fuel_flow = 0.0;
	fuel.fuel_consumption_since_last_time = 0.0;
}
}
