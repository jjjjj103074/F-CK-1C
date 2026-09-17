# FLCC F-16／F-16XL 公開資料控制律重建計畫

## 0. 文件定位

- 狀態：討論中，尚未核准進入實作。
- 建立日期：2026-08-08。
- 本文件是本次 FLCC 控制律重建的獨立計畫，不繼承其他飛控計畫文件的假設、模組拆法或實作順序。
- 舊文件只能作為歷史紀錄或證據索引，不能作為本計畫的設計依據。
- 本文件先記錄已同意的目標、必須更改的錯誤、實作成果定義與待討論問題；待討論問題未決定前，不得自行補成專案自訂行為。

## 1. 總體目標

### Objective

依公開 F-16／AFTI/F-16／F-16XL 文件，完整重建一套可追溯的 FLCC 控制律拓撲，使飛行員輸入、配平、模式排程、CAT、飛行包線限制、回授、積分、控制面命令與致動器回授各自具有明確責任。

此處的「完整」是指：

1. 公開文件描述的主要控制律功能與訊號鏈均有對應實作。
2. 每個控制律 Module 的輸入、輸出、狀態與更新率明確。
3. 控制律拓撲不得由目前專案行為反向合理化。
4. 找不到公開數值時可以調教，但必須標示為 `Project-defined`。
5. 不宣稱還原未公開的 F-CK-1C 或 F-16 原廠係數與飛行認證資料。

## 2. 證據規則

### 2.1 證據優先順序

1. `Confirmed F-CK-1C`：公開且可驗證的 F-CK-1C 資料。
2. `F-16/F-16XL Reference-derived`：F-16、AFTI/F-16、F-16XL 的飛行手冊、NASA 或美國政府報告。
3. `Comparable-aircraft Reference-derived`：F/A-18、F-8 DFBW 等同年代公開研究，只能證明概念或工程方法。
4. `Project-defined`：缺少實機資料，依本 EFM plant 調教的係數、門檻或數值。
5. `Developer-only`：只供開發、測試或非擬真玩法使用，production default 不得啟用。

F-16 家族內不採用一條僵硬的年代排序，而是依問題選來源：

| 問題 | 主要來源 | 使用邊界 |
|---|---|---|
| Production digital FLCS 的正常操作、gain-regime selection、trim 與 limiter 行為 | F-16C/D Block 50/52+ Flight Manual | 本計畫主要的 production-digital observable behavior；公開手冊不等於原始碼或完整方程 |
| 數位 FLCC 的整體控制概念、64 Hz 主迴圈、F-16XL 飛行表現 | F-16XL DFLCS flight-test report | 本計畫的主要世代與飛行控制方向 |
| 內部 control structure、多頻率分工、integrator 與 actuator 架構 | AFTI/F-16 NASA TP-2857 | 補足 F-16XL 公開報告沒有揭露的內部工程細節 |
| F-16C/D 未公開的較早期功能圖與操作對照 | F-16A/B Flight Manual | 只作為類比 FLCC 的功能拓撲參考；不得把圖中的 block 或連線直接當成 production-digital F-16C/D 軟體架構 |
| 無 F-16 資料的切換工程方法 | F/A-18 NASA 報告 | 只能標示為 comparable-aircraft，不得冒充 F-16XL 行為 |

F-16XL 是本計畫最有價值的主方向，但目前公開 flight-test report 不是完整控制律設計規格。它明確提供 blended pitch-rate/normal-acceleration command、limited roll-rate command、CAT III 效果、64 Hz DFLCC 與控制面速率／行程資料；它沒有公開 stick-shaping 公式、q/Nz blend 方程、trim 演算法、完整 CAT parameter tables、AOA/G limiter 方程、完整 mixer、integrator/anti-windup 或 actuator communication protocol。報告引用的內部設計文件 `72PR206` 目前也沒有可驗證的公開版本。因此不能只靠 F-16XL 一份報告完成控制律。

### 2.2 F-16XL 公開報告的直接證據邊界

NASA F-16XL DFLCS flight-test report 明確提供：

- 使用與 production F-16 相同概念的 minimum-displacement force-command sidestick。
- longitudinal law 是 blended pitch-rate and normal-acceleration command system。
- lateral stick 產生 limited roll-rate command；CAT III 會降低最大可命令 roll rate。
- analog computers 被 quadruplex production Block 40 DFLCCs 取代；F-16XL laws 以 64 cycles/s 重新數位化，並修改／新增 gains and filters。
- unpowered-approach 與 powered-approach configurations 的 flight-test 結果。
- CAT III 在該 F-16XL configuration 將最大可命令 roll rate 降低 75 deg/s。
- 試驗發現原 DFLCS 9-g limiter 會 overshoot，因此試驗設定不高於 7.2 g；這是已知缺陷紀錄，不是本專案要複製的目標行為。
- F-16XL 特有控制面資料：aileron 20° up／30° down、60°/s；elevon ±30°、60°/s；LEF 6.4° up／36.5° down、31°/s；rudder ±30°、90°/s。這些只能作為第二部分參考，不能直接套用到 F-CK-1C。

該公開報告沒有提供：

- exact q/Nz blend equation、stick-shaping equation 或完整 gains/filter coefficients。
- pilot trim switch algorithm、trim rate、trim authority 或 mode-specific trim equation。
- 完整 CAT I/III parameter tables、AOA/G limiter equations 或完整 transition logic。
- P／PI／PID 分塊、anti-windup algorithm、surface mixer equation 或 actuator command protocol。
- approach configuration 的完整 selection logic；這部分需由 F-16 flight manual 補足。

### 2.3 F-16A/B 類比圖與數位 FLCC 的證據邊界

- Blocks 10/15 的 Figure 1-42 描述較早期四餘度類比 FLCC 的典型功能圖。它能證明 pilot/trim/autopilot command、sensor-derived augmentation、integrator、selector 與 ISA 的功能關係，但不能證明 F-16C/D 或 F-CK-1C 以相同軟體 class、state 或執行順序實作。
- AFTI/F-16 文件明確把數位系統描述為取代原 F-16 quadruple analog fly-by-wire system；F-16XL 報告則明確記錄由 production Block 40 DFLCC 取代原類比電腦。兩者都支持「功能可延續、實作架構已改變」，不支持逐 block 翻譯類比圖。
- F-16C/D Block 50/52+ 的 production-digital 功能圖公開了 `STABILITY/COMMAND AUGMENTATION`、`CONTROL DYNAMICS`、`GAIN SCHEDULING`、`QUADRUPLE INPUT/OUTPUT SELECTORS`、結構濾波、AOA/yaw/roll-rate limiting、ARI、autopilot 與 digital backup 等責任，但沒有公開可直接重建的軟體流程圖或完整方程。
- AIDC 官方資料確認 F-CK-1C/D 升級採用 32-bit DFLCC。這足以排除把 F-16A/B 類比 block diagram 當成 F-CK-1C 的實體實作，但該資料沒有公開 F-CK-1C control-law topology、gain regimes、limiter equations 或與 F-16 的共用程度。
- 本計畫可把 A/B 圖用作 responsibility topology check；控制律行為、gain-regime selection、trim 與 limiter 優先以 F-16C/D production-digital 手冊為準，數位內部組織再以 F-16XL 與 AFTI/F-16 補足。

