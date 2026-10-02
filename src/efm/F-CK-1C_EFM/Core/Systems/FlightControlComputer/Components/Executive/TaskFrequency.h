#pragma once

#include <chrono>
#include <cstdint>

namespace Core::Systems::Flcc
{
    /// @brief 以正整數分數表示的 Hz；不依賴外層系統的呼叫頻率。
    class TaskFrequency final
    {
    public:
        /// @brief 建立並約分頻率工具，例如 (64) 表示 64 Hz，(5, 2) 表示 2.5 Hz。
        /// @param numerator 每秒執行次數的分子。
        /// @param denominator 每秒執行次數的分母，預設為 1。
        /// @throws std::invalid_argument 任一參數為零，或頻率超過奈秒解析度的 1 GHz。
        explicit TaskFrequency(std::uint32_t numerator, std::uint32_t denominator = 1);

        /// @brief 取得約分後的頻率分子。
        std::uint32_t numerator() const noexcept;

        /// @brief 取得約分後的頻率分母。
        std::uint32_t denominator() const noexcept;

        /// @brief 取得提供給演算法的固定週期，單位為秒；不供排程判斷使用。
        double period_s() const noexcept;

        /// @brief 由整數週期次數計算絕對到期時間，不累加捨入後的週期。
        /// @param tick 從零時刻起的週期次數；第一次呼叫使用 1。
        /// @return 向下取整至奈秒的預定模擬時間。
        /// @throws std::overflow_error 到期時間超出 std::chrono::nanoseconds 的範圍。
        std::chrono::nanoseconds tick_time(std::uint64_t tick) const;

    private:
        std::uint32_t numerator_;   ///< 約分後的正整數分子。
        std::uint32_t denominator_; ///< 約分後的正整數分母。
    };
}
