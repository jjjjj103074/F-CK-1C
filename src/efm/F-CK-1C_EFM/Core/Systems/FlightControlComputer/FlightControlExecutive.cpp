#include "FlightControlExecutive.h"
namespace
{
    // 以 64 Hz 週期數做整數分頻：橫向／方向輸入塑形 32 Hz、增益排程 4 Hz。
    constexpr std::uint64_t kPilotShapingDivisor = 2;
    constexpr std::uint64_t kGainScheduleDivisor = 16;

    // 橫向／方向塑形真正更新時使用的 32 Hz 時間間隔，單位秒。
    constexpr double kPilotShapingDtS = 1.0 / 32.0;

}

namespace Core::Systems
{
    // 三個根目錄 .cpp 之一：擁有 FLCC 的內部模組與更新順序，不直接讀寫資料鍵。
    FlightControlExecutive::FlightControlExecutive(
        const FlightControlComputerConfig &config,
        StartMode start_mode,
        const ThrottleLeverSignal &initial_throttle_levers)
        : developer_direct_control_law_(
              config.values.flight_control_output.developer_direct_control_law),
          input_signals_(config.values.input_signal_management),
          mode_and_gain_(config.values.mode_and_gain),
          command_system_({config.values.flight_control_laws.longitudinal,
                           config.values.guidance_coordination,
                           config.values.automatic_flight_control,
                           {config.values.mode_and_gain.guidance_bank_limit_rad,
                            config.values.mode_and_gain.guidance_roll_rate_limit_rad_s},
                           start_mode != StartMode::HotAir}),
          flight_control_laws_(config.values.flight_control_laws),
          output_system_(config.values.flight_control_laws.surface_mixer,
                         config.values.flight_control_output),
          diagnostics_(config.values.mode_and_gain.g_limiter_override.available,
                       config.values.flight_control_output.developer_direct_control_law,
                       config.values.diagnostics),
          held_pilot_shaping_(config.values.mode_and_gain.cat1.pilot_input)
    {
        // start_mode 用於 AP 初始接地判斷；initial_throttle_levers 用於首週期前的油門輸出。
        // 所有子模組已建構；彙整一次本架飛機可交給 Pipeline 的指令綁定。
        command_bindings_ = mode_and_gain_.command_bindings();
        const auto automatic_bindings = command_system_.command_bindings();
        command_bindings_.insert(
            command_bindings_.end(), automatic_bindings.begin(), automatic_bindings.end());
        result_.engine_throttle_command = {
            initial_throttle_levers.left_normalized,
            initial_throttle_levers.right_normalized};
        result_.diagnostics = diagnostics_.snapshot();
        result_.automatic_flight_control = command_system_.automatic_snapshot();
    }

    const std::vector<FlightControlCommandBinding> &
    FlightControlExecutive::command_bindings() const
    {
        return command_bindings_;
    }

    const FlightControlComputerResult &FlightControlExecutive::update(
        const FlightControlComputerStepInput &input)
    {
        // input 是一次完整的 Core typed 輸入；此函式依固定順序完成整個 FLCC 週期。
        // 以 64 Hz 主週期決定較慢模組是否更新；計數從零開始，首週期兩者都執行。
        // 非更新週期沿用相應慢頻計算的保留值；模組的其餘工作仍逐週期執行。
        const bool shaping_tick = tick_ % kPilotShapingDivisor == 0;
        const bool gain_tick = tick_ % kGainScheduleDivisor == 0;
        // signals 是已驗證、濾波及塑形的觀測／飛行員要求；flight 是由它推得的飛行狀態。
        const auto &signals = input_signals_.update({input.flight_control, held_pilot_shaping_,
                                                     shaping_tick, kPilotShapingDtS});
        const auto flight = compute_flight_state(signals);
        // configuration 是本週期選定的 CAT、增益與飛行包線。
        // held_pilot_shaping_ 保留最近一次選定的塑形設定，供後續週期使用。
        const auto &configuration = mode_and_gain_.update({flight.dt_s, flight.dynamic_pressure_pa,
                                                           flight.mach, gain_tick,
                                                           signals.landing_gear_handle_down});
        held_pilot_shaping_ = configuration.stores.pilot_input;
        // command 包含飛行員與 AP 參考值及權限選擇；laws 把參考值轉成電子控制面需求。
        const auto &command = command_system_.update(
            {flight, signals, configuration});
        const auto laws = flight_control_laws_.update({flight, signals, command.coordinated.reference, configuration});
        // selection 是本週期使用的電子控制律；output 限制需求並產生實際發布的命令。
        const auto selection = developer_direct_control_law_
                                   ? ElectronicControlLawSelection::DeveloperDirect
                                   : ElectronicControlLawSelection::Normal;
        const auto output = output_system_.update({laws, selection,
                                                   input.flight_control.actuator, flight.dt_s});
        result_.actuator_command = output.actuator_command;
        // output_saturated 合併電子需求與致動器回授的飽和事實，供 AP 監測使用。
        const bool output_saturated =
            output.status.electronic_command_saturated ||
            output.status.actuator_saturated;
        // AP 監測本次已完成的控制結果；監測造成的狀態變化從後續週期影響命令。
        command_system_.observe_control_result(make_monitor_observation(
            {flight, command, laws, output_saturated}));
        update_engine_throttle(input.throttle_levers, command.automatic);
        update_subrate_counters(shaping_tick, gain_tick);
        update_diagnostics({command, laws, configuration, signals, output.status});
        result_.automatic_flight_control = command_system_.automatic_snapshot();
        ++tick_;
        return result_;
    }

