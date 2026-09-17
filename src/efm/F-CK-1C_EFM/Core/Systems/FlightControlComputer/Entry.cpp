#include "FlightControlComputer.h"

// 建置時的系統目錄產生器會尋找各系統根目錄的 Entry.cpp。
// 此檔只提供 FLCC 的註冊資料與建立方式，不參與每次飛控計算。
namespace Core
{
namespace Systems
{
SystemEntry make_flight_control_computer_system_entry(
	const FlightControlComputerConfig& config)
{
	validate_flight_control_computer_config(config);
	// 以值保存設定；系統排程器建立飛行實例時才建構 FLCC 物件。
	return {
		"flight_control_computer",
		SystemGroup::Control,
		[owned_config = config](const FlightSetupContext& setup)
		{
			return std::make_unique<FlightControlComputer>(
				owned_config,
				setup.start_mode,
				setup.initial_throttle_levers);
		}
	};
}

namespace Catalog
{
namespace FlightControlComputer
{
SystemEntry create_entry()
{
	return make_flight_control_computer_system_entry(
		fck1c_flight_control_computer_config());
}
}
}
}
}
