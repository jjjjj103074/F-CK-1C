#pragma once
#include "../../ExternalActionOutbox.h"
#include "../../../Contracts/RadarCommands.h"
#include <deque>
namespace Core::Systems::Mc
{
/// @brief Targeting 的雷達要求入口，保留與其他 MC 操作的來源順序。
class RadarRequestQueue final
{
public:
	explicit RadarRequestQueue(ExternalActionOutbox& actions)
	    : actions_(actions)
	{
	}
	std::uint64_t submit(const RadarCommand& command);
	RadarRequests pending();

private:
	ExternalActionOutbox& actions_;
	std::deque<RadarRequest> requests_;
};
}
