#pragma once

#include <vector>

namespace Configuration { class Value; }

namespace Core::Systems::Flcc
{
/// @brief 操縱曲線節點；兩個座標皆為 [-1, 1] 的無因次數值。
struct ControlLawCurvePoint
{
    double input_normalized = 0.0; ///< 拉桿、右滾與左偏航為正。
    double output_normalized = 0.0; ///< 套用增益前的舵面位置需求。
};

/// @brief 單一操縱軸的曲線與增益設定。
struct AxisMappingConfiguration
{
    double gain = 0.0; ///< 有限且非負的曲線輸出增益。
    std::vector<ControlLawCurvePoint> curve; ///< 輸入嚴格遞增且包含中立節點。
};

/// @brief 直接映射設定的三個操縱軸，共用同一組欄位對應規則。
struct DirectMappingConfiguration
{
    AxisMappingConfiguration pitch;
    AxisMappingConfiguration roll;
    AxisMappingConfiguration yaw;
};

/// @brief ControlLawData 的控制律參數子區塊。
/// 尚未實作：回授控制、配平、模式、濾波與積分參數。
struct ControlLawConfiguration
{
    DirectMappingConfiguration direct_mapping;
};

// Configuration::load<T>() 依目標型別呼叫對應的欄位轉換函式。
/// @brief 對應並驗證單一軸的所有必填設定。
void read_configuration(const ::Configuration::Value& value, AxisMappingConfiguration& result);
/// @brief 對應三個操縱軸，逐軸使用相同的曲線與增益規則。
void read_configuration(const ::Configuration::Value& value, DirectMappingConfiguration& result);
/// @brief 對應並驗證控制律設定子區塊。
void read_configuration(const ::Configuration::Value& value, ControlLawConfiguration& result);
}
