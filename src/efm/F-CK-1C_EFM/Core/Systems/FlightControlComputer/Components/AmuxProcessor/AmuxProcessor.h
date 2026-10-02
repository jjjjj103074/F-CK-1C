#pragma once

#include "../Executive/TaskScheduledTime.h"

namespace Core::Systems::Flcc
{
struct AmuxData;

/// @brief 處理 FLCC AMUX 介面的元件。
/// 尚未實作：AMUX 接收、傳送與資料處理。
/// 資料庫於建構時以參照綁定，不擁有資料庫，也不向 Executive 註冊自身。
class AmuxProcessor final
{
public:
	/// @brief 綁定此元件固定使用的資料庫；資料庫必須比元件更晚銷毀。
	/// @param amux 固定綁定的 AMUX 資料分區。
	explicit AmuxProcessor(
		AmuxData& amux);

	/// @brief 接收 Executive 的一次排程呼叫；目前不執行運算或修改資料。
	/// @param scheduled_time 此任務本次預定的絕對模擬時間。
	/// @param dt_s 此任務自身的固定週期，單位為秒。
	/// 尚未實作：AMUX 接收、傳送與資料處理。
	void step(TaskScheduledTime scheduled_time, double dt_s);

private:
	AmuxData& amux_; ///< 固定綁定的 AMUX 資料分區。
};
}
