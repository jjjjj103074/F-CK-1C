#pragma once

#include "../Executive/TaskScheduledTime.h"

namespace Core::Systems::Flcc
{
struct ControlLawData;
struct SystemStatusTables;

/// @brief 管理 FLCC 啟動與重新啟動程序的元件。
/// 尚未實作：啟動檢查、資料初始化與重新啟動程序。
/// 資料庫於建構時以參照綁定，不擁有資料庫，也不向 Executive 註冊自身。
class StartupAndRestart final
{
public:
	/// @brief 綁定此元件固定使用的資料庫；資料庫必須比元件更晚銷毀。
	/// @param control_law 供啟動程序初始化的控制律資料分區。
	/// @param system_status 供啟動狀態讀寫的狀態分區。
	StartupAndRestart(
		ControlLawData& control_law,
		SystemStatusTables& system_status);

	/// @brief 接收 Executive 的一次排程呼叫；目前不執行運算或修改資料。
	/// @param scheduled_time 此任務本次預定的絕對模擬時間。
	/// @param dt_s 此任務自身的固定週期，單位為秒。
	/// 尚未實作：啟動檢查、資料初始化與重新啟動程序。
	void step(TaskScheduledTime scheduled_time, double dt_s);

private:
	ControlLawData& control_law_; ///< 供啟動程序初始化的控制律資料分區。
	SystemStatusTables& system_status_; ///< 供啟動狀態讀寫的狀態分區。
};
}
