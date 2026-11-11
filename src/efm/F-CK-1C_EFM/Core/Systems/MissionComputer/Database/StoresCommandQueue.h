#pragma once

#include "ExternalActionQueue.h"
#include "WeaponUseData.h"
#include "../../../Contracts/CockpitContracts.h"

#include <deque>
#include <optional>
#include <variant>
#include <sigslot/signal.hpp>

namespace Core::Systems::Mc
{
/// @brief 依模式選擇掛點，或在重選同模式時循環掛點。
struct SelectStoresForMode
{
	OperatingMode mode = OperatingMode::Navigation;
	bool cycle_weapon = false;
	bool reassert_selection = false; ///< 重新確認目前候選，保留仍有效的掛點。
};

/// @brief 火控已判定的武器操作，由 SM 依序轉送至 DCS。
struct StoresWeaponAction
{
	MissionActionKind action = MissionActionKind::None;
};

/// @brief 取消尚待執行的選站與武器啟動要求。
struct CancelStoresCommands
{
};

/// @brief 撤回尚待執行的武器啟動要求，保留選站與停止操作。
struct CancelWeaponStarts
{
};

/// @brief 火控明確選擇的投放許可處理方式。
enum class LaunchPermission
{
	Normal,
	Override
};

/// @brief 火控已判定的開始意圖，明確攜帶投放超控選擇。
struct BeginWeaponUse
{
	WeaponUseKind kind = WeaponUseKind::Gun;
	LaunchPermission launch_permission = LaunchPermission::Normal;
};

/// @brief 正常放開一筆已接受的使用要求。
struct EndWeaponUse
{
	WeaponUseReference use;
};

/// @brief 立即中止先前武器使用，安排撤回與必要停止。
struct AbortWeaponUse
{
};

using StoresCommand = std::variant<SelectStoresForMode,
                                   StoresWeaponAction,
                                   CancelStoresCommands,
                                   CancelWeaponStarts,
                                   BeginWeaponUse,
                                   EndWeaponUse,
                                   AbortWeaponUse>;

/// @brief 已入列的原始要求及其 SM 指令編號。
struct QueuedStoresCommand
{
	std::uint64_t ticket;
	StoresCommand command;
};

/// @brief SM 的具型別指令入口，保存原始 FIFO、指令編號及執行回覆。
class StoresCommandQueue final
{
public:
	explicit StoresCommandQueue(ExternalActionQueue& actions);
	StoresCommandQueue(const StoresCommandQueue&) = delete;
	StoresCommandQueue& operator=(const StoresCommandQueue&) = delete;
	StoresCommandQueue(StoresCommandQueue&&) = delete;
	StoresCommandQueue& operator=(StoresCommandQueue&&) = delete;

	/// @brief 提交模式、武器操作或取消要求，回傳供呼叫端追蹤執行確認的 SM 指令編號。
	std::uint64_t submit(const StoresCommand& command);
	/// @brief 回報要求已由 SM 處理，或其外部動作已完成確認。
	bool acknowledged(std::uint64_t ticket) const noexcept;
	/// @brief 依 FIFO 交付下一筆原始要求，並回收已完成的回覆紀錄。
	std::optional<QueuedStoresCommand> take_next();
	/// @brief 登記執行序號；0 表示要求已在 SM 完成處理。
	void resolve(std::uint64_t ticket, std::uint64_t external_sequence);
	/// @brief 原始要求入列後，同步通知 MC 交由 SM 接受與派送。
	sigslot::signal_st<> commands_available;

private:
	struct Receipt
	{
		bool resolved = false;
		std::uint64_t external_sequence = 0;
	};
	void retire_completed();
	std::uint64_t enqueue(const StoresCommand& command);
	ExternalActionQueue& actions_;
	std::deque<QueuedStoresCommand> incoming_;
	std::deque<Receipt> receipts_;
	std::uint64_t last_ticket_ = 0;
	std::uint64_t retired_through_ = 0;
};
}