### 2.4 F-16C/D Block 50/52+ 公開資料索引

- Figure 1-42, sheets 1-2（手冊頁 1-122 至 1-123）：FLCS Functional Schematic；公開輸入來源、數位 FLCC 的功能責任、AMUX/DMUX、INS/CADC/NVP 連線、ISA 與 LEF actuation。
- Figure 1-43（手冊頁 1-124）：FLCS Pitch, Roll & Yaw Schematic；公開 pitch/roll/yaw trim、stick/pedal force、AOA/rates/accelerations/impact pressure 到 FLCC，再到各 ISA 的高階訊號關係。
- Figure 1-44（手冊頁 1-125）：三軸 limiter summary；區分 Cruise 與 Takeoff/Landing 行為。
- Figure 1-45（手冊頁 1-126）：Cruise Gains 的 AOA/G limiter function，包含 CAT I/III 與重量的可觀察曲線。
- FLCS Gains 文字（手冊頁 1-126 起）：`Cruise Gains`、`Takeoff and Landing Gains`、`Standby Gains`、AOS feedback、trim、MPO、ARI 與故障／DBU 行為。
- 這些是飛行手冊級功能資料，不是 source code、software component diagram、完整控制律方程或認證 gain table。

### 2.5 禁止事項

- 不得把「其他 FBW 飛機曾使用」寫成「F-16 確定使用」。
- 不得把目前程式碼中的常數當成實機證據。
- 不得為保留現有手感而保留無來源的控制律路徑。
- 不得使用 silent fallback、隱藏模式切換或假成功路徑。
- 每個限制器必須同時保留 requested value 與 effective value，不能覆寫後失去原始飛行員要求。

## 3. 目標訊號鏈與 Module 接縫

### 3.1 FLCC 一句話職責與範圍

> FLCC 負責根據飛行員／自動飛行指令、飛機感測狀態與構型，決定飛機此刻應如何運動，並產生受保護、協調一致的飛行控制致動器命令。

控制律只是上述責任的計算核心。FLCC 的完整責任範圍包含：

1. **控制來源與 authority**：整合 pilot、trim、autopilot guidance、manual override 與有證據的自動補償；保留 raw demand，不以受限結果覆寫原始要求。
2. **Gain regime 與構型選擇**：選擇 `Cruise`、`TakeoffAndLanding`、未來 `Standby`，並依 `CAT I`／`CAT III` 選取 immutable stores-configuration profile。Gain regime 與 CAT 是正交狀態，不得合併成巨大 mode enum。
3. **控制律與連續排程**：把 pilot／automatic demand 轉成 Nz、pitch rate、roll rate、sideslip、pedal authority 等可計算的 maneuver reference，並依 Mach、dynamic pressure、AOA 與構型排程 gains／filters。
4. **飛行包線與離場保護**：計算 G、AOA、roll-rate、yaw-rate、rudder-authority 與 inertia-coupling/departure-prevention 等 requested/effective references；限制的是 control-law command，不是 DCSBridge 中的 raw pilot signal。
5. **三軸協調與 electronic surface mixing**：負責 ARI、sideslip feedback、turn coordination、對稱／差動 stabilator 與其他已證實的 cross-axis interconnect，最後形成技術中立的 control-surface／actuator demands。
6. **Trim、固定補償與結構濾波**：依 active law 解讀 pitch／roll／yaw trim，並容納有證據的 configuration、gun、asymmetry compensation 與 structural filters。
7. **Automatic flight-control execution**：FLCC 可執行 attitude、altitude、heading 或 navigation-guidance tracking；航點選擇與航線規劃由導航／任務系統負責，FLCC 只接收 guidance reference。
8. **完整性監控與降級空間**：長期容納 sensor selection、redundant-channel voting、fault isolation、reversion 與 digital backup；本次不實作的項目必須明確保持 unavailable，不得以 silent fallback 假裝存在。

FLCC 不負責：

- DCS command ID、鍵盤／搖桿來源 arbitration、虛擬軸 slew／return 或 DCS 座標／單位翻譯；這些屬於 DCSBridge input Adapter。
- 感測器物理、INS／air-data/navigation 解算本身；FLCC 只消費其 typed observations 或 guidance references。
- 航點、航線與任務規劃；導航／任務系統提供目標，FLCC 執行飛行控制。
- 油壓、servo-valve、馬達、致動器 position/rate dynamics 與控制面實際位置；這些屬於 `FlightControlActuationSystem`。
- 飛機受力後的六自由度運動、氣動力、DCS draw arguments 與座艙呈現。

FLCC 與致動鏈的關鍵 seam 是：**FLCC 決定飛機應如何運動以及控制面應被要求做什麼；ActuationSystem 決定控制面實際如何移動，並回傳 actual position／rate／limit／saturation。**

### 3.2 目標訊號鏈

```text
DCS controller / keyboard / clickable cockpit
    -> DCSBridge input Adapter
    -> normalized pilot demand + independent trim controls
    -> PilotControls aircraft state
    -> FLCC input signal management
    -> pilot command gradient / trim interpretation
    -> physical maneuver reference
       (Nz, pitch rate, roll rate, sideslip or pedal authority)
    -> mode and CAT parameter scheduling
    -> envelope and reference limiting
    -> longitudinal / lateral / directional feedback laws
    -> electronic surface mixer
    -> FlightControlActuatorCommand
    -> FlightControlActuationSystem
    -> actual surface state
    -> Aerodynamics and actuator feedback
```

外部接縫維持小型 typed interface；DCS 差異、控制律細節與致動器物理不得互相滲漏。內部可再用 private seams 分割可測試的演算法，但不得把 FLCC 內部 Module 升格成虛構的飛機實體 System。

## 4. 已確認的架構決定

### 4.1 正規化飛行員輸入

- Core 使用 `[-1, 1]` 的 DCS-neutral pilot demand。
- 此數值代表校準後的飛行員控制要求，不代表實際牛頓、磅力或搖桿位移。
- 本次不模擬力感測器、位移感測器、ADC 或實體控制桿的正規化電路。
- 力感測與位移搖桿的差異由輸入 Adapter 吸收，不得改變 FLCC 控制律 interface。
- FLCC 必須完整保留收到的 pilot demand；後續限制器只能產生獨立的 effective reference，不能回寫或篡改 raw demand。

### 4.2 DCSBridge 輸入翻譯

已確認目標：

- DCS command ID、DCS 軸方向、鍵盤虛擬軸、按住／放開狀態、逐漸增加與回中屬於 DCSBridge 輸入 Adapter。
- 搖桿、鍵盤與未來 clickable cockpit 最終應產生相同語意的 Core command 或 pilot signal。
- 真正的飛機配平狀態、CAT 狀態、控制律與飛行包線不屬於 DCSBridge。
- DCSBridge 可以有較深的 implementation，但對 Core 的 interface 必須保持小且 DCS-neutral。

已選定的設計方向是「固定功能、資料驅動」，不是可任意配置的腳本引擎：

