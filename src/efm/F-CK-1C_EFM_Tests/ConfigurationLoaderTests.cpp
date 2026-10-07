#include "TestFileUtils.h"
#include "TestHarness.h"

#include "Common/Configuration/Configuration.h"

#include <filesystem>
#include <fstream>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    struct SampleConfiguration
    {
        std::vector<double> values;
    };

    void read_configuration(const Configuration::Value& root, SampleConfiguration& result)
    {
        root.object({"values"});
        const auto values = root.member("values");
        for (std::size_t index = 0; index < values.array_size(); ++index)
            result.values.push_back(values.element(index).number());
    }

    struct ProgramFailureConfiguration {};
    void read_configuration(const Configuration::Value&, ProgramFailureConfiguration&)
    {
        throw std::logic_error("conversion defect");
    }

    struct ResourceFailureConfiguration {};
    void read_configuration(const Configuration::Value&, ResourceFailureConfiguration&)
    {
        throw std::bad_alloc();
    }

    /// @brief 在測試期間捕捉診斷，離開作用域時清除共用回呼。
    class DiagnosticCapture final
    {
    public:
        explicit DiagnosticCapture(const std::filesystem::path& root)
        {
            Configuration::initialize(root, [this](const Configuration::Diagnostic& diagnostic)
            {
                diagnostics.push_back(diagnostic);
            });
        }

        ~DiagnosticCapture() { Configuration::shutdown(); }

        std::vector<Configuration::Diagnostic> diagnostics;
    };
}

