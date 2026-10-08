#pragma once

namespace Core::Systems::Flcc
{
/// @brief 單一軸增益後的需求與輸出限幅狀態。
struct AxisMappingState
{
    double demand_before_limit_normalized = 0.0; ///< 增益後、限幅前的無因次需求。
    bool limited = false; ///< 最終需求是否受到 [-1, 1] 限幅。
};

/// @brief 三個操縱軸最近一次完成的直接映射狀態。
struct DirectMappingState
{
    AxisMappingState pitch;
    AxisMappingState roll;
    AxisMappingState yaw;
};

/// @brief ControlLawData 的狀態子區塊。
/// 尚未實作：回授控制、模式切換、濾波器與積分器的狀態。
struct ControlLawState
{
    DirectMappingState direct_mapping;
};
}
