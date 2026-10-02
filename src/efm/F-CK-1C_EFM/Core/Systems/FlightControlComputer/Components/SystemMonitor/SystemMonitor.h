#pragma once

#include "../Executive/TaskScheduledTime.h"

namespace Core::Systems::Flcc
{
struct InputOutputData;
struct SystemStatusTables;

/// @brief 監控 FLCC 輸入與系統狀態的元件。
/// 尚未實作：輸入合理性檢查、系統監控與狀態更新。
/// 資料庫於建構時以參照綁定，不擁有資料庫，也不向 Executive 註冊自身。
class SystemMonitor final
{
public:
	/// @brief 綁定此元件固定使用的資料庫；資料庫必須比元件更晚銷毀。
	/// @param input_output 唯讀的輸入輸出分區。
	/// @param system_status 供監控結果寫入的狀態分區。
	SystemMonitor(
		const InputOutputData& input_output,
		SystemStatusTables& system_status);

	/// @brief 接收 Executive 的一次排程呼叫；目前不執行運算或修改資料。
	/// @param scheduled_time 此任務本次預定的絕對模擬時間。
	/// @param dt_s 此任務自身的固定週期，單位為秒。
	/// 尚未實作：輸入合理性檢查、系統監控與狀態更新。
	void step(TaskScheduledTime scheduled_time, double dt_s);

private:
	const InputOutputData& input_output_; ///< 唯讀的輸入輸出分區。
	SystemStatusTables& system_status_; ///< 供監控結果寫入的狀態分區。
};
}
