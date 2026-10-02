#include "FlightControlComputer.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace Core::Systems
{
    FlightControlComputer::FlightControlComputer(const ThrottleLeverSignal &initial_throttle_levers)
        : startup_and_restart_(control_law_data_, system_status_tables_),
          amux_processor_(amux_data_),
          system_monitor_(input_output_data_, system_status_tables_),
          selector_monitor_(input_output_data_, system_status_tables_),
          failure_manager_(system_status_tables_),
          control_laws_(input_output_data_, control_law_data_, system_status_tables_),
          executive_(executive_tables_, startup_and_restart_, amux_processor_,
                     system_monitor_, selector_monitor_, failure_manager_, control_laws_)
    {
        input_output_data_.output.engine_throttle_command = {
            initial_throttle_levers.left_normalized,
            initial_throttle_levers.right_normalized};
    }

    void FlightControlComputer::setup(SystemSetup &setup)
    {
        // 外層仍以專案既有頻率呼叫 FLCC；內部各任務的 Hz 由 Executive 集中設定。
        setup.update_rate_hz(kF16XlDflcsReferenceUpdateRateHz);
        setup.read(AircraftDataKeys::kFlightControlObservation);
        setup.read(AircraftDataKeys::kPilotControlSignal);
        setup.read(AircraftDataKeys::kThrottleLeverSignal);
        setup.read(AircraftDataKeys::kLandingGearData);
        setup.read(AircraftDataKeys::kFlightControlActuatorState);

        setup.publish(AircraftDataKeys::kFlightControlActuatorCommand,
                      input_output_data_.output.actuator_command);
        setup.publish(AircraftDataKeys::kEngineThrottleCommand,
                      input_output_data_.output.engine_throttle_command);
        setup.publish(AircraftDataKeys::kAutomaticFlightControlSnapshot,
                      system_status_tables_.automatic_flight_control);
        setup.publish(AircraftDataKeys::kFlightControlComputerSnapshot,
                      system_status_tables_.flight_control_computer);
    }

    void FlightControlComputer::step(const SystemStepContext &context,
                                     const AircraftDataView &aircraft, SystemResult &result)
    {
        read_inputs(aircraft);
        executive_.step(context.scheduled_time);
        publish_outputs(result);
    }

    void FlightControlComputer::read_inputs(const AircraftDataView &aircraft)
    {
        input_output_data_.input = {
            aircraft.read(AircraftDataKeys::kFlightControlObservation),
            aircraft.read(AircraftDataKeys::kPilotControlSignal),
            aircraft.read(AircraftDataKeys::kThrottleLeverSignal),
            aircraft.read(AircraftDataKeys::kLandingGearData),
            aircraft.read(AircraftDataKeys::kFlightControlActuatorState)};
    }

    void FlightControlComputer::publish_outputs(SystemResult &result) const
    {
        // 尚未實作控制需求與狀態計算；發布資料庫初值，快照的 available 保持 false。
        result.publish(AircraftDataKeys::kFlightControlActuatorCommand,
                       input_output_data_.output.actuator_command);
        result.publish(AircraftDataKeys::kEngineThrottleCommand,
                       input_output_data_.output.engine_throttle_command);
        result.publish(AircraftDataKeys::kAutomaticFlightControlSnapshot,
                       system_status_tables_.automatic_flight_control);
        result.publish(AircraftDataKeys::kFlightControlComputerSnapshot,
                       system_status_tables_.flight_control_computer);
    }
}
