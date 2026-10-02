#pragma once

#include "../Executive/TaskScheduledTime.h"

namespace Core::Systems::Flcc
{
struct SystemStatusTables;

/// @brief 管理 FLCC 故障狀態的元件。
/// 尚未實作：故障判定、故障處理與降級策略；本階段不處理備援。
/// 資料庫於建構時以參照綁定，不擁有資料庫，也不向 Executive 註冊自身。
class FailureManager final
{
public:
	/// @brief 綁定此元件固定使用的資料庫；資料庫必須比元件更晚銷毀。
	/// @param system_status 供故障狀態讀寫的狀態分區。
	explicit FailureManager(
		SystemStatusTables& system_status);

	/// @brief 接收 Executive 的一次排程呼叫；目前不執行運算或修改資料。
	/// @param scheduled_time 此任務本次預定的絕對模擬時間。
	/// @param dt_s 此任務自身的固定週期，單位為秒。
	/// 尚未實作：故障判定、故障處理與降級策略；本階段不處理備援。
	void step(TaskScheduledTime scheduled_time, double dt_s);

private:
	SystemStatusTables& system_status_; ///< 供故障狀態讀寫的狀態分區。
};
}