    void FlightControlExecutive::update_subrate_counters(
        bool shaping_tick,
        bool gain_tick)
    {
        // 兩個布林值表示本週期是否真正執行；計數與 tick_ 只供診斷觀察。
        if (shaping_tick)
        {
            ++pilot_shaping_update_count_;
            pilot_shaping_last_update_tick_ = tick_;
        }
        if (gain_tick)
        {
            ++gain_schedule_update_count_;
            gain_schedule_last_update_tick_ = tick_;
        }
    }

    void FlightControlExecutive::update_engine_throttle(
        const ThrottleLeverSignal &levers,
        const AutomaticFlightGuidanceReference &automatic)
    {
        // levers 為左右油門桿；automatic 提供開發用 A/T 的目標與接通狀態。
        // blend 為自動油門混合比例：接通是 1，否則是 0；關閉時各引擎跟隨油門桿。
        const double blend = automatic.experimental_auto_throttle_engaged
                                 ? 1.0
                                 : 0.0;
        result_.engine_throttle_command = {
            ::Systems::compose_engine_throttle_command({levers.left_normalized,
                                                        automatic.experimental_throttle_normalized, blend, false}),
            ::Systems::compose_engine_throttle_command({levers.right_normalized,
                                                        automatic.experimental_throttle_normalized, blend, false})};
    }

    void FlightControlExecutive::update_diagnostics(
        const FlightControlExecutiveDiagnosticsInput &input)
    {
        // input 只含本週期已完成的中間結果；診斷投影不回饋控制計算。
        result_.diagnostics = diagnostics_.update({tick_,
                                                   pilot_shaping_update_count_, pilot_shaping_last_update_tick_,
                                                   gain_schedule_update_count_, gain_schedule_last_update_tick_,
                                                   mode_and_gain_.target_stores_configuration() ==
                                                       ::Systems::StoresConfiguration::Cat3,
                                                   mode_and_gain_.g_limiter_override_active(), input.command, input.laws,
                                                   input.configuration, input.signals, input.output_status,
                                                   result_.actuator_command});
    }

    FlightControlCommandMonitorInput
    FlightControlExecutive::make_monitor_observation(
        const AutopilotMonitorInput &input) const
    {
        // 誤差是 AP 要求減去實際飛行狀態；分別使用弧度與英尺／秒。
        const double pitch_error_rad =
            input.command.automatic.pitch_attitude_reference_rad -
            input.flight.pitch_attitude_rad;
        const double vertical_speed_error_ft_s =
            input.command.automatic.vertical_speed_reference_ft_s -
            input.flight.vertical_speed_ft_s;
        return {
            input.flight.dt_s,
            input.command.automatic.longitudinal_authority == AuthorityState::Automatic,
            input.command.automatic.lateral_authority == AuthorityState::Automatic,
            input.command.automatic.vertical_type,
            pitch_error_rad,
            vertical_speed_error_ft_s,
            input.command.automatic.bank_angle_reference_rad -
                input.flight.roll_attitude_rad,
            input.command.coordinated.constraint,
            input.control_path_saturated,
            hard_protection_reason(input.laws.status)};
    }

    ConstraintReason FlightControlExecutive::hard_protection_reason(
        const ::Systems::FlightControlLawsStatus &status) const
    {
        // status 可能同時記錄多種限制；依迎角、過載、角速率順序報告主原因。
        if (status.angle_of_attack_limit_active)
            return ConstraintReason::HardAngleOfAttackLimit;
        if (status.load_factor_limit_active)
            return ConstraintReason::HardLoadFactorLimit;
        if (status.body_rate_limit_active)
            return ConstraintReason::HardRateLimit;
        return ConstraintReason::None;
    }

    const FlightControlComputerResult &FlightControlExecutive::result() const
    {
        // 回傳內部快照參照；下一次 update 會覆寫它。
        return result_;
    }
}
