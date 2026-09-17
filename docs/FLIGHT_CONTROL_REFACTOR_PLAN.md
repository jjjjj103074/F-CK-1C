# F-CK-1C 飛行控制電腦與自動駕駛重構計畫

> 歷史施工文件：其中以 `normal_acceleration_reference_g` 與
> `pitch_rate_feedforward_rad_s` 並行的縱向設計已撤銷。現行縱向規範以
> [`LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md`](LONGITUDINAL_FLIGHT_CONTROL_REDESIGN.md)
> 為準；正式 FLCC 邊界以
> [`FLIGHT_CONTROL_ARCHITECTURE_REFACTOR_PLAN.md`](FLIGHT_CONTROL_ARCHITECTURE_REFACTOR_PLAN.md)
> 為準。

## 文件狀態

- 狀態：架構決策與施工順序已完成，尚未授權開始實作。
- 更新日期：2026-08-01。
- 本文件是本次 `FlightControlComputer`／FBW／AP 重構的單一權威計畫。
- Phase 0 行為證據記錄於 [`FLIGHT_CONTROL_BASELINE.md`](FLIGHT_CONTROL_BASELINE.md)。
- 已完成的 `SystemPipeline` 排程與 `FlightControlActuationSystem` 裝置 seam 是固定前提，
  不在本次重新設計。
- 現階段只建立一部 `FlightControlComputer`。多 FLCC、投票、通道同步與故障降級是
  極長期工作，本計畫不為它們新增 Interface、ID、port、factory 或空殼 Module。

本文件把內容分成四種證據身分：

1. **Confirmed F-CK-1**：可靠第一方資料明確支持的 F-CK-1 事實。
2. **Reference**：F-16、AFTI/F-16、F-16XL 或 F/A-18 公開文件提供的設計參考。
3. **Project-defined**：公開資料不足時，專案為可操作模擬所做的明確選擇。
4. **Developer-only**：只供開發、測試或診斷，不屬於模擬機的真實功能。

沒有來源支持的內容不得寫成 F-CK-1 真機事實。

## 1. 重構目標

本次工作的目標不是重新命名舊類別，也不是只替現有 AP 調 PID。完成後應達到：

- 一個 C++ `System` 對應一個實際飛機裝置；`FlightControlComputer` 代表飛控電腦，
  FBW、AP 與限制器是機箱內的軟體 Module。
- 玩家控制、飛行狀態、AP reference、控制律 command 與 actuator demand 各有清楚語意，
  不再以同一組 `[-1, 1]` 值假裝它們是同一種訊號。
- AP 是外迴路：決定飛機要維持或追蹤的物理目標；FBW 是內迴路：依飛機狀態、限制器
  與控制律產生控制面需求。
- AP 不再覆寫玩家的原始搖桿輸入。玩家與 AP 的權限選擇發生在飛控電腦內明確的
  reference-selection seam。
- 控制律的設定、狀態、輸入、輸出與診斷分開，不再集中於大型可變 struct。
- 每個 Module 有小而完整的 Interface；外部只看得到需要的語意，不依賴內部積分器、
  filter 或 mode-transition 細節。
- 先以測試保存「目前有哪些功能」，再修正已知的 ALT Hold 擺盪與 reference 操作表現；
  舊行為只作為回歸證據，不作為正確飛行控制的規格。
- Lua 仍只負責 DCS 座艙輸入與呈現；C++ 是飛機狀態與飛控邏輯的唯一權威。

## 2. 本次範圍

### 2.1 包含

- `FlightControlComputer` 的外部 System 邊界與內部軟體架構。
- 玩家操縱訊號進入飛控電腦後的 conditioning 與 mode-dependent command mapping。
- 現有 FBW normal-law、gain schedule、CAT provisional model、limiter 與 actuator feedback。
- 現有 AP master、vertical、lateral、reference capture／adjust、bypass、disconnect，以及
  Reference-derived paddle／stick-steering／mode-monitor 語意與診斷。
- AP 與 FBW 之間由 normalized stick-equivalent 改為物理 reference 的 Interface。
- 現有 Auto-throttle 程式碼的隔離，目的只是不讓它污染飛控架構；本次不重設其功能。
- 檔案拆分、build 規則、automated tests、telemetry、CSV 與 DCS 驗證規格。
- 移除重構後不再使用的舊型別、舊轉接與 dead code。

### 2.2 不包含

- 多部 FLCC instance、triplex／quadruplex channel、voting、cross-channel data link。
- AIU、完整 BIT、power monitor、startup／restart、故障投票或 reversion law。
- 重新設計 `SystemPipeline`、固定更新率、time bucket 或 commit 規則。
- 重新設計 `PilotControls`、`FlightControlActuationSystem`、airframe physics 或 engine plant。
- 新增沒有 Reference 支持的 AP panel、模式、限制值或故障功能；本計畫採用的 F-16 限制值
  必須保留 Reference-derived 標籤，不得宣稱是 F-CK-1 真值。
- 證明 F-CK-1 使用 F-16 的 CAT I／III、MPO、gain 或 command law。
- 重新設計擬真或非擬真的 Auto-throttle；它屬於後續 AP／gameplay-assist 議題。
- 為尚未存在的 control effector 建立 `ControlAllocation` 系統或空殼架構。
- 以隨機 phase、額外 clock domain 或多執行緒改變目前 deterministic scheduler。

### 2.3 唯一的長期相容約束

目前不設計多 FLCC，但新程式碼不得使用 process-global mutable state、singleton、靜態
積分器或用 System ID 判斷控制邏輯。所有執行狀態都由單一
`FlightControlComputer` 物件擁有。這只是正常物件封裝，不新增多機箱抽象。

## 3. 資料依據與使用限制

### 3.1 Confirmed F-CK-1

- 中科院公開資料確認 F-CK-1 使用 digital fly-by-wire flight control system。
- 漢翔公開資料確認 F-CK-1C/D 配備 32-bit digital flight control computer。
- F-CK-1 是雙發動機機體；不能直接搬用單發 F-16 的推進與不對稱推力假設。

來源：

