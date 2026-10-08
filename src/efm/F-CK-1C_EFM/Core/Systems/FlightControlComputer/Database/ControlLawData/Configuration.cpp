#include "Configuration.h"

#include "Common/Configuration/ConfigurationTools.h"

#include <cmath>

namespace Core::Systems::Flcc
{
void read_configuration(const ::Configuration::Value& value, AxisMappingConfiguration& result)
{
    value.object({ "gain", "curve" });
    const auto gain = value.member("gain");
    result.gain = gain.number();
    gain.require(result.gain >= 0.0, "gain 必須非負");

    const auto curve = value.member("curve");
    const auto count = curve.array_size();
    curve.require(count >= 3, "curve 至少需要三個節點");
    result.curve.clear();
    result.curve.reserve(count);
    double previous_input = -2.0;
    bool has_neutral = false;
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto point = curve.element(index);
        point.require(point.array_size() == 2, "節點必須為 [輸入, 輸出] 兩個數值");
        const auto input = point.element(0);
        const auto output = point.element(1);
        const double x = input.number();
        const double y = output.number();
        input.require(std::abs(x) <= 1.0, "輸入座標必須位於 [-1, 1]");
        output.require(std::abs(y) <= 1.0, "輸出座標必須位於 [-1, 1]");
        input.require(x > previous_input, "輸入座標必須嚴格遞增");
        output.require((x <= 0.0 || y >= 0.0) && (x >= 0.0 || y <= 0.0),
                       "曲線不得反轉操縱方向");
        output.require(x != 0.0 || y == 0.0, "中立輸入必須對應中立輸出");
        input.require(index != 0 || x == -1.0, "第一個輸入端點必須為 -1");
        input.require(index + 1 != count || x == 1.0, "最後一個輸入端點必須為 1");
        has_neutral = has_neutral || x == 0.0;
        previous_input = x;
        result.curve.push_back({ x, y });
    }
    curve.require(has_neutral, "曲線必須包含 [0, 0]");
}

void read_configuration(const ::Configuration::Value& value, DirectMappingConfiguration& result)
{
    value.object({ "pitch", "roll", "yaw" });
    read_configuration(value.member("pitch"), result.pitch);
    read_configuration(value.member("roll"), result.roll);
    read_configuration(value.member("yaw"), result.yaw);
}

void read_configuration(const ::Configuration::Value& value, ControlLawConfiguration& result)
{
    value.object({ "direct_mapping" });
    read_configuration(value.member("direct_mapping"), result.direct_mapping);
}
}
