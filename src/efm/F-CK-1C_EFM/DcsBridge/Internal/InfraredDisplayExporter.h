#pragma once
#include "CockpitParameterEndpoint.h"
#include "../../Core/Contracts/DisplayContracts.h"
#include <array>
namespace DcsBridge::Internal
{
/// @brief 將 IR 前視圖形快照發布至座艙準直指示器。
class InfraredDisplayExporter final
{
public:
	explicit InfraredDisplayExporter(cockpit_param_api api);
	void reset();
	CockpitParameterEvents export_snapshot(const Core::InfraredDisplaySnapshot& snapshot);

private:
	cockpit_param_api api_;
	std::array<CockpitParameterEndpoint, 7> parameters_;
};
}
