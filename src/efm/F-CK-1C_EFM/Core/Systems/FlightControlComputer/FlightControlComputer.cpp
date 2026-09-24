#include "FlightControlComputer.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

#include <stdexcept>

namespace Core::Systems
{
    // 三個根目錄 .cpp 之一：把 SystemPipeline 的 typed 資料轉成 Executive 輸入，
    // 再將完整結果發布回 Pipeline；不在這裡實作控制律或保存演算法狀態。
    FlightControlComputer::FlightControlComputer(
        StartMode start_mode,
        const ThrottleLeverSignal &initial_throttle_levers)
        : FlightControlComputer(
              fck1c_flight_control_computer_config(),
              start_mode,
              initial_throttle_levers)
    {
    }

    FlightControlComputer::FlightControlComputer(
        const FlightControlComputerConfig &config,
        StartMode start_mode,
        const ThrottleLeverSignal &initial_throttle_levers)
        : executive_(config, start_mode, initial_throttle_levers)
    {
    }

    void FlightControlComputer::setup(SystemSetup &setup)
    {
        // 登記遙測、更新率及五種已完成 DCS 座標與單位轉換的輸入。
        debug_telemetry_.declare_channels(setup);
        setup.update_rate_hz(kF16XlDflcsReferenceUpdateRateHz);
        setup.read(AircraftDataKeys::kFlightControlObservation);
        setup.read(AircraftDataKeys::kPilotControlSignal);
        setup.read(AircraftDataKeys::kThrottleLeverSignal);
        setup.read(AircraftDataKeys::kLandingGearData);
        setup.read(AircraftDataKeys::kFlightControlActuatorState);

        // 發布建構時的完整初值，供其他 System 在第一個 64 Hz 週期前讀取。
        const FlightControlComputerResult &initial = executive_.result(); // Executive 持有的初始結果。
        setup.publish(
            AircraftDataKeys::kFlightControlActuatorCommand,
            initial.actuator_command);
        setup.publish(
            AircraftDataKeys::kEngineThrottleCommand,
            initial.engine_throttle_command);
        setup.publish(
            AircraftDataKeys::kAutomaticFlightControlSnapshot,
            initial.automatic_flight_control);
        setup.publish(
            AircraftDataKeys::kFlightControlComputerSnapshot,
            initial.diagnostics);

        // 子模組已提供 ID 與交付函式；此處只轉接 Pipeline 的處理器格式。
        for (const FlightControlCommandBinding &binding : executive_.command_bindings())
        {
            if (!binding.deliver)
                throw std::logic_error("FLCC command binding has no handler.");
            setup.register_command_handler(
                binding.id,
                [deliver = binding.deliver](const SystemActionContext &, const Command &command)
                { deliver(command); });
        }

        // 將 Executive 建構時的狀態記為時間零的除錯遙測。
        debug_telemetry_.publish_initial(
            initial.diagnostics, initial.automatic_flight_control);
    }

    void FlightControlComputer::step(
        const SystemStepContext &context,
        const AircraftDataView &aircraft,
        SystemResult &result)
    {
        // 將 Pipeline 的資料鍵轉成一次 Executive 運算所需的輸入。
        const RawFlightControlInput flight = make_pipeline_input(context, aircraft); // 本週期飛控輸入值。
        const FlightControlComputerResult &output = executive_.update({
            flight,
            aircraft.read(AircraftDataKeys::kThrottleLeverSignal) // 油門為臨時職責 之後應分出
        });                                                       // Executive 持有的本週期完整結果。

        // 遙測觀察已完成的結果，不修改控制命令。
        debug_telemetry_.publish_step({context.scheduled_time,
                                       flight.observation, output.diagnostics,
                                       output.automatic_flight_control});

        // 發布兩種控制命令與兩種狀態快照；致動器需求的單位為弧度。
        result.publish(AircraftDataKeys::kFlightControlActuatorCommand,
                       output.actuator_command);
        result.publish(AircraftDataKeys::kEngineThrottleCommand,
                       output.engine_throttle_command);
        result.publish(AircraftDataKeys::kAutomaticFlightControlSnapshot,
                       output.automatic_flight_control);
        result.publish(AircraftDataKeys::kFlightControlComputerSnapshot,
                       output.diagnostics);
    }

    RawFlightControlInput FlightControlComputer::make_pipeline_input(
        const SystemStepContext &context,
        const AircraftDataView &aircraft) const
    {
        // 只擷取 Core 已定義的訊號；context.dt_s 為秒，其他欄位由 key 的
        // typed struct 指定格式。DCS 座標與單位轉換在進入 Core 前完成。
        return {
            context.dt_s,
            aircraft.read(AircraftDataKeys::kFlightControlObservation),
            aircraft.read(AircraftDataKeys::kPilotControlSignal),
            aircraft.read(AircraftDataKeys::kLandingGearData),
            aircraft.read(AircraftDataKeys::kFlightControlActuatorState)};
    }

}
