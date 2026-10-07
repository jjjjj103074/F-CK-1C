#include "Configuration.h"

#include <utility>

namespace
{
    std::filesystem::path fm_root;                       // 共用的 FM 根目錄。
    Configuration::DiagnosticReporter report_diagnostic; // 設定載入失敗的診斷出口。
}

namespace Configuration
{
    std::optional<Diagnostic> detail::load(const std::filesystem::path &relative_path,
                                           const std::function<void(const Value &)> &convert)
    {
        // 路徑解析失敗時保留請求位置，成功後使用實際檔案位置。
        auto file_path = fm_root / relative_path;
        try
        {
            // 解析與驗證檔案位置
            file_path = Tools::resolve_file(fm_root, relative_path);

            // 讀取檔案並驗證
            auto source_text = Tools::read_file(file_path);

            // 將設定檔轉成 JSON 資料樹
            auto json_document = Tools::parse_jsonc(std::move(source_text));

            // 將 JSON 資料樹轉換成指定的設定結構，並填入load的result
            convert(json_document.root());

            return std::nullopt;
        }
        catch (const Error &error)
        {
            // 補上來源檔案，回報並回傳同一份完整診斷。
            Diagnostic diagnostic = error.diagnostic();
            diagnostic.file = file_path;
            if (report_diagnostic)
                report_diagnostic(diagnostic);
            return diagnostic;
        }
    }

    void initialize(std::filesystem::path root, DiagnosticReporter reporter)
    {
        if (!reporter)
            throw std::invalid_argument("Configuration 必須提供診斷回報出口");
        const auto absolute_root = root.empty() ? root : std::filesystem::absolute(root);
        fm_root = absolute_root;
        report_diagnostic = std::move(reporter);
    }

    void shutdown() noexcept
    {
        report_diagnostic = nullptr;
        fm_root.clear();
    }

}
