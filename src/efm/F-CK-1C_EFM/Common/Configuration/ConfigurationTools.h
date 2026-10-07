#pragma once

#include <filesystem>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>

/// @brief JSONC 文件、欄位檢視、診斷及檔案讀取工具。
namespace Configuration
{
    /// @brief 一次設定診斷；來源檔案由 load 補上，行列的 0 表示該資訊不存在。
    struct Diagnostic
    {
        std::filesystem::path file;
        std::string stage;
        std::string field;
        std::size_t line = 0;
        std::size_t column = 0;
        std::string reason;

        /// @brief 將檔名、處理階段、位置、欄位與原因組成 UTF-8 訊息。
        std::string message() const;
    };

    /// @brief 設定處理失敗的例外，保存階段、欄位與原因；what() 提供原因。
    class Error final : public std::runtime_error
    {
    public:
        Error(std::string stage, std::string reason, std::string field = {});
        const Diagnostic &diagnostic() const noexcept { return diagnostic_; }

    private:
        Diagnostic diagnostic_;
    };

    using DiagnosticReporter = std::function<void(const Diagnostic&)>;

    class JsonDocument;

    namespace Tools
    {
        /// @brief 拒絕絕對路徑，確認解析後的實際文件位於 FM 內並回傳其位置。
        std::filesystem::path resolve_file(const std::filesystem::path& fm_root,
                                           const std::filesystem::path& relative_path);
        /// @brief 開啟檔案並從串流取得大小，一次讀入原文；單份上限 16 MiB。
        /// I/O 失敗與內容長度變動均回報為設定讀取錯誤。
        std::string read_file(const std::filesystem::path& file_path);
        /// @brief 拒絕原始 NUL，由 nlohmann 一次解析 JSONC。
        /// 語法與資料字串的 UTF-8 由解析器驗證，原生行列訊息保留於診斷原因。
        JsonDocument parse_jsonc(std::string source_text);
    }

    /// @brief 本次載入中某個 JSON 值的唯讀檢視；保留完整欄位路徑。
    /// 參照所屬 JsonDocument 的內容，有效期間由該 JsonDocument 的生命週期決定。
    class Value final
    {
    public:
        /// @brief 驗證物件的欄位集合；多餘或缺少欄位均拋出 Error。
        void object(std::initializer_list<const char *> fields) const;
        /// @brief 取得必填欄位；缺漏時指出該欄位的完整路徑。
        Value member(const char *name) const;
        /// @brief 取得陣列元素並保留索引位置。
        Value element(std::size_t index) const;
        /// @brief 要求此值為陣列並取得元素數量。
        std::size_t array_size() const;
        /// @brief 驗證 JSON 數值型別，取得可由 double 表示的有限數值。
        double number() const;
        /// @brief 驗證 JSON 布林型別並取得其值。
        bool boolean() const;
        /// @brief 驗證 JSON 字串型別並取得 UTF-8 內容。
        std::string string() const;

        /// @brief 驗證型別自己的數值規則；失敗指出完整欄位路徑與原因。
        void require(bool condition, const char *reason) const;

    private:
        friend class JsonDocument;
        Value(JsonDocument &json_document, std::string field);
        const auto &json() const;

        JsonDocument &json_document_;
        std::string field_;
    };

    /// @brief 擁有本次載入的 JSON 根節點，於解構時釋放。
    class JsonDocument final
    {
    public:
        ~JsonDocument();
        JsonDocument(JsonDocument &&) noexcept;
        JsonDocument &operator=(JsonDocument &&) noexcept;
        Value root();

    private:
        friend class Value;
        friend JsonDocument Tools::parse_jsonc(std::string);
        JsonDocument();
        struct Storage; ///< JSON 根節點，定義於實作檔。
        std::unique_ptr<Storage> storage_;
    };

}
