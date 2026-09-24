# FlightControlComputer architecture

`FlightControlComputer` represents one physical FLCC box and is the only
flight-control `System` registered with `SystemPipeline`. Its software modules
are ordinary composed C++ objects; they are not separately scheduled aircraft
devices.

## 根目錄三個 `.cpp` 的職責

根目錄有三個實作檔與兩個對應的宣告檔；`README.md` 說明它們之間的資料契約。

| 檔案 | 負責 | 接收與產出 |
|---|---|---|
| `Entry.cpp` | 只登記系統識別碼與建構工廠，不處理 FLCC 設定。 | `create_entry()` 不接收參數，產出 `SystemEntry`。工廠在建立飛行實例時接收 `FlightSetupContext`。不參與週期計算。 |
| `FlightControlComputer.cpp` | 唯一的 `SystemPipeline` 轉接入口；宣告更新率與資料鍵、直接登記模組提供的指令處理器、組成單次輸入、呼叫 Executive、發布結果與 debug telemetry。 | 從 `AircraftDataView` 讀取五種 typed 訊號，並向 `SystemResult` 發布四種 typed 結果。 |
| `FlightControlExecutive.cpp` | 持有 FLCC 運算狀態，彙整子模組指令綁定，決定輸入處理、模式／增益、命令選擇、控制律、電子輸出與診斷的順序及子頻率。 | 接收 `FlightControlComputerStepInput`；產出 `FlightControlComputerResult` 與建構時整理的指令綁定。不直接讀寫 Pipeline 資料鍵。 |

### 輸入與輸出格式

這個接縫使用 C++ 的具名 struct 與 `AircraftDataKey<T>`；每個 key 的型別固定，
不是依位置解讀的數值陣列。DCS 原始座標與軸值在進入 Core 前轉換。
建立時的 `StartMode` 與初始 `ThrottleLeverSignal` 用於第一次運算前的狀態及輸出。
正式建構子從模組內取得一次建立並驗證完成的唯讀設定；測試先取得可修改的
`FlightControlComputerConfigDraft`，只覆蓋指定欄位，再以
`finalize_flight_control_computer_config()` 驗證並封存。Entry 不保存或合併設定。

| 方向 | `AircraftDataKey<T>`／事件 | 內容與格式 |
|---|---|---|
| 輸入 | `kFlightControlObservation` | `FlightControlObservation`：飛行狀態；氣壓高度 ft、垂直速度 ft/s、空速 m/s、姿態與角速率 rad／rad/s 等。感測值另有可用性旗標。 |
| 輸入 | `kPilotControlSignal` | `PilotControlSignal`：三軸與三軸配平的正規化飛行員要求，範圍 `[-1, 1]`；拉桿、右滾、左偏航為正。 |
| 輸入 | `kThrottleLeverSignal` | `ThrottleLeverSignal`：左右油門桿正規化位置，範圍 `[0, 1]`。 |
| 輸入 | `kLandingGearData` | `LandingGearData`：完整起落架資料；FLCC 目前使用 `handle_down` 與 `any_weight_on_wheels` 判定控制狀態。 |
| 輸入 | `kFlightControlActuatorState` | `FlightControlActuatorState`：三個控制面通道的實際位置、速率與限制／飽和回授。 |
| 事件 | `Command` | Core `CommandId` 加 `value_normalized`；CAT、AP 與開發功能指令在週期之間送達。Computer 在 `setup()` 直接登記 Executive 彙整的 ID 與處理函式，Pipeline 分派後交付持有狀態的模組。實驗性自動油門指令只在設定啟用時登記。 |
| 輸出 | `kFlightControlActuatorCommand` | `FlightControlActuatorCommand`：三個控制面通道的電子需求，單位 rad；不是實際控制面位置。 |
| 輸出 | `kEngineThrottleCommand` | `EngineThrottleCommand`：左右引擎正規化油門命令。 |
| 輸出 | `kAutomaticFlightControlSnapshot` | `AutomaticFlightControlSnapshot`：AP 模式與參考值的狀態快照。 |
| 輸出 | `kFlightControlComputerSnapshot` | `FlightControlComputerSnapshot`：FLCC 狀態與診斷快照。 |

`SystemPipeline` 以 64 Hz 呼叫 `FlightControlComputer::step`，並提供以秒為單位的
`dt_s`。`setup` 先發布四種初始結果；每個週期完成後再一起發布四種新結果。
`FlightControlExecutive::update` 回傳其內部結果的 const 參照，下一次更新會覆寫內容；
需要跨週期保留時，呼叫者必須複製。需要單獨驗證運算時，測試直接呼叫
`FlightControlExecutive::update`；`FlightControlComputer` 只透過 Pipeline 步進。
詳細單位與座標約定見 [`docs/EFM_UNIT_CONVENTIONS.md`](../../../../../../docs/EFM_UNIT_CONVENTIONS.md)。

## Source layout

The root contains only the `Entry` factory, the `FlightControlComputer`
System adapter, the `FlightControlExecutive` orchestration, and this README.
Implementation files are grouped by responsibility without changing the
execution order or introducing independently scheduled Systems:

- `Configuration/`: 組成完整 FLCC 設定、執行跨模組驗證並封存；每個子模組的
  設定型別與局部驗證仍放在該子模組目錄。
- `Contracts/`: internal flight-control references and status types.
- `Input/`: observation and pilot-signal management.
- `CommandSystem/`: pilot/AP command production, selection, and coordination.
- `ModeAndGainScheduling/`: stores configuration and gain scheduling.
- `ControlLaws/`: control-law calculations and surface mixing.
- `Output/`: electronic output selection, limits, and actuator-command status.
- `Diagnostics/`: snapshots, cockpit projection, and debug telemetry.
- `Util/`: stateless flight-state and throttle-command calculations.

