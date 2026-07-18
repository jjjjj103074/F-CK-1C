#pragma once

#include "AutopilotBridge.h"
#include "CockpitParams.h"
#include "ModulePaths.h"
#include "SimulationEvents.h"
#include "../Core/Fck1cEfm.h"
#include "../Diagnostics/RuntimeDiagnostics.h"
#include "../Diagnostics/SuspensionDiagnostics.h"
#include "../include/Cockpit/CockpitAPI_Declare.h"
#include "../include/FM/wHumanCustomPhysicsAPI.h"

namespace DcsBridge
{
struct DebugWatchBuffer
{
	char* data;
	size_t capacity;
};

struct ConfigStringTarget
{
	char* data;
	size_t capacity;
};

class DcsRuntime final : public Core::Fck1cEfmRuntime
{
public:
	DcsRuntime();

	Core::AutopilotCommand read_autopilot();
	Core::MaxPowerCommand read_max_power();
	void on_first_frame(const Core::Fck1cEfmSnapshot& snapshot) override;
	void on_engine_shutdown(const Core::Fck1cEfmSnapshot& snapshot) override;
	void on_thrust_updated(
		const Core::Fck1cEfmSnapshot& snapshot,
		const Core::MaxPowerCommand& command) override;
	void on_ground_diagnostics(const Core::Fck1cEfmSnapshot& snapshot, double dt) override;
	void on_release(const Core::Fck1cEfmSnapshot& snapshot) override;

	void configure(const char* config_path, const Core::Fck1cEfmSnapshot& snapshot);
	void export_temperature(double dcs_temperature);
	void reset_startup_suspension_probe();
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
	size_t debug_watch(
		const Core::Fck1cEfmSnapshot& snapshot,
		int level,
		const DebugWatchBuffer& buffer) const;

private:
	const char* active_fm_config_path();
	void build_mod_path(char* output, size_t output_size, const char* relative_path);
	bool config_flag_is_true(const char* flag_name);
	double config_number_or_default(const char* key_name, double default_value);
	void config_string_or_default(
		const char* key_name,
		const char* default_value,
		const ConfigStringTarget& target);
	double active_suspension_radius_add();
	double active_suspension_wheel_y_offset();
	void active_suspension_node_names(const char*& nose, const char*& left, const char*& right);
	void refresh_suspension_diagnostics_config(const Core::Fck1cEfmSnapshot& snapshot);
	Diagnostics::SuspensionDiagnosticsSnapshot make_suspension_snapshot(
		const Core::Fck1cEfmSnapshot& snapshot) const;
	void copy_suspension_wheel_snapshot(
		const Core::Fck1cEfmSystems& systems,
		Diagnostics::SuspensionDiagnosticsSnapshot& snapshot) const;
	Diagnostics::ThrustDiagnosticsSnapshot make_thrust_snapshot(
		const Core::Fck1cEfmSnapshot& snapshot,
		const Core::MaxPowerCommand& command) const;
	void write_module_log(const char* message);
	void write_probe_log(double simulation_time, const char* message);

	ModulePaths module_paths_ = { ".", "FM\\config.lua", false };
	EDPARAM cockpit_interface_;
	CockpitParamHandles cockpit_params_;
	AutopilotParamHandles autopilot_params_;
	AutopilotState autopilot_state_ = {};
	CarrierLaunchState carrier_launch_ = {};
	Diagnostics::SuspensionDiagnosticsState suspension_diagnostics_;
	Diagnostics::SuspensionDiagnosticsConfig suspension_diagnostics_config_;
	bool suspension_diagnostics_config_loaded_ = false;
};
}