```text
analog axis command ─┐
negative key ────────┼─> one VirtualAxisBinding ─> one Core pilot axis
positive key ────────┘
```

- 每個 binding 固定只有一個 analog command、兩個相反方向的 keyboard commands 與一個 Core output。
- 共用演算法負責按住／放開、逐漸增加、回中與來源切換；新增軸時新增資料列，不新增一套流程。
- 每列資料只描述 command IDs、正負方向、slew/return rate 與必要的虛擬軸行為。
- Core 不得看到 DCS command ID、鍵盤來源或 controller 來源。
- trim switch、AP switch 與 cockpit switch 不是 virtual axis；它們輸出獨立的 semantic state/event。
- 所有 binding 由相同的參數化 native tests 驗證，避免表格擴充後出現未測試的符號或 release 行為。
- 已決定 authority handoff：keyboard key press 取得該軸 authority；只有 analog axis 實際發生明顯位移時才取回 authority。不能以 DCS 重送相同 axis event、事件先後或隱藏 timeout 判斷。
- keyboard virtual axis 依 `dt` 逐漸朝按鍵要求的目標移動；它不是瞬間跳到端點，因此來源交接不額外加入 smoothing 補丁。

尚未決定：

- stateful Adapter 的最小外部 interface，以及虛擬軸由哪個 DCSBridge tick 依 `dt` 推進。
- `PilotControls` 只保留哪些 aircraft-control semantic state；所有純 DCS 來源狀態必須移回 DCSBridge。

以上內容在細節設計時討論；目前只固定責任，不固定實作形狀。

### 4.3 配平

目前做法不能視為「能動但不準」，而是控制律語意錯誤，必須替換。

現況問題：

- pitch、roll、yaw trim 直接加到 raw axis。
- stick 與 trim 一起通過 cubic shaping、低通濾波與 normalized rate limit。
- pitch／roll trim 使用對稱 `[-0.3, 0.3]`，yaw 使用 `[-0.2, 0.2]`，沒有對應物理命令語意。
- roll trim 沒有以 F-16 手冊描述的方式進入 ARI。
- cruise 與 takeoff/landing 對 trim 的不同解釋沒有獨立建模。

必須達成：

- stick demand 與 trim control 在 FLCC 接縫保持獨立。
- 不把 F-16/F-16XL stick 誤稱為「desired attitude」。F-16XL 公開報告描述 longitudinal stick command 是 pitch rate `q` 與 normal acceleration `Nz` 的混合，lateral stick command 是受限 roll rate。
- neutral stick 代表目前 active law 的中立 command condition，不保證固定俯仰姿態。
- F-16A/B Figure 1-42 的原始 command path 是 `PITCH TRIM -> MECH LIMIT`，再與 `HORIZ TAIL STICK FORCE` 於 `STICK LIMIT` 前合併；`AUTOPILOT PITCH` 與 `MANUAL PITCH OVERRIDE` 也在 pitch-integrator 上游的 command summing path 合併。
- Figure 1-42 的 `STABILITY/COMMAND AUGMENTATION` 有兩條可見輸出：一條向上回到 `STICK LIMIT`／`AUTOPILOT PITCH`／`MANUAL PITCH OVERRIDE` 之後、`PITCH INTEGRATOR` 之前的 command summing node；另一條直接送到 `PITCH INTEGRATOR -> SELECTOR` 之後、horizontal-tail output selectors 之前的 output summing node。
- `STICK LIMIT` 是 command-path 的範圍限制功能；`MECH LIMIT` 是 trim/pedal mechanical-command authority 的範圍限制功能。該圖沒有公開精確飽和值、transfer function、單位或是否另含 shaping，因此不得把它們擴張解讀為 rate limiter 或 filter。
- `PITCH INTEGRATOR` 只代表其中一條動態積分路徑，並非整個 pitch controller；`STABILITY/COMMAND AUGMENTATION` 依可見 AOA、pitch-rate、normal-acceleration 與 cross-axis sensor signals 產生兩種 feedback/augmentation contributions。正常 stick command 在此圖中沒有直接進入該 block。圖中的 `SELECTOR` 負責餘度通道的選擇／路由，不是主要控制律計算器。
- cruise gains 下，pitch trim 會改變 hands-off equilibrium/reference；實作必須保留獨立 trim command 及其 authority limit，再由 active digital command law 解釋。不得只因類比 Figure 1-42 的拓撲，就固定數位 FLCC 的 integrator 數量或 state ownership。
- takeoff/landing gains 下，trim 依該模式的 pitch-rate／AOA law 解釋；不得沿用 cruise 的 raw-axis offset。
- roll trim、rudder trim 與 ARI／方向控制律的關係獨立建模。
- trim authority、trim rate、重置與起飛／落地行為在細節討論後決定。

公開資料能證明 trim 的路徑與模式相關性，但沒有提供完整 pitch-trim integrator equation、rate、authority 或所有 mode initialization 數值；這些不能冒充已知 F-16XL 係數。

### 4.4 Pilot command shaping 與 filtering

#### 已證實存在的功能

NASA TP-2857 對 AFTI/F-16 明確記載：

- 所有 longitudinal modes 以 64 Hz 運作。
- lateral-directional control inputs、roll stick 與 pedals 在 32 Hz 進行 shaping and filtering。
- lateral-directional feedback paths 與 interconnects 以 64 Hz 運作。

F-16XL DFLCS 報告也明確提到重新編碼後修改或新增 gains and filters，但沒有公開每個 filter 的完整公式與係數。

因此可以確認「F-16 系列飛控具有 input shaping/filtering」，但不能確認目前專案的以下做法：

- 所有軸都使用相同的一階低通公式。
- pitch 使用目前的 `0.05/0.10 s` time constant。
- CAT I／III profiles 含有不同 raw-input time constant。
- CAT I／III profiles 含有不同 normalized input slew rate。
- cubic weight `0.10/0.20` 是 F-16 行為。
- 一階濾波位於目前程式碼的位置。

#### 功能目標

Pilot command shaping/filtering 若存在，其目標應是：

1. 建立所需的 pilot-to-aircraft 頻率響應與 handling qualities。
2. 限制不需要的高頻控制內容，避免放大感測雜訊或激發結構／致動器限制。
3. 讓不同 command mode 具有預定的 rise time、overshoot 與 tracking response。
4. 在有明確設計依據時降低 PIO 風險。

它不是：

- 模擬一般遊戲搖桿「不會瞬間移動」。
- DCS 軸曲線的替代品。
- 隱藏控制律不穩定或致動器飽和的補丁。
- CAT III 專用的 raw-input 延遲，除非後續找到直接證據。

目前的一階濾波與 normalized command-rate limiter 列為必須重新設計的內容。是否保留一階拓撲、應放在 raw pilot signal 或 physical command reference、三軸是否不同，均在控制律細節討論時決定。

### 4.5 CAT I／CAT III

CAT 不得成為 pilot demand 的編輯器。

固定原則：

```text
raw pilot demand ───────────────────────────────┐
                                                v
CAT selector -> selected immutable profile ─────> same control-law topology
aircraft state ─────────────────────────────────> same control-law topology
                                                |
                                                v
                                   requested/effective references
```

