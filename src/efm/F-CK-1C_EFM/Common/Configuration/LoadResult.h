#pragma once

#include "ConfigurationTools.h"

#include <utility>
#include <variant>

namespace Configuration
{
    /// @brief 一次載入的結果，僅保存完整設定或失敗診斷其中之一。
    template <typename T>
    class [[nodiscard]] LoadResult final
    {
    public:
        static LoadResult success(T value)
        {
            return LoadResult(Storage(std::in_place_index<0>, std::move(value)));
        }

        static LoadResult failure(Diagnostic error)
        {
            return LoadResult(Storage(std::in_place_index<1>, std::move(error)));
        }

        /// @brief 表示是否已取得完整且通過驗證的設定。
        bool succeeded() const noexcept { return result_.index() == 0; }

        /// @brief 取得成功設定；失敗結果呼叫此方法會拋出 std::bad_variant_access。
        T& value() { return std::get<0>(result_); }
        const T& value() const { return std::get<0>(result_); }

        /// @brief 取得失敗診斷；成功結果呼叫此方法會拋出 std::bad_variant_access。
        const Diagnostic& error() const { return std::get<1>(result_); }

    private:
        using Storage = std::variant<T, Diagnostic>;

        explicit LoadResult(Storage result) : result_(std::move(result)) {}

        Storage result_;
    };
}
