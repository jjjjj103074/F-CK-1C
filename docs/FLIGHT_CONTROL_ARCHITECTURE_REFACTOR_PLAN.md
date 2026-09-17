# F-CK-1C 正式數位飛控架構重構計畫

## 文件狀態

- 狀態：外部 FLCC／actuator 架構與新版縱向控制律均已實作並通過自動驗證；等待 DCS 高迎角、放輪 q-command 與 AFCS 共路徑驗證。
- 修訂日期：2026-08-05。
- 主要參考：AFTI/F-16 DFCS、F-16 Block 40/F-16XL DFLCS、F/A-18 701E/RFCS 公開資料。
- 設計方法：一部實體 FLCC 對應一個 C++ `System`；內部採物件組合、深 Module、小 Interface、單一狀態 owner。
- 外部 System 排程與 64/256 Hz seam 已確認，不在本計畫重新設計。
- 舊施工文件：[`FLIGHT_CONTROL_REFACTOR_PLAN.md`](FLIGHT_CONTROL_REFACTOR_PLAN.md)。
- 歷史行為基線：[`FLIGHT_CONTROL_BASELINE.md`](FLIGHT_CONTROL_BASELINE.md)；施工前已重建並由 characterization tests 固定結果。

### 完工驗證紀錄（2026-08-05）

- 歷史自動驗證曾通過 Release native tests、EFM／Cockpit architecture、baseline、DLL exports 與 Standards／Spec review。
- 以上只代表當時的靜態／自動驗證結果，不再構成縱向控制律的完成證明。DCS 持續後拉測試已暴露積分歷史、迎角限制與致動器飽和形成的週期性突然卸載。

### 縱向控制重新設計（2026-08-05）

- 舊設計同時產生 Nz 與 q feed-forward，再以 Nz-to-q 串級與 q PI 控制控制面；這不符合本計畫要求的 F-16 參考拓樸，已由新版 tagged command 與單一縱向 law 取代。
- 不再混用 YF-16 limiter slopes、F-16XL endpoints 與專案自訂 q scaling 組成一條無法證明的混合控制律。
- 新的正式縱向邏輯、證據分級、狀態 owner、測試 seam、完成狀態與 DCS 驗收方式見 [`LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md`](LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md)。
- 本文件其餘外部 System 邊界、64/32/4 Hz 排程、64/256 Hz FLCC／actuator seam、單位責任與橫向／方向軸架構仍有效。

本文件取代上一版「依現有檔案重新整理」的內部架構。新的起點是完整數位飛控電腦應具有的功能，再把現有程式依責任合併、拆分、內縮或刪除；不要求一個舊檔案對應一個新 Module。

## 1. 參考架構能證明什麼

### 1.1 AFTI/F-16

NASA TP-2857 記錄的是由一架 F-16A 改裝的研究型三餘度 DFCS，不是早期量產 F-16A/B 的類比飛控軟體。報告公開七個最高層軟體區域：

1. Executive。
2. System Monitor。
3. Selector Monitor。
4. Failure Manager。
5. Startup and Restart。
6. AMUX Processor。
7. Control Laws。

Control Laws 內有五種 longitudinal control structures、三種 lateral-directional structures，以及 gain scheduling、mode switching、reconfiguration、limiter、filter 和 surface-command calculation。Longitudinal laws 以 64 Hz 執行；lateral-directional feedback/interconnect 以 64 Hz 執行，roll-stick/pedal shaping 以 32 Hz 執行；依慢變 air-data 的 gain scheduler 以 4 Hz 執行。

報告採 top-down structured design 與 assembly language。「超過 50 個 control-law modules」是當年程式分解，不表示本專案應建立 50 個 C++ class。可移植的是功能責任、時序與資料 ownership，不是檔案數量或語言結構。

