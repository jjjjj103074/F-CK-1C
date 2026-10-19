#include "DigitalFlightControlComputer.h"

namespace Core::Systems::Catalog::DigitalFlightControlComputer
{
/// @brief 登記 DFLCC 的系統識別碼與延後建立實例的方法。
/// @return 回傳 SystemPipeline 建立飛機 DFLCC 實例的目錄項目；不建立實例。
SystemEntry create_entry()
{
	SystemEntry entry;

	entry.id = "digital_flight_control_computer"; // Pipeline 識別系統及回報錯誤時使用的 ID。

	// Pipeline 建立一架飛機時才呼叫工廠，建立此架飛機持有的電腦。
	entry.factory = [](const FlightSetupContext&)
	{
		return std::make_unique<Core::Systems::DigitalFlightControlComputer>();
	};
	return entry;
}
}
