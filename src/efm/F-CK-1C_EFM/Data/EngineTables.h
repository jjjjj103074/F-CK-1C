#pragma once

#include <array>
#include <cstddef>

namespace Data
{
constexpr std::size_t kEngineTableSize = 11;
using EngineTable = std::array<double, kEngineTableSize>;

struct EngineTables
{
	double fuel_consumption = 0.0;
	double start_time = 0.0;
	double spool_up_tau = 0.0;
	double spool_down_tau = 0.0;
	EngineTable mach;
	EngineTable max_thrust;
	EngineTable throttle_input;
	EngineTable power;
};

const EngineTables& fck1c_engine_tables();
}
