#pragma once

#include "../../Database/MissionData.h"
#include "../../../../Contracts/Commands.h"

namespace Core::Systems::Mc
{
/// @brief 依模式與顯示來源選擇 SOI，並處理 DMS 的焦點切換。
class SensorOfInterest final
{
public:
	SensorOfInterest(const ModeData& mode, const DisplaySnapshot& display, SensorOfInterestSnapshot& selection);
	/// @brief 接收最新顯示對應，保留有效來源並套用模式預設焦點。
	void refresh();
	void handle(const Command& command);

private:
	SensorControlSource source_at(DisplayLocation location) const;
	bool allowed(SensorControlSource source) const;
	bool select(DisplayLocation location);
	void select_mfd();

	const ModeData& mode_;
	const DisplaySnapshot& display_;
	SensorOfInterestSnapshot& selection_;
};
}
