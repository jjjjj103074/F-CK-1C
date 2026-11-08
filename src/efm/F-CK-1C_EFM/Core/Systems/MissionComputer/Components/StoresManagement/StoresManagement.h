#pragma once

#include "../../Database/InputOutputData.h"
#include "../../Database/StoresData.h"
#include "../../Database/StoresCommandQueue.h"
#include "../../StoresEvents.h"
#include "Internal/StationSelection.h"

namespace Core::Systems::Mc
{
/// @brief 統整掛載觀測，依序消費 SM 指令入口，完成資料更新後發布通知。
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

	/// @brief 在本次操作前更新觀測與自動選站，保留新要求的執行順序。
	void prepare(std::chrono::nanoseconds time);
	/// @brief 更新觀測、消費指令並發布已完成的資料變化。
	void step(std::chrono::nanoseconds time, double dt_s);
	/// @brief 派送本次火控計時器產生的後續指令，並發布資料變化。
	void dispatch();

private:
	MissionAction selection_action() const;
	void drain_commands();
	void notify_changes();
	const MissionInputs& input_;
	StoresData& stores_;
	StoresEvents& events_;
	StoresCommandQueue& commands_;
	ExternalActionQueue& actions_;
	Internal::StationSelection selection_;
	InventoryChanged inventory_change_;
	bool gun_changed_ = false;
	bool selection_changed_ = false;
	bool dispatching_ = false;
};
}