- FLCC 完整接收並保存 pilot demand。
- CAT 只是一個 selector；控制律讀取其選定的 immutable profile。CAT 本身不修改參數、訊號或演算法。
- 同一個控制律讀取不同 CAT profile，因此相同 pilot demand 可計算出不同的 effective roll-rate、AOA/G 或 rudder-authority result。
- Debug telemetry 必須能同時看到 raw pilot demand、requested reference、effective reference 與目前 CAT parameters。
- `CatConfigurationProfile` 建立後不可被 CAT 或控制律修改，也不可在 CAT I/III 間動態混合成第三份 profile。
- profile 可包含 limiter breakpoints/slopes、roll-rate schedule、rudder-authority schedule、command authority 與有直接證據的 gains/interconnect constants；不能只把 CAT 簡化成一個最大值 clamp。
- takeoff/landing law 依 F-16A/B 手冊不採用 CAT cruise schedule；mode scheduler 必須先決定 active law，再提供適用的 parameter profile。
- CAT I 與 CAT III profiles 在 FLCC 建立時一次建構並驗證；`STORES CONFIG` switch 只在執行期選取其中一個 immutable profile reference。
- 切換 switch 或每個 FLCC tick 都不得進行檔案 I/O、重新建構 profile 或修改既有 profile。
- STORES CONFIG 切換立即選用目標 profile；在找到 F-16 transition 證據前，不發明 profile interpolation、延遲或 smoothing。既有 controller state 保持可觀察，切換造成的不連續必須由測試暴露。
- runtime telemetry 必須至少顯示 switch position、active profile identity，以及受到該 profile 影響的 requested/effective limits。

`STORES CONFIG` 訊號鏈必須區分三件事：

```text
DCS command ID
    -> DCSBridge translation
    -> Core semantic switch state: StoresConfigurationSelection
    -> FLCC input
    -> active immutable CAT profile

future Stores Management System
    -> required loading category
    -> mismatch warning against switch state
```

- 目前階段只需要第一條鏈；`DCSBridge` 不持有 CAT profile，也不判斷武器是否屬於 CAT I/III。
- `StoresConfigurationSelection` 表示駕駛艙 switch position，不等於實際掛載分類。未來 SMS 可另外提供 required category 與 mismatch warning，但不能偷偷改寫 switch state。
- FLCC 只依 semantic selection 讀取 profile，不接觸 DCS command ID；switch state 最終由哪個 Core cockpit/control-state owner 保存，在 DCSBridge input adapter 細節設計時決定。

目前 `StoresControlLawSchedule` 把 `PilotInputShapingConfig` 放入 CAT schedule，並以 `stores_transition_0_1` 在 CAT I/III 間混合 time constant、normalized rate、cubic weight 與其他參數。此動態混合不符合「讀取選定 profile」語意，列為必須移除。

CAT 設計尚餘兩項細節：profile 先採 typed C++ static data 或外部檔案，以及缺少公開表格的 CAT I／III 數值。第一版推薦 typed static data，避免在沒有 hot-reload 需求時引入檔案格式、I/O 與失敗路徑；所有無公開依據的數值必須標示 `Project-defined`。

### 4.6 64／32／4 Hz 功能

不先全部移回 64 Hz，也不任意重新分配。

重新核對 NASA TP-2857 後，公開資料已足以支持下列 AFTI/F-16 分配：

| 功能 | 更新率 | 證據狀態 |
|---|---:|---|
| Longitudinal control modes | 64 Hz | AFTI/F-16 直接資料 |
| Lateral-directional feedback paths | 64 Hz | AFTI/F-16 直接資料 |
| Lateral-directional interconnects | 64 Hz | AFTI/F-16 直接資料 |
| Roll-stick and pedal shaping/filtering | 32 Hz | AFTI/F-16 直接資料 |
| Slow-moving air-data gain scheduling | 4 Hz | AFTI/F-16 直接資料 |
| F-16XL DFLCS main control-law cycle | 64 Hz | F-16XL 直接資料 |

本計畫保留目前的多頻率機制及上述功能分類。重建時只允許：

- 有直接 F-16/F-16XL 證據的功能使用對應 subrate。
- 無證據且屬高頻內迴路的功能先使用 64 Hz。
- 新的 32/4 Hz 功能必須說明訊號頻寬、zero-order hold 行為與延遲影響。
- 不因為方便或效能而把限制器、actuator feedback 或 mode safety logic 降到 4 Hz。

### 4.7 Production-digital FLCS 的離散狀態與連續排程

不得把所有條件組合成一個巨大 mode enum，也不得複製兩套完整 end-to-end controller。F-16C/D 公開手冊支持以下分層：

手冊雖稱為 `Gains`，實際描述的不只是乘法增益。每個 gain regime 同時決定 command-system family、trim interpretation、gain/limit schedules 與部分 protection behavior。G-command／G-AOA blend 或 pitch-rate／pitch-rate-AOA blend 是 active gain regime 內部的 subordinate command schedule，不是與 gain regime 平行、獨立切換且同時計算的 top-level mode。

| 類型 | 項目 | 本計畫狀態 |
|---|---|---|
| 離散 gain regime | `Cruise Gains` | 第一部分實作 |
| 離散 gain regime | `Takeoff and Landing Gains` | 第一部分實作 |
| 離散 reversion regime | `Standby Gains` | 保留 typed enum/interface，不在第一部分實作控制律 |
| 離散 stores selection | `CAT I`／`CAT III` | 第一部分實作；只選 immutable profile，主要供 Cruise consumers 讀取 |
| 離散 authority | Autopilot／Auto TF／manual override | 與 primary gain regime 正交；本計畫只維持既定 AP 邊界，不把 AP 當成第三套 manual law |
| 連續 command schedule | Cruise：低 AOA G-command，AOA 增加後 blended G/AOA | 第一部分實作；缺少的曲線數值標示 `Project-defined` |
| 連續 command schedule | Takeoff/Landing：低於約 10° AOA 的 pitch-rate command，之後 blended pitch-rate/AOA | 第一部分實作；轉換曲線需保留可調參數與來源標籤 |
| 連續 authority schedule | Cruise roll authority 隨空速、AOA、horizontal-tail position、total rudder command 變化 | 第一部分實作 |
| 連續 authority schedule | Takeoff/Landing 固定且較低的 roll-rate limit，不讀取 CAT cruise schedule | 第一部分實作 |
| guarded overlay | AOA/G limiting、yaw-rate limiting、high-AOA roll-rate limiting | 第一部分實作；各 limiter 保留 requested/effective values |
| guarded overlay/interconnect | ARI、turn coordination、sideslip feedback | 第一部分依可證實範圍實作 |
| slow schedule | air-data gain scheduling | 4 Hz 更新，供 64 Hz 控制律以 zero-order hold 讀取 |
| failure/reversion | sensor validity、digital backup、fault voting | 未來範圍；不得用 silent fallback 假裝已實作 |

共享架構指共享訊號生命週期與輸出 contract，不代表 Cruise 與 Takeoff/Landing 使用相同方程：

