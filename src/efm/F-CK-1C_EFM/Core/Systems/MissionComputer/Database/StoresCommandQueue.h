#pragma once

#include "ExternalActionQueue.h"
#include "../../../Contracts/CockpitContracts.h"

#include <deque>
#include <variant>

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

using StoresCommand = std::variant<SelectStoresForMode, StoresWeaponAction, CancelStoresCommands>;
class StoresManagement;

/// @brief SM 的具型別指令入口，保存 FIFO、指令編號及各筆要求的執行回覆。
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
	/// @brief 回報該筆要求已完成執行或取消。
	bool acknowledged(std::uint64_t ticket) const noexcept;

private:
	friend class StoresManagement;
	struct Entry
	{
		std::uint64_t ticket;
		StoresCommand command;
		bool canceled = false;
	};
	struct Receipt
	{
		bool resolved = false;
		std::uint64_t external_sequence = 0;
	};
	void resolve(std::uint64_t ticket, std::uint64_t external_sequence);
	void retire_completed();
	std::uint64_t enqueue(const StoresCommand& command);
	void cancel_pending();
	bool has_pending_mode() const;
	OperatingMode latest_mode_ = OperatingMode::Navigation;
	ExternalActionQueue& actions_;
	std::deque<Entry> pending_;
	std::deque<Receipt> receipts_;
	std::uint64_t last_ticket_ = 0;
	std::uint64_t retired_through_ = 0;
};
}
