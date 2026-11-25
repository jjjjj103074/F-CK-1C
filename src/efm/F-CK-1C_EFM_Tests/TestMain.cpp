#include "TestHarness.h"
#include "Common/Configuration/Configuration.h"

#include <iostream>

void run_common_tests(Tests::Context& context);
void run_abi_boundary_tests(Tests::Context& context);
void run_boundary_validator_tests(Tests::Context& context);
void run_bridge_context_tests(Tests::Context& context);
void run_carrier_bridge_tests(Tests::Context& context);
void run_cockpit_bridge_tests(Tests::Context& context);
void run_cockpit_snapshot_exporter_tests(Tests::Context& context);
void run_aerodynamics_physics_tests(Tests::Context& context);
void run_dcs_command_router_tests(Tests::Context& context);
void run_dcs_kinematics_adapter_tests(Tests::Context& context);
void run_dcs_damage_mapper_tests(Tests::Context& context);
void run_debug_csv_writer_tests(Tests::Context& context);
void run_debug_indicator_exporter_tests(Tests::Context& context);
void run_debug_telemetry_hub_tests(Tests::Context& context);
void run_output_adapter_tests(Tests::Context& context);
void run_airframe_device_system_tests(Tests::Context& context);
void run_engine_system_tests(Tests::Context& context);
void run_event_log_tests(Tests::Context& context);
void run_frame_input_collector_tests(Tests::Context& context);
void run_flight_control_actuation_system_tests(Tests::Context& context);
void run_digital_flight_control_executive_tests(Tests::Context& context);
void run_mission_computer_tests(Tests::Context& context);
void run_radar_system_tests(Tests::Context& context);
void run_radar_display_tests(Tests::Context& context);
void run_countermeasure_system_tests(Tests::Context& context);
void run_sensor_control_tests(Tests::Context& context);
void run_mode_management_tests(Tests::Context& context);
void run_stores_management_tests(Tests::Context& context);
void run_configuration_ownership_tests(Tests::Context& context);
void run_configuration_loader_tests(Tests::Context& context);
void run_control_laws_tests(Tests::Context& context);
void run_input_system_tests(Tests::Context& context);
void run_landing_gear_system_tests(Tests::Context& context);
void run_mass_delta_tests(Tests::Context& context);
void run_output_store_tests(Tests::Context& context);
void run_param_export_tests(Tests::Context& context);
void run_propulsion_diagnostics_tests(Tests::Context& context);
void run_phase_four_system_timing_tests(Tests::Context& context);
void run_simulation_model_tests(Tests::Context& context);
void run_state_csv_writer_tests(Tests::Context& context);
void run_suspension_feedback_tests(Tests::Context& context);
void run_system_pipeline_data_tests(Tests::Context& context);
void run_system_pipeline_handler_tests(Tests::Context& context);

namespace
{
void run_suite(const char* name, void (*suite)(Tests::Context&), Tests::Context& context)
{
	std::printf("[test] %s\n", name);
	std::fflush(stdout);
	suite(context);
}

void initialize_configuration()
{
	// 原生測試使用專案 FM 目錄，診斷輸出至標準錯誤串流。
	Configuration::initialize("FM",
	                          [](const Configuration::Diagnostic& diagnostic)
	                          {
		                          std::cerr << diagnostic.message() << '\n';
	                          });
}
}

void run_dcs_aircraft_interface_tests(Tests::Context& context);
void run_native_gun_access_tests(Tests::Context& context);
void run_native_countermeasure_probe_tests(Tests::Context& context);
void run_gun_ammunition_source_tests(Tests::Context& context);

void run_air_to_air_integration_tests(Tests::Context&);
void run_ir_seeker_access_tests(Tests::Context&);

void run_weapon_catalog_tests(Tests::Context&);
void run_weapon_procedures_tests(Tests::Context&);

