#pragma once

namespace Systems
{
struct FuelSystem
{
	double internal_fuel = 0.0;
	double external_fuel = 0.0;
	double total_fuel = 0.0;
	double fuel_consumption_since_last_time = 0.0;
};

inline void simulate_fuel_consumption(
	FuelSystem& fuel,
	double dt,
	double base_fuel_consumption,
	double left_throttle_output,
	double right_throttle_output,
	double left_afterburner_ratio,
	double right_afterburner_ratio,
	double afterburner_fuel_factor)
{
	const double ab_avg = 0.5 * (left_afterburner_ratio + right_afterburner_ratio);
	const double ab_fuel_mult = 1.0 + ab_avg * (afterburner_fuel_factor - 1.0);

	// Fuel drain at full throttle in Kg/s.
	fuel.fuel_consumption_since_last_time =
		base_fuel_consumption * ((left_throttle_output + right_throttle_output + 1) / 3) * ab_fuel_mult * dt;

	if (fuel.external_fuel >= 0) // Drain external fuel first
	{
		if (fuel.fuel_consumption_since_last_time > fuel.external_fuel)
			fuel.fuel_consumption_since_last_time = fuel.external_fuel;
		fuel.external_fuel -= fuel.fuel_consumption_since_last_time;
	}
	else // Drain internal fuel
	{
		if (fuel.fuel_consumption_since_last_time > fuel.internal_fuel)
			fuel.fuel_consumption_since_last_time = fuel.internal_fuel;
		fuel.internal_fuel -= fuel.fuel_consumption_since_last_time;
	};
}

inline bool change_mass(
	FuelSystem& fuel,
	double& delta_mass,
	double& delta_mass_pos_x,
	double& delta_mass_pos_y,
	double& delta_mass_pos_z,
	double& delta_mass_moment_of_inertia_x,
	double& delta_mass_moment_of_inertia_y,
	double& delta_mass_moment_of_inertia_z)
{
	if (fuel.fuel_consumption_since_last_time > 0)
	{
		delta_mass = fuel.fuel_consumption_since_last_time;
		delta_mass_pos_x = -1.0;
		delta_mass_pos_y = 1.0;
		delta_mass_pos_z = 0;

		delta_mass_moment_of_inertia_x = 0;
		delta_mass_moment_of_inertia_y = 0;
		delta_mass_moment_of_inertia_z = 0;

		fuel.fuel_consumption_since_last_time = 0; // set it 0 to avoid infinite loop, because it called in cycle
		// better to use stack like structure for mass changing
		return true;
	}
	else
	{
		return false;
	}
}

inline void set_internal_fuel(FuelSystem& fuel, double value)
{
	fuel.internal_fuel = value;
}

inline double get_internal_fuel(const FuelSystem& fuel)
{
	return fuel.internal_fuel;
}

inline void set_external_fuel(FuelSystem& fuel, int station, double value, double x, double y, double z)
{
	(void)fuel;
	(void)station;
	(void)value;
	(void)x;
	(void)y;
	(void)z;
	// Not sure how to work with this.
}

inline double get_external_fuel(const FuelSystem& fuel)
{
	(void)fuel;
	return 0;
}
}
