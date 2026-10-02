#include "TaskFrequency.h"

#include <limits>
#include <numeric>
#include <stdexcept>

namespace Core::Systems::Flcc
{
namespace
{
constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000;
}

TaskFrequency::TaskFrequency(std::uint32_t numerator, std::uint32_t denominator)
    : numerator_(numerator), denominator_(denominator)
{
    if (numerator == 0 || denominator == 0 ||
        numerator > kNanosecondsPerSecond * denominator)
    {
        throw std::invalid_argument("FLCC task frequency must be positive and at most 1 GHz.");
    }
    const auto divisor = std::gcd(numerator, denominator);
    numerator_ /= divisor;
    denominator_ /= divisor;
}

std::uint32_t TaskFrequency::numerator() const noexcept
{
    return numerator_;
}

std::uint32_t TaskFrequency::denominator() const noexcept
{
    return denominator_;
}

double TaskFrequency::period_s() const noexcept
{
    return static_cast<double>(denominator_) / numerator_;
}

std::chrono::nanoseconds TaskFrequency::tick_time(std::uint64_t tick) const
{
    const std::uint64_t period_numerator = kNanosecondsPerSecond * denominator_;
    const std::uint64_t whole_period = period_numerator / numerator_;
    const std::uint64_t remainder = period_numerator % numerator_;
    const auto maximum = static_cast<std::uint64_t>(
        (std::numeric_limits<std::chrono::nanoseconds::rep>::max)());
    if (tick > maximum / whole_period)
    {
        throw std::overflow_error("FLCC task schedule exceeded its time range.");
    }

    // 分解商與餘數，避免直接計算 tick * period_numerator 的中間乘法溢位。
    // 餘數相乘的兩個運算元均小於 32 位元分子，因此乘積可由 uint64_t 表示。
    const std::uint64_t whole = tick * whole_period;
    const std::uint64_t fraction = (tick / numerator_) * remainder +
        ((tick % numerator_) * remainder) / numerator_;
    if (fraction > maximum - whole)
    {
        throw std::overflow_error("FLCC task schedule exceeded its time range.");
    }
    return std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(whole + fraction));
}
}
