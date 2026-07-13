#include "AircraftConfig.h"

#include "AeroTables.h"
#include "EngineTables.h"

namespace
{
Systems::AerodynamicsSystemConfig make_aerodynamics_config()
{
	const Data::AeroTables& data = Data::fck1c_aero_tables();
	Systems::AerodynamicsSystemConfig config;
	config.wing_area = 24.26;
	config.wingspan = 8.53;
	config.length = 14.48;
	config.height = 4.7;
	config.mach_max = 1.8;
	config.cy_zero = data.cy_zero;
	config.cz_beta = data.cz_beta;
	config.cx_gear = data.cx_gear;
	config.cx_airbrake = data.cx_airbrake;
	config.cx_flap = data.cx_flap;
	config.cx_lift_k = data.cx_lift_k;
	config.cx_alpha_k = data.cx_alpha_k;
	config.cx_elevator_k = data.cx_elevator_k;
	config.cy_flap = data.cy_flap;
	config.airbrake_pitch_comp_k = data.airbrake_pitch_comp_k;
	config.mach_table.assign(data.mach.begin(), data.mach.end());
	config.cx_zero_table.assign(data.cx_zero.begin(), data.cx_zero.end());
	config.cy_alpha_table.assign(data.cy_alpha.begin(), data.cy_alpha.end());
	config.roll_rate_max_table.assign(data.roll_rate_max.begin(), data.roll_rate_max.end());
	config.alpha_max_table.assign(data.alpha_max.begin(), data.alpha_max.end());
	config.cy_max_table.assign(data.cy_max.begin(), data.cy_max.end());
	return config;
}

Systems::EngineSystemConfig make_engine_config()
{
	const Data::EngineTables& data = Data::fck1c_engine_tables();
	Systems::EngineSystemConfig config;
	config.start_time = data.start_time;
	config.spool_up_tau = data.spool_up_tau;
	config.spool_down_tau = data.spool_down_tau;
	config.mach_table.assign(data.mach.begin(), data.mach.end());
	config.max_thrust_table.assign(data.max_thrust.begin(), data.max_thrust.end());
	config.throttle_input_table.assign(data.throttle_input.begin(), data.throttle_input.end());
	config.power_table.assign(data.power.begin(), data.power.end());
	return config;
}

Data::AircraftConfig make_aircraft_config()
{
	Data::AircraftConfig config;
	config.aerodynamics = make_aerodynamics_config();
	config.engine = make_engine_config();
	config.fuel.consumption_rate = Data::fck1c_engine_tables().fuel_consumption;
	config.left_engine_position = Common::Vec3(-3.793, -0.391, -0.716);
	config.right_engine_position = Common::Vec3(-3.793, -0.391, 0.716);
	return config;
}
}

namespace Data
{
const AircraftConfig& fck1c_aircraft_config()
{
	static const AircraftConfig config = make_aircraft_config();
	return config;
}
}
