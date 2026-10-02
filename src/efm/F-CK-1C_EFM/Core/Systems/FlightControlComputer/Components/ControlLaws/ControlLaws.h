#pragma once

#include <chrono>

namespace Core::Systems::Flcc
{
struct InputOutputData;
struct ControlLawData;
struct SystemStatusTables;

/// @brief 執行 FLCC 控制律的元件。
/// 尚未實作：控制律、模式、濾波、積分與控制需求計算。
/// 資料庫於建構時以參照綁定，不擁有資料庫，也不向 Executive 註冊自身。
class ControlLaws final
{
public:
	/// @brief 綁定此元件固定使用的資料庫；資料庫必須比元件更晚銷毀。
	/// @param input_output 供讀取輸入與寫入控制需求的分區。
	/// @param control_law 控制律元件間的共用資料分區。
	/// @param system_status 唯讀的系統狀態分區。
	ControlLaws(
		InputOutputData& input_output,
		ControlLawData& control_law,
		const SystemStatusTables& system_status);

	/// @brief 接收 Executive 的一次排程呼叫；目前不執行運算或修改資料。
	/// @param scheduled_time 此任務本次預定的絕對模擬時間。
	/// @param dt_s 此任務自身的固定週期，單位為秒。
	/// 尚未實作：控制律、模式、濾波、積分與控制需求計算。
	void step(std::chrono::nanoseconds scheduled_time, double dt_s);

private:
	InputOutputData& input_output_; ///< 供讀取輸入與寫入控制需求的分區。
	ControlLawData& control_law_; ///< 控制律元件間的共用資料分區。
	const SystemStatusTables& system_status_; ///< 唯讀的系統狀態分區。
};
}
