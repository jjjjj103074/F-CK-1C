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

/// @brief SM 的具型別指令入口，保存 FIFO 並為每筆要求保留 MC 全域執行順序。
class StoresCommandQueue final
{
public:
	explicit StoresCommandQueue(ExternalActionQueue& actions);
	StoresCommandQueue(const StoresCommandQueue&) = delete;
	StoresCommandQueue& operator=(const StoresCommandQueue&) = delete;
	StoresCommandQueue(StoresCommandQueue&&) = delete;
	StoresCommandQueue& operator=(StoresCommandQueue&&) = delete;

	/// @brief 提交模式、武器操作或取消要求，回傳既有動作確認所使用的序號。
	std::uint64_t submit(const StoresCommand& command);

private:
	friend class StoresManagement;
	struct Entry
	{
		std::uint64_t sequence;
		StoresCommand command;
		bool canceled = false;
	};
	std::uint64_t enqueue(const StoresCommand& command);
	void cancel_pending();
	bool has_pending_mode() const;
	OperatingMode latest_mode_ = OperatingMode::Navigation;
	ExternalActionQueue& actions_;
	std::deque<Entry> pending_;
};
}
