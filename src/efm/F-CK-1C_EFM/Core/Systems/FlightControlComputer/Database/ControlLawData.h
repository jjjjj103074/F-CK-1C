#pragma once

#include "ControlLawData/Configuration.h"
#include "ControlLawData/State.h"

namespace Core::Systems::Flcc
{
/// @brief 控制律資料庫；設定於建構後唯讀，狀態由 ControlLaws 更新。
struct ControlLawData
{
    /// @brief 載入 FLCC/ControlLaws.jsonc，初始化唯讀設定與運算狀態。
    ControlLawData();
	const ControlLawConfiguration configuration; ///< 本次飛行使用的完整設定。
	ControlLawState state; ///< 控制律最近一次完成運算的狀態。
};
}
