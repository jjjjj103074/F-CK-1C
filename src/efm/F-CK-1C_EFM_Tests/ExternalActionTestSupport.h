#pragma once
#include "Core/Systems/ExternalActionOutbox.h"
#include "DcsBridge/Internal/ExternalActionTransport.h"
namespace Tests
{
/// @brief 將單一生產端的設備動作投影為測試檢查視圖。
inline Core::MissionActionBatch pending_actions(Core::Systems::ExternalActionOutbox& source)
{
	const auto pending = source.pending();
	Core::MissionActionBatch result;
	for (const auto& request : pending.items)
	{
		if (result.count == result.actions.size())
			break;
		if (result.count == 0)
			result.first_sequence = request.sequence;
		result.actions[result.count++] = request.action;
	}
	return result;
}
}
