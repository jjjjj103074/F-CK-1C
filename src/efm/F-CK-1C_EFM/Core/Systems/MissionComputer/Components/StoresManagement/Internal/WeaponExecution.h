#pragma once

#include "../../../Database/StoresCommandQueue.h"
#include "../../../Database/WeaponUseData.h"

#include <chrono>
#include <optional>

namespace Core::Systems::Mc::Internal
{
/// @brief 推進一條武器操作的執行確認、正常放開與取消狀態。
class WeaponExecution final
{
public:
	WeaponExecution(WeaponUseExecutionState& state, const StoresCommandQueue& commands);
	/// @brief 接受可執行的開始要求，或記錄本次拒絕。
	bool begin(std::uint64_t request_id, bool available);
	/// @brief 保存相符程序的正常結束意圖。
	void request_end(std::uint64_t request_id);
	/// @brief 觀察開始 ACK，判斷正常結束是否已滿足保持時間。
	bool end_due(std::chrono::nanoseconds time);
	/// @brief 停止已排入後，將相符程序標為正常結束。
	void stop_scheduled(std::uint64_t request_id);
	/// @brief 取消尚待確認的啟動程序。
	void cancel_pending();
	/// @brief 取消目前程序，回報是否需要安排停止。
	bool abort();

private:
	void finish(WeaponUsePhase phase);
	WeaponUseExecutionState& state_;
	const StoresCommandQueue& commands_;
	std::optional<std::chrono::nanoseconds> started_at_;
	bool end_requested_ = false;
	bool stop_required_ = false; ///< 已接受啟動後，保留必要停止的責任。
};
}
