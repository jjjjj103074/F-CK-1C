#include "TestHarness.h"

void run_common_tests(Tests::Context& context);
void run_abi_boundary_tests(Tests::Context& context);
void run_boundary_validator_tests(Tests::Context& context);
void run_bridge_context_tests(Tests::Context& context);
void run_carrier_bridge_tests(Tests::Context& context);
void run_cockpit_bridge_tests(Tests::Context& context);
void run_aerodynamics_system_tests(Tests::Context& context);
void run_dcs_command_router_tests(Tests::Context& context);
void run_dcs_damage_mapper_tests(Tests::Context& context);
void run_output_adapter_tests(Tests::Context& context);
void run_airframe_device_system_tests(Tests::Context& context);
void run_engine_system_tests(Tests::Context& context);
void run_event_log_tests(Tests::Context& context);
void run_fbw_controller_tests(Tests::Context& context);
void run_fck1c_efm_characterization_tests(Tests::Context& context);
void run_fck1c_efm_tests(Tests::Context& context);
void run_frame_input_collector_tests(Tests::Context& context);
void run_immutable_data_tests(Tests::Context& context);
void run_input_system_tests(Tests::Context& context);
void run_landing_gear_system_tests(Tests::Context& context);
void run_mass_delta_tests(Tests::Context& context);
void run_output_store_tests(Tests::Context& context);
void run_param_export_tests(Tests::Context& context);
void run_phase_three_system_tests(Tests::Context& context);
void run_phase_four_system_timing_tests(Tests::Context& context);
void run_state_csv_writer_tests(Tests::Context& context);
void run_suspension_system_tests(Tests::Context& context);
void run_system_pipeline_data_tests(Tests::Context& context);
void run_system_pipeline_handler_tests(Tests::Context& context);

int main()
{
	Tests::Context context;
	run_common_tests(context);
	run_abi_boundary_tests(context);
	run_boundary_validator_tests(context);
	run_bridge_context_tests(context);
	run_carrier_bridge_tests(context);
	run_cockpit_bridge_tests(context);
	run_aerodynamics_system_tests(context);
	run_airframe_device_system_tests(context);
	run_input_system_tests(context);
	run_landing_gear_system_tests(context);
	run_dcs_command_router_tests(context);
	run_dcs_damage_mapper_tests(context);
	run_output_adapter_tests(context);
	run_mass_delta_tests(context);
	run_output_store_tests(context);
	run_param_export_tests(context);
	run_phase_three_system_tests(context);
	run_phase_four_system_timing_tests(context);
	run_state_csv_writer_tests(context);
	run_suspension_system_tests(context);
	run_engine_system_tests(context);
	run_event_log_tests(context);
	run_fbw_controller_tests(context);
	run_fck1c_efm_characterization_tests(context);
	run_fck1c_efm_tests(context);
	run_frame_input_collector_tests(context);
	run_immutable_data_tests(context);
	run_system_pipeline_data_tests(context);
	run_system_pipeline_handler_tests(context);
	return context.finish();
}
