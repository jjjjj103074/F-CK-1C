#pragma once

#include "../../Database/InputOutputData.h"
#include "../../Database/StoresData.h"
#include "../../ModeEvents.h"
#include "../../StoresEvents.h"
#include "Internal/StationSelection.h"

namespace Core::Systems::Mc
{
/// @brief 統整固定掛點與機砲觀測，管理選站意圖，完成資料更新後發布同步通知。
class StoresManagement final
{
public:
	/// @brief 綁定唯讀輸入、SM 資料分區及本台 MC 的掛載事件通道。
	StoresManagement(const MissionInputs& input, StoresData& stores, StoresEvents& events);
	StoresManagement(const StoresManagement&) = delete;
	StoresManagement& operator=(const StoresManagement&) = delete;
	StoresManagement(StoresManagement&&) = delete;
	StoresManagement& operator=(StoresManagement&&) = delete;

	/// @brief 保存模式選擇意圖，於本次更新的庫存上決定掛點。
	void on_operating_mode_selected(const OperatingModeSelected& event);
	/// @brief 依序更新庫存、機砲彈量及選站，最後通知已完成的資料變化。
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	const MissionInputs& input_;
	StoresData& stores_;
	StoresEvents& events_;
	Internal::StationSelection selection_;
};
}