- [NCSIST — Ching-kuo IDF High Performance Fighter](https://www.ncsist.org.tw/eng/csistdup/products/product.aspx?catalog=9&product_Id=18)
- [AIDC — F-CK-1 C/D](https://www.aidc.com.tw/en/military/fck1)
- [中華民國空軍 — IDF](https://air.mnd.gov.tw/TW/Weapon/Weapon_Detail.aspx?CID=55&ID=89)

這些資料支持「有數位飛控電腦」及本次機箱邊界，但沒有公開內部軟體分區、AP placement、
command law、gain、limiter、更新率或冗餘細節。

### 3.2 F-16／F/A-18 Reference

NASA AFTI/F-16 TP-2857 公開的 FLCC 軟體包含 Executive、Control Laws、System
Monitor、Selector Monitor、AMUX Processor、Startup and Restart、Failure Manager
及 data/status tables。相同報告也把 pilot controllers、mode selection、inertial／air-data
reference 與 actuator signals 當成不同輸入，而不是一個通用 normalized intent。

F-16C/D flight manual 的公開鏡像把 AP 描述為 FLCC 的功能，並提供本計畫可直接使用的
Reference：ALT HOLD 使用 air-data altitude，vertical command 限制在 +0.5 g 到 +2.0 g；
Heading／Steering Select 的 bank command 最大 30°、roll rate 最大 20°/s。相同資料也清楚
區分 AP disconnect、paddle 暫時中斷全部 AP，以及只適用 attitude-hold mode 的 stick
steering。這些內容足以決定本專案的 AP／FBW seam 與人工接管語意，但仍不是 F-CK-1 真機
實作的證明。

NASA AFTI/F-16 TM-86026 顯示，依任務與模式不同，飛控可以使用 normal acceleration、
pitch rate 或 flight-path maneuver 等不同 command 形式；它不支持把所有飛控需求壓成單一
q 或單一 Nz。Space Shuttle DFCS 的 longitudinal control 又提供 q 與 normal acceleration
混合、以及 steady-turn pitch compensation 的公開例子。因此本計畫採用角色固定的
Nz objective 加 q feed-forward，而不是通用 command bag。

本計畫只採用以下可泛化的架構觀念：

- 飛控電腦是實體裝置，內部可有多個軟體責任。
- 玩家操縱、模式、感測資料與 actuator feedback 是不同訊號。
- control laws、gain scheduling、monitoring 與 failure handling 是不同責任。
- 不同飛行模式可以把同一個控制器輸入解讀成不同 command law。

F-16XL 公開報告記載其重寫的 DFLCS control laws 以 64 cycles/s 執行；專案目前以
64 Hz 更新 `FlightControlComputer`，這是 **F-16XL Reference**，不是 F-CK-1C 真機數值。

F/A-18 文件顯示其飛控系統提供 stability、control 與 autopilot functions；PSFCC 架構
則提供 input signal management、control laws、output selection 與 actuator signal
management 的清楚分層。Formation Autopilot 報告同時證明外迴路也可以位於外部研究
電腦，再把 stick-equivalent command 送入內迴路。因此公開資料支持軟體分層，但不足以
斷言所有機型的 AP 一定在同一塊硬體。

來源：

- [NASA TP-2857 — AFTI/F-16 Digital Control System](https://ntrs.nasa.gov/citations/19890014956)
- [F-16C/D Flight Manual 公開鏡像 — Block 50](https://studylib.net/doc/26036763/block-50)
- [NASA TM-86026 — AFTI/F-16 Digital Flight Control System](https://ntrs.nasa.gov/citations/19840006089)
- [NASA TP-3547 — F-16XL Digital Flight Control System](https://ntrs.nasa.gov/citations/20040040334)
- [NASA — F/A-18 Flight Control System](https://ntrs.nasa.gov/citations/19840014515)
- [NASA TM-1999-206581 — F/A-18 Production Support Flight Control Computer](https://ntrs.nasa.gov/citations/19990060322)
- [NASA TM-2002-210729 — F/A-18 Formation Autopilot](https://ntrs.nasa.gov/citations/20030005820)

### 3.3 民航 AP Reference 的適用邊界

民航 AP 對本計畫有參考價值，但只用於 **flight-guidance outer loop**，不拿來決定戰鬥機
FBW inner loop。FAA AC 25.1329 把 autopilot、flight director、automatic thrust control，
以及它們和 stability augmentation／trim 的互動視為完整 flight-guidance system；FAA
飛行教材也把 altitude/heading hold、vertical-speed mode、manual override 與 mode indication
列為常見 AP 行為，並明確指出不同飛機的 AP 差異很大。

可採用的參考：

- vertical／lateral mode 分軸管理、armed／capture／hold transition。
- altitude -> vertical path／vertical speed -> attitude／rate 的 cascade 概念。
- heading／course -> bank／roll guidance 的 cascade 概念。
- AP／Flight Guidance 負責依升降率、加速度與姿態限制平滑捕獲 target，不只發布最終高度。
- reference capture、manual override、mode annunciation、bumpless transfer 與 trim interaction。
- 以 stability、command tracking、disturbance rejection、control activity 共同判斷整定結果。

不可直接搬用：

- 民航機的 gain、bandwidth、舒適性限制、servo 動態或 certification threshold。
- 民航 AP mode set、FMS／approach coupling、auto-throttle coordination 或 redundancy。
- 民航 AP 最後送給 inner loop／servo 的訊號，作為 F-CK-1 command law 的真機證據。

因此民航資料能支持 outer-loop cascade、mode transition 與整定方法，但不能單獨決定
硬體 placement，也不能決定本專案最終採用 Nz、pitch-rate 或 flight-path reference。簡單
民航 AP 可能直接驅動 servo；本專案要求 AP 經過共用 FBW inner loop 與 protection，是依
F-CK-1 數位線傳邊界、F-16／F/A-18 Reference 及本專案架構目標共同做出的選擇。

來源：

- [FAA AC 25.1329-1C Change 2 — Approval of Flight Guidance Systems](https://www.faa.gov/documentLibrary/media/Advisory_Circular/AC_25.1329-1C_CHG2.pdf)
- [FAA Pilot's Handbook of Aeronautical Knowledge](https://www.faa.gov/regulations_policies/handbooks_manuals/aviation/phak)
- [NASA — Robust Integrated Autopilot/Autothrottle Design](https://ntrs.nasa.gov/search.jsp?R=19910012814)

### 3.4 太空梭與火箭 Guidance／Control Reference

跨領域公開資料沒有顯示一種所有載具共用的 command protocol；它們共同採用「Guidance
先產生有物理意義的 reference，Control 再追蹤 reference 並輸出 effector demand」。實際
物理量依下一層控制問題選擇，不要求 pitch／roll／yaw 使用相同維度。

可採用的證據：

- Space Shuttle entry guidance 以 drag／energy／range 為高階目標，主要使用 bank-angle
  magnitude／sign 控制 lift vector，並依 flight phase 使用 AoA reference。
- Space Shuttle autoland 用 vertical-acceleration command 接到 pitch control，用 bank-angle
  command 接到 roll control，speedbrake 另負責 energy control。
- Saturn V Guidance 由 position／velocity 計算 required thrust direction／desired attitude；
  Flight Control 再把 attitude error 與 rate-sensor signal 結合，產生 engine-gimbal command。
- NASA fixed-wing navigation 範例把 heading correction 轉成 commanded bank angle，再交給
  bank-angle、pitch-rate、altitude 與 yaw controller。
- Space Shuttle DFCS 以 blending coefficient 把 pitch-rate feedback 與 normal-acceleration
  feedback 組合，並加入 steady-turn pitch compensation；這證明 longitudinal payload 可以
  同時具有慢速 maneuver objective 與快速 rate 項，但兩者角色必須固定。

這些資料支持：

1. AP 不應直接輸出 elevator／aileron／rudder demand。
2. AP 不應只把未整形的最終 altitude／heading target 丟給 inner-loop control laws。
3. AP／Guidance 應輸出隨時間更新、可捕獲且受 rate／acceleration 約束的 physical reference。
4. 同一 tick 的 vertical／lateral reference 必須合成一份完整結果，再做跨軸 coordination。
5. Control 負責快速穩定、feedback、protection、anti-windup 與 effector demand。

它們不能證明 F-CK-1C 採用 Shuttle 的 acceleration command、Saturn V 的 attitude command，
或任何相同 gain；本計畫只採用 Guidance／Control 分層與物理 Interface 原則。

來源：

- [NASA — Space Shuttle Entry Guidance](https://ntrs.nasa.gov/search.jsp?R=19790037248)
- [NASA — Space Shuttle Autoland Design](https://ntrs.nasa.gov/citations/19820056897)
- [NASA CR-134001 — Space Shuttle Digital Flight Control System](https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19730022103.pdf)
- [NASA — Saturn V Flight Manual SA-507](https://www.nasa.gov/wp-content/uploads/static/history/afj/ap12fj/pdf/a12_sa507-flightmanual.pdf)
- [NASA — GPS Auto-Navigation Design for Unmanned Air Vehicles](https://ntrs.nasa.gov/citations/20040000570)

### 3.5 Project-defined 的現況

本專案接受第 3.2–3.4 節的 Reference 作為架構與第一版行為的充分證據；採用後仍標記為
**Reference-derived project decision**，不得改寫成 Confirmed F-CK-1。下列內容仍保留
Project-defined 標籤：

- 現有 AP mode 名稱、engage 條件、reference step 與 controller gain。
- CAT I／CAT III 名稱及其控制律差異。
- 64 Hz FLCC rate 與 256 Hz actuation numerical integration rate。
- Reference 沒有給出的 capture gain、filter、failure-persistence、stick-steering threshold 與
  closed-loop tuning 數值。
- 現有 blended alpha／Nz／pitch-rate longitudinal law 的確切 blending schedule。
- F-16 Reference 未覆蓋的 limiter、gain schedule 與 actuator command normalization。

第一版 AP guidance envelope 採用 F-16 Reference 的 30° bank、20°/s roll rate，以及 ALT
HOLD +0.5 g 到 +2.0 g；這些是 AP 外迴路限制，不是整部 FBW 的 hard envelope。它們的
F-CK-1 適配性仍須經 closed-loop 與 DCS 實飛驗證，但不再是架構未決問題。

## 4. 固定的外部裝置邊界

```text
DCS Commands
    |
    v
PilotControls -- PilotControlSignal -------------------------+
                                                               |
AircraftSimulation -- FlightControlObservation ---------------+--> FlightControlComputer
                                                               |
FlightControlActuationSystem -- FlightControlActuatorState ----+
                                                                    |
                                                                    v
                                                  FlightControlActuatorCommand
                                                                    |
                                                                    v
                                                 FlightControlActuationSystem
```

### 4.1 `PilotControls`

`PilotControls` 是實際裝置責任的 System；它整合 DCS 軸、按鍵、set、toggle、press、
release 與虛擬軸。`PilotControlSignal` 是該 System 發布在「線上」的被動資料 snapshot。
兩者不是兩個 System，也不是同一概念的重複類別。

`PilotControls` 不把搖桿換算成 Nz、pitch rate、roll rate 或控制面角度；這是飛控電腦
依 mode 與 flight condition 做的工作。

### 4.2 `FlightControlComputer`

`FlightControlComputer` 是本次唯一的飛控機箱 System，負責：

- 接收、整理與驗證飛控所需輸入。
- 保存 AP／FBW mode、reference、filter、integrator 與 transition state。
- 執行手動 command law、AP 外迴路、內迴路、gain scheduling 與 protection。
- 選擇玩家或自動 reference 的 authority。
- 發布 normalized actuator authority command、診斷與座艙 snapshot。

它不擁有 actuator 的實際位置、airframe motion 或 engine plant 狀態。

### 4.3 `FlightControlActuationSystem`

此 System 擁有 servo lag、rate／position limit、saturation 與控制面實際狀態。FCC 只發布
actuator command，不得直接改寫實際 elevator／aileron／rudder position。

### 4.4 `AircraftSimulation`

這是 simulation layer，不是飛機裝置。它使用實際控制面狀態計算力與力矩，並回傳
flight-control observation。它不能呼叫 FCC 內部 Module。

### 4.5 Lua 邊界

Lua 可以送出座艙 command、顯示 C++ snapshot 及驅動可點擊座艙；不得保存 AP／FBW
權威狀態，也不得重新計算 reference、controller 或 limiter。

## 5. 目前邏輯鏈與問題

目前主要鏈為：

```text
PilotControlSignal --------------------------+
                                              v
AutomaticFlightControl -> normalized pitch/roll/throttle
                                              |
                                              v
                         overwrite FBWControllerInput.pitch_input/roll_input
                                              |
                                              v
                              update_fbw_controller(FBWControllerState)
                                              |
                                              v
                              FlightControlActuatorCommand
```

主要架構問題：

1. `AutomaticFlightControlDemand` 把 AP 結果降成 `pitch_normalized`／`roll_normalized`，
   使 AP 假裝成玩家搖桿，丟失 altitude、vertical speed、heading、bank、Nz 或 rate 等物理
   意義。
2. `apply_automatic_flight_control()` 直接覆寫玩家輸入，authority、manual override、
   transition 與 bumpless transfer 沒有自己的可測試 seam。
3. `AutomaticFlightControl` 同時負責 command queue、engage guards、mode state、reference
   capture、vertical controller、lateral controller、A/T、integrator 與 snapshot，變更理由太多。
4. `ControlLawTypes.h` 混合 configuration、production default、input、output、全部可變 state
   與診斷資料，讓任何修改都需要理解整個控制器。
5. `FlightControlComputer` 同時處理 System adapter、AP orchestration、pilot mapping、FBW
   execution、油門 composition 與 diagnostics。
6. 現有檔案雖尚未全部超過 700 行，但 `ControlLaws.cpp` 與
   `AutomaticFlightControl.cpp` 已集中多個不同責任，繼續擴充會破壞可讀性與測試隔離。
7. 現有 AP controller 的穩定性不佳；ALT Hold、vertical／heading reference 操作的問題不能
   只靠調一組 gain 解決，必須先確認 cascade、authority、anti-windup 與 plant response。

## 6. 目標 FlightControlComputer 內部架構

```text
FlightControlComputer (System / physical box boundary)
|
+-- System adapter
|   +-- setup declarations and command registration
|   +-- AircraftData read / publication
|   +-- one 64 Hz tick orchestration
|
+-- InputSignalManagement
|   +-- unit/sign convention and validity
|   +-- observation conditioning and named filtered signals
|   +-- pilot-controller signal conditioning
|
+-- Autopilot
|   +-- ModeLogic: engage, disconnect, capture, paddle bypass, pending commands
|   +-- ModeMonitor: tracking / constraint result -> degraded or release reason
|   +-- VerticalGuidance: target -> smoothed vertical physical reference
|   +-- LateralGuidance: target -> smoothed lateral physical reference
|   +-- one atomic AutomaticFlightGuidanceReference per FCC tick
|   +-- AutopilotState and AutopilotSnapshot
|
+-- FlightControlLaws
|   +-- ConfigurationAndMode: CAT / gear / flight-region schedule and ManeuverEnvelope
|   +-- PilotCommandLaw: controller signal -> physical maneuver command
|   +-- ControlReferenceSelection: pilot / AP authority and transitions
|   +-- GuidanceCoordination: simultaneous vertical / lateral / directional integration
|   +-- LongitudinalControlLaw
|   +-- LateralDirectionalControlLaw
|   +-- EnvelopeProtection and gain scheduling
|   +-- ControlLawState and ControlLawOutput
|
+-- ExperimentalAutoThrottleAssist
|   +-- existing compatibility behavior only; no new authenticity claim
|
+-- Diagnostics composition
```

這張圖表示責任，不要求每一行都建立一個 class。只有具有獨立狀態、規則、測試邊界或
明確變更理由的責任才建立 Module。不得建立只做一對一轉送的 façade。

### 6.1 `InputSignalManagement`

第一版只處理目前確實存在的單通道資料：

- 明確單位、軸向、符號與 availability。
- 把 raw observation 與 filter 後訊號分開命名。
- 統一 pilot signal conditioning，避免各控制律各自套 deadband／shape。
- 產生 immutable `ConditionedFlightControlInput` 給同一 tick 的其他 Module。

本次不假裝具有 triplex selector、voter 或 sensor failure masking。未來有真實需求時才新增。

### 6.2 `Autopilot`

AP 是 FCC 內的外迴路 Module，擁有：

- master、vertical、lateral mode 與 engage／disconnect reason。
- selected／captured target、reference adjustment 及 mode-transition state。
- paddle bypass、attitude-hold stick steering 與 tracking/degradation monitor state。
- target capture path、reference rate／acceleration shaping 與 mode transition。
- vertical／lateral controller 的積分器、filter 與 anti-windup。
- 物理單位的 `AutomaticFlightGuidanceReference`，不擁有 maneuver／actuator demand。

Vertical 與 lateral channel 可以分檔案／內部類別，但仍由一個 `Autopilot` Interface 對
FCC 提供完整結果。ModeLogic 先決定 active mode 和 captured reference；guidance channel
再依該結果計算，避免兩邊各自改 mode。兩條 channel 讀取同一個 conditioned state；完成後
一次組成 immutable result，不能因函式先後順序看見彼此的半成品。

第一版 Project-defined guidance Interface 固定為：

- Pitch Hold：AP 保存 `target_pitch_attitude_rad`，發布平滑的 pitch-attitude reference。
- Altitude Hold：AP 保存 `target_altitude_m`，但先經 capture law 轉成隨時間變化且受垂直
  acceleration 約束的 `vertical_speed_reference_mps`；不得把 raw altitude target 直接交給
  inner loop。
- Heading Select：AP 保存 0–359 的整數 `target_heading_deg`；只有 guidance 計算時轉成
  radians，再經 wrap-aware capture law、bank 與 roll-rate shaping，發布
  `bank_angle_reference_rad`。

航空語意的 Heading 使用 cockpit `getMagneticHeading()` 形成的 typed observation。
DCS body-kinematics yaw 只叫 `world_yaw_rad`，保留給模擬姿態／物理與 state CSV，不能當成
AP Heading。磁航向缺失時 `HDG SEL` 必須明確拒絕接通或釋放 lateral channel，不得靜默
改用 world yaw。因找不到已確認的 F-CK-1C 採樣率，Lua Adapter 暫採 Project-defined 64 Hz。

上述選擇是民航／航太 Guidance 分層與現有模式共同支持的 Project-defined Interface，不是
已確認的 F-CK-1C command law。未來取得真機資料時，可更換 AP 內部 target-to-reference
Implementation，而不改變 System seam。

### 6.3 `FlightControlLaws`

此 deep Module 同時接收：

- conditioned observation。
- conditioned pilot-controller signals。
- AP 的 physical reference 與 engagement state。
- actuator feedback。
- configuration／mode selection。

它負責把來源轉成 shared inner-loop reference，再經 limiter、feedback controller 與 output
composition 產生 `FlightControlActuatorCommand`。AP 不得繞過 protection；玩家與 AP 都受
同一組真實性層級相同的 limiter／gain schedule 約束。

### 6.4 `ControlReferenceSelection`

這是有實際行為的 seam，不是通用 message bus。它負責：

- 依 `ModeLogic` 的明確 authority decision，選擇 longitudinal、lateral 與 directional 的
  manual／automatic source。
- 執行 engage、disconnect、paddle bypass 與 attitude stick steering 的 source transition。
- 以 reference tracking／recapture 完成 bumpless transfer，避免切換瞬間出現不連續 command。
- 記錄 active source，供 diagnostics 與測試使用。

人工接管分成三種不同事件，不使用一個含義模糊的 `override` boolean：

1. **AP Disconnect**：所有 AP mode 永久解除，必須重新下 engage command 才能恢復。
2. **Paddle Bypass**：明確的 momentary command；按住時全部 AP axis 暫時交回 manual，
   mode selection 保留，放開時依 mode 的 target rule 恢復。它不依搖桿位移或姿態變化 threshold。
3. **Attitude Stick Steering**：只在支援的 attitude-hold mode 逐軸生效。第一版只有 Pitch
   Hold 支援 longitudinal stick steering；ALT Hold、Heading Select 不會因一般
   stick input 暗中釋放 AP。未來若新增 Roll Attitude Hold，才新增相同的 lateral 規則。

此處「全部 AP axis」只指 flight-control longitudinal／lateral／directional authority；
`ExperimentalAutoThrottleAssist` 的 bypass／takeover 政策仍依第 9.7、15.6 節延後，不在本次
順便改寫。

第一版 paddle release target rule 固定如下：

| Mode | Paddle 放開後的行為 |
|---|---|
| Pitch Hold | capture 當前 pitch attitude |
| Altitude Hold | capture 當前 altitude |
| Heading Select | 保留原 selected heading |

每次 paddle release 都執行表中的規則，不保留現有任意的「姿態改變夠大才 recapture」判斷。
Stick steering 的啟動使用 conditioned input 後明確、可測試的 activation threshold；該 threshold
只用來排除軸噪聲，不改變 paddle 行為，也不得套用到其他 AP mode。

`ControlReferenceSelection` 不判斷 AP 是否可 engage、不保存 selected target，也不直接產生
控制面需求。前兩項由 `Autopilot::ModeLogic` 擁有；它完成後只形成包含各軸 active source 與
物理 reference 的 immutable `SelectedFlightReference`。Interface 第一版是一個純 `select()`
行為，不建立 strategy hierarchy、port 或只有轉送作用的 class。

### 6.5 `GuidanceCoordination`

這是 FCC 內部、位於 source selection 與 inner-loop control laws 之間的 seam。它在同一
FCC tick 讀取完整 `SelectedFlightReference`、conditioned flight state、actuator feedback 與
當前 `ManeuverEnvelope`，輸出一份 immutable `CoordinatedManeuverReference`：

```cpp
struct LongitudinalManeuverReference {
    double normal_acceleration_reference_g;
    double pitch_rate_feedforward_rad_s;
};

struct LateralDirectionalManeuverReference {
    double roll_rate_reference_rad_s;
    double sideslip_reference_rad;
    double yaw_rate_feedforward_rad_s;
};

struct CoordinatedManeuverReference {
    LongitudinalManeuverReference longitudinal;
    LateralDirectionalManeuverReference lateral_directional;
};
```

這是固定 schema，不是 optional field bag：

- `normal_acceleration_reference_g` 是 vertical-path／load-factor feedback objective；只有 selected
  vertical source 要維持 flight path 時，才在此加入 bank-to-lift compensation。`_g` 表示 load
  factor，正負方向由唯一的軸向規範定義。
- `pitch_rate_feedforward_rad_s` 只表達 attitude capture／快速預期項，不是第二套互相競爭的
  longitudinal authority。
- `roll_rate_reference_rad_s` 是 bank capture 的 inner-loop reference。
- `sideslip_reference_rad` 是 directional feedback objective；AP coordinated turn 第一版為 0，
  但 manual rudder command 仍經自己的 directional authority 路徑合成，不能被 AP 清零。
- `yaw_rate_feedforward_rad_s` 是 turn coordination 的預期項，不取代 sideslip feedback。

`LongitudinalControlLaw` 把 Nz feedback 項與 q feed-forward 合成唯一 q reference，再經 q limit、
inner rate loop 與 actuator feedback；alpha／G protection 最後仍能限制或取代不安全 command。
同一 selected reference 的 bank compensation 只在 coordination 做一次，control law 不得再次補償。

第一版至少負責：

- 同時處理 vertical、lateral、directional reference，不依賴 channel 呼叫順序。
- 建立 bank 對垂直 lift component 的補償需求。
- 建立 coordinated-turn 的 yaw／sideslip 需求。
- 把 mode-specific attitude／path reference 轉成 inner loop 可追蹤的 maneuver reference。
- 在 G／AoA／rate／actuator authority 不足時發布 constraint／degradation diagnostics。

`ConfigurationAndMode` 由同一組 immutable config、flight condition 與 actuation feedback 計算
一份 `ManeuverEnvelope`。其中明確分開 AP guidance envelope 與 FBW hard-protection envelope：
前者永遠位於後者之內；`GuidanceCoordination` 使用前者求可行 reference，`EnvelopeProtection`
使用後者做最後強制限制。兩者讀同一份 limit source，不互相呼叫，也不複製 magic number。

vertical／lateral request 的規則不是固定選一軸優先，而是解同一份可行性問題：先由完整 snapshot
計算 vertical load demand 與 lateral bank demand，再依可用 Nz、G／AoA margin、rate 與 actuator
authority 限制可用 bank。30° level coordinated turn 約需 `1 / cos(30°) = 1.155 g`，因此在
F-16 Reference 的 +2.0 g ALT envelope 內，正常 altitude＋heading request 應可同時追蹤；這是
由公開 limit 做出的工程推論，不是手冊對設計意圖的原文敘述。

若組合 request 超出 envelope：

1. 先產生最接近目標且可行的 combined reference；heading capture 因剩餘 lift 不足而放慢時，
   發布 `LateralConstrainedByVerticalAuthority` 等具體 reason。
2. hard protection 在當 tick 仍具有最高權限，不能為了保持任一 AP mode 而繞過。
3. 若即使 zero-bank 也不能維持 vertical request，發布 `VerticalReferenceUnmaintainable`；
   `ModeMonitor` 依明確 persistence 規則解除受影響 vertical mode，而不是讓 integrator 永久 windup。
4. transient constraint 只限制 reference 並保留 mode；sustained tracking failure 才進入 degraded
   並解除受影響 channel。explicit disconnect 或 safety guard 才關閉全部 AP。
5. 所有 constraint、degraded、release 與 disconnect 都必須有 reason code；不得 silent fallback。

`GuidanceCoordination` 本身保持純計算、無 persistent state。它不保存 AP target、不決定 AP
mode，也不直接輸出 actuator demand；constraint persistence 由 `Autopilot::ModeMonitor` 擁有。
它具有真實跨軸行為，因此 seam 已成立，但不需要 virtual base、port、Adapter 或可替換策略。

### 6.6 為何目前不建立 `FlightControlExecutive` class

NASA 報告中的 Executive 有真實責任：排程 control-law tasks、管理 cyclic completion，並與
startup／restart、monitoring 等軟體互動。現在整部 FCC 只有一個 64 Hz System tick，外部
排程已由 `SystemPipeline` 負責。如果此時新增 `FlightControlExecutive`，它只會把
`FlightControlComputer::step()` 搬到另一個檔案，沒有形成更深的 Module。

因此本次由 `FlightControlComputer::step()` 依固定順序協調內部 Module。只有未來真的出現
以下任一需求才抽出 Executive：FCC 內部多更新率、startup／restart state machine、task
deadline／health monitoring，或可重排的 operational program schedule。

### 6.7 單一 FCC tick 的固定順序

每個 64 Hz tick 只使用一份 input snapshot，依下列順序執行：

1. `InputSignalManagement` 建立 conditioned input，並檢查 non-finite／availability。
2. `ConfigurationAndMode` 建立本 tick 的 schedule 與 `ManeuverEnvelope`。
3. `ModeLogic` 讀取該 envelope，套用 pending command 與上一 tick 的 monitor result，決定 mode、
   target 與 authority。
4. `Autopilot` 的 vertical／lateral guidance 由相同 state 產生一份 atomic result。
5. `PilotCommandLaw` 由 conditioned controller signal 產生 manual physical reference。
6. `ControlReferenceSelection` 逐軸選擇來源，輸出一份 atomic selected reference。
7. `GuidanceCoordination` 一次求出完整 maneuver reference 與 constraint diagnostics。
8. longitudinal 與 lateral/directional control laws 由同一 maneuver reference 計算。
9. `EnvelopeProtection` 與 output composition 產生 actuator command。
10. 組成 diagnostics／snapshot，並保存本 tick monitor observation 供下一 tick 的 `ModeLogic` 使用。

不在同一 tick 內因 constraint 回頭改 mode、再重算 guidance；這會形成隱藏的 algebraic loop。
hard protection 當 tick 生效，mode degradation 最多延後一個 FCC tick（15.625 ms），因果關係
固定且可測試。

### 6.8 SOLID 與 dependency 規則

- **SRP**：ModeLogic、Guidance、Reference Selection、Coordination、inner loops 與 Protection 各自
  只有第 6 節定義的一種變更理由；state owner 依第 8 節固定。
- **OCP**：新增 AP mode 時擴充 `Autopilot` 內部 mode／guidance mapping，不修改 System catalog、
  actuator seam 或既有 maneuver schema；但沒有第二種 implementation 前不建立 strategy factory。
- **LSP**：本次沒有合理的 Module inheritance hierarchy，因此不為形式上符合原則而建立 base
  class；未使用繼承就沒有虛假的可替換承諾。
- **ISP**：FCC 只看 `Autopilot` 的 atomic guidance result 與 `FlightControlLaws` 的 actuator result；
  private integrator、filter、monitor timer 與 implementation helper 不進 public header。
- **DIP**：控制演算法依賴 typed immutable input／config，不直接讀 DCS、Lua 或其他 concrete
  System。`FlightControlComputer` 可用 value member 擁有同一機箱內 Module，但不得在業務計算中
  自行尋找全域 service 或建立外部 System；測試以 config、input snapshot 與 actuator feedback
  注入依賴。

這些規則的目的不是增加 class 數量，而是讓 dependency 只朝資料 contract 與機箱內 deep
Interface 前進。若一個拆分只增加轉送函式、沒有隱藏複雜度，就不建立該拆分。

## 7. 訊號語彙與邏輯鏈

### 7.1 外部「電線」

| 訊號 | Publisher | Consumer | 語意 |
|---|---|---|---|
| `PilotControlSignal` | `PilotControls` | FCC | 玩家控制器當前位置／trim；normalized 是硬體 authority，不是飛機 command |
| `ThrottleLeverSignal` | `PilotControls` | FCC／Engine boundary | 左右油門桿等效位置 |
| `FlightControlObservation` | Simulation | FCC | 機體姿態、角速率、空速、Mach、AoA、Nz 等觀測 |
| `FlightControlActuatorState` | Actuation System | FCC／Simulation | 實際控制面位置、rate、saturation |
| AP／FBW `Command` | DCS adapter | FCC handler | 離散操作事件；由下一個 FCC tick 套用 |
| `FlightControlActuatorCommand` | FCC | Actuation System | normalized actuator authority demand |
| FCC／AP Snapshot | FCC | Lua／diagnostics | 唯讀呈現與測試資料，不是控制回授 |

### 7.2 內部 command 層級

不要建立只有 `pitch/roll/yaw [-1,1]` 的通用 `ControlIntent`。內部保留以下不同語意：

1. **Selected／captured target**：altitude、heading、vertical speed、pitch attitude 等 AP 私有
   mode state；不直接跨越 Guidance／Control seam。
2. **Controller signal**：玩家 stick／pedal 的硬體 authority。
3. **Flight-guidance reference**：AP 把 target 轉成隨時間變化的 physical path／attitude
   reference；第一版包含 mode-specific pitch-attitude／vertical-speed 與 bank-angle reference。
4. **Maneuver reference**：coordination 後供 inner loop 追蹤的固定 schema；longitudinal 為
   `normal_acceleration_reference_g` 加 `pitch_rate_feedforward_rad_s`，lateral/directional 為
   `roll_rate_reference_rad_s`、`sideslip_reference_rad` 加 `yaw_rate_feedforward_rad_s`。
5. **Effector demand**：控制律送往 elevator／aileron／rudder actuator 的 normalized authority。

每個型別只包含該層級真正需要的欄位，並以 `_rad`、`_rad_s`、`_mps`、`_g`、`_pa` 等
suffix 表明單位。不得使用無單位的 `pitch`、`roll`、`g` 或 `speed_scalar`。

同一 FCC 內這些是 in-process immutable value Interface，不是 AircraftData、queue 或外部
transport protocol；它們全部使用同一 scheduled time、`dt_s` 與 conditioned input snapshot。

### 7.3 手動駕駛鏈

```text
DCS axis/button
-> PilotControls command integration
-> PilotControlSignal
-> InputSignalManagement
-> PilotCommandLaw (mode-dependent physical command)
-> ControlReferenceSelection
-> GuidanceCoordination with all selected axes
-> longitudinal / lateral-directional inner loops
-> protection and output composition
-> FlightControlActuatorCommand
```

### 7.4 自動駕駛鏈

```text
DCS cockpit command
-> FCC pending command queue
-> next FCC tick: ModeLogic validates / captures reference
-> VerticalGuidance and LateralGuidance shape target capture paths
-> one atomic AutomaticFlightGuidanceReference
-> ControlReferenceSelection
-> GuidanceCoordination combines all selected axes and current flight state
-> CoordinatedManeuverReference
-> same inner loops and protection used by manual flight
-> FlightControlActuatorCommand
```

### 7.5 關閉與人工接管鏈

三種事件有不同狀態轉移，不共用模糊的自動 threshold：

```text
AP Disconnect command / hard safety condition
-> ModeLogic clears all AP modes and records reason
-> ControlReferenceSelection selects manual sources
-> AP does not resume without a new engage command

Paddle pressed
-> ModeLogic keeps mode selection but sets all-axis bypass
-> ControlReferenceSelection selects manual sources for all axes
-> AP integrators track the current controlled state instead of winding up
Paddle released
-> ModeLogic applies the per-mode capture / retain table
-> ControlReferenceSelection restores automatic sources

Conditioned pitch input crosses stick-steering threshold while Pitch Hold is active
-> only longitudinal authority becomes manual
-> Pitch Hold remains selected and target tracks current pitch attitude
Input returns below threshold
-> capture current pitch and restore automatic longitudinal source
```

VS／ALT／Heading mode 的一般 stick movement 不會自行觸發最後一條鏈；要人工接管必須使用
paddle 或 disconnect。任一鏈完成後，`GuidanceCoordination` 都從完整 selected-axis snapshot
重新計算，不沿用半套舊 maneuver command。

## 8. 狀態與設定 ownership

| 狀態 | 唯一 owner |
|---|---|
| DCS 軸、按鍵保持與虛擬軸 | `PilotControls` |
| AP pending commands、mode、authority、selected/captured target、paddle／stick-steering state、guidance state、integrators | `Autopilot` |
| tracking／constraint persistence timer、degraded／release reason | `Autopilot::ModeMonitor` |
| filtered flight-control inputs | `InputSignalManagement` |
| 本 tick manual／automatic active-source result | `ControlReferenceSelection` 純計算；不保存跨 tick state |
| 本 tick schedule、AP guidance envelope 與 hard-protection envelope | `ConfigurationAndMode` 的 immutable result |
| cross-axis constraint diagnostics | `GuidanceCoordination` 的純計算 result；不保存跨 tick state |
| longitudinal law integrators／filters／limit state | `LongitudinalControlLaw` |
| lateral／directional integrators／filters／limit state | `LateralDirectionalControlLaw` |
| actuator position／rate／saturation | `FlightControlActuationSystem` |
| airframe kinematics／dynamics | `AircraftSimulation` |
| cockpit presentation | Lua parameters／draw arguments，來源仍是 C++ snapshot |

規則：

- configuration 在建構後 immutable；不得在 step 中修改 gain table。
- 每個 Module 私有保存自己的 state，不把全部狀態暴露成可任意修改的 shared struct。
- Module 之間傳 immutable input/result；不得互相取得可變 state reference。
- Snapshot 是由權威 state 組成的唯讀投影，不得被下一個 tick 讀回成控制狀態。
- 設定檔可使用 degree 方便編輯，但建立 production config 時只轉換一次；內部運算使用 SI。
- Developer-only G-limiter override 保持預設 unavailable，availability 與 active 分開發布。

## 9. AP 功能政策

### 9.1 現有模式

現有 master、pitch hold、vertical-speed hold、altitude hold、heading hold、heading select、
reference increase／decrease 與 bypass 先保留，身分全部是 **Project-defined compatibility**。
重構前先以測試記錄 command、狀態轉移與 snapshot；不得把目前不穩定的控制輸出凍結成
正確規格。

### 9.2 AP 與 FBW 的責任分界

AP 負責回答「要往哪裡／維持什麼」；FBW control laws 負責回答「在限制內如何讓飛機
做到」。例如 altitude hold 可形成 vertical／pitch guidance，但不能直接決定 elevator
authority。Heading hold 可形成 bank／roll guidance，但不能直接決定 aileron authority。

AP 也不能只發布 raw target。它必須把 target 轉成可逐 tick 追蹤的 physical reference；
例如 altitude capture 先依目前垂直速度、剩餘高度與允許垂直加速度形成平滑的
vertical-speed reference，heading capture 則形成受 bank／roll-rate 限制的 bank reference。

### 9.3 三層平滑責任

平滑不是單一 Module 的責任，但每一層只處理自己的時間尺度：

1. `Autopilot`：target capture path、reference rate／acceleration、armed/capture/hold transition。
2. `ControlReferenceSelection`／`GuidanceCoordination`：manual/AP bumpless transfer、同 tick
   cross-axis consistency、bank-to-lift compensation 與 constraint handling。
3. `FlightControlLaws`／`FlightControlActuationSystem`：closed-loop damping、anti-windup、
   command／actuator rate、position limit 與實際 servo dynamics。

因此不得用 actuator lag 掩蓋突變 guidance，也不得把 altitude／heading capture 規則塞進
inner-loop controller。

### 9.4 Reference-derived AP envelope 與失效政策

第一版把 F-16 flight manual 的數值放在具證據標籤的 immutable AP config：

- Heading／Steering 類 guidance：`bank_angle_limit_rad = 30°`。
- bank capture：`roll_rate_limit_rad_s = 20°/s`。
- ALT guidance 的 normal-acceleration reference：`0.5 g` 到 `2.0 g`。

這些只限制 AP guidance，不縮小玩家手動 FBW 的完整 hard envelope。建構 config 時必須驗證
AP envelope 位於 hard-protection envelope 內；不相容時直接報錯，不自動改小或採 default。

FAA flight-guidance 規範要求上游 steering command 與下游 pitch／bank limit 相容；若
protection 必須壓過 guidance，系統必須有可見的 mode reversion、degradation 或 disengagement。
F-16 Reference 也明確監控無法維持 selected mode 或長時間超出 AP operation range 的狀況。
本專案因此固定四類結果：

1. **Engage rejected**：進入條件不合法，mode 不改變並發布 reason。
2. **Transient constrained**：本 tick command 被可行性限制，mode 保留並發布 reason。
3. **Sustained tracking failure**：超過明確 persistence 後標記 degraded，解除失敗的 vertical
   或 lateral channel；其他仍可維持的 channel 不被無故關閉。
4. **Hard disconnect**：explicit disconnect、必要安全條件或必要 input 無效時，解除全部 AP。

`tracking_failure_persistence_s` 與 error threshold 是 Project-defined tuning config，必須由 Phase 0
baseline、closed-loop test 與 DCS 實飛決定；數值尚待整定不會改變上述狀態機，也不阻擋施工。

### 9.5 人工接管政策

第一版完整採用第 6.4、7.5 節的三分法：AP Disconnect、all-axis Paddle Bypass、僅適用
attitude-hold 的 per-axis Stick Steering。F-16 的 PITCH switch 會一起管理選定 pitch／roll AP
mode，但本專案不為複製面板行為而合併既有 command；snapshot 必須分別顯示 vertical 與
lateral active／degraded state，避免使用者看不出是哪個 axis 仍在控制。

### 9.6 CAT 與 protection

CAT I／III 暫時是 F-16-derived provisional model，屬於 `FlightControlLaws` 的 configuration
與 gain/limit scheduling，不屬於 `Autopilot`。AP 和玩家命令都必須經過同一個 CAT 與
envelope protection 路徑。

### 9.7 Auto-throttle

Auto-throttle 不參與本次 AP／飛控數學重建。因為現有程式碼與 AP 類別耦合，本次只把它
移至明確標示的 `ExperimentalAutoThrottleAssist` Module，維持既有 command、state、輸出與
測試，不宣稱是真機功能，也不讓其設定混入 vertical／lateral AP config。

最終 placement、擬真模式是否提供、availability switch、manual takeover 與 Engine seam
另立計畫決策；本次不得偷偷改變。

## 10. 更新率、時間與數值規則

- FCC 繼續由 `SystemPipeline` 以 64 Hz 執行；標籤是 F-16XL Reference。
- `FlightControlActuationSystem` 維持 256 Hz；標籤是 Project-defined numerical rate。
- 同一 FCC tick 的所有內部 Module 使用同一 `SystemStepContext::dt_s` 與同一 immutable
  AircraftData snapshot。
- 不在 FCC 內另建 scheduler、queue、clock domain 或隨機 phase。
- command handler 只 queue／latch 意圖；依賴 observation 的 engage、capture 與 mode
  transition 在下一個 FCC tick 發生。
- 所有 filter、integrator、rate limiter 與 transition 都顯式使用 `dt_s`。
- angle difference 使用 wrap-aware helper；內部以 radians 比較，避免 359°／1° 產生 358°
  錯誤。
- 不以 host `FrameInput::dt_s` 積分控制器。

## 11. 目標檔案結構

```text
Core/Systems/FlightControlComputer/
|-- Entry.cpp
|-- FlightControlComputer.h
|-- FlightControlComputer.cpp
|-- FlightControlComputerConfig.h
|-- FlightControlComputerConfig.cpp
|-- FlightControlExecutive.h/.cpp
|-- FlightControlCommandSystem.h/.cpp
|-- FlightControlOutputSystem.h/.cpp
|-- FlightControlDiagnostics.h/.cpp
|-- FlightStateComputation.h/.cpp
|-- InputSignalManagement.h
|-- InputSignalManagement.cpp
|-- CommandSystem/
|   |-- AutomaticFlightControlTypes.h
|   |-- AutomaticFlightControlConfig.cpp
|   |-- ExperimentalAutoThrottleAssistConfig.h
|   `-- Internal/
|       |-- PilotCommandLaw.cpp
|       |-- ControlReferenceSelection.cpp
|       |-- GuidanceCoordination.cpp
|       `-- Autopilot/
|           |-- AutomaticFlightControl.cpp
|           |-- AutopilotModeLogic.cpp
|           |-- AutopilotModeMonitor.cpp
|           |-- VerticalGuidance.cpp
|           `-- LateralGuidance.cpp
|-- ControlLaws/
|   |-- ControlLaws.h
|   |-- ControlLaws.cpp
|   |-- ControlLawConfig.h
|   |-- ControlLawSignals.h
|   |-- ControlLawState.h
|   `-- Internal/
|       |-- ControlLawMath.h
|       `-- InnerLoopControl.cpp
`-- ModeAndGainScheduling/
    |-- ModeAndGainScheduling.h/.cpp
    `-- Internal/ModeAndGainMath.h
```

這是目標責任拓樸，不要求一次建立全部空檔案。施工時只在對應邏輯被搬入時建立檔案。
`CommandSystem/Internal/` 與 `ControlLaws/Internal/` 是各自深 Module 的私有實作樹，不能被
FCC adapter、其他 Module 或測試直接依賴，也不得變成所有內部 state 都能互改的 shared
header。`FlightControlCommandSystem` 使用 PImpl 持有其 command／AFCS implementation，目的
是從編譯期強制隱藏 `CommandSystem/Internal/Autopilot`，讓 production caller 與測試都只使用
public facade。這個 PImpl 是邊界機制，不是為每個小型 Module 統一引入 heap allocation。
`FlightControlComputer.h` 的 public method、parameter 與 return type 仍不得暴露任何內部
controller state。

### 11.1 Build 前置修改

目前 `EfmCore.props` 的 `Systems\*\*.cpp`／`.h` 只涵蓋 System 目錄下一層。採用上述巢狀
Module 後必須：

- 讓 System source/header recursive include 子目錄。
- 只把 `Core/Systems/*/Entry.cpp` 當 concrete System catalog entry；內部子目錄不得生成
  System entry。
- `ObjectFileName` 必須包含足夠的相對路徑資訊，避免不同子目錄同名 `.cpp` 產生 `.obj`
  collision。
- native tests 與 production DLL 使用相同 source discovery 規則。
- architecture check 仍以 `FlightControlComputer` 第一層目錄作為 owner，內部 Module 不得
  越過 System boundary include 其他 concrete System。

## 12. 實作順序與 commit 邊界

每一階段都必須能 build、通過當階段測試並形成獨立 commit；不得用長期 compatibility
fallback 隱藏未完成遷移。Phase 7 是唯一必須原子完成的 seam 變更：開發期間可以分小步修改，
但 normalized 與 physical AP path 不得以兩條 production path 的形式留在任何完成 commit。

### Phase 0 — 建立行為與資料 baseline

目的：先知道目前程式碼真的做了什麼，區分「必須保留的功能」與「要修正的缺陷」。

工作：

- 固定重構起點 commit、native build／test／architecture-check 指令與 DCS install 步驟。
- 列出所有 FCC／AP command、mode、snapshot field、AircraftData read/publication 及其 owner。
- 為 engage／reject／disconnect、selected/captured target、reference adjust、bypass、manual FBW、
  CAT、G-limiter override、actuator command 與 A/T compatibility 建 characterization tests。
- 保存相同 scenario 的 input、mode、normalized AP demand、actuator command、actuator feedback、
  flight response 與 saturation CSV；baseline 不能只記 snapshot boolean。
- 把功能清單與已知缺陷分開：命令可用、模式可進入屬於保留項；ALT Hold 擺盪、reference
  反應不佳、任意 recapture threshold 不是正確性 contract。
- 測試資料或必要欄位缺失時直接失敗，不補假值、不吞 non-finite、不產生 mock success。

完成條件：baseline 可重複執行；每個現有 mode、command 與 protection path 都有對應證據；
每個已知缺陷有 failing test 或明確 expected-defect 記錄。

### Phase 1 — 準備巢狀 Module build 與純檔案拆分

目的：建立可持續擴充的檔案拓樸，不改變控制行為。

工作：

- 修改 recursive source discovery、Entry 過濾與 object path；production DLL 和 native tests 使用
  相同規則。
- 先搬移 `AutomaticFlightControl*`、`ControlLaw*` 與 config 至目標子目錄；只做 include／namespace
  整理，不改演算法、gain、狀態生命週期或 command routing。
- 驗證只有第一層 `Entry.cpp` 進入 System catalog，內部 Module 不生成 System entry。
- 執行 production/native source manifest、duplicate-object 與 architecture tests。

完成條件：build 產物包含每個預期 source、沒有 duplicate object、輸出與 Phase 0 相同。

### Phase 2 — 整理 typed signals、configuration 與 state ownership

目的：先把資料語意變清楚，再改控制鏈。

工作：

- 將 `ControlLawTypes.h` 拆成 config、input/result、private state 與 diagnostics。
- 將無單位欄位改為有 suffix 的物理量；config 中 degree 只在建構時轉 SI。
- 先建立本 Phase 立即使用的互不相容 target、controller signal、normalized legacy demand、
  actuator demand 與 control-law result 型別；不得用同一 struct 跨層重解讀。
- 第 6.5 節已固定 final physical schema，但 `AutomaticFlightGuidanceReference`、
  `SelectedFlightReference`、`CoordinatedManeuverReference` 與 `ManeuverEnvelope` 只在 Phase 6／7
  實際接入 production path 時建立，不預先留下未使用型別。
- 建立 typed `AuthorityState`、`ConstraintReason`、`DegradationReason` 與 `DisconnectReason`；
  不使用一個 boolean 同時表示 reject、constrained、degraded 與 disconnected。
- `FlightControlComputerStepInput` 只保留真正在 System seam 需要的資料，不暴露 FBW
  controller state。
- 確保 snapshot 不能成為控制輸入。

完成條件：編譯器能阻止跨層混用 controller signal、legacy AP demand 與 actuator demand；
演算法輸出仍維持 baseline，且沒有未使用的 final-seam placeholder。

### Phase 3 — 建立 `InputSignalManagement`

目的：讓每個控制律讀到同一份、同單位、同 tick 的 conditioned input。

工作：

- 從 `FlightControlComputer::make_pipeline_input()` 與 control-law 大型 state 移出 raw mapping、
  availability、sign/unit conversion、angle wrapping 與共用 filtering。
- 明確區分 raw、conditioned、filtered signal。
- 定義 Nz、AoA、body rate、bank、vertical speed、heading 與 actuator feedback 的唯一軸向／符號；
  unit test 必須覆蓋 level-flight、左右轉與升降的方向。
- pilot deadband／shape 只執行一次，stick-steering threshold 讀 conditioned signal。
- 保留 single-channel 行為；不加入 voter 或假故障 fallback。

完成條件：手動 FBW 輸出在容差內維持 baseline；每個 filter 具 unit test 與 `dt` 測試。

### Phase 4 — 拆分 AP mode、vertical、lateral 與 experimental A/T

目的：降低 `AutomaticFlightControl` 的變更耦合，仍暫時維持現有 AP-to-FBW seam。

工作：

- ModeLogic 唯一擁有 pending commands、engage/disconnect、capture、bypass 與 reason。
- VerticalGuidance 唯一擁有 vertical controller state。
- LateralGuidance 唯一擁有 lateral controller state。
- A/T config/state/controller 移至 `ExperimentalAutoThrottleAssist`，行為不變。
- `Autopilot` 對外只提供一份 immutable result 與 snapshot。
- 先把 selected/captured target 與 controller output 分開命名；此 Phase 不提前改變現有
  normalized AP-to-FBW production path。

完成條件：模式與 command characterization tests 全部通過；A/T 關閉／啟用行為未被改變；
vertical/lateral/A/T 測試可以各自建立，不需建整個 FCC。

### Phase 5 — 固定 mode、paddle 與 stick-steering 語意

目的：先把人工接管與 target capture 做成明確狀態機，避免 seam 遷移時把 authority 問題混入
控制律數學。

工作：

- 把 AP Disconnect、Paddle Bypass、Attitude Stick Steering 寫成三個不同 event／state transition。
- Paddle 按住時全部 axis manual、mode 保留；放開時無條件執行第 6.4 節 mode target table，移除
  現有姿態變化 threshold。
- 第一版 stick steering 只支援 Pitch Hold longitudinal axis；使用 post-conditioning activation
  threshold，release 時 capture 當前 pitch。其他 mode 不因一般 stick input 自動 bypass。
- AP integrator 在 bypass／stick steering 時採 tracking 或明確 reset 規則，不在沒有 authority 時
  繼續積分。
- snapshot／diagnostics 分開顯示 selected mode、active authority、bypass、stick steering 與 reason。
- 本 Phase 仍使用舊 normalized AP-to-FBW seam，只改 authority／capture 語意；baseline 中被明確
  列為缺陷的 recapture 行為允許更新，其他 AP output 保持。

完成條件：三種人工接管的 tests 全部通過；paddle release table 每個 mode 都有測試；Heading
Select 不被 recapture；VS／ALT／Heading 的一般 stick movement 不會暗中解除 AP。

### Phase 6 — 深化 FlightControlLaws 並建立單一 envelope source

目的：先整理 physical seam 的 consumer，保持手動飛行與既有 normalized AP 行為，再做原子遷移。

工作：

- 依實際責任拆分現有 `ControlLaws.cpp`，不建立一對一 forwarding class。
- 將 pilot mapping、alpha／Nz／q blending、p／q／r inner-loop feedback、gain scheduling、limiter、
  anti-windup 與 output composition 的順序寫成 code-level contract。
- 建立並立即接入 `ManeuverEnvelope`；`ConfigurationAndMode` 從一份 immutable config 產生它，AP guidance
  envelope 與 hard-protection envelope 欄位分開，建構時驗證前者包含於後者。
- 寫入 F-16 Reference 的 30° bank、20°/s roll rate、ALT 0.5–2.0 g；其餘現有值保留證據標籤。
- CAT provisional model 只存在 configuration/scheduling responsibility。
- G-limiter override 仍是 Developer-only availability，不成為正常控制模式。
- 移除舊 `FBWControllerState` 中已由 Module 私有 state 取代的欄位。

完成條件：每個軸、schedule 與 protection 可單獨測試；同一輸入只在一處 filter／limit；
guidance／hard limit 不複製 magic number；Phase 0 手動飛行與 AP output 在容差內維持。

### Phase 7 — 原子移除 normalized AP stick seam

目的：建立真正的 AP outer loop、reference authority 與共用 FBW inner loop。

工作：

- AP vertical／lateral channel 由 selected target 產生同 tick immutable physical reference：
  Pitch Hold -> pitch attitude、ALT Hold -> 經 capture law 的 vertical speed、Heading Select ->
  經 capture law 的 bank angle。
- 建立並在同一變更接入 `AutomaticFlightGuidanceReference`、`SelectedFlightReference` 與
  `CoordinatedManeuverReference`；不得先提交未使用的 final-seam type。
- `PilotCommandLaw` 把 controller signal 轉成 mode-dependent physical manual reference，保留現有
  alpha／Nz／q 與 lateral/directional command-law 意圖。
- `ControlReferenceSelection` 依 Phase 5 authority state 逐軸選擇來源，一次輸出完整
  `SelectedFlightReference`。
- `GuidanceCoordination` 一次計算 Nz objective＋q feed-forward、p reference、beta objective＋r
  feed-forward，並完成 bank-to-lift compensation 與 turn coordination。
- 使用 `ManeuverEnvelope` 解 combined feasibility；正常 30° bank＋ALT case 同時滿足，超限時
  發布具體 constraint reason，zero-bank 仍不可行時發布 `VerticalReferenceUnmaintainable`。
- longitudinal law 只在一處把 Nz feedback 與 q feed-forward 合成 q reference；lateral/directional
  law 同樣只在一處合成 p、beta 與 r 項；禁止 double compensation。
- 玩家與 AP 都進入同一 CAT schedule、inner loop、protection 與 actuator feedback path。
- 同一 commit 刪除 `AutomaticFlightControlDemand::pitch_normalized`、`roll_normalized`、
  `apply_automatic_flight_control()` overwrite path 及所有 obsolete adapter tests。
- 以新的 deep Interface behavior tests 取代直接測舊內部 normalized output 的 shallow tests。

完成條件：production 中只有 physical-reference path；編譯器無法把 controller signal 當 AP
reference；active source、guidance、selected、maneuver、constraint 與 effector demand 都可診斷；
交換 vertical／lateral 計算的 implementation 順序不改變結果。

### Phase 8 — 完成 tracking monitor、degradation 與失效鏈

目的：讓 constraint／protection 的結果能明確影響 AP mode，但不形成同 tick feedback loop。

工作：

- `ModeMonitor` 在本 tick 結束保存 tracking error、constraint、saturation 與 protection result，
  下一個 FCC tick 才交給 `ModeLogic`。
- 實作 engage rejected、transient constrained、sustained tracking failure 與 hard disconnect 四類
  transition；每類有 typed reason 與 snapshot／CSV 欄位。
- sustained vertical failure 只解除 vertical channel；sustained lateral failure 只解除 lateral
  channel；explicit／hard safety disconnect 才解除全部 AP。
- `VerticalReferenceUnmaintainable`、持續 error 不收斂與持續 saturation 都必須有明確 persistence／
  recovery 規則；解除 constraint 時 integrator 不得保留 windup。
- 非有限或必要 input 無效直接暴露錯誤／disconnect reason，不使用 last-good-value fallback。

完成條件：hard protection 當 tick 生效、mode transition 固定延後最多一個 FCC tick；測試不存在
同 tick 重算迴圈；每個失效 scenario 都能從 reason code 解釋最終 authority。

### Phase 9 — 修復與整定 AP 閉迴路行為

目的：在正確架構上處理已知 ALT Hold 與 reference 操作問題。

工作：

- 建立 deterministic closed-loop test harness，使用專案實際 airframe/actuation model 或
  明確的小訊號測試 plant；測試不得回傳假成功。
- Vertical channel 依序驗證 reference generation、capture、inner/outer loop bandwidth、
  saturation、anti-windup 與 mode transition，再調 gain。
- Lateral channel同樣驗證 heading error wrap、bank command、roll response、saturation 與
  reference step。
- Combined channel 驗證 altitude/vertical-path 與 heading/bank 同時改變時的 lift compensation、
  joint feasibility、mode stability、constraint recovery 與 axis-order independence。
- 記錄 rise time、overshoot、settling、steady-state error 與 control activity，作為調整前後
  的比較資料；本次不先制定假裝是真機規格的數值門檻。
- 對不同速度、高度、重量、CAT provisional mode 與 host FPS 進行測試矩陣。

完成條件：第 13.4 節的穩定性底線全部通過，使用者完成 DCS 實飛並判定操縱表現足夠好，
已知 AP defects 關閉。

### Phase 10 — 清理、文件、code review 與 DCS 驗證

工作：

- 移除舊檔名、dead fields、過渡 include 與只測 implementation detail 的 obsolete tests。
- 更新 Systems README、AircraftData/command ownership、證據標籤與 telemetry schema。
- 執行全部 native tests、architecture check、Release x64 DLL build 與安裝。
- 依第 13.3 節進行 DCS 手動驗證，保存 CSV/log。
- 對完整變更執行 code review；Standards 與 Spec findings 都處理完才 commit。

完成條件：只有一條 production control path，文件與程式碼一致，DCS 沒有新增錯誤，且
第 14 節每一條完成定義都有 test、log、source link 或 code-review evidence。

## 13. 驗證計畫

### 13.1 Automated unit tests

- Input conditioning：單位、符號、filter、angle wrapping、non-finite input exposure。
- Mode logic：每個 command、guard、capture、typed reject／constraint／degradation／disconnect reason。
- Manual takeover：AP Disconnect、Paddle Bypass、Pitch Hold Stick Steering 各自的 authority
  transition；paddle release 五種 mode rule；Heading Select retain target；其他 mode 不自動 stick steer。
- Vertical/lateral guidance：raw target 不外流、capture path、reference rate／acceleration、
  integrator reset、heading wrap、bank limit 與 anti-windup。
- Reference selection：manual/AP 每軸 authority、engage/disengage、三種人工接管與 bumpless
  transfer。
- Guidance coordination：同時 vertical/lateral request、bank-to-lift compensation、coordinated
  turn、constraint diagnostics、交換 channel 計算順序時輸出不變；30° level-turn case 約為
  1.155 g，且位於 2.0 g AP envelope 內。
- Maneuver payload：Nz objective 與 q feed-forward 角色固定、p／beta／r 欄位皆有限、bank
  compensation 只做一次、manual directional authority 不被 AP coordinated turn 清除。
- Configuration：30°、20°/s、0.5–2.0 g 的 Reference 標籤及 SI conversion；AP guidance
  envelope 超出 hard envelope 時建構直接失敗。
- Control laws：pilot command mapping、CAT schedule、limiter、actuator feedback、Developer-only
  override unavailable/active。
- Mode monitor：transient constraint 不改 mode、sustained failure 下一 tick 解除正確 channel、
  hard disconnect 全部解除；沒有 same-tick feedback loop。
- State ownership：snapshot 不可回寫、兩個獨立 C++ 物件不共享 state。後者是一般物件
  隔離測試，不是多 FLCC 功能。

### 13.2 System／closed-loop tests

- 64 Hz FCC 與 256 Hz actuator 的 causality、delay 與 host-FPS independence。
- AP engage 後 physical guidance reference 經 selection、coordination 與同一 inner loop 產生
  actuator command。
- altitude hold 與 heading change 同時作用時，高度／航向 channel 使用同一 scheduled
  snapshot，且不因內部呼叫順序得到不同 actuator command。
- combined request 超限時先得到明確 constrained result；zero-bank vertical request 仍不可行時，
  monitor 依 persistence 解除 vertical channel，不能永久積分或暗中改成別的 mode。
- actuator saturation 能回到 FCC anti-windup／disconnect 診斷。
- AP、manual、bypass、CAT 與 limiter 組合沒有繞過 protection 的路徑。
- 不規則 host `dt`、大 `dt` catch-up 及 reset/lifecycle 不改 controller 的 scheduled `dt`。

### 13.3 DCS 手動驗證

至少執行：

1. Cold/Hot start 中可用的基本 pitch、roll、yaw、trim 與控制面方向。
2. AP master engage/disengage、paddle 按住／放開與 hard disconnect；確認 paddle 保留 mode、
   release 按表 capture／retain，hard disconnect 不會自行恢復。
3. Pitch Hold 的 stick steering 只釋放 longitudinal axis，放手 capture 新 pitch；VS／ALT／Heading
   mode 的一般 stick movement 不會暗中 bypass。
4. Pitch、vertical-speed、altitude hold 的 capture、reference increase/decrease、長時間穩定性。
5. Heading hold/select 的 0/360° 跨界、reference increase/decrease、30° bank limit 與回正。
6. Altitude hold 與 heading select 同時改變，確認轉彎時無持續掉高／過度補償，回正後沒有
   vertical integrator 殘留造成的明顯擺盪。
7. 低／中／高空速與不同高度至少各一組 AP 測試。
8. CAT provisional mode 切換及 G-limiter developer switch 關閉時無作用。
9. A/T compatibility smoke test 只確認沒有被本次結構搬移破壞，不評定真實性。
10. 檢查 DCS log、EFM log 與 CSV 中的 non-finite、exception、scheduler error、mode reason、
   active source、reference、measured response、command 與 saturation。

### 13.4 行為驗收量測

每個 AP mode 必須記錄：

- capture 前後 reference 與 measured state。
- selected／captured target、`AutomaticFlightGuidanceReference`、`SelectedFlightReference`、
  `CoordinatedManeuverReference` 與 effector demand；不得只記錄一個含義不明的 `command`。
- rise time、maximum overshoot、settling time、steady-state error。
- actuator command peak、rate、saturation 時間與反轉次數。
- mode transition 時的 command discontinuity。
- altitude 與 heading 同時改變時，兩軸 error、bank-to-lift compensation、constraint reason
  與解除 constraint 後的恢復狀態。
- 測試條件：IAS/Mach、高度、重量、CAT、host FPS。

這些量測用來比較、找出回歸與協助調參，不作為虛構的 F-CK-1 certification threshold。
本次採用以下最低穩定性底線：

- reference 固定且沒有外部擾動時，不得持續等幅／增幅擺盪或發散。
- command、state 與 integrator 必須保持有限且在明確 limit 內。
- capture 後誤差趨勢必須收斂；不得反覆 capture/disconnect 或 mode hunting。
- reference increase／decrease 必須方向正確、反應連續且不造成長時間 actuator saturation。
- 合法的 altitude＋heading 同時 request 必須一起收斂；若超出可用 envelope，必須發布明確
  constraint／degradation reason，不能暗中放棄其中一軸。
- engage、disconnect、bypass 與人工接管不得產生危險的 command jump。
- 不同測試條件下若無法達到可接受表現，失敗必須直接暴露，不能以 silent fallback 隱藏。

通過上述底線後，由使用者依 DCS 實飛判斷是否「足夠好」。不要求預先制定真機等級的
rise time、overshoot 或 settling time 數值；若某項感受仍不好，再利用保存的量測資料定位
並調整。

## 14. 完成定義

架構完成：

- `FlightControlComputer` 仍是唯一 concrete System，內部 Module 不出現在 System catalog。
- AP 不再輸出或覆寫 normalized stick-equivalent pitch／roll。
- selected target、guidance reference、selected reference、coordinated maneuver reference 與
  actuator demand 型別不同且單位明確。
- coordinated maneuver 的 longitudinal payload 固定為 Nz objective＋q feed-forward，
  lateral/directional payload 固定為 p reference＋beta objective＋r feed-forward；沒有 generic bag。
- raw altitude／heading target 不跨越 Guidance／Control seam；同一 FCC tick 只發布一份完整、
  immutable 的 guidance result，再由 coordination 同時計算各軸。
- AP Disconnect、Paddle Bypass 與 Attitude Stick Steering 是三種不同狀態轉移；不存在 generic
  stick-activity override，也不存在任意的 paddle-release 姿態變化 threshold。
- guidance 與 hard-protection limits 來自同一份 immutable config；constraint、degraded、release、
  disconnect 都有 typed reason，沒有 silent fallback。
- state/config/input/output/diagnostics 分離；沒有 process-global mutable controller state。
- A/T、CAT 與 G-limiter override 的證據身分和 availability 清楚。
- 沒有空殼 multi-FLCC、voter、Executive 或 ControlAllocation abstraction。

品質完成：

- 函式不超過 50 行、nesting 不超過 3、cyclomatic complexity 不超過 10。
- 原始碼檔不超過 700 行；超過前按責任拆分。
- 沒有 magic number；所有 gain／limit／rate 位於具證據標籤的 immutable config。
- native tests、architecture check、Release DLL build、DCS smoke／AP matrix 全部通過。
- code review 沒有未處理的 Standards 或 Spec finding。

功能完成：

- 現有 AP command 與模式仍可操作，除非另有已核准的模式變更決策。
- ALT Hold 不再持續上下擺盪，vertical／heading reference 達到第 13.4 節的穩定性底線，並由
  使用者在 DCS 實飛判定為足夠好。
- 正常 altitude＋heading request 同時收斂；超出 envelope 時限制結果、受影響 channel 與
  recovery／release 原因可觀察，不會永久 windup 或暗中放棄一軸。
- manual flight、AP、bypass、disconnect 與 protection 切換沒有明顯 command step。
- 關閉 experimental/developer features 時，擬真路徑不受其輸出影響。

## 15. 決策結論與剩餘資料缺口

### 15.1 `GuidanceCoordination` 到 longitudinal inner loop 的物理 payload

**決策完成。** 採用第 6.5 節的 structured longitudinal maneuver：

- `normal_acceleration_reference_g` 是 vertical-path feedback objective；vertical-path mode active
  時才包含一次且僅一次的 bank-to-lift compensation。
- `pitch_rate_feedforward_rad_s` 是 attitude capture／快速項，不是獨立競爭 authority。
- `LongitudinalControlLaw` 依 flight region、alpha／G margin 與 schedule，把 Nz feedback 項及
  q feed-forward 合成唯一 q reference，再交給 q inner loop。
- AP mode 不跨越此 seam；mode-specific target-to-reference 演算法只留在 `Autopilot`。

Pitch Hold 主要由 pitch-attitude error 與 turn kinematics 形成 q feed-forward；其 Nz 欄位使用
當前 control-law schedule 的 neutral load objective，不因此暗中變成 altitude／flight-path hold。
VS／ALT mode 才由 vertical-path error 形成 Nz objective 並加入 bank compensation，shaped capture
的快速變化形成 q feed-forward。manual command 也轉成相同 schema，因此兩種來源使用相同
inner loop 與 protection。確切 gain、blend schedule 與 filter 由 Phase 9 閉迴路整定，不再影響
Interface，且不得增加 optional mode-specific field。

### 15.2 同時 vertical／lateral request 的可行性規則

**決策完成。** 不採用任一 mode 永久壓過另一 mode 的 priority table；
`GuidanceCoordination` 在每個 tick 解一份 combined feasible reference：

1. 由 vertical guidance 計算所需 load，由 lateral guidance 計算所需 bank／roll rate。
2. 依同一 `ManeuverEnvelope` 計算轉彎後仍可用的 Nz、G／AoA margin、rate 與 actuator authority。
3. request 可行時同時滿足；30° level turn 約需 1.155 g，應落在 2.0 g ALT guidance limit 內。
4. request 不可行時，bank 使用 vertical load demand 以外的剩餘 envelope，因此 heading capture
   會明確變慢並發布 lateral constraint reason。這是物理可行性分配，不是靜態 mode priority。
5. zero-bank 仍無法維持 vertical request 時，發布 vertical unmaintainable；transient 只限幅，
   sustained failure 由下一 tick 的 `ModeMonitor` 解除 vertical channel。
6. hard protection 永遠最高；任何保護介入、degradation 或 release 都有 reason code。

實作不得在 vertical 與 lateral 函式之間靠呼叫順序分配 authority，也不得在 constraint 時暗中
保留舊 command、換 mode 或停止積分卻不發布狀態。

### 15.3 Pilot override／bypass 的每軸 authority

**決策完成。** A／B／C 不是互斥選項；F-16 Reference 證明飛機本來就有三種不同機制：

- AP Disconnect：解除全部 mode，需重新 engage。
- Paddle Bypass：明確 all-axis momentary bypass，保留 mode，release 依第 6.4 節逐 mode
  capture／retain。
- Attitude Stick Steering：只對支援的 attitude-hold axis 生效；第一版只有 Pitch Hold。

Paddle release 每次都執行規則，不再用「姿態變化夠大」決定是否 recapture。stick-steering
threshold 只排除 conditioned axis noise；VS／ALT／Heading mode 不因一般 stick input 自動釋放。
這些是 Reference-derived project decision，不宣稱 F-CK-1 使用完全相同的開關或 threshold。

### 15.4 Target capture 與平滑責任

**專案決策已完成。** AP Guidance 擁有 selected target 到 time-varying physical reference 的
capture path、rate／acceleration shaping 與相關 integrator；Reference Selection／Guidance
Coordination 擁有 source transition 與跨軸一致性；inner control／actuation 擁有 feedback、
protection、anti-windup、rate／position limit 與 actuator lag。此分工依第 3.3、3.4 節的公開
跨領域證據採用，但不宣稱是真實 F-CK-1C 內部實作。

### 15.5 F-CK-1 真實 AP 模式與性能

**真機資料仍缺少，但專案決策已完成。** 現有模式以 Project-defined compatibility 保留；
本次不制定假裝精確的真機性能數值。Phase 9 依第 13.4 節先滿足客觀穩定性底線，再由
使用者進行 DCS 實飛，調整到主觀判定足夠好。結果不得宣稱為 F-CK-1 真機 AP 性能。

### 15.6 Auto-throttle 最終政策

**不屬於本次重構決策。** HOTAS 與 digital engine control 都不能證明有或沒有 A/T。
本次只隔離現有 experimental behavior。其真實性、預設 availability、人工接管和 Engine
Interface 另立計畫，不阻擋本次 AP／FBW 架構工作。

### 15.7 明確延後的長期項目

- 多 FLCC instance 與 redundancy architecture。
- selector/voter、failure manager、BIT、startup/restart 與 power channels。
- 由實際故障需求驅動的 `FlightControlExecutive`。
- 新控制面或 thrust-vectoring 出現後才需要的 control allocation。
- 真實 F-CK-1 CAT、MPO、reversion law、gain、limiter 與 actuator data。

上述項目不得在本次施工中以 placeholder Interface、空類別或 feature flag 預先實作。

### 15.8 施工前決策狀態

**沒有剩餘的架構 blocker。** 第 15.1–15.4 節已固定 payload、combined feasibility、人工接管
與 smoothing ownership；第 6.7 節已固定 tick causality；第 12 節可以在使用者授權後由 Phase 0
依序施工。

仍需在實作中量測的項目只有 gain、capture rate／acceleration、stick-steering activation threshold、
tracking error threshold 與 failure persistence。它們都是 immutable config 的數值整定，不會
改變 System 邊界、Module seam、訊號 schema 或 mode transition 類型，因此不需要再做前置架構
決策。若未來取得與 Reference 衝突的 Confirmed F-CK-1 資料，再以新證據另立變更計畫。

## 16. 施工與驗證紀錄

### DCS 實機驗證後的修正決策

本輪依 `debug.csv` 與玩家操作確認三項根因，且不改變既定 System／Module 邊界：

1. DCS body callback 使用模擬器數學座標，必須只在 `DcsKinematicsAdapter` 轉成
   Core/DCS 本體座標慣例：`x -> roll`、`z -> pitch`、`y -> yaw`，保留 DCS
   右手座標正負號；右滾與抬頭為正、向左偏航為正。角度使用 rad、角速度
   使用 rad/s、角加速度使用 rad/s²；正值分別代表右滾、抬頭與左偏航。
2. `PITCH ATT HOLD` 產生 pitch-rate objective。其 Nz 迴路保留瞬時比例回饋與既有
   保護，但不得累積積分量去抵銷 pitch-rate reference；ALT／vertical-path 模式仍由
   normal-acceleration tracking objective 負責。
3. `ROLL ATT HOLD` 的捕捉、stick-steering recapture、shaped reference 與 monitor
   必須共享相同 guidance bank limit。成功重新接通 lateral／vertical channel 時，清除
   該 channel 的舊 degradation 狀態，不留下與目前模式矛盾的 reason。

全專案的權威規範由 [`EFM_UNIT_CONVENTIONS.md`](EFM_UNIT_CONVENTIONS.md)
維護；本節只是飛控重構如何套用該規範，不能解讀成單位規範只適用於 AP 或 FCC。

各階段單位契約固定如下：DCS raw 值在 adapter seam 轉換；Core 姿態與所有角度控制
計算使用 rad，角速度使用 rad/s，線性距離使用 m，速度使用 m/s，壓力使用 Pa，法向
加速度使用 g，控制權限使用 `[-1, 1]` normalized 值。唯一刻意保留 degree 的控制狀態是
玩家 Heading Set：內部保存 `0..359` 的整數度，只在送入 lateral guidance 時轉成 rad。
`world_yaw_rad` 僅代表 DCS 世界姿態，不得當成航空磁航向；航空航向只取
`getMagneticHeading()` 的獨立 observation。

此規範不只約束 AP：`FrameInput`、`AircraftState`、`SystemPipeline` 的跨系統資料、
FCC/AP、致動器、氣動模型私有輸入、DCS 力／力矩輸出與 CSV 欄位都必須遵守。
物理量欄位名稱必須明示單位；只有無量綱值、布林值或列舉可以不帶單位後綴。
註解只補充座標軸、正方向與有效範圍，不能取代欄位名稱中的單位。

### 16.1 Phase 0–8

Phase 0–8 已依序完成並各自形成可建置、可測試的 commit：

| Phase | Commit | 結果 |
|---:|---|---|
| 0 | `84dce51` | 固定重構 baseline 與完整計畫 |
| 1 | `98886e2` | 巢狀 Module source discovery 與 System entry 邊界 |
| 2 | `b90caec` | typed flight-control signals |
| 3 | `e9d36db` | 單一 input conditioning path |
| 4 | `7dbc376` | AP mode／vertical／lateral／A/T Module 邊界 |
| 5 | `ae490f1` | disconnect／paddle／stick-steering authority |
| 6 | `62adb1f` | 單一 maneuver envelope source |
| 7 | `e34a7fe` | normalized AP seam 原子替換為 physical references |
| 8 | `e92289e` | mode monitor、degradation 與下一 tick 失效鏈 |

### 16.2 Phase 9 deterministic closed-loop baseline

測試使用真正的 `FlightControlComputer`、256 Hz
`FlightControlActuationSystem` 與明確的小訊號 airframe plant。名目案例在 150 m/s、CAT I、
初始 2000 m 高度同時要求 +100 ft 高度與 +10° 航向，執行 30 秒。rise time 定義為兩軸誤差
都進入初始誤差的 10%；settling band 是高度 ±2 m 且航向 ±1°。以下數值只供專案回歸與調參，
不是 F-CK-1C 性能或 certification threshold：

| 量測 | 結果 |
|---|---:|
| combined rise time | 22.046875 s |
| altitude maximum overshoot | 0.267292 m |
| combined settling time | 23.109375 s |
| final altitude error（target - actual） | -0.246761 m |
| final heading error（target - actual） | -0.014318 rad |
| cumulative elevator＋aileron activity | 0.517554 normalized command |
| maximum surface-command rate | 0.388378 normalized/s |
| command-direction reversals | 8 |
| maximum absolute surface command | 0.154022 normalized |
| final five-second altitude peak-to-peak | 0.967228 m |
| saturated FCC ticks | 0 |

Automated matrix 另涵蓋 130／150／220 m/s、CAT I／CAT III、vertical/lateral command
arrival order、combined constraint／recovery、heading wrap、reference 方向、monitor persistence、
anti-windup 與 actuator limit。scheduler fixtures 比較 30／60／144 FPS 與 irregular host frame
partition，確認固定 System tick 不受 host FPS 改變。另有完整 paddle-release capture matrix、
path-mode stick authority、engage／bypass／release／disconnect command-step regression，以及
manual／automatic active-source diagnostics 與 CSV schema 測試。名目案例的最後十秒拆成前後兩半，
要求後半 peak-to-peak 不得成長，避免只看最後五秒而漏掉持續振盪。所有 guidance、monitor、
experimental A/T 與 developer-only override 的 tuning limit 均由 immutable production config
持有並在 FCC 建立時驗證。高度、重量與真實非線性 airframe response 不由小訊號 plant 假裝
覆蓋，保留給第 13.3 節的 DCS 實飛矩陣。

### 16.3 尚待使用者驗證

目前 Release x64 solution build、2536 項 native checks、architecture check／fixtures、System catalog
generator fixtures、DCS ID generator fixtures 與 37-name DLL export baseline 均已通過；最終
Standards／Spec code review 均無未處理 finding。安裝本次 DLL 後開始第 13.3 節 DCS 測試。DCS
實飛、CSV／log 檢查與使用者對操縱品質的判定完成前，Phase 9／10 不標記為完整完成，也不關閉
已知 AP defects。
