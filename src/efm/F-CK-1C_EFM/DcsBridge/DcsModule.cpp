#include "DcsModule.h"

#include "DcsRuntime.h"
#include "../FM_data.h"

namespace
{
Core::Fck1cEfmConfig make_fck1c_efm_config()
{
	Core::Fck1cEfmConfig config;
	config.aerodynamics.wing_area = FM_DATA::wing_area;
	config.aerodynamics.wingspan = FM_DATA::wingspan;
	config.aerodynamics.length = FM_DATA::length;
	config.aerodynamics.height = FM_DATA::height;
	config.aerodynamics.mach_max = FM_DATA::mach_max;
	config.aerodynamics.cy_zero = FM_DATA::Cy0;
	config.aerodynamics.cz_beta = FM_DATA::Czbe;
	config.aerodynamics.cx_gear = FM_DATA::cx_gear;
	config.aerodynamics.cx_airbrake = FM_DATA::cx_brk;
	config.aerodynamics.cx_flap = FM_DATA::cx_flap;
	config.aerodynamics.cx_lift_k = FM_DATA::cx_lift_k;
	config.aerodynamics.cx_alpha_k = FM_DATA::cx_alpha_k;
	config.aerodynamics.cx_elevator_k = FM_DATA::cx_elevator_k;
	config.aerodynamics.cy_flap = FM_DATA::cy_flap;
	config.aerodynamics.airbrake_pitch_comp_k = FM_DATA::airbrake_pitch_comp_k;
	config.aerodynamics.mach_table = FM_DATA::mach_table;
	config.aerodynamics.cx_zero_table = FM_DATA::cx0;
	config.aerodynamics.cy_alpha_table = FM_DATA::Cya;
	config.aerodynamics.roll_rate_max_table = FM_DATA::OmxMax;
	config.aerodynamics.alpha_max_table = FM_DATA::Aldop;
	config.aerodynamics.cy_max_table = FM_DATA::CyMax;
	config.aerodynamics.table_size = FM_DATA::kAeroTableSize;
	config.fuel.consumption_rate = FM_DATA::fuel_consumption;
	config.engine.start_time = FM_DATA::engine_start_time;
	config.engine.spool_up_tau = FM_DATA::engine_spool_up_tau;
	config.engine.spool_down_tau = FM_DATA::engine_spool_down_tau;
	config.engine.mach_table = FM_DATA::engine_mach_table;
	config.engine.max_thrust_table = FM_DATA::max_thrust;
	config.engine.mach_table_size = FM_DATA::kEngineTableSize;
	config.engine.throttle_input_table = FM_DATA::throttle_input_table;
	config.engine.power_table = FM_DATA::engine_power_table;
	config.engine.throttle_table_size = sizeof(FM_DATA::throttle_input_table) / sizeof(float);
	config.left_engine_position = Common::Vec3(-3.793, -0.391, -0.716);
	config.right_engine_position = Common::Vec3(-3.793, -0.391, 0.716);
	return config;
}

class DcsModuleState
{
public:
	DcsModuleState()
		: efm_(make_fck1c_efm_config(), runtime_)
	{
	}

	DcsBridge::DcsRuntime runtime_;
	Core::Fck1cEfm efm_;
};

DcsModuleState g_module;
}

namespace DcsBridge
{
Core::Fck1cEfm& efm()
{
	return g_module.efm_;
}

DcsRuntime& runtime()
{
	return g_module.runtime_;
}
}
