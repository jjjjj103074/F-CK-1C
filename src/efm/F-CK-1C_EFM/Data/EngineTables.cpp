#include "EngineTables.h"

namespace
{
Data::EngineTables make_fck1c_engine_tables()
{
	Data::EngineTables tables;
	tables.fuel_consumption = 0.37;
	tables.start_time = 60.0;
	tables.spool_up_tau = 2.5;
	tables.spool_down_tau = 4.0;
	tables.mach = {
		0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0
	};
	tables.max_thrust = {
		54000.0, 53600.0, 53200.0, 52800.0, 52300.0, 51600.0,
		50800.0, 49900.0, 48900.0, 47800.0, 46600.0
	};
	tables.throttle_input = {
		0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0
	};
	tables.power = {
		0.0, 0.01, 0.02, 0.06, 0.08, 0.1, 0.3, 0.5, 0.7, 0.9, 1.0
	};
	return tables;
}
}

namespace Data
{
const EngineTables& fck1c_engine_tables()
{
	static const EngineTables tables = make_fck1c_engine_tables();
	return tables;
}
}