void run_configuration_loader_tests(Tests::Context& tests)
{
    TestFiles::TemporaryDirectory root("cfg");
    TEST_EXPECT(tests, root.valid());
    if (!root.valid())
        return;
    const auto fm = root.path() / "FM";
    std::filesystem::create_directory(fm);
    const auto file = fm / "LoaderTests.jsonc";
    DiagnosticCapture capture(fm);

    // 讀檔工具接受空內容與大小上限，原文是否為有效 JSONC 由解析器判斷。
    TestFiles::write_text(file, "");
    TEST_EXPECT(tests, Configuration::Tools::read_file(file).empty());
    constexpr std::size_t maximum_size = 16 * 1024 * 1024;
    std::filesystem::resize_file(file, maximum_size);
    TEST_EXPECT(tests, Configuration::Tools::read_file(file).size() == maximum_size);
    std::filesystem::resize_file(file, maximum_size + 1);
    bool oversized_file = false;
    try
    {
        (void)Configuration::Tools::read_file(file);
    }
    catch (const Configuration::Error& error)
    {
        oversized_file = error.diagnostic().file.empty() &&
                         error.diagnostic().stage == "讀取" &&
                         error.diagnostic().reason.find("16 MiB") != std::string::npos;
    }
    TEST_EXPECT(tests, oversized_file);
    const auto oversized = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
    TEST_EXPECT(tests, !oversized.succeeded());
    TEST_EXPECT(tests, capture.diagnostics.size() == 1);
    if (!oversized.succeeded() && capture.diagnostics.size() == 1)
    {
        TEST_EXPECT(tests, oversized.error().file == std::filesystem::canonical(file));
        TEST_EXPECT(tests, oversized.error().stage == "讀取");
        TEST_EXPECT(tests, oversized.error().message().find("[Error] ") == 0);
        TEST_EXPECT(tests, capture.diagnostics[0].message() == oversized.error().message());
    }
    capture.diagnostics.clear();

    // 工具錯誤保留操作名稱；load 補上來源並回報相同的完整診斷。
    const auto missing_file = fm / "Missing.jsonc";
    bool open_failure = false;
    try
    {
        (void)Configuration::Tools::read_file(missing_file);
    }
    catch (const Configuration::Error& error)
    {
        open_failure = error.diagnostic().file.empty() &&
                       error.diagnostic().stage == "讀取" &&
                       error.diagnostic().reason.find("開啟設定檔") != std::string::npos;
    }
    TEST_EXPECT(tests, open_failure);
    TEST_EXPECT(tests, capture.diagnostics.empty());
    const auto unresolved = Configuration::load<SampleConfiguration>("Missing.jsonc");
    TEST_EXPECT(tests, !unresolved.succeeded());
    TEST_EXPECT(tests, capture.diagnostics.size() == 1);
    if (!unresolved.succeeded() && capture.diagnostics.size() == 1)
    {
        TEST_EXPECT(tests, unresolved.error().file == missing_file);
        TEST_EXPECT(tests, unresolved.error().stage == "讀取");
        TEST_EXPECT(tests, capture.diagnostics[0].message() == unresolved.error().message());
    }
    capture.diagnostics.clear();

    // 註解中的二、三、四位元組 UTF-8 序列與設定資料都能正常載入。
    TestFiles::write_text(file, "// UTF-8：é、繁體中文、\xf0\x9f\x98\x80\n{\"values\": [0, 1]}");
    const auto loaded = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
    TEST_EXPECT(tests, loaded.succeeded());
    if (loaded.succeeded())
        TEST_EXPECT(tests, loaded.value().values == std::vector<double>({0.0, 1.0}));
    TEST_EXPECT(tests, capture.diagnostics.empty());

    // 路徑中的 .. 與副檔名不限制載入，解析後的文件必須位於 FM 內。
    std::filesystem::create_directory(fm / "Nested");
    const auto parent_path = Configuration::load<SampleConfiguration>("Nested/../LoaderTests.jsonc");
    TEST_EXPECT(tests, parent_path.succeeded());
    if (parent_path.succeeded())
        TEST_EXPECT(tests, parent_path.value().values == std::vector<double>({0.0, 1.0}));
    TestFiles::write_text(fm / "LoaderTests.txt", "{\"values\": [0, 1]}");
    const auto other_extension = Configuration::load<SampleConfiguration>("LoaderTests.txt");
    TEST_EXPECT(tests, other_extension.succeeded());
    if (other_extension.succeeded())
        TEST_EXPECT(tests, other_extension.value().values == std::vector<double>({0.0, 1.0}));
    TEST_EXPECT(tests, capture.diagnostics.empty());

    // 絕對路徑即使指向 FM 內仍被拒絕；相對路徑也不能解析至 FM 外。
    bool path_failure = false;
    try
    {
        (void)Configuration::Tools::resolve_file(fm, std::filesystem::absolute(file));
    }
    catch (const Configuration::Error& error)
    {
        path_failure = error.diagnostic().file.empty() &&
                       error.diagnostic().stage == "讀取" &&
                       error.diagnostic().reason.find("絕對路徑") != std::string::npos;
    }
    TEST_EXPECT(tests, path_failure);
    TEST_EXPECT(tests, capture.diagnostics.empty());
    const auto absolute_path = Configuration::load<SampleConfiguration>(std::filesystem::absolute(file));
    TEST_EXPECT(tests, !absolute_path.succeeded());
    if (!absolute_path.succeeded())
        TEST_EXPECT(tests, absolute_path.error().reason.find("絕對路徑") != std::string::npos);
    TestFiles::write_text(root.path() / "Outside.jsonc", "{\"values\": [0, 1]}");
    const auto outside_path = Configuration::load<SampleConfiguration>("../Outside.jsonc");
    TEST_EXPECT(tests, !outside_path.succeeded());
    if (!outside_path.succeeded())
        TEST_EXPECT(tests, outside_path.error().reason.find("不在 FM 內") != std::string::npos);
    TEST_EXPECT(tests, capture.diagnostics.size() == 2);
    capture.diagnostics.clear();

    // 欄位診斷以完整路徑定位，跳脫字串的解碼長度不影響診斷內容。
    const char* invalid_field_sources[] = {
        "{\"values\": [\"\\u0031\"]}",
        "{\"values\": [\"line\\nnext\"]}",
        "{\"values\": [\"\\uD83D\\uDE00\"]}",
        "{\"values\": [true]}",
        "{\"values\": [null]}"
    };
    for (const auto* source : invalid_field_sources)
    {
        capture.diagnostics.clear();
        TestFiles::write_text(file, source);
        const auto result = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
        TEST_EXPECT(tests, !result.succeeded());
        if (!result.succeeded())
        {
            TEST_EXPECT(tests, result.error().field == "/values/0");
            TEST_EXPECT(tests, result.error().line == 0 && result.error().column == 0);
            TEST_EXPECT(tests, result.error().message().find("欄位=/values/0") != std::string::npos);
            TEST_EXPECT(tests, result.error().message().find("LoaderTests.jsonc 階段=") != std::string::npos);
        }
        TEST_EXPECT(tests, capture.diagnostics.size() == 1);
        if (!result.succeeded() && capture.diagnostics.size() == 1)
        {
            TEST_EXPECT(tests, result.error().file == std::filesystem::canonical(file));
            TEST_EXPECT(tests, capture.diagnostics[0].message() == result.error().message());
        }
    }

    // 多餘欄位立即中止載入；即使同時缺少必填欄位，也只回報第一個錯誤。
    const char* extra_field_sources[] = {
        "{\"extra\": 1.0000, \"values\": [0]}",
        "{\"extra\": 1}"
    };
    for (const auto* source : extra_field_sources)
    {
        capture.diagnostics.clear();
        TestFiles::write_text(file, source);
        const auto result = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
        TEST_EXPECT(tests, !result.succeeded());
        TEST_EXPECT(tests, capture.diagnostics.size() == 1);
        if (!result.succeeded() && capture.diagnostics.size() == 1)
        {
            TEST_EXPECT(tests, result.error().field == "/extra");
            TEST_EXPECT(tests, result.error().reason == "多餘的欄位");
            TEST_EXPECT(tests, result.error().file == std::filesystem::canonical(file));
            TEST_EXPECT(tests, capture.diagnostics[0].message() == result.error().message());
        }
    }

    // 語法錯誤保留解析器的行列訊息，不另填結構化行列。
    capture.diagnostics.clear();
    TestFiles::write_text(file, "{\"values\": [0,\n]}");
    const auto syntax = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
    TEST_EXPECT(tests, !syntax.succeeded());
    if (!syntax.succeeded())
    {
        TEST_EXPECT(tests, syntax.error().stage == "解析");
        TEST_EXPECT(tests, syntax.error().line == 0 && syntax.error().column == 0);
        TEST_EXPECT(tests, syntax.error().reason.find("line 2, column 1") != std::string::npos);
    }
    TEST_EXPECT(tests, capture.diagnostics.size() == 1);
    if (!syntax.succeeded() && capture.diagnostics.size() == 1)
    {
        TEST_EXPECT(tests, syntax.error().file == std::filesystem::canonical(file));
        TEST_EXPECT(tests, capture.diagnostics[0].message() == syntax.error().message());
    }

    // 原始 NUL 不能截斷有效 JSON 後的內容，載入結果與日誌均回報解析錯誤。
    capture.diagnostics.clear();
    const auto nul_source = std::string("{\"values\": [0]}") + '\0' + "{\"values\": [1]}";
    {
        std::ofstream output(file, std::ios::binary);
        output.write(nul_source.data(), static_cast<std::streamsize>(nul_source.size()));
    }
    const auto nul = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
    TEST_EXPECT(tests, !nul.succeeded());
    if (!nul.succeeded())
    {
        TEST_EXPECT(tests, nul.error().stage == "解析");
        TEST_EXPECT(tests, nul.error().reason.find("NUL（0x00）") != std::string::npos);
        TEST_EXPECT(tests, nul.error().reason.find("位元組 16") != std::string::npos);
    }
    TEST_EXPECT(tests, capture.diagnostics.size() == 1);

    // JSON 的 Unicode 跳脫序列可產生 NUL 字元，原文仍是可完整解析的文字。
    auto escaped_nul_document = Configuration::Tools::parse_jsonc("\"\\u0000\"");
    TEST_EXPECT(tests, escaped_nul_document.root().string() == std::string(1, '\0'));

    // 解析與欄位工具只提供階段、欄位及原因，不填寫來源或直接送出錯誤。
    capture.diagnostics.clear();
    bool parse_failure = false;
    try
    {
        (void)Configuration::Tools::parse_jsonc("{");
    }
    catch (const Configuration::Error& error)
    {
        parse_failure = error.diagnostic().file.empty() &&
                        error.diagnostic().stage == "解析" &&
                        !error.diagnostic().reason.empty();
    }
    TEST_EXPECT(tests, parse_failure);
    bool field_failure = false;
    try
    {
        auto json_document = Configuration::Tools::parse_jsonc("{\"values\": [true]}");
        (void)json_document.root().member("values").element(0).number();
    }
    catch (const Configuration::Error& error)
    {
        field_failure = error.diagnostic().file.empty() &&
                        error.diagnostic().stage == "轉換與驗證" &&
                        error.diagnostic().field == "/values/0" &&
                        std::string(error.what()) == error.diagnostic().reason;
    }
    TEST_EXPECT(tests, field_failure);
    bool extra_field_failure = false;
    try
    {
        auto json_document = Configuration::Tools::parse_jsonc("{\"nested\": {\"unknown/key~\": 1}}");
        json_document.root().member("nested").object({});
    }
    catch (const Configuration::Error& error)
    {
        extra_field_failure = error.diagnostic().file.empty() &&
                              error.diagnostic().field == "/nested/unknown~1key~0" &&
                              error.diagnostic().reason == "多餘的欄位";
    }
    TEST_EXPECT(tests, extra_field_failure);
    TEST_EXPECT(tests, capture.diagnostics.empty());

    // 原文數值溢位保留解析器原因與來源文件，不提供欄位路徑。
    capture.diagnostics.clear();
    TestFiles::write_text(file, "{\"values\": [0, 1e400]}");
    const auto overflow = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
    TEST_EXPECT(tests, !overflow.succeeded());
    if (!overflow.succeeded())
    {
        TEST_EXPECT(tests, overflow.error().field.empty());
        TEST_EXPECT(tests, overflow.error().stage == "解析");
        TEST_EXPECT(tests, overflow.error().file.filename() == file.filename());
        TEST_EXPECT(tests, overflow.error().reason.find("overflow") != std::string::npos);
    }
    TEST_EXPECT(tests, capture.diagnostics.size() == 1);

    // 重複名稱交由 nlohmann 解析，載入器不額外檢查。
    capture.diagnostics.clear();
    TestFiles::write_text(file, "{\"values\": [0], \"values\": [1]}");
    const auto duplicate = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
    TEST_EXPECT(tests, duplicate.succeeded());
    if (duplicate.succeeded())
        TEST_EXPECT(tests, duplicate.value().values.size() == 1);
    TEST_EXPECT(tests, capture.diagnostics.empty());

    capture.diagnostics.clear();
    TestFiles::write_text(file, "{}");
    const auto missing = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
    TEST_EXPECT(tests, !missing.succeeded());
    if (!missing.succeeded())
        TEST_EXPECT(tests, missing.error().field == "/values");
    TEST_EXPECT(tests, capture.diagnostics.size() == 1);
    if (!missing.succeeded() && capture.diagnostics.size() == 1)
    {
        TEST_EXPECT(tests, capture.diagnostics[0].file == std::filesystem::canonical(file));
        TEST_EXPECT(tests, capture.diagnostics[0].message() == missing.error().message());
    }

    // 註解不另做編碼驗證，資料字串與欄位名稱中的無效 UTF-8 由解析器拒絕。
    const char* invalid_sequences[] = {
        "\xc0\xaf", "\xe0\x80\x80", "\xed\xa0\x80",
        "\xf4\x90\x80\x80", "\xe2\x82"
    };
    for (const auto* sequence : invalid_sequences)
    {
        capture.diagnostics.clear();
        const auto source = std::string("{\"values\": [0]} // ") + sequence;
        TestFiles::write_text(file, source.c_str());
        const auto result = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
        TEST_EXPECT(tests, result.succeeded());
        if (result.succeeded())
            TEST_EXPECT(tests, result.value().values == std::vector<double>({0.0}));
        TEST_EXPECT(tests, capture.diagnostics.empty());

        const std::string invalid_data_sources[] = {
            std::string("{\"values\": [\"") + sequence + "\"]}",
            std::string("{\"values\": [0], \"") + sequence + "\": 1}"
        };
        for (const auto& invalid_source : invalid_data_sources)
        {
            capture.diagnostics.clear();
            TestFiles::write_text(file, invalid_source.c_str());
            const auto invalid_data = Configuration::load<SampleConfiguration>("LoaderTests.jsonc");
            TEST_EXPECT(tests, !invalid_data.succeeded());
            if (!invalid_data.succeeded())
            {
                TEST_EXPECT(tests, invalid_data.error().stage == "解析");
                TEST_EXPECT(tests, invalid_data.error().line == 0 && invalid_data.error().column == 0);
                TEST_EXPECT(tests, invalid_data.error().reason.find("parse error at line ") != std::string::npos);
                TEST_EXPECT(tests, invalid_data.error().reason.find("UTF-8") != std::string::npos);
            }
            TEST_EXPECT(tests, capture.diagnostics.size() == 1);
        }
    }

    capture.diagnostics.clear();
    TestFiles::write_text(file, "{\"values\": [0]}");
    bool program_failure = false;
    try
    {
        (void)Configuration::load<ProgramFailureConfiguration>("LoaderTests.jsonc");
    }
    catch (const std::logic_error& error)
    {
        program_failure = std::string(error.what()) == "conversion defect";
    }
    catch (const std::exception&) {}
    TEST_EXPECT(tests, program_failure);

    bool resource_failure = false;
    try
    {
        (void)Configuration::load<ResourceFailureConfiguration>("LoaderTests.jsonc");
    }
    catch (const std::bad_alloc&)
    {
        resource_failure = true;
    }
    catch (const std::exception&) {}
    TEST_EXPECT(tests, resource_failure);
    TEST_EXPECT(tests, capture.diagnostics.empty());
}