參考：[NASA TP-2857 — Development and Flight Test Experiences With a Flight-Crucial Digital Control System](https://ntrs.nasa.gov/citations/19890014956)。

### 1.2 F-16 Block 40／F-16XL

F-16XL 換裝四餘度量產 F-16 Block 40 DFLCC，控制律以 64 cycles/s 執行。報告明確指出 leading-edge-flap scheduling、air-data functions 與 angle-of-attack functions 曾從 ECA 移入 DFLCC。這支持「數位 FLCC 可能同時擁有 primary laws、部分 flight-state computation 與 secondary-surface scheduling」，但公開報告沒有提供 Block 40 完整軟體 Module 圖。

參考：[NASA TP-2004-212046 — F-16XL DFLCS](https://ntrs.nasa.gov/api/citations/20040040334/downloads/20040040334.pdf?attachment=true)。

### 1.3 F/A-18 701E

NASA FAST/RFCS 圖顯示基礎 F/A-18 701E 的主要功能鏈：

```text
Input Signal Management
  -> F/A-18 Control Laws
  -> Output Signal Select / Fading Logic
  -> Actuator Signal Management
```

同一電腦另有 built-in test、executive/data management、mode logic、envelope monitoring 與 fault checking。F/A-18 AAW 報告又把控制律描述成 Longitudinal、Lateral、Directional controllers，再將控制誤差與回饋分配到 stabilator、aileron、LEF/TEF 等控制面。

參考：[NASA FAST F/A-18 architecture](https://ntrs.nasa.gov/api/citations/20140006737/downloads/20140006737.pdf?attachment=true)、[NASA F/A-18 AAW control laws](https://ntrs.nasa.gov/api/citations/20060003626/downloads/20060003626.pdf?attachment=true)。

### 1.4 採用原則

- 頂層 FLCC 功能分工以 AFTI/F-16 與 F/A-18 701E 的共同部分為主。
- Control-law axis 分工同時保留 AFTI 的 lateral-directional 整體責任與 F/A-18 的 lateral/directional 內部分工。
- F-16XL 只用來確認部分數位化功能歸屬與 64 Hz，不直接借用 gain。
- 公開資料沒有證明 F-CK-1C/D 完全採同一設計；所有移植行為依證據標成 `Confirmed F-CK-1`、`Reference-derived`、`Project-defined` 或 `Developer-only`。

## 2. 物件導向與 Module 原則

### 2.1 實體與軟體的對應

- `FlightControlComputer` 是唯一對應實體 FLCC 機箱的 C++ `System`。
- FLCC 內部的 Executive、signal management、control laws 等都是普通 Module，不註冊為獨立 `System`。
- 多部 FLCC 的長期目標以建立多個 `FlightControlComputer` instance 實現，不在內部預埋 singleton 或跨 instance global state。

### 2.2 何時建立 class

- 有跨 tick 狀態、完整生命週期或大量隱藏規則：建立 stateful class Module。
- 純數學轉換、filter equation、interpolation：使用 pure function/private helper。
- 跨 Module 資料：使用 immutable value type，名稱與型別表達單位及語意。
- 只有一個 adapter 時不建立抽象基底；出現第二個真實 implementation 才引入 polymorphic Interface。
- 使用 composition，不用 inheritance 表達 CAT、Gain Mode 或 AP mode。

### 2.3 SOLID 的具體約束

- **SRP**：一個 Module 只有一個系統責任與一個狀態 owner；不是一條公式一個 class。
- **OCP**：新增 guidance source、law mode 或 effector 時，修改 owning Module 的內部組合，不擴大所有 caller 的 Interface。
- **LSP**：未建立第二個真實 implementation 前，不創造虛構的 law hierarchy。
- **ISP**：caller 只接觸自己需要的 result type；不共享 giant `FlightControlContext`。
- **DIP**：System adapter 依賴 typed input/output contracts；控制 Module 不依賴 DCSBridge、`AircraftDataView` 或 diagnostics。

### 2.4 深 Module 規則

- 每個頂層 Module 原則上只有 `update/evaluate/generate` 之類的一個主要操作。
- internal seams 可以獨立測試，但不自動成為 public Interface。
- 刪除一個 Module 若只會刪掉轉接碼，而不會把複雜度推回 caller，該 Module 應合併。
- `Manager`、`Context`、`State`、`Data` 不可成為可隨意增加欄位的容器。
- production behavior 只有一條路徑；不保留 old/new law toggle、silent fallback 或 fake-success Module。

## 3. 固定的外部 System seam

```text
PilotControls
    -> PilotControlSignal

FlightControlObservation + PilotControlSignal + actuator feedback
    -> FlightControlComputer @ 64 Hz
    -> FlightControlActuatorCommand

FlightControlActuationSystem @ 256 Hz
    -> FlightControlActuatorState

FlightControlActuatorState
    -> AerodynamicsModel
    -> ForceMomentOutput
```

固定規則：

- `FlightControlComputer` 是 actuator command 的唯一 publisher。
- `FlightControlActuationSystem` 是 actuator physical state 的唯一 publisher。
- 同一 scheduled-time bucket 讀相同 immutable snapshot，完成後 batch commit。
- FLCC 只使用 `SystemStepContext::dt_s`，不使用 DCS host-frame dt 積分。
- DCSBridge 只負責 DCS 資料/命令 adapter；Lua 不擁有飛控狀態。
- Snapshot、Debug Watch、CSV 是唯讀 projection，不得回饋控制計算。

## 4. 正式 FLCC 功能架構

### 4.1 主控制資料流

```text
System Adapter
  -> Executive
      -> Input Signal Management
      -> Flight-State Computation
      -> Mode and Gain Scheduling
      -> Command System (Pilot + AFCS)
      -> Flight Control Laws
          -> axis control laws
          -> aircraft-specific surface command mixer
      -> Output Signal Management
      -> Actuator Signal Management
  -> FlightControlActuatorCommand
```

### 4.2 完整性與生命週期

```text
Input / Mode / Laws / Output status
  -> System Integrity (monitor + failure + reconfiguration)
  -> next-tick availability and law configuration

Startup / Restart / BIT
  -> Executive operational state

Immutable tick record
  -> Diagnostics / Debug Watch / snapshots
```

Input Signal Management 在本 tick 先完成 input validity/selection；Output Signal Management 也必須在本 tick 阻止無效 command 離開 FLCC。控制律完成後才得到的 tracking、disagreement 或 output failure 不可讓同一 tick 重跑控制律，而是形成下一 tick 的 reconfiguration。Primary hard protection仍在本 tick 完成。這區分「同 tick 訊號安全」與「next-tick 模式重構」，並保留 deterministic execution。

## 5. `FlightControlComputer` — 實體 System adapter

**參考來源：** AFTI/F-16 FLCC hardware/software seam、F/A-18 FCC、現有 System 架構。

**責任：**

- 將 `AircraftDataView` 與 handler commands 轉成 typed `FlightControlComputerInput`。
- 呼叫內部 `FlightControlExecutive` 一次。
- 將結果寫入 `SystemResult`。
- 持有所有內部 Module instances 與 config root。
- 隔離 SystemPipeline、DCSBridge 與 Operational Flight Program implementation。

**不得擁有：** control-law math、filter、mode logic、AP、limiter、diagnostics mapping 或內部狀態副本。

這個 class 可以很薄，因為它是有價值的 adapter；它的存在不是用來假裝內部邏輯很深。

## 6. `FlightControlExecutive` — OFP 執行與時序

**參考來源：** AFTI/F-16 Executive、F/A-18 executive/data management。

**責任：**

- 固定一個 FLCC tick 的執行相位與 subrate eligibility。
- 套用上一 tick 已排隊 commands。
- 讀取 lifecycle/integrity 提供的 operational availability。
- 呼叫各 Module，建立 immutable tick record 與 final result。
- 確保 monitor/reconfiguration 的 next-tick 規則。

**狀態：** tick counter、subrate phase、operational state reference；不保存各 Module 演算法狀態。

**Interface 草案：**

```cpp
FlightControlComputerResult update(
    const FlightControlComputerInput& input);
```

Executive 不是另一個 `System`，也不自行排入全域 queue。若最後只剩無狀態的固定函式串接，應合併回 `FlightControlComputer::step()`；只有 subrate/lifecycle orchestration 足以形成深度時才保留 class。

## 7. `InputSignalManagement` — 訊號完整性與 conditioning

**參考來源：** F/A-18 Input Signal Management、AFTI Selector Monitor/analog-input mechanization。

**責任：**

- 驗證 sensor、pilot controller、gear/WOW、actuator feedback 的可用性與時間一致性。
- 單位、符號、cyclic angle 與 coordinate convention 正規化。
- sensor filtering、input selection、validity/freshness status。
- stick/pedal/trim deadband、shaping、slew 與相關歷史。
- 產生 `ManagedFlightControlSignals`，保留 measurement 與 validity，不產生控制決策。

**不得擁有：** CAT/Gain/Law mode、aircraft envelope、AP、Nz/q/alpha law、surface demand 或 failure-driven law selection。

目前 DCS 只有一份 observation 時，不模擬三餘度投票；selection Interface 只處理真實存在的資料來源與失效狀態。

## 8. `FlightStateComputation` — FLCC 使用的飛行狀態

**參考來源：** F-16XL DFLCC 的 air-data/AOA functions、太空梭 measurement incorporation；實際數值仍依本專案 DCS seam。

**責任：**

- 由 managed measurements 建立 FLCC 使用的 Mach、qbar、alpha、beta、Nz、body rates、attitude/heading 等一致狀態。
- 區分 measured、derived 與 filtered quantities。
- angle wrapping、磁航向/世界座標分離、SI/radian 計算單位。
- 回報 computation validity，不選擇 flight-control mode。

**不得擁有：** gain schedule、AOA limit、CAT policy、command shaping 或 actuator command。

若 DCS 已提供可靠 measurement，Module 仍負責語意/單位與 derived state，但不重建虛構 INS/air-data sensor physics。

## 9. `ModeAndGainScheduling` — 模式、構型與 schedules

**參考來源：** AFTI/F-16 mode selection、gain scheduler、reconfiguration inputs；F/A-18 mode logic。

**對外責任：** 由 flight state、pilot configuration commands 與 integrity availability 產生 immutable `ActiveFlightControlConfiguration`。

**內部 seams：**

- `FlightControlModeLogic`：Normal/Direct-or-Backup、Gain Mode、合法 transition。
- `StoresConfigurationLogic`：CAT I/III target 與 transition。
- `GainScheduler`：依 Mach、qbar、configuration 計算 axis schedules。
- `LawAvailabilityResolver`：根據 Integrity 結果限制可用 law；不自行偵測故障。

**必須分開的維度：**

```text
StoresConfiguration: CAT I / CAT III
GainMode: Cruise / TakeoffLanding / Standby
ControlLawMode: Normal / Direct-or-Backup
AutomaticControlAuthority: per axis, supplied by Command System
```

**不得擁有：** input filter、AP target、Nz/q/alpha controller、surface mixing 或 Developer-only feature state。Developer G override 移至獨立 development configuration，不污染 production mode model。

## 10. `FlightControlCommandSystem` — command sources 與 authority

**參考來源：** AFTI/F-16 multimode/integrated flight-control command paths、F/A-18 mode logic；具體 F-CK-1 AP 行為依已確認/推定資料。

**對外 Interface：**

```cpp
ResolvedFlightControlCommand generate(
    const FlightControlCommandInput& input);
```

**內部 seams：**

- `PilotManeuverCommandLaw`：normalized stick/pedal 到物理 maneuver objective。
- `AutomaticFlightControl`：ATT/ALT/ROLL/HDG 等 target、capture、guidance、stick steering、paddle、monitor。
- `CommandAuthorityLogic`：每軸 pilot/automatic authority 與 override。
- `CommandCoordination`：vertical+lateral feasibility、bank-to-lift、coordinated-turn feed-forward。

**輸出：** 不是 stick-equivalent `-1..1`，而是具單位的 longitudinal、lateral、directional objectives 與每軸 authority/constraint status。

**不得擁有：** p/q/r inner-loop integrator、surface demand、primary G/AOA protection、actuator saturation physics 或 throttle/A/T。

`AutomaticFlightControl` 是同一 FLCC 內的 stateful internal Module；ModeLogic/Monitor 屬其 private implementation，Vertical/Lateral Guidance 可保留 internal seams。Experimental A/T 仍是獨立 side branch。

## 11. `FlightControlLaws` — 穩定、操縱與控制面命令

**參考來源：** AFTI/F-16 longitudinal + lateral-directional structures、F/A-18 longitudinal/lateral/directional controllers。

**對外 Interface：**

```cpp
FlightControlLawsResult update(
    const FlightControlLawsInput& input);
```

**內部結構：**

```text
FlightControlLaws
  ├─ LongitudinalControlLaw
  ├─ LateralControlLaw
  ├─ DirectionalControlLaw
  ├─ LateralDirectionalCoordination
  └─ SurfaceCommandMixer
```

對外維持一個深 Module；內部將 roll/yaw 狀態與算法分開，再由 coordination seam 擁有 aileron-rudder interconnect、beta/turn coordination 與 cross-axis limits。`SurfaceCommandMixer` 是同一 Module 的 private seam，負責把各軸結果轉成具名、具 radians 單位的控制面需求。這同時保留 AFTI 的 surface-command calculation、F-16 MATV 的 aircraft-specific surface mixer與 F/A-18 controller-to-surface 分工，又不把緊密耦合的計算拆成兩個淺 public Modules。

### 11.1 Longitudinal

- 縱向正式規格由 [`LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md`](LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md) 定義。
- Cruise 使用單一 Nz command；q 是 washed-out feedback，不是並行的 pilot feed-forward。Takeoff/Landing 使用互斥的 pitch-rate command。
- alpha/Nz command limiting 與 alpha static-stability feedback 是兩條不同路徑，但由同一個 `LongitudinalControlLaw` 擁有狀態與合成順序。
- 禁止 Nz-to-q 雙 PI 串級、同 tick 同時有效的 Nz/q 目標，以及在鏈尾直接夾住 control surface 的迎角補釘。

### 11.2 Lateral

- Roll-rate command、roll limit、p feedback、integrator 與 lateral objective。
- CAT 可以改變 roll authority/gains，不可改變 controller deadband。

### 11.3 Directional

- Sideslip/yaw objective、r feedback、yaw damper、rudder contribution 與 integrator。
- 不自行重複 coordinated-turn 或 aileron-rudder interconnect。

### 11.4 Final policy

- generic `RATE/HOLD/DEGRADE` 整體移除。
- Neutral stick behavior 由當前 manual law 表達；ATT/ALT/HDG hold 由 AFCS 表達。
- public `InnerLoopControl`、`ControlLawMath` 與 lifecycle free functions 內縮為 private helpers。
- Mode/CAT transition 的 integrator 必須 reset、track 或 transfer，不能沿用不相容狀態。

### 11.5 `SurfaceCommandMixer`

**參考來源：** F/A-18 controller-to-surface branches、F-16XL LEF scheduling、AFTI/F-16 surface-command calculation。

- Axis controllers 先產生 dimensionless control effort；這只是 `FlightControlLaws` 內部演算法量，不得跨 Module seam。
- Mixer 以 aircraft-specific limits 將 effort 轉成 `symmetric_stabilator_demand_rad`、`differential_flaperon_demand_rad` 與 `rudder_demand_rad`。
- 現有氣動與致動模型只支援三個等效 primary channels，因此不虛構左右獨立 stabilator/flaperon actuator。
- 已有 secondary-surface 可觀察行為先由既有 `SecondaryFlightControls` 保存；未取得 F-CK-1 LEF/TEF scheduling 證據前，不搬入 FLCC 或新增行為。

**不採用通用最佳化 `ControlAllocation`。** 現階段使用 aircraft-specific effector synthesis；只有未來出現多個可互換 effectors、故障重配置或 thrust vectoring 時，才把 allocation 提升為獨立深 Module。

**輸出：** `ControlSurfaceDemandSet`；所有 primary surface demands 都是具名物理角度 radians，不再輸出 normalized surface command。

## 13. `FlightControlOutputSystem` — output 與 actuator signal seam

F/A-18 701E 將 Output Select/Fading 與 Actuator Signal Management 分開。為避免現階段建立兩個淺 class，本計畫先使用一個對外深 Module，內部保留兩個責任明確的 seams：

### 13.1 `OutputSignalManagement`

- Normal/Backup command-set selection。
- mode/reconfiguration command fading 與 transient-free transfer。
- command validity、finite/range/consistency checks。
- final numeric bounds；不得在此加入 G/AOA/heading aircraft behavior。

### 13.2 `ActuatorSignalManagement`

- surface demand 到 `FlightControlActuatorCommand` 的 typed conversion。
- actuator feedback validity/routing 與 electronic-interface status。
- 未來 channel selection/monitoring 的擴充位置。

**與外部 Actuation System 的差別：** 這裡處理電子 command、選擇與介面；`FlightControlActuationSystem` 處理 256 Hz servo lag、rate、position、physical saturation 與 measured surface state。

兩個 internal seams 任一形成獨立狀態、第二個 adapter 或明顯測試面後才拆成 public Modules。

未來加入冗餘致動器或多電子通道時，擴充方向固定如下：

- `OutputSignalManagement` 選擇/淡入淡出 FLCC law command set，不處理液壓或 servo physics。
- `ActuatorSignalManagement` 才展開成 public Module，建立 surface-to-actuator-channel command、回授選擇、channel disagreement 與 channel availability。
- `SystemIntegrity` 接收 channel status，產生下一 tick reconfiguration；不得由 actuator adapter 自行切換 control law。
- `FlightControlActuationSystem` 繼續代表實體 actuator/servo/hydraulic dynamics；若未來一個實體裝置對應一個 `System` instance，透過 typed channel contracts 組合，不把物理狀態搬回 FLCC。
- Output 與 Actuator Signal Management 必須先出現各自 persistent state 或第二個真實 adapter 才拆 public Interface，避免預先建立空殼。

## 14. `SystemIntegrity` — monitor、failure 與 reconfiguration

**參考來源：** AFTI/F-16 System Monitor、Selector Monitor、Failure Manager；F/A-18 envelope monitoring/fault checking。

**對外 Interface：**

```cpp
IntegrityAssessment evaluate(
    const FlightControlIntegrityInput& input);
```

**內部 seams：**

- `SystemMonitor`：execution、input、law、output 與 interface health。
- `SelectorMonitor`：存在多來源/多 channel 時執行 selection 與 disagreement persistence。
- `FailureManager`：記錄 confirmed failures、reset policy 與 annunciation state。
- `ReconfigurationLogic`：產生下一 tick law/effector availability。

目前沒有多餘度 sensor/FLCC/actuator channel 時，不製作假投票或永遠成功的 BIT。第一階段只實作真實可檢查的 freshness、finite、range、execution deadline、actuator tracking/saturation persistence；多通道功能保留架構位置但不建立空 class。

即時 input source selection 屬 `InputSignalManagement`；即時 output rejection 屬 `FlightControlOutputSystem`。`SystemIntegrity` 負責需要 persistence、failure record 或 law reconfiguration 的較高層判斷，避免同一項 validity policy 在三處重複。

Primary G/AOA protection 不屬 SystemIntegrity；它屬正常 Flight Control Laws。SystemIntegrity 處理硬體/資料/計算功能是否可信。

## 15. `FlightControlLifecycle` — Startup、Restart 與 BIT

**參考來源：** AFTI/F-16 Startup/Restart/BIT、F/A-18 BIT。

**責任：**

- FLCC operational states：Uninitialized、Initializing、Operational、Degraded、Unavailable。
- 初始化各 stateful Module，協調 reset/restore 與 restart transient policy。
- 執行目前真實可驗證的 power-on/continuous checks。
- 將 lifecycle availability 交給 Executive/SystemIntegrity，不直接產生 control command。

在尚無電源、冷啟動與故障模型時，不讓 Lifecycle 擋住現有 hot-start；但必須以明確的 `HotStartDevelopmentConfiguration` 啟動 Operational，不使用隱藏 fallback。本輪不建立空 class，先在 FlightControlComputer README 完整記錄未來 Interface、狀態機、觸發條件與 ownership。

## 16. Data Interface、Diagnostics 與 Experimental branch

### 16.1 AMUX/Data Management 對應

AFTI 的 AMUX Processor 與 F/A-18 data management，在目前專案由 DCSBridge、`FlightControlComputer` System adapter、typed contracts 與 SystemPipeline snapshot 共同承擔。DCSBridge 是「遊戲 ABI/座標/可用資料」與「擬真 Core measurement contracts」之間的翻譯橋梁；它不得計算 flight-control law，也不得把 DCS world orientation 假裝成航空 heading。現階段只有一個真實 adapter，不另建 `DataBus` 抽象 class。未來模擬 1553、sensor electronics、quantization 或實體資料匯流排時，這些 aircraft-system behaviors 必須進 Core 的 sensor/data-bus Module，而不是塞進 DCSBridge。

### 16.2 `FlightControlDiagnostics`

- 從 immutable tick record 產生 FCC/AP snapshots、Debug Watch 與 debug.csv values。
- 合併目前 FCC `refresh_*()` 與 debug telemetry mapping。
- 不保存控制 state，不得被任何 control Module include 或讀取。

### 16.3 Experimental A/T

- `ExperimentalAutoThrottleAssist` 與 throttle composition 合併於 `Experimental/AutoThrottleAssist`。
- 有獨立 feature switch 與 Developer/Experimental 標記。
- 只輸出 `EngineThrottleCommand`，不進入 Flight Control Laws 或 surface synthesis。

## 17. 資料契約、狀態與時序

### 17.1 主要 contracts

| Contract | Owner/用途 |
|---|---|
| `FlightControlComputerInput` | System seam；dt、raw observations、pilot/gear/actuator/throttle signals |
| `ManagedFlightControlSignals` | Input Signal Management；normalized measurements + validity |
| `ComputedFlightState` | Flight-State Computation；derived/filtered physical state |
| `ActiveFlightControlConfiguration` | Mode/Gain；stores、gain、law availability、axis schedules |
| `ResolvedFlightControlCommand` | Command System；每軸 physical objective、authority、constraints |
| `FlightControlLawsResult` | Flight Control Laws；具名 radians surface demands + law status |
| `FlightControlOutputResult` | Output System；actuator command + electronic-interface status |
| `IntegrityAssessment` | SystemIntegrity；next-tick availability/reconfiguration reason |
| `FlightControlTickRecord` | Diagnostics-only immutable projection source |

### 17.2 單位與 reference-frame 分層

不能用「DCS 單位」、「美國英制」或「航空顯示單位」統一整套 Core。真實飛機也有不同層次：transducer voltage/count、bus word/scaling、control-law engineering quantity、pilot display。FlightControlComputer 採 domain-native aviation engineering units，並依下表固定 ownership：

| 層級 | 單位政策 | Owner |
|---|---|---|
| DCS ABI／EFM world | 驗證並轉換 DCS callback 的單位、座標與符號；Core 不得感知 DCS world convention | DCSBridge focused adapters |
| Simulation physics | SI、rad、rad/s；只描述氣動、力學與世界狀態 | Simulation Core contracts |
| Aircraft-system measurement | 依系統語意採 domain-native engineering unit；不得被 Simulation Core 的 SI 政策綁定 | DCSBridge -> aircraft-system contracts |
| Flight-control math | 每個 closed loop 使用一套明確且一致的 unit；只在 Module seam 轉換 | Input/State/Command/Laws owners |
| Pilot-selected/display value | 保留航空操作的精確離散單位，如 heading integer degrees、未來 altitude feet/airspeed knots | Cockpit command/presentation owner |

具體規則：

- Sensor 本身不「天然輸出度、英尺或 knots」；真實硬體常先輸出 voltage、frequency、count 或 scaled bus word。未模擬 sensor electronics 前，不為了看起來擬真而在 Core 來回轉換。
- Magnetic heading 是 reference-frame semantic，不只是角度單位。`MagneticHeading`、`WorldYaw`、`BodyYawRate` 必須是不同型別/欄位。
- Heading loop 使用 degrees：Heading Set 以 `[0,359]` integer magnetic degrees 保存，measured magnetic heading 使用 continuous degrees，wrapped heading error 使用 `[-180,180)` degrees；Lateral Guidance 輸出 `BankReferenceRad` 時才跨越 degree/radian seam。
- Altitude loop 使用 feet：selected altitude、measured pressure altitude 與 altitude error 全部使用 feet；Simulation 的 geometric altitude meters 不得直接進 AFCS altitude loop。Vertical Guidance 最後輸出 `PitchReferenceRad`。
- Attitude/AOA/body-rate/control-surface loops 使用 rad/rad/s；Nz 使用 g；Mach 與 aerodynamic coefficients 無因次。HUD pitch ladder 等顯示由 presentation seam 轉 degrees。
- 不建立 generic `Angle`，也不允許 degree/radian implicit conversion；型別名稱必須同時表達 quantity、reference 與 unit，例如 `MeasuredMagneticHeadingDeg`、`RollAttitudeRad`、`BodyPitchRateRadPerSec`。
- 若未來面板 airspeed selector 以 knots 操作，setpoint 與該 guidance error 保留 exact knots；這不表示 Simulation Core 的 airspeed state 改成 knots。
- `FlightControlLawsResult` 的具名 primary surface channels 使用 radians；normalized pilot authority 與 surface angle 不得共用型別。
- 每次 conversion 只有一個 owner；所有 config、debug channel、CSV header 與 test 都必須標明 unit/reference。
- 現有 [`EFM_UNIT_CONVENTIONS.md`](EFM_UNIT_CONVENTIONS.md) 繼續是規範文件，但實作 Phase 1 必須把「SI canonical」限縮到 Simulation physics；Aircraft Systems 改依本節採 typed domain-native engineering units。

### 17.3 persistent state ownership

| State | 唯一 owner |
|---|---|
| Sensor/pilot conditioning history | `InputSignalManagement` |
| Derived-state filter/estimator history | `FlightStateComputation` |
| CAT/Gain/Law mode、schedule phase | `ModeAndGainScheduling` |
| AP modes/targets/capture/monitor | `AutomaticFlightControl` |
| Longitudinal controller state | `LongitudinalControlLaw` |
| Lateral controller state | `LateralControlLaw` |
| Directional controller state | `DirectionalControlLaw` |
| Effector scheduling/trim state | `FlightControlLaws::SurfaceCommandMixer` |
| Output fading/selection state | `FlightControlOutputSystem` |
| Failure persistence/reconfiguration | `SystemIntegrity` |
| Startup/restart/BIT state | `FlightControlLifecycle` |
| Servo lag/rate/position | external `FlightControlActuationSystem` |
| Snapshot/debug values | 非控制 state；由 tick record projection |

### 17.4 subrate policy

- Executive 每 64 Hz FLCC tick 執行一次，不建立第二個全域 queue。
- Primary laws、fast feedback、output 與必要 monitoring 每 tick 執行。
- AFTI 的 32 Hz input shaping、4 Hz slow gain scheduling 作為 `Reference-derived` 候選；不是 `Confirmed F-CK-1`。
- 同一 FLCC 內以 base-tick counter 執行整數分頻：32 Hz 每 2 個 64 Hz ticks 更新，4 Hz 每 16 ticks 更新。這是 OFP Executive 的 cyclic schedule，不是另一個 `SystemPipeline` queue。
- slow Module 執行時收到實際 scheduled interval（例如 1/32 s 或 1/4 s）；未執行 tick 使用上一份 immutable output（zero-order hold）。
- subrate eligibility 只由 scheduled tick index 決定，不依 DCS host FPS 或 wall clock。
- 可以指定固定 phase offset 平衡工作量，但 AFTI 公開報告未提供其確切 phase 表；未取得依據前不得宣稱模擬原機 offset。
- 對應 Module cutover 時直接啟用其 reference-derived subrate：roll-stick/pedal shaping 32 Hz、slow air-data gain scheduling 4 Hz；每個頻率變更仍須獨立提交、測試與 DCS 驗證。
- Exact F-CK-1 rates 未確認前不得標成 `Confirmed F-CK-1`。

## 18. 已決策事項

### 18.1 已決策

- `FlightControlOutputSystem` 保持一個 public deep Module；Output Select/Fading 與 Actuator Signal Management 先作 internal seams。冗餘致動器的未來拆法依第 13 節。
- DCSBridge 是遊戲現實到擬真 Core measurement contracts 的唯一 adapter；Core 不依賴 DCS unit/axes/ABI。
- `SystemIntegrity` 與 `FlightControlLifecycle` 暫不建立空 class。正式責任、Interface、state machine 與未來觸發條件必須寫入 FlightControlComputer README；實作真實功能時再 materialize Module。
- Secondary-surface behavior 先保持現有功能，但移到正確 owner、清除重複/錯誤邏輯，並保留 typed effector seam 與 per-surface config 擴充點。
- Direct/Backup law 保留為 `Developer-only` feature，與 Experimental A/T、Developer G-limiter override 一樣由獨立 development configuration 啟用；預設擬真玩法不可啟用，也不得暗示真機具備。
- Executive 直接實作 deterministic subrate；對應 Module cutover 時即啟用 AFTI reference-derived 的 32/4 Hz behavior，不等待全部 64 Hz 架構完成。

### 18.2 Flight Control Laws 與 surface contracts

- `FlightControlLaws` 是一個 public deep Module；axis controllers、lateral-directional coordination 與 `SurfaceCommandMixer` 都是 internal seams。
- `FlightControlLawsResult` 直接輸出 `symmetric_stabilator_demand_rad`、`differential_flaperon_demand_rad`、`rudder_demand_rad`。
- 外部 Actuation 與 Aerodynamics 都只接收物理 radians；normalized primary-surface value 只允許在 DCSBridge visual draw-argument adapter 內產生。
- 目前不虛構左右獨立致動器或通用 Control Allocation；未來有真實需求與證據時才深化 mixer。

### 18.3 直接啟用 32/4 Hz behavior

- `FlightControlExecutive` 建立 64 Hz deterministic base tick 與整數分頻能力。
- `InputSignalManagement` cutover 時啟用 32 Hz roll-stick/pedal shaping；`ModeAndGainScheduling` cutover 時啟用 4 Hz slow air-data gain scheduling。
- 每個 subrate 變更各自成為可回溯的原子提交，不與其他軸控制律調校混在同一提交。
- Debug Watch/debug.csv 必須記錄 FLCC tick、各 subrate 累積 update counter、last-update tick、output age 與 held value；不能只記單 tick flag，避免較慢或非同步的 CSV logger 漏掉更新。`fck1c_state.csv` 繼續只保存飛機可觀察狀態。
- 測試必須覆蓋 scheduled dt、zero-order hold、固定 phase、不同 host FPS 的相同結果，以及長時間不累積 tick drift。

## 19. 現有程式到正式架構的重整

| 現有項目 | 決策 | 新 owner |
|---|---|---|
| `FlightControlComputer` | 保留為 System adapter、移出 orchestration 細節 | `FlightControlComputer` |
| FCC step/orchestration | 抽出；若最後太淺再內聯 | `FlightControlExecutive` |
| `InputSignalManagement` | 拆出 state computation 與 CAT policy 後深化 | `InputSignalManagement` |
| FCC observation/angle helpers | 合併 | `FlightStateComputation` |
| `ConfigurationAndMode`、CAT blend、gain/alpha schedules | 重新拆分後合併 | `ModeAndGainScheduling` internals |
| `FBWCatParams` / `FBWControllerConfig` | 按 owner 拆分並刪除 | 各 Module config |
| `PilotCommandLaw` | 深化 | Command System / Pilot law |
| `AutomaticFlightControl` | 保留、收入 Command System | AFCS internal Module |
| `ControlReferenceSelection` + `GuidanceCoordination` | 合併、重新命名 | Command authority/coordination internals |
| `ControlLaws` / `FBWFrame` | 取代 | `FlightControlLaws` |
| `FBWControllerState` | 依第 17.3 節拆分並刪除 | 各 state owner |
| `InnerLoopControl` / math / lifecycle helpers | 內縮 | Flight Control Laws private implementation |
| 目前 surface composition | 合併與擴充 | `FlightControlLaws::SurfaceCommandMixer` |
| final clamps / mode transition output logic | 合併 | `FlightControlOutputSystem` |
| actuator command conversion/feedback mapping | 分清電子/物理責任 | Output System / external Actuation System |
| saturation/degrade timers | 依語意拆分 | law anti-windup、Output、SystemIntegrity、AFCS monitor |
| FCC diagnostic refresh/debug telemetry | 合併 | `FlightControlDiagnostics` |
| Experimental A/T + throttle composition | 合併並隔離 | `Experimental/AutoThrottleAssist` |
| Aerodynamics artificial limiters | 稽核後保留/搬移/刪除 | Aerodynamics、Flight Control Laws 或 explicit assist |

## 20. 實作順序與原子 cutover

每個 Phase 結束時必須可編譯、可測試且只有一條 production path。不得建立長期 compatibility layer。

### Phase 0 — 重建基線與責任清冊

- 跑 Release x64 tests、architecture check、DLL build，記錄 commit、dirty files、test count 與 DCS version。
- 以 Debug Watch/debug.csv 記錄 manual/AP/limiter/surface inputs and outputs。
- 列出所有目前 behavior，區分 must-preserve、known defect、unverified/project-defined。
- 將每個 config/state/function 指派到本文件唯一 owner；無 owner 的先停止，不塞入雜項類別。

### Phase 1 — Contracts、units 與 System adapter

- 建立第 17.1 節 grouped contracts 與 unit-safe types。
- 讓 `FlightControlComputer` 只做 System adaptation；建立 Executive Interface，但不先製作空 pass-through Modules。
- 更新 [`EFM_UNIT_CONVENTIONS.md`](EFM_UNIT_CONVENTIONS.md)，明訂 DCS ABI、Core measurement、control-law math 與 pilot-selected/display 四層單位及 reference-frame ownership。
- 建立 `Core/Systems/FlightControlComputer/README.md`，記錄暫不實作的 Integrity/Lifecycle、未來 redundant actuator channel 路徑、建立 Module 的觸發條件與 Developer-only feature 規則。
- 在 Output/Actuation seam 留一則精簡 source comment，指向上述 README；詳細未來設計只保存一份，不在多個 header 重複。
- 建立 include/dependency architecture checks。

### Phase 2 — Input Signal 與 Flight State cutover

- 將 validity/conditioning 與 derived-state computation 分開。
- 移除 Input 中的 CAT/control-policy；移除 FCC 重複 observation/angle mapping。
- 依第 17.4、18.3 節直接啟用 32 Hz roll-stick/pedal shaping；其他 fast measurement conditioning 保持 64 Hz，兩者不得共用含糊的 update cadence。
- 建立 units、sign、wrapping、filter、freshness 與 non-finite tests。

### Phase 3 — Mode、Configuration 與 Gain Scheduling cutover

- 拆除 `FBWCatParams`/giant config，建立 Stores/Gain/Law 正交狀態與 axis schedules。
- command 於下一 FLCC tick 生效；transition 具明確 lifecycle。
- 依第 17.4、18.3 節直接啟用 4 Hz slow air-data gain scheduling，不與 host FPS 綁定。

### Phase 4 — Command System cutover

- 建立 physical pilot/automatic/resolved command types。
- 將 AFCS、authority、override、joint coordination 收入深 Module。
- 刪除 stick-equivalent AP output、舊 selected-reference variants 與兩段 public seam。

### Phase 5 — Flight Control Laws cutover

- 建立 Longitudinal、Lateral、Directional、coordination 與 Surface Command Mixer internal seams。
- 分配 integrators/filters/transitions state，刪除 `FBWControllerState` 與 generic HOLD/DEGRADE。
- 縱向依 [`LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md`](LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md) 重建單一 tagged objective、Nz forward-loop integration、washed-out q feedback、alpha static-stability feedback 與 alpha/Nz command limiting；未知 gain 標成 Project-defined。
- 移除正常模式的 pilot q feed-forward、Nz-to-q 雙 PI 串級與混合來源 schedule；另以獨立 q-command mode 處理放輪／進場。
- 同一 cutover 建立 axis effort 到 named-radian surface demand；保持目前 secondary-surface 可觀察行為，不在本輪新增無依據的氣動行為。
- 以 named-surface typed contract 與 per-surface config 保留未來 LEF/TEF/slat/flap 擬真 scheduling seam，不先建立未使用的 polymorphic hierarchy。
- 建立 surface identity、authority、priority 與 coupling tests。

### Phase 6 — Output/Actuator Signal cutover

- 建立 selection/fading/validity/typed actuator command。
- 將 electronic command limits 與 256 Hz physical actuator dynamics徹底分開。
- 驗證 Normal/Backup transitions、finite/range、tracking/saturation feedback。

### Phase 7 — Integrity、Lifecycle 與 diagnostics

- 依第 14、15、18.1 節只實作目前真實可驗證的 monitor；本輪不建立空的 Integrity/Lifecycle classes，也不建立假 redundancy/BIT。
- 只有存在真實 failure source、persistent state 與 reconfiguration consumer 時，才建立 next-tick reconfiguration；否則以 README 中的 future contract 保留設計，不加入 production path。
- 合併 diagnostics projection，移除 FCC refresh mappings。

### Phase 8 — Aerodynamics/secondary-surface ownership audit

- 將 `apply_aerodynamic_limiters()` 各行為分類成真實氣動、gameplay assist、FLCC control policy 或歷史補償。
- 真實氣動留在 Aerodynamics；assist 由 explicit option 控制；FLCC policy 搬移後刪除原路徑。
- 記錄尚無證據的 F-CK-1 LEF/TEF/slat/flap ownership，不以猜測填滿。

### Phase 9 — Verification 與收尾

- 跑 native tests、architecture check、DLL build、export verification。
- 執行 `install.bat` 後做 manual law、CAT/Gain、AFCS、high AOA/G、surface/output transitions DCS tests。
- 完成 standards/spec code review，回寫證據標籤、實際 subrates、Module decisions 與 contributor docs。
- 驗證預設擬真 configuration 無法啟用 Direct/Backup law、Experimental A/T 或 Developer G-limiter override；三者只能由明示的 Developer configuration 分別開啟。

### 20.1 實際 cutover 紀錄

- `FlightControlComputer` 已縮成 64 Hz 實體 System adapter；固定執行鍊由 `FlightControlExecutive` 擁有。
- `InputSignalManagement`、`FlightStateComputation`、`ModeAndGainScheduling`、`FlightControlCommandSystem`、`FlightControlLaws`、`FlightControlOutputSystem` 與 `FlightControlDiagnostics` 已形成單一 production path。
- Pilot shaping 以 64 Hz base tick 的整數二分頻在 32 Hz 更新；slow gain scheduling 以整數十六分頻在 4 Hz 更新；held output、counter、last tick 與 age 均可由 diagnostics 觀察。
- AFCS 與 manual reference 已在 Command System 內依軸選擇並共同協調，再進入相同的 protection、axis laws、surface mixer 與 output path。
- Heading target/measured/error 保持 magnetic degrees；altitude target/measured/error 與 vertical speed 使用 feet/ft/s；attitude、AOA、beta、body rate 與 primary surface 使用 rad/rad/s。
- `FlightControlLawsResult`、`FlightControlActuatorCommand`、Actuation state 與 Aerodynamics primary-surface input 已全程使用具名 physical radians；DCS draw argument 才負責視覺 normalized conversion。
- 原 Aerodynamics 人工 roll/yaw rate-control moments 已刪除，避免和 FLCC protection/laws 重複；超速阻力只保留在明示 `easy_flight` assist；airbrake pitching moment 留在 Aerodynamics 並以真實氣動責任命名。
- Direct law、G-limiter override 與 experimental A/T 已由獨立 development configuration 隔離，production defaults 不可啟用。
- Integrity、Lifecycle、redundant electronic actuator channels 沒有以空殼實作；唯一 future design 與 materialization triggers 記錄於 `Core/Systems/FlightControlComputer/README.md`。
- Characterization hashes 與 golden force/moment 已在移除重複氣動控制 policy 後更新；這是核准架構造成的刻意行為變更，不是無理由接受測試漂移。

## 21. 驗證矩陣

### 21.1 Module tests

- Input Signal：validity、units、sign、wrapping、filter、deadband、trim、slew、freshness。
- Flight State：measured/derived separation、Mach/qbar/alpha/beta/Nz、magnetic/world axes。
- Mode/Gain：CAT/Gain/Law 正交、schedule continuity、subrate determinism、transition lifecycle。
- Command：pilot mapping、AFCS capture/reference、per-axis authority、stick/paddle override、joint feasibility。
- Flight Control Laws：單一縱向 objective、Nz forward-loop integration、q washout、alpha command limiting／static-stability feedback 分離、G/AOA、p/beta/r、damper、anti-windup、bumpless transition、axis-to-surface synthesis、surface identity 與 no duplicated compensation。
- Output：selection/fading、validity、finite/range、electronic vs physical saturation distinction。
- Integrity/Lifecycle：persistence、next-tick reconfiguration、hot-start explicitness、no fake-success paths。
- Diagnostics：projection 完整且啟閉不影響任何 command。

### 21.2 DCS tests

1. Clean CAT I Cruise：neutral、step、sustained pitch/roll/yaw。
2. CAT III：相同 profile，比較 G、AOA、roll-rate 與 continuity。
3. Gain/config transition：觀察 command、surface、integrator 是否 bumpless。
4. High AOA/G：分開測試有足夠動壓的持續後拉筋斗與低動壓 authority-limited 機動；逐步、持續、釋放、再輸入，檢查 protection、anti-windup、控制面連續性與是否形成週期性突然卸載。
5. AP ATT/ALT/ROLL/HDG：分開、組合、reference adjust、stick steering、paddle。
6. Effector/output：surface mapping、fading、tracking、saturation 與 next-tick monitor。
7. 不同 host FPS：相同 input sequence 應產生相同 64/32/4 Hz scheduled results。

每次保存 `fck1c_state.csv`、`debug.csv`、`fck1c_efm.log`、`dcs.log`，並記錄構型、速度、高度、CAT、Gain/Law/AP modes、subrates 與輸入方式。

## 22. 完成定義

- 一部 FLCC 仍是一個 64 Hz `FlightControlComputer` System；內部 Modules 沒有被註冊為 Systems。
- 正式架構涵蓋 Executive、Input、Flight State、Mode/Gain、Command、Flight Control Laws、Output/Actuator Signal 與 Diagnostics；Integrity/Lifecycle 的責任、Interface 方向與 materialization 條件完整記錄，但在沒有真實 failure/lifecycle source 前不建立空 class。
- `FlightControlComputer` 只有 adapter 責任；Executive 只有 execution/lifecycle orchestration。
- 不存在 `FBWControllerState`、`FBWCatParams` 或等價 giant state/config/context bag。
- 每份 persistent state 只有一個 owner，所有跨 Module contracts immutable 且具單位。
- manual 與 AFCS 在 Command System 會合，共用 Flight Control Laws 與 Output path。
- longitudinal 在每個 mode 只接受一種 tagged objective；正常 Nz law 依公開 F-16 拓樸整合 Nz forward loop、q washout、alpha stability feedback 與 command protection，lateral/directional 分工明確並由 coordination 擁有 coupling。
- surface synthesis 位於 `FlightControlLaws` private mixer，output signal management 不再藏在 final clamp 或 Aerodynamics。
- electronic actuator-signal responsibility 與 physical actuation dynamics 分離。
- FlightControlComputer README 已記錄 deferred Integrity/Lifecycle、redundant actuator channels、Developer-only features 與其 materialization conditions；source seam 只留連結式精簡註解。
- generic HOLD/DEGRADE、silent fallback、fake BIT/redundancy 與 old compatibility path 已移除。
- Automated verification 與 standards/spec review 必須完成；縱向 DCS 驗證未通過前不得標成完成，所有未通過項目必須明確暴露。

## 23. 實作與審查收斂紀錄

以下是本計畫落地後依 Standards／Spec 雙軸審查補強的責任邊界；縱向控制律部分以新設計文件為準：

- 壓力高度由 Lua 的 DCS `getBarometricAltitude()` 明確提供 availability 與公尺值，`CockpitBridge` 一次轉為 feet。AFCS 不得以 geometric ASL 代替；資料缺失時 ALT HOLD 明確拒絕接通或解除垂直通道。
- Mach 對 AOA 上限的 schedule 完全屬於 `ModeAndGainScheduling`；`FlightStateComputation` 只計算量測與衍生飛行狀態。
- `LongitudinalControlLaw`、`LateralControlLaw`、`DirectionalControlLaw` 各自擁有狀態；只有 `LateralDirectionalCoordination` 能建立 roll/yaw coupling，最後由 private surface mixer 轉為具名 radians surface demand。
- `FlightControlOutputSystem` 擁有 Normal／Developer Direct electronic-law selection、切換 fading、command/feedback validation、electronic saturation、tracking assessment；256 Hz `FlightControlActuationSystem` 保持物理 servo owner。
- 電子 demand 在送出前超過 configured travel，與致動器實際到達 physical stop，分別投影為 `electronic_command_saturated` 與 `actuator_saturated`；AFCS monitor 可以合併判斷控制路徑 authority，但 diagnostics 不得混成同一個來源。
- `FlightControlCommandSystem` 是 command/AFCS/authority/coordination 的唯一 public boundary；pilot mapping、reference selection 與 joint coordination 位於其 `CommandSystem/Internal`。`FlightControlLaws` 的 numerical helpers 與 inner rate loop 位於 `ControlLaws/Internal`，外部測試只能經由兩個 public deep Modules。
- 64 Hz 執行 pitch shaping、axis laws 與 output；32 Hz 僅更新 roll-stick／pedal shaping；4 Hz 更新 slow gain schedule。counter、last tick、age 與 held values 全部投影到 Debug Watch／`debug.csv`。
- Easy Flight 的 damping 與 side-force 使用 physical surface radians 除以明確 travel calibration 後的 dimensionless ratio，不再把 radians 當 normalized。
- CAT I／CAT III 不用不同 deadband 製造模式差異；模式差異限於已有明確 owner 的 envelope、gain 與 control-law schedule。
- 自動測試覆蓋 boundary unit conversion、pressure-altitude unavailability、axis ownership、output selection/fading/tracking、64/32/4 固定 phase、長時間 tick drift 與 30/60/144 host FPS 不變性。