```text
pilot / trim / autopilot command sources
    -> command authority and command-path limits
    -> active gain-regime command interpretation
       Cruise: G -> blended G/AOA
       Takeoff/Landing: pitch rate -> blended pitch rate/AOA
    -> sensor-derived stability/command-augmentation contributions
    -> regime-selected control dynamics and limit schedules
    -> electronic surface mixing
    -> redundant output selection / ISA command seam
```

此流程是依 production-digital 公開責任整理的軟體邊界，不宣稱是 F-16C/D 未公開的逐指令執行順序。integrator 的數量與 state ownership 必須由數位資料及閉迴路轉換測試決定，不能只照抄 F-16A/B 類比圖。

## 5. Key Results

### KR-1：乾淨且可追溯的 pilot-control interface

- DCSBridge 將控制器、鍵盤與 clickable cockpit 轉成同一套 DCS-neutral pilot commands。
- Core 不再以 `normalized DCS command authority` 描述自己的 command contract。
- stick、pedal、trim 保持獨立 typed values。
- raw pilot demand 進入 FLCC 後保持 immutable。
- repository default axes 維持線性；玩家個人 DCS 曲線不屬於 Core 控制律。

### KR-2：重建 production-digital F-16 Cruise Gains

- 依 F-16C/D production flight manual：低 AOA 時 pitch stick 為 G-command；AOA 增加時轉為 blended G/AOA command。
- pitch rate `q` 在 Cruise Gains 中作為 stability/command-augmentation feedback；不因 F-16XL 對其特殊 airframe 使用 q/Nz blended command 的描述，就把 production F-16C/D Cruise Gains 改寫成 q-command law。
- 依公開 F-16/AFTI/F-16 拓撲建立 stick feed-forward、Nz command/feedback、q feedback、alpha feedback 與 forward-loop integration。
- 重建 filtered Nz feedback、washed-out q feedback、forward-loop integration 與 alpha artificial-stability feedback。
- requested Nz、effective Nz 與每個 feedback contribution 可獨立觀察。
- 控制律邏輯依來源建立；係數可以依 F-CK-1C EFM plant 調教並標示 `Project-defined`。

### KR-3：重建 Takeoff and Landing Gains 的 mixed command law

- 不再使用純 pitch-rate command 取代完整 landing law。
- 實作 F-16C/D 手冊描述的行為：10° AOA 以下為 pitch-rate command，以上逐漸混合 pitch-rate/AOA command。
- `landing gear handle DOWN`，或在 airspeed below 400 knots 時 `alternate flaps EXTEND`／`air-refuel door OPEN`，選用 Takeoff and Landing Gains；不能把缺少的輸入默認為 false。
- Takeoff and Landing Gains 的 roll-rate law 不讀取 CAT I/III roll-rate profile；對應 fixed roll-rate schedule 需獨立建模。
- trim、三個 selection inputs、AOA 轉換門檻與模式轉換狀態各自獨立建模。
- 模式轉換不得造成未要求的 surface step。

### KR-4：重建 G、AOA 與 CAT protection

- 依公開 F-16/F-16XL 邏輯重建 G limiter。
- 依公開分段曲線重建 AOA/G limiter，而不是以單一 clamp 或修改 stick input 代替。
- 同一套演算法讀取選定的 CAT profile；profile 本身保持 immutable。
- 重建 control law 如何讀取 CAT profile 中的 roll-rate、rudder-authority 與 AOA/G schedules。
- `Developer-only` G-limiter override 保留，但不能改變 production default。

### KR-5：重建 lateral／directional law

- 依 F-16 公開資料重建 roll-rate command、roll-rate feedback、yaw damping 與 ARI。
- 32 Hz 只負責有證據的 roll-stick/pedal shaping/filtering。
- 64 Hz 負責 feedback paths 與 interconnects。
- control law 從所選 CAT profile 讀取 effective maneuver authority schedule；raw roll/pedal demand 保持不變。

### KR-6：重建 integrator 與 anti-windup

- 每個 integrator 有唯一狀態 owner、明確物理誤差與單位。
- anti-windup 同時考慮 electronic command limit 與實際 actuator saturation/position/rate limit。
- requested、unsaturated、limited、actual surface state 均可觀察。
- 不得以重置 integrator、凍結整條控制律或 silent clamp 隱藏根因。
- 不把整個 FLCC 標成 PID。公開 AFTI/F-16 資料支持的是 scheduled multi-loop control：stick feed-forward、state feedback、filtered/washout feedback 與 integrator 同時存在；integrator 只是其中一個動態 block。

### KR-7：證據、測試與診斷可追溯

- 每個主要 control-law block 標記證據等級與來源。
- 每個 `Project-defined` 數值集中於 owning Module 的 configuration 並有驗證。
- 建立 open-loop block tests、closed-loop rig tests、mode-transition tests、CAT comparison tests 與 actuator-saturation tests。
- `debug.csv` 能觀察 raw demand、requested/effective references、limiter contributions、feedback efforts、integrator 與 actuator state。
- DCS 手動測試只驗證整體飛行與人機手感，不代替 native control-law tests。

## 6. 第一部分：FLCC 控制律重建

### 6.1 範圍

第一部分僅處理：

- DCS-neutral pilot-control seam。
- trim interpretation。
- Input Signal Management。
- command gradient 與 command modes。
- mode/CAT parameter scheduling。
- longitudinal、lateral、directional laws。
- G/AOA/rate/authority limiters。
- feedback、integrator、anti-windup。
- electronic surface mixer 的現有輸出語意。
- FLCC diagnostics 與 native tests。

第一部分不要求建立左右獨立的控制面或完整液壓模型。現有 actuator command interface 可在第一部分暫時保持三個 virtual control channels，但不能新增更多依賴該簡化模型的控制律假設。

第一部分必須先建立不綁定致動器技術的 typed seam：

```text
FLCC actual-surface demand
    -> ActuatorCommand interface
    -> ActuationSystem
    -> actual position / rate / saturation / limit feedback
    -> FLCC
```

此 seam 不假設 electric motor、液壓 servo、DCS draw argument 或單一一階 lag。完整 actuator 物理留在第二部分。

### 6.2 必須移除或替換的現況

| 現況 | 判定 | 目標 |
|---|---|---|
| axis + trim 後一起 shaping | 錯誤 | stick 與 trim 分開進入模式相關 command law |
| CAT schedule 內含 raw pilot shaping | 錯誤 | CAT 只選擇 control-law parameters |
| CAT I/III 使用不同 raw-input time constant | 無直接證據 | 移除；是否有其他 command filter 待討論 |
| CAT I/III 使用不同 normalized slew limit | 無直接證據 | 移除；改為 physical reference/response logic |
| CAT I/III 使用不同 cubic curve | 無直接證據 | 移除；shaping 拓撲待討論 |
| CAT I/III 共用主要 AOA limit | 與 F-16 手冊不符 | 建立各 CAT parameter schedule |
| 固定 soft/hard G 代替 AOA/G 曲線 | 不完整 | 重建分段或表格化 limiter |
| 簡化 pitch-rate landing law | 不完整 | 重建 pitch-rate/AOA mixed law |
| project-defined PI 拼接 | 不足以代表 F-16 | 依公開 topology 重建 feedback/integration |

