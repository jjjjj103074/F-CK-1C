#pragma once

#include <chrono>

namespace Core::Systems::Flcc
{
struct InputOutputData;
struct SystemStatusTables;

/// @brief 管理資料選擇與選擇結果監控的元件。
/// 尚未實作：輸入選擇、選擇有效性判斷與狀態更新。
/// 資料庫於建構時以參照綁定，不擁有資料庫，也不向 Executive 註冊自身。
class SelectorMonitor final
{
public:
	/// @brief 綁定此元件固定使用的資料庫；資料庫必須比元件更晚銷毀。
	/// @param input_output 唯讀的輸入輸出分區。
	/// @param system_status 供選擇狀態寫入的狀態分區。
	SelectorMonitor(
		const InputOutputData& input_output,
		SystemStatusTables& system_status);

	/// @brief 接收 Executive 的一次排程呼叫；目前不執行運算或修改資料。
	/// @param scheduled_time 此任務本次預定的絕對模擬時間。
	/// @param dt_s 此任務自身的固定週期，單位為秒。
	/// 尚未實作：輸入選擇、選擇有效性判斷與狀態更新。
	void step(std::chrono::nanoseconds scheduled_time, double dt_s);

private:
	const InputOutputData& input_output_; ///< 唯讀的輸入輸出分區。
	SystemStatusTables& system_status_; ///< 供選擇狀態寫入的狀態分區。
};
}
