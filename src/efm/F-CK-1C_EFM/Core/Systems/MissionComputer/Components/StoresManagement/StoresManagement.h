#pragma once

#include "../../Database/InputOutputData.h"
#include "../../Database/StoresData.h"
#include "../../Database/StoresCommandQueue.h"
#include "../../StoresEvents.h"
#include "Internal/StationSelection.h"

#include <deque>
#include <optional>

namespace Core::Systems::Mc
{
/// @brief 統整掛載觀測，處理指令規則與執行順序，完成資料更新後發布通知。
class StoresManagement final
{
public:
	/// @brief 綁定觀測、SM 狀態、事件、指令入口與有序執行出口。
	StoresManagement(const MissionInputs& input,
	                 StoresData& stores,
	                 StoresEvents& events,
	                 StoresCommandQueue& commands,
	                 ExternalActionQueue& actions);
	StoresManagement(const StoresManagement&) = delete;
	StoresManagement& operator=(const StoresManagement&) = delete;
	StoresManagement(StoresManagement&&) = delete;
	StoresManagement& operator=(StoresManagement&&) = delete;

	/// @brief 接受入口要求並更新觀測與自動選站。
	void prepare(std::chrono::nanoseconds time);
	/// @brief 更新觀測、消費指令並發布已完成的資料變化。
	void step(std::chrono::nanoseconds time, double dt_s);
	/// @brief 接受入口要求、派送就緒操作，並發布資料變化。
	void dispatch();

private:
	struct PendingCommand
	{
		std::optional<std::uint64_t> ticket; ///< 對應外部要求的完成回覆。
		StoresCommand command;
		bool canceled = false;
	};
	/// @brief 接受入口要求，安排停止、取消與模式接續。
	void accept_commands();
	/// @brief 撤回待執行的選站與武器啟動，保留停止操作。
	void cancel_pending_commands();
	/// @brief 撤回執行佇列中的選站與啟動，記錄選站取消版本。
	void cancel_dispatched_commands();
	/// @brief 查詢待執行的作戰模式選站要求。
	bool has_pending_mode() const;
	MissionAction selection_action() const;
	void drain_commands();
	void notify_changes();
	const MissionInputs& input_;
	StoresData& stores_;
	StoresEvents& events_;
	StoresCommandQueue& commands_;
	ExternalActionQueue& actions_;
	OperatingMode latest_mode_ = OperatingMode::Navigation;
	std::deque<PendingCommand> pending_commands_;
	Internal::StationSelection selection_;
	InventoryChanged inventory_change_;
	bool gun_changed_ = false;
	bool selection_changed_ = false;
	bool dispatching_ = false;
};
}
