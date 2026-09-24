#pragma once

#include "Core/Systems/FlightControlComputer/CommandSystem/FlightControlCommandBinding.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace Tests::Fck1c
{
/// @brief 在不建立 Pipeline 的模組測試中，走與正式系統相同的指令綁定。
/// @param bindings 待測 FLCC 實例提供的指令綁定。
/// @param command 要交付的 Core 指令與正規化數值。
inline void deliver_flight_control_command(
	const std::vector<Core::Systems::FlightControlCommandBinding>& bindings,
	const Core::Command& command)
{
	const auto binding = std::find_if(
		bindings.begin(), bindings.end(),
		[&command](const Core::Systems::FlightControlCommandBinding& candidate)
		{ return candidate.id == command.id; });
	if (binding == bindings.end())
		throw std::logic_error("Flight-control test command is not bound.");
	binding->deliver(command);
}
}
