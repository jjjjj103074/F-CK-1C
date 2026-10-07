#pragma once

#include "LoadResult.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <utility>

/// @brief 共用 JSONC 載入入口；設定與載入時機由各資料庫管理。
namespace Configuration
{
    namespace detail
    {
        /// @brief 執行文件載入及轉換的共用實作，供 load<T> 的型別接合使用。
        /// @return 成功回傳 std::nullopt；設定錯誤回報並回傳完整診斷。
        /// 設定錯誤由本流程補上來源檔案後回報。
        /// 其他例外維持原樣傳遞。
        [[nodiscard]] std::optional<Diagnostic> load(
            const std::filesystem::path& relative_path,
            const std::function<void(const Value&)>& convert);
    }

    /// @brief 取得完整且已驗證的設定結構；每次呼叫獨立讀取指定文件。
    /// @param relative_path 以 FM 為起點的非絕對路徑；解析後的實際文件必須位於 FM 內。
    /// @return 完整設定或失敗診斷，由呼叫端決定後續處理。
    /// 程式或資源例外維持原樣傳遞，不轉成設定錯誤。
    template <typename T>
    LoadResult<T> load(const std::filesystem::path& relative_path)
    {
        T result{};
        auto error = detail::load(relative_path, [&result](const Value& root)
        {
            // C++ 的引數相依查找依 T 所屬命名空間選擇對應的欄位轉換函式。
            read_configuration(root, result);
        });
        if (error)
            return LoadResult<T>::failure(std::move(*error));
        return LoadResult<T>::success(std::move(result));
    }

    /// @brief 設定單執行緒共用的 FM 根目錄與診斷回報函式。
    void initialize(std::filesystem::path fm_root, DiagnosticReporter report_diagnostic);

    /// @brief 清除共用的 FM 根目錄與診斷回報函式。
    void shutdown() noexcept;
}
