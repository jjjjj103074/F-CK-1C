#include "FlightControlComputerConfig.h"

#include <stdexcept>
#include <utility>

namespace
{
/// @brief 驗證只有完整 FLCC 才知道的跨模組關係。
/// @param config 已通過各模組局部驗證的設定草稿。
/// @throws std::invalid_argument 控制律的迎角保護目標超出飛行包線時擲出。
void validate_cross_module_relationships(
	const Core::Systems::FlightControlComputerConfigDraft& config)
{
	const double terminal_g = config.flight_control_laws.longitudinal.
		angle_of_attack_limited_normal_acceleration_g;
	if (terminal_g >=
			config.mode_and_gain.cat1.envelope.hard_positive_load_factor_g ||
		terminal_g >=
			config.mode_and_gain.cat3.envelope.hard_positive_load_factor_g)
	{
		throw std::invalid_argument(
			"FLCC alpha protection exceeds stores limits.");
	}
}

/// @brief 依模組邊界執行完整 FLCC 設定驗證。
/// @param config 已套用所有差異、尚未封存的設定草稿。
/// @throws std::invalid_argument 任一局部或跨模組規則不成立時擲出。
void validate_complete_config(
	const Core::Systems::FlightControlComputerConfigDraft& config)
{
	::Systems::validate_input_signal_management_config(
		config.input_signal_management);
	::Systems::validate_mode_and_gain_scheduling_config(config.mode_and_gain);
	::Systems::validate_guidance_coordination_config(
		config.guidance_coordination);
	::Systems::validate_flight_control_laws_config(config.flight_control_laws);
	::Systems::validate_flight_control_output_config(
		config.flight_control_output);
	::Systems::validate_flight_control_diagnostics_config(config.diagnostics);
	Core::Systems::validate_automatic_flight_control_config(
		config.automatic_flight_control);
	validate_cross_module_relationships(config);
}
}

namespace Core::Systems
{
FlightControlComputerConfigDraft
	make_fck1c_flight_control_computer_config_draft()
{
	FlightControlComputerConfigDraft config;
	config.mode_and_gain =
		::Systems::make_fck1c_mode_and_gain_scheduling_config();
	config.automatic_flight_control = fck1c_automatic_flight_control_config();
	return config;
}

FlightControlComputerConfig finalize_flight_control_computer_config(
	FlightControlComputerConfigDraft draft)
{
	validate_complete_config(draft);
	return FlightControlComputerConfig(std::move(draft));
}

const FlightControlComputerConfig& fck1c_flight_control_computer_config()
{
	static const FlightControlComputerConfig config =
		finalize_flight_control_computer_config(
			make_fck1c_flight_control_computer_config_draft());
	return config;
}
}
