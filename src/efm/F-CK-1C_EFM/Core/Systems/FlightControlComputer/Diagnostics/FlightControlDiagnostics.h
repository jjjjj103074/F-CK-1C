#pragma once

#include "FlightControlDiagnosticsConfig.h"
#include "../CommandSystem/FlightControlCommandSystem.h"
#include "../Output/FlightControlOutputSystem.h"
#include "../ControlLaws/ControlLaws.h"

#include <cstdint>

namespace Core
{
namespace Systems
{
struct FlightControlDiagnosticsInput
{
	std::uint64_t flight_control_tick = 0;
	std::uint64_t pilot_shaping_update_count = 0;
	std::uint64_t pilot_shaping_last_update_tick = 0;
	std::uint64_t gain_schedule_update_count = 0;
	std::uint64_t gain_schedule_last_update_tick = 0;
	bool cat3_selected = false;
	bool developer_g_limiter_override_active = false;
	const FlightControlCommandSystemResult& command;
	const ::Systems::FlightControlLawsResult& laws;
	const ::Systems::ActiveFlightControlConfiguration& configuration;
	const ::Systems::ManagedFlightControlSignals& signals;
	const FlightControlOutputStatus& output_status;
	const FlightControlActuatorCommand& actuator_command;
};

class FlightControlDiagnostics
{
public:
	FlightControlDiagnostics(
		bool g_limiter_override_available,
		bool developer_direct_control_law,
		const ::Systems::FlightControlDiagnosticsConfig& config = {});
	const FlightControlComputerSnapshot& update(
		const FlightControlDiagnosticsInput& input);
	const FlightControlComputerSnapshot& snapshot() const;

private:
	void update_subrates(const FlightControlDiagnosticsInput& input);
	void update_status(const FlightControlDiagnosticsInput& input);
	void update_longitudinal_status(
		const FlightControlDiagnosticsInput& input);
	void update_output_status(const FlightControlDiagnosticsInput& input);
	void update_control_authority(const FlightControlDiagnosticsInput& input);
	void update_selected_reference(const SelectedFlightReference& selected);
	void update_longitudinal(const SelectedFlightReference& selected);
	void update_lateral(const SelectedFlightReference& selected);
	void update_coordinated_reference(
		const FlightControlDiagnosticsInput& input);

	FlightControlComputerSnapshot snapshot_;
	const ::Systems::FlightControlDiagnosticsConfig config_;
	double previous_angle_of_attack_rad_ = 0.0;
	double authority_limit_time_s_ = 0.0;
	bool angle_of_attack_initialized_ = false;
};
}
}
