#include "FlightControlComputer.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

#include <stdexcept>

namespace Core::Systems
{
    FlightControlComputer::FlightControlComputer(
        const ThrottleLeverSignal &initial_throttle_levers)
        : initial_throttle_levers_(initial_throttle_levers)
    {
    }

    void FlightControlComputer::setup(SystemSetup &setup)
    {
        // 宣告固定排程頻率與本系統在每個週期需要讀取的資料。
        setup.update_rate_hz(kF16XlDflcsReferenceUpdateRateHz);
        setup.read(AircraftDataKeys::kFlightControlObservation);
        setup.read(AircraftDataKeys::kPilotControlSignal);
        setup.read(AircraftDataKeys::kThrottleLeverSignal);
        setup.read(AircraftDataKeys::kLandingGearData);
        setup.read(AircraftDataKeys::kFlightControlActuatorState);

        // Pipeline 建立時要求每個輸出先有一個值。兩個快照保持 unavailable，
        // 代表 FLCC 尚未完成任何運算；第一個 step 仍會明確拋出未實作錯誤。
        setup.publish(AircraftDataKeys::kFlightControlActuatorCommand,
                      FlightControlActuatorCommand{});
        setup.publish(AircraftDataKeys::kEngineThrottleCommand,
                      EngineThrottleCommand{initial_throttle_levers_.left_normalized,
                                            initial_throttle_levers_.right_normalized});
        setup.publish(AircraftDataKeys::kAutomaticFlightControlSnapshot,
                      AutomaticFlightControlSnapshot{});
        setup.publish(AircraftDataKeys::kFlightControlComputerSnapshot,
                      FlightControlComputerSnapshot{});

        // Executive 負責彙整未來子模組的指令；Computer 只轉接到 Pipeline。
        for (const FlightControlCommandBinding &binding : executive_.command_bindings())
        {
            if (!binding.deliver)
            {
                throw std::logic_error("FLCC command binding has no handler.");
            }
            setup.register_command_handler(
                binding.id,
                [deliver = binding.deliver](const SystemActionContext &, const Command &command)
                {
                    deliver(command);
                });
        }
    }

    void FlightControlComputer::step(
        const SystemStepContext &context,
        const AircraftDataView &aircraft,
        SystemResult &result)
    {
        // 目前 Executive 會明確拋出未實作錯誤，因此不會發布本週期輸出。
        // 保留 result 參數是 System 介面要求，未來運算完成後由此發布結果。
        (void)result;
        executive_.update(make_step_input(context, aircraft));
    }

    FlightControlComputerStepInput FlightControlComputer::make_step_input(
        const SystemStepContext &context,
        const AircraftDataView &aircraft) const
    {
        return {
            context.dt_s,                                                // 本週期時間間隔，單位為秒。
            aircraft.read(AircraftDataKeys::kFlightControlObservation),  // 飛行狀態。
            aircraft.read(AircraftDataKeys::kPilotControlSignal),        // 駕駛控制輸入。
            aircraft.read(AircraftDataKeys::kThrottleLeverSignal),       // 左右油門桿。
            aircraft.read(AircraftDataKeys::kLandingGearData),           // 起落架與接地狀態。
            aircraft.read(AircraftDataKeys::kFlightControlActuatorState) // 致動器回授。
        };
    }
}