### 6.3 第一部分實作順序

1. 建立 evidence ledger 與現況 characterization tests。
2. 修正 DCSBridge／PilotControls／FLCC pilot-control seam。
3. 將 trim 從 raw axis shaping 中完全分離。
4. 保留並驗證 64/32/4 Hz scheduler 與資料保持行為。
5. 重建 longitudinal normal/cruise command and feedback law。
6. 重建 Takeoff and Landing Gains 的 mixed command law。
7. 重建 lateral/directional command、feedback 與 ARI。
8. 重建 CAT parameter scheduling 與 G/AOA/roll/yaw limiters。
9. 重建 integrator、anti-windup 與 actuator-feedback interaction。
10. 移除被新控制律取代的 legacy paths、compatibility logic 與過時測試。
11. 完成 native verification 後再設計針對性 DCS 測試。

每個步驟開始前，相關待討論問題必須先決定；不得在實作中臨時猜測。

## 7. 第二部分：控制面分配與致動鏈

第二部分與第一部分獨立討論和實作。第一部分完成前，只記錄目標，不決定詳細 interface。

### 7.1 第二部分 Objective

建立從 FLCC electronic control effort 到各實際控制面位置、氣動效果、DCS 模型動畫與 actuator feedback 的一致資料鏈。

### 7.2 必須討論的範圍

1. 保留三個 virtual control channels，或升級為左右獨立 physical surface demands。
2. Pitch/roll/yaw effort 如何分配到：
   - left/right stabilator；
   - left/right flaperon/aileron；
   - rudder；
   - 未來 LEF/TEF。
3. ARI、差動 stabilator、flaperon 與 rudder 的混控關係。
4. 每個控制面的 position limit、rate limit、lag 與非對稱行程。
5. actuator command、actual position、aerodynamic input 與 draw argument 的單一資料來源。
6. actual surface feedback 如何回到 FLCC anti-windup、monitoring 與未來 fault logic。
7. 哪些數值可採用 AFTI/F-16 或 F/A-18 參考，哪些只能標為 `Project-defined`。

### 7.3 工程範圍原則

- 只重建 electronic surface mixer：可以限制在 FLCC，但不能宣稱完成控制面模型。
- 建立左右獨立控制面：必須同時修改 FLCC contract、Actuation、Aerodynamics、FrameOutput/DrawArgs 與 feedback。
- 每片控制面獨立的一階 lag、position/rate limits 屬於可接受的第二部分中等擬真目標。
- hydraulic pressure、external hinge load、servo-valve、二階液壓模型、冗餘與故障屬於後續高擬真目標，除非找到足夠 F-CK-1C/F-16 資料。
- DCS 模型動畫必須由 actual surface state 驅動，不得由 pilot input 或未完成的 command demand 直接驅動。

## 8. 待討論決策清單

### D-01：DCSBridge stateful input Adapter

- 已選定：固定三輸入／一輸出的 `VirtualAxisBinding` 資料列，共用一套 stateful algorithm，輸出 DCS-neutral sampled pilot state。
- 已選定：keyboard virtual axis、slew、return 與 DCS source state 由 DCSBridge 維護；Core 只收 semantic pilot state。
- 已選定：keyboard press 取得 authority；analog value 只有在實際明顯位移時取回 authority；不使用 event order 或 timeout。
- 待決定：Adapter 的最小 public interface 與依 `dt` 推進的擁有者。

### D-02：F-16 pilot command shaping/filtering 拓撲

- AFTI/F-16 已證實 lateral normal law 的 roll-rate command prefilter 具有「停止 roll 比開始 roll 更快」的非對稱 response，並另有 roll-stick/pedal shaping/filtering、feedback/interconnects 與 structural filters。
- AFTI/F-16 已證實 stick feed-forward gain `N1` 由 altitude/Mach scheduling；不能將整個 command path 說成單一低通。
- F-16XL 已證實 force-stick input 先通過 stick-shaping function 才用於頻率響應分析，但公開報告沒有給公式或係數。
- 待決定：pitch 是否需要獨立 command prefilter；目前公開 F-16XL report 沒有足夠公式。
- 待決定：32 Hz roll stick/pedal shaping/filtering 各 block 的函數；不能從「有 filtering」推論為目前的一階 low-pass。
- filter 應作用於 raw pilot demand、command-gradient output 或 physical reference？
- 是否需要 explicit prefilter；若需要，目標 bandwidth、phase lag 和 PIO criteria 是什麼？

### D-03：配平控制律

- 已證實：Figure 1-42 顯示 `PITCH TRIM -> MECH LIMIT` 與 `HORIZ TAIL STICK FORCE` 在 `STICK LIMIT` 前合併；`AUTOPILOT PITCH` 與 `MANUAL PITCH OVERRIDE` 也在 pitch-integrator 上游的 command path 合併。不能再描述成 trim 直接輸入 integrator。
- 已證實：sensor-derived `STABILITY/COMMAND AUGMENTATION` 有兩條輸出；一條進入 pitch-integrator 前的 command summing node，另一條在 pitch-integrator selector 後進入 horizontal-tail output summing node。正常 stick command 沒有直接進入該 block。公開手冊未定義兩條 contribution 的方程、state unit 或 gain。
- 已證實：AFTI pitch-rate command structures 可具有 attitude-hold/auto-trim，且不同 control structures 的 surface trim positions 不同；不平順切換會被飛行員感覺為擾動。
- 待決定：cruise pitch trim 的 equilibrium/reference mapping 與正負 authority。
- 待決定：landing pitch trim 如何進入 pitch-rate/AOA mixed law。
- trim hat 的速率、保持、重置與 wheel-spin/weight-on-wheels 行為。
- roll trim 與 ARI、rudder trim 的完整訊號鏈。

### D-04：CAT parameter model

- 已選定：CAT 只是 selector；同一套主控制律讀取被選中的 immutable `CatConfigurationProfile`。CAT 不修改 raw input、profile 或演算法，也不是獨立 controller mode。
- profile 範圍：有證據的 AOA/G curves、roll-rate schedules、rudder authority、command authority、onset schedules，以及文件明示的 gains/interconnect constants。
- F-16XL 公開資料只量化 CAT III 使最大可命令 roll rate 降低 75 deg/s；沒有完整 F-16XL profile tables。
- 已選定：不在 CAT I/III 間 interpolation 或 mutation。selector 改變後讀取另一份完整 profile。
- 已選定：CAT I/III profiles 於 FLCC 建立時一次建構與驗證；`STORES CONFIG` switch 在 runtime 只選取 const profile reference，不觸發檔案 I/O 或 profile reconstruction。
- 已選定：profile selection 立即生效；不先發明 smoothing、interpolation 或隱藏的 bumpless transition。controller state 不因 CAT switch 被靜默重置；若測試顯示不連續，保留 telemetry 後另以證據決定真正的 state-transfer requirement。
- 尚待實作前選定：第一版採 typed C++ static data，或使用外部靜態設定檔。推薦先採 typed static data；公開資料未提供的 profile 數值一律標示 `Project-defined`。