int main()
{
	initialize_configuration();
	Tests::Context context;
	run_suite("run_weapon_catalog_tests", run_weapon_catalog_tests, context);
	run_suite("run_weapon_procedures_tests", run_weapon_procedures_tests, context);
	run_suite("run_air_to_air_integration_tests", run_air_to_air_integration_tests, context);
	run_suite("run_ir_seeker_access_tests", run_ir_seeker_access_tests, context);
	run_suite("run_dcs_aircraft_interface_tests", run_dcs_aircraft_interface_tests, context);
	run_suite("run_native_gun_access_tests", run_native_gun_access_tests, context);
	run_suite("run_native_countermeasure_probe_tests", run_native_countermeasure_probe_tests, context);
	run_suite("run_gun_ammunition_source_tests", run_gun_ammunition_source_tests, context);
	run_suite("run_configuration_loader_tests", run_configuration_loader_tests, context);
	run_suite("run_control_laws_tests", run_control_laws_tests, context);
	initialize_configuration();
	run_suite("run_common_tests", run_common_tests, context);
	run_suite("run_abi_boundary_tests", run_abi_boundary_tests, context);
	initialize_configuration();
	run_suite("run_boundary_validator_tests", run_boundary_validator_tests, context);
	run_suite("run_bridge_context_tests", run_bridge_context_tests, context);
	initialize_configuration();
	run_suite("run_carrier_bridge_tests", run_carrier_bridge_tests, context);
	run_suite("run_cockpit_bridge_tests", run_cockpit_bridge_tests, context);
	run_suite("run_cockpit_snapshot_exporter_tests", run_cockpit_snapshot_exporter_tests, context);
	run_suite("run_aerodynamics_physics_tests", run_aerodynamics_physics_tests, context);
	run_suite("run_airframe_device_system_tests", run_airframe_device_system_tests, context);
	run_suite("run_input_system_tests", run_input_system_tests, context);
	run_suite("run_landing_gear_system_tests", run_landing_gear_system_tests, context);
	run_suite("run_dcs_command_router_tests", run_dcs_command_router_tests, context);
	run_suite("run_dcs_kinematics_adapter_tests", run_dcs_kinematics_adapter_tests, context);
	run_suite("run_dcs_damage_mapper_tests", run_dcs_damage_mapper_tests, context);
	run_suite("run_debug_csv_writer_tests", run_debug_csv_writer_tests, context);
	run_suite("run_debug_indicator_exporter_tests", run_debug_indicator_exporter_tests, context);
	run_suite("run_debug_telemetry_hub_tests", run_debug_telemetry_hub_tests, context);
	run_suite("run_output_adapter_tests", run_output_adapter_tests, context);
	run_suite("run_mass_delta_tests", run_mass_delta_tests, context);
	run_suite("run_output_store_tests", run_output_store_tests, context);
	run_suite("run_param_export_tests", run_param_export_tests, context);
	run_suite("run_propulsion_diagnostics_tests", run_propulsion_diagnostics_tests, context);
	run_suite("run_phase_four_system_timing_tests", run_phase_four_system_timing_tests, context);
	run_suite("run_simulation_model_tests", run_simulation_model_tests, context);
	run_suite("run_state_csv_writer_tests", run_state_csv_writer_tests, context);
	run_suite("run_suspension_feedback_tests", run_suspension_feedback_tests, context);
	run_suite("run_engine_system_tests", run_engine_system_tests, context);
	run_suite("run_event_log_tests", run_event_log_tests, context);
	run_suite("run_frame_input_collector_tests", run_frame_input_collector_tests, context);
	run_suite("run_flight_control_actuation_system_tests", run_flight_control_actuation_system_tests, context);
	run_suite("run_digital_flight_control_executive_tests", run_digital_flight_control_executive_tests, context);
	run_suite("run_mission_computer_tests", run_mission_computer_tests, context);
	run_suite("run_radar_system_tests", run_radar_system_tests, context);
	run_suite("run_radar_display_tests", run_radar_display_tests, context);
	run_suite("run_countermeasure_system_tests", run_countermeasure_system_tests, context);
	run_suite("run_sensor_control_tests", run_sensor_control_tests, context);
	run_suite("run_mode_management_tests", run_mode_management_tests, context);
	run_suite("run_stores_management_tests", run_stores_management_tests, context);
	run_suite("run_configuration_ownership_tests", run_configuration_ownership_tests, context);
	run_suite("run_system_pipeline_data_tests", run_system_pipeline_data_tests, context);
	run_suite("run_system_pipeline_handler_tests", run_system_pipeline_handler_tests, context);
	return context.finish();
}
