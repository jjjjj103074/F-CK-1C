#pragma once

#include "../CommandSystem/AutomaticFlightControlTypes.h"
#include "../CommandSystem/GuidanceCoordinationConfig.h"
#include "../ControlLaws/ControlLawConfig.h"
#include "../Diagnostics/FlightControlDiagnosticsConfig.h"
#include "../Input/InputSignalManagementConfig.h"
#include "../ModeAndGainScheduling/ModeAndGainSchedulingConfig.h"
#include "../Output/FlightControlOutputConfig.h"

#include <utility>

namespace Core::Systems
{
/// @brief FLCC 完整設定的資料內容。
/// 建構階段可修改此值；封存後由 FlightControlComputerConfig 以唯讀方式持有。
struct FlightControlComputerConfigValues
{
	::Systems::InputSignalManagementConfig input_signal_management;  // 觀測與駕駛訊號的濾波設定。
	::Systems::ModeAndGainSchedulingConfig mode_and_gain;  // CAT、包線與增益排程設定。
	::Systems::GuidanceCoordinationConfig guidance_coordination;  // 飛行目標的三軸協調設定。
	::Systems::FlightControlLawsConfig flight_control_laws;  // 三軸控制律及控制面混合設定。
	::Systems::FlightControlOutputConfig flight_control_output;  // 電子輸出選擇、過渡與限制設定。
	::Systems::FlightControlDiagnosticsConfig diagnostics;  // 診斷狀態的判定設定。
	AutomaticFlightControlConfig automatic_flight_control;  // 自動飛行模式與控制參數。
};

/// @brief 尚可套用測試差異的 FLCC 設定草稿。
using FlightControlComputerConfigDraft = FlightControlComputerConfigValues;

/// @brief 已完成局部與跨模組驗證的唯讀 FLCC 建構設定。
/// 只有 finalize_flight_control_computer_config() 能建立此型別。
class FlightControlComputerConfig final
{
public:
	FlightControlComputerConfig(const FlightControlComputerConfig&) = default;
	FlightControlComputerConfig(FlightControlComputerConfig&&) = default;

	const FlightControlComputerConfigValues values;  // 已完成驗證且不可再修改的完整設定值。

private:
	explicit FlightControlComputerConfig(FlightControlComputerConfigDraft draft)
		: values(std::move(draft))
	{
	}

	friend FlightControlComputerConfig finalize_flight_control_computer_config(
		FlightControlComputerConfigDraft draft);
};

/// @brief 建立含完整 F-CK-1C 正式值、仍可套用測試差異的設定草稿。
/// @return 尚未完成最終驗證的獨立值物件。
FlightControlComputerConfigDraft
	make_fck1c_flight_control_computer_config_draft();

/// @brief 驗證並封存完整 FLCC 設定。
/// @param draft 已套用所有差異的設定草稿；函式取得其內容所有權。
/// @return 後續只能讀取的完整建構設定。
/// @throws std::invalid_argument 任一設定無效時不建立結果。
FlightControlComputerConfig finalize_flight_control_computer_config(
	FlightControlComputerConfigDraft draft);

/// @brief 取得模組內唯一的完整 F-CK-1C 正式設定。
/// @return 已驗證的唯讀靜態設定，生命週期涵蓋整個程式。
const FlightControlComputerConfig& fck1c_flight_control_computer_config();
}