### D-05：Longitudinal control-law equations

- 已證實但不合併：F-16XL longitudinal law 是 blended pitch-rate `q`／normal-acceleration `Nz` command；F-16C/D production Cruise Gains 則由操作手冊描述為低 AOA G-command、高 AOA blended G/AOA command。兩者是不同 airframe/control-law evidence，不能拼成一條虛構 law。
- 已選定：本專案的兩個正常 gain regimes 依 production-digital F-16C/D observable behavior；F-16XL 用於 digital execution、handling qualities 與可相容的內部拓撲證據，不覆寫 F-16C/D command interpretation。
- 已證實：AFTI/F-16 longitudinal laws 使用 stick feed-forward、main feedback、gain scheduling、integrator 與限制／防失速功能；normal control structure 以 gear-up G-command、gear-down pitch-rate command 為基礎。
- 已證實：AFTI/F-16 initial longitudinal design 使用 LQS，`N1` stick feed-forward gain 隨 altitude/Mach 排程。這不是一個單一 PID controller。
- 需由 evidence ledger 固定：stick gradient、filtered Nz、washed q、forward integrator、alpha feedback 與 limiter 的實際拓撲；每段都要區分直接 F-16 資料與 F-16-derived simulator 資料。
- exact gains、blend weights、breakpoints 若未公開，標為 `Project-defined` 並由測試調教；不得改變來源支持的拓撲。
- Mach、dynamic pressure、gear 與 AOA 對 gain/limiter 的排程。
- 若 F-16XL 特殊 airframe behavior 與 production F-16C/D 衝突，command interpretation 以 F-16C/D 為準；只有 production manual 未公開的內部細節才使用 F-16XL/AFTI 並保留證據標籤。

### D-06：Takeoff/landing mixed law

- F-16A/B 與 F-16C/D flight manuals 的正式名稱是 `Cruise Gains`、`Takeoff and Landing Gains` 與故障時的 `Standby Gains`；不是本專案先前使用的 `UpAndAway` 正式 mode name。F-16XL report 的 `up-and-away configuration` 是測試構型用語。
- 本階段只實作兩個正常 gain regimes：`Cruise` 與 `TakeoffAndLanding`。`Standby` 保留未來故障／air-data reversion 擴充位置，但不在本階段實作。
- 這兩個 gain regimes 不是兩套完整且互不相干的 end-to-end algorithms。它們在同一 FLCS pipeline 中選擇 command interpretation、gain sets 與 limiter schedules；AOA/G protection、ARI、yaw-rate limiter、air-data scheduling 等各自仍有獨立 condition/schedule。
- gain-regime、stores selection、autopilot authority、continuous schedules 與 guarded overlays 的完整分類以第 4.7 節為準；不得把它們合成 Cruise/CAT/AP 等條件的巨大笛卡兒積 mode。
- 現況不是上述兩個完整 gain regimes：程式只有 `NormalAccelerationCommand` 與 `PitchRateCommand` 兩種 command variant，以 landing-gear handle 二選一；沒有 typed `FlightControlGainRegime`，也沒有完整 gain-regime selector。
- 現況 gear-up path 是線性 stick-to-G、project-defined Nz PI、q washout、static alpha feedback 與簡化 G/AOA protection；不是已證實的完整 F-16XL q/Nz blended law。
- 現況 gear-down path 是簡化 pitch-rate PI；高 AOA 時只逐漸削減正 pitch-rate command 並加入 alpha feedback，沒有建立文件描述的 pitch-rate/AOA command blending。
- F-16C/D production-digital reference selection：landing gear handle DOWN，或在 airspeed below 400 knots 時 alternate flaps EXTEND／air-refuel door OPEN，即選 `TakeoffAndLanding`；其他正常情況選 `Cruise`。
- 已證實 `TakeoffAndLanding` behavior：10° AOA 以下 pitch-rate command；10° AOA 以上逐漸混入 AOA command。該 gain regime 的 roll-rate limit 不依 CAT、AOA、airspeed 或 horizontal-tail position 改變。
- 已證實 `Cruise` behavior：低 AOA 為 G-command；AOA 增加時轉為 blended G/AOA command。roll-rate authority 隨 airspeed、AOA、horizontal-tail position 等 schedule。
- 架構討論必須同時涵蓋兩個 gain regimes；實作依序完成 shared pipeline、Cruise、TakeoffAndLanding、最後的 regime-transition verification，不能複製兩套完整 FLCS。
- 待決定：專案目前缺少的 alternate-flaps 與 air-refuel-door typed signals 應從哪個 aircraft system 提供。
- 待決定：公開資料沒有給出 Cruise G/AOA 與 TakeoffAndLanding 10° 以上 q/AOA 的 exact blend equations/weights；需設計連續、可測的 `Project-defined` schedules。
- F-16C/D manual 已提供 trim observable behavior：Cruise 中 trim 是 G-command，centered trim/no stick 目標為 1 g，full nose-up/down 約為 +3.4 g／-1.4 g；TakeoffAndLanding 中 zero pitch trim 在 10° AOA 以下命令 zero pitch rate。這些行為列為 target，內部 integrator equation 仍未公開。
- 待決定：公開 F-16XL 資料沒有給 mode-transition state initialization；需採用可追溯的 bumpless state matching，並明確標示 comparable/project-defined 部分。

### D-07：Integrator 與 anti-windup

- 已證實：F-16/AFTI 結構具有 pitch integrator、forward-loop integrator 與多條 feedback path；integrator 並不等於整套 controller 是 PID。
- F-16A/B Figure 1-42 的類比 functional schematic 只顯示一個 `PITCH INTEGRATOR`，位於 pilot/trim/autopilot command path；它沒有顯示 Cruise 與 TakeoffAndLanding 各自擁有一個 integrator，但也不能據此斷言 production-digital F-16C/D 只有一個軟體 integrator state。
- 現況 `normal_acceleration_integral_effort` 與 `pitch_rate_integral_effort` 兩套獨立 state，再於 command variant 切換時互相 pre-load，缺少直接 production-digital F-16 證據，列為必須重新設計的內容。
- integrator 數量與 state ownership 尚未決定。決策必須依 F-16C/D observable law、F-16XL/AFTI digital topology 與 regime-transition closed-loop tests；不得因類比圖或現有程式結構預設「恰好一個」或「每個 regime 一個」。
- 已證實：AFTI/F-16 不同 channel 的 integrator initialization 不一致曾造成 output divergence；mode/state transition 必須管理 integrator state。
- 已證實：AFTI 的 forward integrator 用於 steady-state decoupling；其職責不能與一般 raw-input smoothing 混為一談。
- 待決定：哪些 production-target loops 保留 integrator，哪些只屬 AFTI decoupled/research structures 而不納入。
- integrator error 的物理量與單位。
- electronic limit、surface position limit、rate limit 與 tracking error 的 anti-windup 條件。

### D-08：多頻率 Module 的精確歸屬

