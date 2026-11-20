#pragma once

#include "CockpitParameterEndpoint.h"
#include "../../Core/Contracts/DisplayContracts.h"

#include <array>

namespace DcsBridge::Internal
{
/// @brief 將前視框與雷達圖的顯示快照寫入座艙圖形參數。
class RadarDisplayExporter final
{
public:
	explicit RadarDisplayExporter(cockpit_param_api api);
	void reset();
	CockpitParameterEvents export_snapshot(const Core::RadarDisplaySnapshot& snapshot);

private:
	const cockpit_param_api api_;
	std::array<CockpitParameterEndpoint, 149> slots_;
};
}
