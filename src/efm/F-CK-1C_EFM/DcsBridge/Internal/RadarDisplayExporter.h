#pragma once

#include "CockpitParameterEndpoint.h"
#include "../../Core/Contracts/DisplayContracts.h"

#include <array>
#include <cstddef>

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
	enum class Parameter : std::size_t
	{
		Available,
		Visible,
		SensorAvailable,
		CueKind,
		CueVisible,
		BoreHalfWidthMrad,
		BoreHalfHeightMrad,
		TargetXMrad,
		TargetYMrad,
		RangeScaleM,
		RangeScaleNm,
		ScanLeft,
		ScanRight,
		SweepVisible,
		SweepX,
		CursorVisible,
		CursorX,
		CursorY,
		TargetVisible,
		TargetX,
		TargetY,
		Count
	};

	struct PlotParameters
	{
		CockpitParameterEndpoint x, y, opacity, source_slot;
	};

	const cockpit_param_api api_;
	std::array<CockpitParameterEndpoint, static_cast<std::size_t>(Parameter::Count)> slots_;
	std::array<PlotParameters, Core::kRadarDisplayPlotCount> plots_;
};
}