- 維持已證實的 64/32/4 Hz 功能分類。
- 根因不是 scheduler 本身，而是 Module 責任分類與演算法錯置：正確頻率上的錯誤 generic filter 仍然是錯誤控制律。
- 依目前決定，先完成 command/protection/feedback/integrator 的邏輯與功能定義，再整理 Module 歸屬；不得先搬檔案或重命名來假裝控制律已修復。
- DCSBridge virtual-axis dynamics 依 DCS simulation `dt` 推進；不占用 FLCC 32 Hz shaping 的責任。
- raw pilot demand 保持 immutable；longitudinal law、lateral/directional feedback 與 interconnects 使用 64 Hz；有證據的 roll/pedal shaping 使用 32 Hz；slow air-data gain schedule 使用 4 Hz。
- 確認每個新 block 的訊號頻寬、取樣保持與跨 subrate 資料一致性。
- 判斷 F-16XL Block 40 DFLCS 與 AFTI/F-16 subrate 設計的差異是否需要反映。

### D-09：第二部分 surface allocation

- virtual channels 或 individual surfaces。
- allocation 的輸入是 axis effort、desired angular acceleration、moment command 或其他物理量。
- 控制分配在 FLCC 內的 interface 與 ActuationSystem 的 interface。
- 已證實的可比架構：AFTI/F-16 使用七個 integrated servoactuators；FLCC 以 electrical command 驅動 electrohydraulic servovalves，hydraulic power ram 移動控制面，並有 mechanical position/rate feedback。這證明 FBW 不等於 electric-motor actuation。
- F-16XL 報告提供 aileron/elevon/LEF/rudder 的行程與 rate limits，但該機翼型與控制面配置不同於 F-CK-1C；只能作為數值範圍與模型結構參考，不能直接成為 F-CK-1C configuration。
- 目前沒有足夠公開資料證明 F-CK-1C 使用相同 ISA、伺服閥配置或通訊協定；第一部分只能建立技術中立的 command/feedback seam。

### D-10：驗證標準

- 完成判定採三道且必須同時通過的 gate：
  1. 所有 native automated tests 通過後，才可要求 DCS 驗證。
  2. `$code-review` 的 Standards 與 Spec 兩軸均確認 OKR/KR、專案規範與 traceability matrix 完整。
  3. 使用者依固定條件完成 DCS hands-on validation，且 CSV/debug evidence 能解釋結果。
- native tests 至少涵蓋：input binding 全表、符號、press/release、analog/keyboard equivalence、frame-interval independence、source authority；raw/trim independence；reference/CAT schedule points；64/32/4 Hz ZOH；三個 landing selectors 與 10° AOA transition；open-loop block response；closed-loop step/doublet/ramp/reversal；limiter priority；integrator recovery；deterministic replay；legacy path absence。
- code review 必須產出 `Requirement -> Source -> Module -> Test -> DCS case` 追溯矩陣，不能只確認測試為綠色。
- DCS 測試必須固定重量、重心、速度、高度、CAT、mode、輸入波形與觀察欄位；其結論是本專案 acceptance，不宣稱法規或原廠飛行認證。

## 9. 第一部分完成條件

第一部分只有在下列條件全部成立時才算完成：

1. raw pilot demand 在 FLCC 內可追蹤且不被 CAT/input filtering 覆寫。
2. trim 不再與 raw stick 簡單相加後共用 shaping。
3. `Cruise Gains` 與 `Takeoff and Landing Gains` 各有來源可追溯的 command interpretation、gain schedules 與 limiter behavior，並共用同一 FLCS pipeline。
4. CAT 只選擇 immutable parameter profile；同一套演算法讀取該 profile，CAT 不修改 profile、演算法或 raw pilot demand。
5. G/AOA/roll/yaw protections 具有 requested/effective diagnostics。
6. alpha feedback、q feedback、Nz feedback、integrator 與 anti-windup 各有唯一責任與測試。
7. 64/32/4 Hz 更新率符合本文件的證據分類，跨頻率資料行為有測試。
8. 被取代的 legacy control paths 已移除，沒有雙重控制律或 fallback。
9. Native tests、architecture checks 與 Release build 通過後，`$code-review` 以 Standards／Spec 兩軸和完整追溯矩陣確認所有 OKR/KR 達成。
10. 依固定測試條件完成使用者 DCS 實際驗證，結果能由 `debug.csv` 和 flight-state CSV 解釋；這是最後一道必要 gate。

## 10. 暫不納入本計畫的內容

- 精確重現未公開的 F-CK-1C FLCC 係數。
- 多部冗餘 FLCC、voting、failure management 與 reversionary law。
- 完整液壓管路、servo-valve 與負載相關 actuator model。
- 控制面戰損、卡死與左右不對稱故障。
- 新增自動油門；既有功能維持 `Developer-only`／非擬真隔離。
- 擴增新的 AP mode；既有 AP reference 必須改走新控制律，但 AP 面板／模式設計不是第一部分主目標。

## 11. 主要參考資料

1. [NASA TP-2857 — Development and Flight Test Experiences With a Flight-Crucial Digital Control System](https://ntrs.nasa.gov/api/citations/19890014956/downloads/19890014956.pdf)
2. [NASA — Flight Test Results for the F-16XL With a Digital Flight Control System](https://ntrs.nasa.gov/api/citations/20040040334/downloads/20040040334.pdf)
3. [F-16A/B Flight Manual, Blocks 10 and 15](https://www.aahs-online.org/resources/e-library/fm/USAF-F16A_B-Flight-Manual.pdf)
4. [NASA — Simulator Study of an Automatic High-AOA Control System](https://ntrs.nasa.gov/api/citations/19760017178/downloads/19760017178.pdf)
5. [NASA — Loss-of-Control-Inhibitor Systems for Aircraft](https://ntrs.nasa.gov/citations/20100011189)
6. [NASA — Design and Development Experience With a Digital Fly-by-Wire Flight-Control System](https://ntrs.nasa.gov/api/citations/19760024053/downloads/19760024053.pdf)
7. [NASA — F/A-18 HARV Simulation Model](https://ntrs.nasa.gov/api/citations/19920024293/downloads/19920024293.pdf?attachment=true)
8. [NASA — F-16 AFTI Control Allocation Example](https://ntrs.nasa.gov/api/citations/20240014917/downloads/ControlAllocationAnalysis_LJM_V4.pdf)
9. [NASA — AFTI/F-16 Digital Flight-Control-System Design Description](https://ntrs.nasa.gov/api/citations/19820024503/downloads/19820024503.pdf)
10. [T.O. GR1F-16CJ-1 — F-16C/D Block 50/52+ Flight Manual public mirror](https://studylib.net/doc/26036763/block-50)
11. [AIDC — F-CK-1C/D 32-bit Digital Flight Control Computer](https://www.aidc.com.tw/en/military/fck1)
9. [NASA — F/A-18 701E Research Flight-Control System Architecture](https://ntrs.nasa.gov/api/citations/19980007172/downloads/19980007172.pdf)
10. [NASA — F/A-18 HARV Flight-Control and Actuation Reference](https://ntrs.nasa.gov/api/citations/19900019226/downloads/19900019226.pdf?attachment=true)
11. [F-16C/D Flight Manual, Blocks 50/52+ public mirror — T.O. GR1F-16CJ-1](https://studylib.net/doc/26036763/block-50)
