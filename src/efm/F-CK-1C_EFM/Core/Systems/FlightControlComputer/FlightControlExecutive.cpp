#include "FlightControlExecutive.h"

#include <stdexcept>

namespace Core::Systems
{
const std::vector<FlightControlCommandBinding>&
FlightControlExecutive::command_bindings() const noexcept
{
	return command_bindings_;
}

FlightControlComputerResult FlightControlExecutive::update(
	const FlightControlComputerStepInput& input)
{
	// 保留輸入介面，讓後續測試能從邊界逐步加入實際運算。
	// 在第一個運算尚未完成前，明確回報不可用，避免輸出看似正常的假資料。
	(void)input;
	throw std::logic_error("FLCC internal computation is not implemented.");
}
}