## Current production chain

> **Longitudinal status:** the failed Nz-to-q cascade has been replaced by a
> tagged Nz/q command contract and a single state-owning longitudinal law.
> Release tests pass; targeted DCS validation is still required before the
> behavior is accepted. The normative design and test procedure are documented
> in [`docs/LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md`](../../../../../../docs/LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md).

```text
FlightControlComputer adapter @ 64 Hz
  -> FlightControlExecutive
       -> InputSignalManagement
       -> FlightStateComputation
       -> ModeAndGainScheduling
       -> FlightControlCommandSystem
            -> PilotManeuverCommandLaw
            -> AutomaticFlightControl
            -> authority selection and joint guidance coordination
       -> FlightControlLaws
            -> longitudinal, lateral, and directional laws
            -> aircraft-specific surface command mixer
       -> FlightControlOutputSystem
       -> FlightControlDiagnostics (read-only projection)
  -> FlightControlActuatorCommand (physical radians)
```

The Executive owns deterministic execution order and integer subrates. Fast
measurement handling, pitch shaping, all three axis laws, and output run at
64 Hz. Roll-stick and pedal shaping run at the AFTI/F-16-reference 32 Hz
cadence and hold their last result between updates.
Slow air-data gain scheduling runs at the reference-derived 4 Hz cadence.
These rates are not confirmed F-CK-1 data. The external
`FlightControlActuationSystem` remains a separate physical-device System at the
project-defined 256 Hz integration rate.

The landing-gear handle selects the longitudinal command law for both pilot
and AFCS references: gear-up guidance is converted to a normal-acceleration
command, while gear-down guidance is converted to a pitch-rate command.
`FlightStateComputation` derives flight-path angle from vertical speed and
airspeed so this conversion follows the point-mass relation between flight-path
rate and normal acceleration instead of switching law according to AP mode.

The normative unit and reference-frame contract is
[`docs/EFM_UNIT_CONVENTIONS.md`](../../../../../../docs/EFM_UNIT_CONVENTIONS.md).
Pressure-altitude guidance uses feet, magnetic-heading guidance uses degrees,
and attitude, body-rate, AOA, sideslip, and primary-surface paths use radians.
The DCS world yaw is never an aviation heading.

Debug telemetry keeps the raw boundary `Actual Nz` separate from `Filtered Nz`.
The latter is the input-signal-managed value used to form the published `Nz
Error`, so a DCS trace can reconstruct the control-law calculation.

## Output and future actuator channels

`FlightControlOutputSystem` owns electronic-law selection, bumpless fading on
selection changes, command and feedback validation, physical demand limits,
electronic saturation reporting, actuator-tracking assessment, and the typed
conversion to the three modeled primary channels. It does not hide invalid
commands or feedback: those fail explicitly. The external actuation System
owns servo lag, rate, position, and physical saturation at 256 Hz.
Each actuator surface reports its signed physical position-limit state, whether
the stop has actually been reached, and its rate-limit state explicitly. The
64 Hz FLCC consumes those device facts; it never reconstructs a physical stop
from its electronic mixer limits.

Electronic command saturation and physical actuator saturation are separate
facts. The former means the FLCC demand exceeded configured surface travel
before transmission; the latter is actuator feedback from the 256 Hz physical
device. The AFCS monitor may combine them to assess available control authority,
but diagnostics retain both sources independently.
`ControlAuthorityLimited` is reported separately after an AOA-protection
nose-down command has exhausted longitudinal electronic or symmetric-
stabilator travel authority and alpha has failed to recover for the project-
defined qualification interval. Saturation on another axis cannot raise this
longitudinal state.

Create a separate public `ActuatorSignalManagement` module only when a real
second electronic channel, channel-specific feedback, or persistent channel
selection state exists. At that point it owns surface-to-channel routing,
feedback selection, disagreement, and channel availability. It must not own
hydraulic/servo physics or select a control law. A future integrity module
consumes its status and applies reconfiguration on the following FLCC tick.

## Deferred integrity and lifecycle modules

Do not add empty BIT, voting, redundancy, or failure-manager classes. Materialize
`SystemIntegrity` only when all three exist:

1. a real failure or disagreement source;
2. persistent qualification state;
3. a control-law or channel reconfiguration consumer.

Its future result is an immutable next-tick availability/reconfiguration
assessment. Same-tick input and output validity checks remain in their signal
owners; the Executive must not rerun a completed control-law tick.

Materialize `FlightControlLifecycle` only when cold start, restart, BIT, or
power-state transitions change FLCC operational availability. It will own that
state machine while the Executive only sequences it. Hot-start defaults alone
do not justify a lifecycle class.

## Developer-only behavior

Direct control law, G-limiter override, and experimental auto-throttle are
independent developer features. They are disabled and unavailable in the
production configuration unless a test explicitly changes the corresponding
configuration draft before finalization. None is evidence that the real
F-CK-1 provides that feature. Experimental auto-throttle remains a throttle
side branch and never enters the surface control laws.

## Dependency rules

- Internal control modules depend only on typed Core contracts and their
  immutable configuration.
- Consumers include only `FlightControlCommandSystem` and `FlightControlLaws`.
  Pilot mapping, reference selection, guidance coordination, numerical helpers,
  and inner-loop mechanics remain under their respective `Internal` folders.
- No internal module reads `AircraftDataView`, DCS parameters, CSV, or Debug
  Indicator state.
- `FlightControlDiagnostics` may observe completed tick results but cannot feed
  any control calculation.
- Persistent algorithm state has one owner. The Executive stores counters and
  held subrate configuration only; it does not duplicate controller state.
- Production has one control path. Do not add an old/new-law switch, a silent
  fallback, or fake-success redundancy.
