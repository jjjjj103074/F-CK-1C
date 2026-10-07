#include "ConfigurationTools.h"
#include "ThirdParty/nlohmann/json.hpp"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <fstream>
#include <utility>

namespace
{
    /// @brief 將欄位名稱轉為 JSON Pointer 片段，保留 / 與 ~ 的原始含義。
    std::string escape_json_pointer_token(const std::string &name)
    {
        std::string result;
        for (char character : name)
            result += character == '~' ? "~0" : character == '/' ? "~1"
                                                                 : std::string(1, character);
        return result;
    }

    /// @brief 比較實際路徑片段；Windows 檔案路徑依作業系統慣例忽略大小寫。
    bool same_component(const std::filesystem::path &left, const std::filesystem::path &right)
    {
#ifdef _WIN32
        return _wcsicmp(left.c_str(), right.c_str()) == 0;
#else
        return left == right;
#endif
    }
}

namespace Configuration
{
    std::string Diagnostic::message() const
    {
        std::string result = "[Error] " + file.u8string();
        if (line != 0)
            result += ":" + std::to_string(line) + ":" + std::to_string(column);
        result += " 階段=" + stage;
        if (!field.empty())
            result += " 欄位=" + field;
        return result + " 原因=" + reason;
    }

    Error::Error(std::string stage, std::string reason, std::string field)
        : std::runtime_error(reason),
          diagnostic_{{}, std::move(stage), std::move(field), 0, 0, std::move(reason)}
    {
    }

    struct JsonDocument::Storage
    {
        nlohmann::json root_value;
    };

    JsonDocument::~JsonDocument() = default;
    JsonDocument::JsonDocument(JsonDocument &&) noexcept = default;
    JsonDocument &JsonDocument::operator=(JsonDocument &&) noexcept = default;

    JsonDocument::JsonDocument()
        : storage_(std::make_unique<Storage>())
    {
    }

    Value JsonDocument::root()
    {
        return Value(*this, "");
    }

    Value::Value(JsonDocument &json_document, std::string field)
        : json_document_(json_document), field_(std::move(field))
    {
    }

    const auto &Value::json() const
    {
        return json_document_.storage_->root_value.at(nlohmann::json::json_pointer(field_));
    }

    void Value::require(bool condition, const char *reason) const
    {
        if (!condition)
            throw Error("轉換與驗證", reason, field_.empty() ? "/" : field_);
    }

    void Value::object(std::initializer_list<const char *> fields) const
    {
        require(json().is_object(), "預期為物件");
        // 拒絕未列於欄位定義的鍵，再檢查必填欄位。
        for (auto iterator = json().begin(); iterator != json().end(); ++iterator)
            if (std::find(fields.begin(), fields.end(), iterator.key()) == fields.end())
                throw Error("轉換與驗證", "多餘的欄位", field_ + "/" + escape_json_pointer_token(iterator.key()));
        for (const char *field : fields)
            (void)member(field);
    }

    Value Value::member(const char *name) const
    {
        require(json().is_object(), "預期為物件");
        const auto iterator = json().find(name);
        if (iterator == json().end())
            throw Error("轉換與驗證", "缺少必填欄位", field_ + "/" + escape_json_pointer_token(name));
        return Value(json_document_, field_ + "/" + escape_json_pointer_token(name));
    }

    Value Value::element(std::size_t index) const
    {
        require(json().is_array(), "預期為陣列");
        require(index < json().size(), "缺少必要的陣列元素");
        return Value(json_document_, field_ + "/" + std::to_string(index));
    }

    std::size_t Value::array_size() const
    {
        require(json().is_array(), "預期為陣列");
        return json().size();
    }

    double Value::number() const
    {
        require(json().is_number(), "預期為數值");
        const double result = json().get<double>();
        require(std::isfinite(result), "數值必須有限且可由 double 表示");
        return result;
    }

    bool Value::boolean() const
    {
        require(json().is_boolean(), "預期為布林值");
        return json().get<bool>();
    }

    std::string Value::string() const
    {
        require(json().is_string(), "預期為字串");
        return json().get<std::string>();
    }

    namespace Tools
    {
        std::filesystem::path resolve_file(const std::filesystem::path &fm_root,
                                           const std::filesystem::path &relative_path)
        {
            const auto requested = fm_root / relative_path; // 組合 FM 根目錄與相對路徑
            // 檢查輸入是否為絕對路徑
            if (relative_path.is_absolute())
                throw Error("讀取", "設定檔不得使用絕對路徑");

            // 嘗試解析FM根目錄
            std::error_code error;
            const auto root = std::filesystem::canonical(fm_root, error);
            if (error)
                throw Error("讀取", "無法取得 FM 根目錄：" + error.message());

            // 嘗試解析檔案位置
            const auto target = std::filesystem::canonical(requested, error);
            if (error)
                throw Error("讀取", "無法取得設定檔：" + error.message());

            // 檢查實際檔案位置是否在 FM 根目錄內
            auto root_part = root.begin();
            auto target_part = target.begin();
            for (; root_part != root.end(); ++root_part, ++target_part)
                if (target_part == target.end() || !same_component(*root_part, *target_part))
                    throw Error("讀取", "實際檔案位置不在 FM 內");

            return target;
        }

        std::string read_file(const std::filesystem::path &file_path)
        {
            const char *operation;
            try
            {
                operation = "開啟設定檔";
                std::ifstream input;
                input.exceptions(std::ios::failbit | std::ios::badbit);
                input.open(file_path, std::ios::binary | std::ios::ate);

                operation = "取得設定檔大小";
                const std::streamoff size = input.tellg();
                if (size < 0)
                    throw Error("讀取", "無法取得有效的設定檔大小");
                constexpr std::streamoff maximum_size = 16 * 1024 * 1024; // 16 MiB
                if (size > maximum_size)
                    throw Error("讀取", "設定檔超過單份 16 MiB 的大小上限");

                operation = "定位設定檔開頭";
                input.seekg(0, std::ios::beg);
                // 依已開啟串流的大小配置緩衝；少讀由串流例外回報，多讀以剩餘內容判斷。
                std::string source(static_cast<std::size_t>(size), '\0');

                operation = "讀取設定檔";
                input.read(source.data(), static_cast<std::streamsize>(source.size()));
                if (input.peek() != std::char_traits<char>::eof())
                    throw Error("讀取", "讀取期間檔案大小改變，存在超出原定長度的內容");
                return source;
            }
            catch (const std::ios_base::failure &error)
            {
                throw Error("讀取", std::string(operation) + "失敗：" + error.what());
            }
        }

        JsonDocument parse_jsonc(std::string source_text)
        {
            // 檢查是否有 NUL 字元
            const auto nul_offset = source_text.find('\0');
            if (nul_offset != std::string::npos)
                throw Error("解析", "原文位元組 " + std::to_string(nul_offset + 1) +
                                        " 含 NUL（0x00）；無法完整解析文件");

            // 將字串轉換成 JSON 樹
            JsonDocument json_document;
            auto &document_storage = *json_document.storage_;
            try
            {
                // 轉換
                document_storage.root_value = nlohmann::json::parse(source_text, nullptr, true, true);
            }
            catch (const nlohmann::json::parse_error &error)
            {
                // ERROR回報
                throw Error("解析", error.what());
            }
            catch (const nlohmann::json::out_of_range &error)
            {
                // 406 表示原文數值溢位；其他解析器例外維持原樣傳遞。
                if (error.id != 406)
                    throw;
                throw Error("解析", error.what());
            }
            return json_document;
        }
    }
}
