#pragma once

#include <cstdint>

namespace Core::Systems::Mc
{
/// @brief 武器使用要求的操作種類。
enum class WeaponUseKind
{
	Gun,
	Release
};

/// @brief SM 保存的單次使用執行階段。
enum class WeaponUsePhase
{
	Idle,
	StartPending, ///< 已接受開始要求，等待執行確認。
	StartAcknowledged, ///< 開始操作已取得執行確認。
	StopScheduled, ///< 正常停止已排入共用動作佇列。
	Canceled ///< 使用要求已取消，SM 保存停止收尾。
};

/// @brief 對應一筆已接受的武器使用要求。
struct WeaponUseReference
{
	WeaponUseKind kind = WeaponUseKind::Gun;
	std::uint64_t request_id = 0;
};

/// @brief 單一操作種類的執行結果；SM 唯一寫入，火控唯讀。
struct WeaponUseExecutionState
{
	std::uint64_t request_id = 0; ///< 最近接受的開始要求，以 SM 指令編號識別。
	WeaponUsePhase phase = WeaponUsePhase::Idle;
	std::uint64_t rejected_request_id = 0; ///< 最近拒絕的開始要求，與已接受程序分別保存。

	bool in_progress() const noexcept
	{
		return phase == WeaponUsePhase::StartPending || phase == WeaponUsePhase::StartAcknowledged;
	}
};

/// @brief 機砲與投放各自保存的使用執行資料。
struct WeaponUseData
{
	WeaponUseExecutionState gun;
	WeaponUseExecutionState release;
};
}
