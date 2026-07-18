#pragma once

#include "AutopilotBridge.h"
#include "CockpitParams.h"
#include "ModulePaths.h"
#include "SimulationEvents.h"
#include "../Core/Fck1cEfm.h"
#include "../include/Cockpit/CockpitAPI_Declare.h"
#include "../include/FM/wHumanCustomPhysicsAPI.h"

namespace DcsBridge
{
class DcsRuntime final : public Core::Fck1cEfmRuntime
{
public:
	DcsRuntime();

	Core::AutopilotCommand read_autopilot();
	Core::MaxPowerCommand read_max_power();

	void configure(const char* config_path);
	void export_temperature(double dcs_temperature);
	void reset_carrier_launch();
	void log_damage(const Core::Fck1cEfmSnapshot& snapshot, int element, double integrity);
	void update_suspension_feedback(
		Core::Fck1cEfm& efm,
		int index,
		const ed_fm_suspension_info* info);
	bool pop_simulation_event(
		const Core::Fck1cEfmSnapshot& snapshot,
		double max_dry_thrust,
		ed_fm_simulation_event& out);
	bool push_simulation_event(const ed_fm_simulation_event& in);

private:
	void build_mod_path(char* output, size_t output_size, const char* relative_path);
	void write_module_log(const char* message);

	ModulePaths module_paths_ = { ".", "FM\\config.lua", false };
	EDPARAM cockpit_interface_;
	CockpitParamHandles cockpit_params_;
	AutopilotParamHandles autopilot_params_;
	AutopilotState autopilot_state_ = {};
	CarrierLaunchState carrier_launch_ = {};
};
}
