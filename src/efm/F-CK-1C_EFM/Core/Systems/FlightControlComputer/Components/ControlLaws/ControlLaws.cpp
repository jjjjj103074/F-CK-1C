#include "ControlLaws.h"

#include "../../Database/InputOutputData.h"
#include "../../Database/ControlLawData.h"
#include "../../Database/SystemStatusTables.h"

#include <algorithm>
#include <cmath>

namespace
{
    using namespace Core::Systems::Flcc;

    /// @brief 對已驗證的曲線進行線性插值；輸入已限制於 [-1, 1]。
    double interpolate_curve(const AxisMappingConfiguration &configuration, double input)
    {
        for (std::size_t index = 1; index < configuration.curve.size(); ++index)
        {
            const ControlLawCurvePoint &upper = configuration.curve[index];
            if (input <= upper.input_normalized)
            {
                const ControlLawCurvePoint &lower = configuration.curve[index - 1];
                const double fraction = (input - lower.input_normalized) /
                                        (upper.input_normalized - lower.input_normalized);
                return lower.output_normalized + fraction *
                                                     (upper.output_normalized - lower.output_normalized);
            }
        }
        return configuration.curve.back().output_normalized;
    }

    /// @brief 映射一軸並回報增益後的限幅狀態；非有限輸入視為中立。
    double map_axis(double input, const AxisMappingConfiguration &configuration,
                    AxisMappingState &state)
    {
        const double normalized = std::isfinite(input) ? std::clamp(input, -1.0, 1.0) : 0.0;
        state.demand_before_limit_normalized =
            configuration.gain * interpolate_curve(configuration, normalized);
        const double demand = std::clamp(state.demand_before_limit_normalized, -1.0, 1.0);
        state.limited = demand != state.demand_before_limit_normalized;
        return demand;
    }
}

namespace Core::Systems::Flcc
{
    ControlLaws::ControlLaws(
        InputOutputData &input_output,
        ControlLawData &control_law,
        const SystemStatusTables &system_status)
        : input_output_(input_output),
          control_law_(control_law),
          system_status_(system_status)
    {
    }

    void ControlLaws::step(std::chrono::nanoseconds scheduled_time, double dt_s)
    {
        // 三軸映射依本次輸入與設定計算，時間參數符合 Executive 的任務介面。
        (void)scheduled_time;
        (void)dt_s;
        const PilotControlSignal &pilot = input_output_.input.pilot_control;                         // 駕駛員輸入
        const DirectMappingConfiguration &configuration = control_law_.configuration.direct_mapping; // 控制律設定
        ControlLawState next_state;                                                                  // 下一次運算的結果

        // 依設定映射三軸輸入
        const FlightControlActuatorCommand command = {
            map_axis(pilot.pitch_axis_normalized, configuration.pitch, next_state.direct_mapping.pitch),
            map_axis(pilot.roll_axis_normalized, configuration.roll, next_state.direct_mapping.roll),
            map_axis(pilot.yaw_axis_normalized, configuration.yaw, next_state.direct_mapping.yaw)};

        // 更新狀態與輸出
        control_law_.state = next_state;
        input_output_.output.actuator_command = command;
    }
}
