#pragma once

namespace Common
{
// 各系統共用的按鈕值門檻；此檔只解讀數值，不依賴具體指令型別。
inline constexpr double kCommandPressThreshold = 0.5;

/**
 * @brief 判斷正規化指令值是否表示按鈕已按下。
 * @param value_normalized 已轉成共用格式的正規化按鈕指令值。
 * @param threshold 判定門檻；未指定時使用共用的 0.5。
 * @return 數值嚴格大於指定門檻時為 true，其餘情況為 false。
 */
constexpr bool command_value_is_pressed(
	double value_normalized,
	double threshold = kCommandPressThreshold) noexcept
{
	return value_normalized > threshold;
}
}
