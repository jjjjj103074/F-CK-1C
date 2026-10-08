#include "ControlLawData.h"

#include "Common/Configuration/Configuration.h"

#include <stdexcept>
#include <utility>

namespace
{
    Core::Systems::Flcc::ControlLawConfiguration load_control_law_configuration()
    {
        auto result = ::Configuration::load<Core::Systems::Flcc::ControlLawConfiguration>(
            "FLCC/ControlLaws.jsonc");
        // 控制律需要完整設定才能建立資料庫；文件診斷已由載入入口送出。
        if (!result.succeeded())
            throw std::runtime_error("ControlLawData 初始化失敗：控制律設定載入失敗");
        return std::move(result.value());
    }
}

namespace Core::Systems::Flcc
{
ControlLawData::ControlLawData()
    : configuration(load_control_law_configuration())
{
}
}
