# F-CK-1C EFM Core 架構計畫

## 文件狀態

本文件描述 F-CK-1C EFM Core 的目標責任、依賴方向、執行模型與檔案
結構。它是架構討論的決策紀錄，不是檔案搬移順序或重構施工清單。

文件中的內容分為：

- **已確認**：目前討論已達成共識，後續實作應遵守。
- **待討論**：架構需要，但具體實作方式尚未決定。
- **暫不實作**：保留未來能力，但目前不增加程式碼。

## 目標

- 符合 SOLID、DRY、關注點分離與 YAGNI。
- 對 DcsBridge 提供簡單且穩定的 Core Interface。
- 新增 System 時，不需要修改 `Fck1cEfm` 或既有 System。
- 飛機設備邏輯與模擬物理有明確維修位置。
- 所有連續狀態只由 `step()` 推進。
- 每幀結果完整完成後才對外發布。
- 系統間通訊可擴充，但不直接模擬大量實體線束。
- Core 保持同步、確定性，並使用 DCS 提供的同一個 `dt`。

## 非目標

- 本階段不實作 Lua 座艙 device 或互動儀表板。
- 本階段不實作 cockpit `unavailable` 狀態。
- 不建立通用、可任意註冊的物理 Model 框架。
- 不模擬線束的電氣特性、傳播延遲或訊號品質。
- 不在本文件決定實際搬移順序或 commit 切分。

## 整體責任

```text
DCS
└── DcsBridge
    └── Fck1cEfm
        └── AircraftSimulation
            ├── SystemPipeline
            │   ├── Control Systems
            │   ├── Equipment Systems
            │   └── AircraftData
            │
            └── SimulationPipeline
                ├── AerodynamicsModel
                ├── PropulsionModel
                ├── GroundInteractionModel
                └── MassPropertiesModel
```

### DcsBridge

**已確認**

DcsBridge 是 DCS EFM C ABI 與 DCS-neutral Core 之間的 Adapter，負責：

- DCS callback 與生命週期協調。
- DCS command ID、damage ID 與事件格式轉換。
- 輸入驗證。
- DCS 座標、方向與單位轉換。
- 收集非同步 callback 的最新輸入。
- 發布完成的 Core frame 結果。
- C ABI exception seam、記錄、CSV 與 runtime 工具。

DcsBridge 不得：

- 包含具體 System。
- 呼叫空氣動力、推力或地面力公式。
- 讀取 Core 內部狀態。
- 保存飛機設備的權威狀態。

### Fck1cEfm

**已確認**

`Fck1cEfm` 是 Core 對外唯一入口。它的 Interface 保持小型且穩定，不建立
獨立 `Interface/` 目錄。

`Fck1cEfm`：

- 不知道有哪些具體 System。
- 不負責 System 執行順序。
- 不直接整理各 System 的輸出。
- 長期存在於 DcsBridge 的 process-lifetime context。
- 持有目前飛行的 `AircraftSimulation`。

### Core Contracts

**已確認**

`Core/Contracts/` 集中 Core 各 Module 共同使用的純資料契約與具名常數：

- `Commands.h`：semantic `CommandId` 與 `Command`。
- `Events.h`：`DamageEvent`、`RepairEvent` 等離散事件。
- `AircraftData.h`：明確列出的共享飛機狀態型別、具名 Data ID 與 Data
  Key；本階段不建立可在執行期任意新增型別的通用 registry。
- `FrameContracts.h`：Core 與 DcsBridge 交換的 frame input/output 型別。

Contracts 只包含 immutable 型別宣告、enum 與 `inline constexpr` key，不
保存 runtime state，也不包含執行流程。

不得在 Contracts 放置：

- DCS raw numeric ID。
- System 私有狀態。
- 氣動、引擎或控制律參數表。
- 模擬公式與 helper implementation。
- mutable global 或 singleton。
- 無明確 owner 的通用 `Constants.h`。

`Contracts/` 不是額外的公開 Interface 層。DcsBridge 仍以
`Fck1cEfm.h` 作為 Core 入口；Contracts 只是 Systems、Simulation 與
入口共同使用的穩定語言。

新增只使用既有資料的 System 不修改 Contracts；只有新增真正的跨
Module 飛機概念時才擴充 AircraftData schema。這是共享語言的明確變更，
不是對其他具體 System 的依賴。

### AircraftSimulation

**已確認**

`AircraftSimulation` 代表一次飛行，負責：

- 建立與銷毀該次飛行的所有狀態。
- 接收一幀完整的 Core `FrameInput`。
- 將 DCS-owned 狀態提供給該幀的模擬。
- 先執行 `SystemPipeline`，再執行 `SimulationPipeline`。
- 保存模擬政策，例如無限燃料、無敵與簡易飛行。
- 統一整理並回傳 `FrameOutput`。

`AircraftSimulation` 是深模組。下列概念先作為其私有實作，不預先拆成
根目錄 Module：

- Simulation policy storage。
- Frame output projection。
- Flight setup helper。
- 完成幀快照。

## 飛行生命週期

**已確認**

- DcsBridge 與 `Fck1cEfm` 可以在 process lifetime 內持續存在。
- 每次 cold、hot ground 或 hot air start 都建立新的
  `AircraftSimulation`。
- `ed_fm_release` 對外 callback 必須保留。
- `release()` 銷毀目前的 `AircraftSimulation`，而不是逐一重設所有
  System。
- 個別 System 不需要 `release()`；其資源隨物件析構釋放。
- command 可以在兩幀之間送入，但不會立即發布新的 frame 狀態。

### 飛行前準備狀態

`release()` 之後、下一次 `start()` 之前，DCS 仍可能設定燃油與模擬選項。
因此 `Fck1cEfm` 私有持有一份小型 `FlightPreparation`，而不是把這些資料
留在已銷毀的 `AircraftSimulation`。

- `FlightPreparation` 只保存下一次飛行需要的內部燃油、外部油箱與模擬
  選項。
- `start()` 使用它建立新的 `AircraftSimulation`。
- 飛行中收到的相同設定同時更新準備狀態與目前模擬。
- 重新開始或釋放前，需延續的燃油與設定同步回準備狀態。
- frame 暫態、力、力矩與單次質量變化不得跨飛行保留。
- 無 active flight 時，普通 command 與 damage/repair event 不保留到
  下一次飛行；只有明列的 preparation setters 可以跨越此邊界。

## System 模型

### 執行組別

**已確認**

System 依執行角色分成：

1. `Control`
2. `Equipment`

執行順序固定為：

```text
Control group
→ 統一提交結果
→ Equipment group
→ 統一提交結果
```

同一組的所有 System 讀取同一份輸入快照。同組執行期間看不到其他
System 尚未提交的結果。這使 System 的結果不依賴同組迭代順序，也保留
類似實體訊號傳遞的一幀延遲。

這項規則會讓目前依賴同幀呼叫順序的路徑產生可觀察變化，例如
`LandingGear → SecondaryFlightControls` 與 `Engine → Fuel`。它們必須
集中在一個明確的「切換分組提交語意」變更中，先以多幀測試記錄舊行為，
再只接受已確認的一幀延遲，不得在搬檔或抽取類別時順便改變。

組別只屬於排程 metadata：

- 不放進資料 key。
- 不放進外部查詢路徑。
- 不用作 `Systems/Control/` 或 `Systems/Equipment/` 資料夾。

### System 獨立性

**已確認**

- System 不直接呼叫或持有其他 System。
- System 不查詢其他 System 是否存在。
- System 只依賴所需資料是否已註冊與初始化。
- System 不在乎資料由哪個 System 產生。
- System 可保留自己的演算法私有狀態。
- 需要跨 System、Simulation 或未來儀表讀取的狀態，使用統一的
  AircraftData 管道。

### System Interface

**部分確認**

所有 System 使用類別，並至少具有：

- 一次性的 `setup()`。
- 唯一的連續狀態推進函式 `step()`。

Entry factory 接收 immutable `FlightSetupContext`，其中只有 Core
`StartMode` 與建立該次飛行所需的共同資料。`setup()` 用於宣告：

- 需要讀取的資料。
- 發布的資料。
- 接受的 command。
- 接受的 event。
- 必要資料的初始要求與自身初始發布值。

`step()`：

- 讀取該組固定快照。
- 更新自己的私有狀態。
- 回傳待提交的 `SystemResult`。
- 不直接改寫共享資料。

System 不需要通用 `release()`。

System 會在 `setup()` 將自己擁有的專用 command 或 event handler 註冊給
`SystemPipeline`。Pipeline 不將所有 command 廣播給所有 System，也不
要求每個 System 實作中央 switch。handler 只能改變離散狀態、模式或下
一幀使用的 requested state；連續狀態仍由 `step()` 推進。

handler 使用 `std::function`、member function pointer 或其他 type-erased
表示方式，留待實作時決定。

### Command

**已確認**

- command 不跟隨 `step()`。
- DcsBridge 將原始 DCS command 轉換成 Core semantic command。
- DCS 數字 ID 只存在於 DcsBridge；System 不得註冊或比較 DCS magic
  number。
- Core 使用具名且強型別的 semantic Command ID。
- `SystemPipeline` 根據 System 在 setup 的註冊，將 command 送給唯一
  接收者。
- 同一個 Command ID 重複註冊視為 setup 錯誤。
- command 只改變開關、模式、目標或請求狀態。
- 連續設備狀態仍只在下一次 `step()` 推進。
- command 模型保持簡單，不建立複雜訊息匯流排。

### Event 與 Damage

**已確認**

- Damage 不是飛機部件，因此不建立 `DamageSystem`。
- DcsBridge 將 DCS damage area 轉換為 semantic `DamageEvent`。
- 無敵政策由 `AircraftSimulation` 在事件進入 System 前處理。
- 實際部件擁有自己的 integrity、condition 與 failure 狀態。
- 機翼、尾翼與機體狀態由 `AirframeStructure` 持有。
- 引擎損壞由 `Engine` 持有。
- 起落架損壞由 `LandingGear` 持有；Core 使用單一 semantic
  `DamageArea::LandingGear`，並以 nose、left main、right main 三個 segment
  表示 DCS 的 `WHEEL_F`、`WHEEL_L`、`WHEEL_R`。
- 起落架 integrity 只影響其 owner 的設備能力：鼻輪按比例縮放 NWS，左右
  主輪分別按比例縮放對應煞車；本階段不模擬爆胎、支柱折斷或收放卡死。
- `RepairEvent` 將三個起落架 segment 恢復為完整 integrity。
- `DamageEvent` 依 semantic area 或 component 路由到唯一 owner；raw
  damage ID 不得進入 Core。
- `RepairEvent` 可以由多個可修復部件訂閱。Event 與 Command 不共用
  「唯一接收者」規則。

目前無敵狀態仍會先寫入 segment damage，只是不立即 refresh aggregate
integrity；之後可能形成潛伏損壞。目標行為是在事件進入 owner 前就忽略，
此差異必須在 damage ownership commit 中單獨測試與修正。

目前不建立額外 `DamageResolver` Module；只有在出現實際的損壞傳播或
多部件解析邏輯時才拆出。

## SystemPipeline

**已確認**

`SystemPipeline` 是 Systems 的深模組，負責隱藏：

- System instance ownership。
- System catalog 載入。
- setup 註冊與驗證。
- Control、Equipment 分組。
- AircraftData 儲存。
- 每組讀取快照。
- `SystemResult` 批次提交。
- command 路由。
- event 路由。

因此目前不建立下列獨立 Module：

- `SystemRegistry`
- `CommandRouter`
- `EventRouter`
- `AircraftDataStore`
- `SystemResult` 專用檔案
- `SystemContext` 專用檔案

`AircraftSimulation` 建立時會配置該次飛行共用的 pending result
storage。所有 System 使用此 storage 中互不重疊的區域，結果在組結束
前同時保留，提交後再重複使用。已提交的 AircraftData 與 pending
results 在邏輯上必須分離，避免同組 System 看到尚未提交的更新。

本階段的 AircraftData 採用明確 typed schema：System 在 `setup()` 宣告
讀取與發布的具名 key，Pipeline 驗證型別、初始化與單一 writer。資料在
幀與幀之間保留最後值；多個來源需要合併時，必須建立一個明確的 owner，
不開放多 writer 或隱式合併。

Setup 必須分成「全部宣告」與「統一驗證/提交初值」兩階段；System 不得
在 setup 時讀取另一個 System 的值或依賴 catalog 順序。完成初始提交後，
`start()` 才能投影第一份完整 FrameOutput。

它對 `AircraftSimulation` 提供的概念 Interface 為：

```text
send(command)
apply(event)
step(frame input) -> completed aircraft snapshot
```

## System 註冊

**已確認 build-time 自動產生，MSBuild 整合待討論**

每個 System 目錄提供一個 `Entry.cpp`，作為唯一整合入口。Entry 宣告：

- 穩定 System ID。
- 執行組別。
- System factory。

System 自己在 `setup()` 註冊資料、command 與 event。

目標是讓新增 System 時不修改：

- `Fck1cEfm`
- `AircraftSimulation`
- `SystemPipeline`
- 其他既有 System

因 C++ DLL 執行時不存在原始碼目錄，不能在 runtime 掃描
`Systems/*/Entry.cpp`。建置流程必須自動掃描 Entry，並在編譯前建立
固定 catalog。產生檔應位於明確的 generated 或 intermediate 位置；
確切 MSBuild 掛載方式仍待決定。

## 目標 Systems

| System | Group | 責任 |
|---|---|---|
| `FlightControlComputer` | Control | 飛行員輸入、trim、FBW、autopilot demand、控制需求 |
| `PrimaryFlightControls` | Equipment | 升降舵、副翼、方向舵實際位置與設備狀態 |
| `SecondaryFlightControls` | Equipment | 襟翼、前緣縫翼、減速板實際位置與設備狀態 |
| `Engine` | Equipment | 引擎開關、spool、燃燒、噴嘴、燃油流量、condition |
| `Fuel` | Equipment | 油箱、油量、供油與燃油轉移 |
| `LandingGear` | Equipment | 起落架、煞車、鼻輪、輪胎、避震器設備狀態 |
| `AirframeStructure` | Equipment | 機翼、尾翼與機體結構完整度 |

目前責任對應：

| 現有內容 | 目標 |
|---|---|
| `InputSystem` 的 Core 邏輯 | `FlightControlComputer` |
| `FBWController*` | `FlightControlComputer` |
| `ControlSurfaceState` | `PrimaryFlightControls` |
| `AirframeDeviceSystem` | `SecondaryFlightControls` |
| `EngineSystem` 的設備部分 | `Engine` |
| `FuelSystem` 的設備部分 | `Fuel` |
| `LandingGearSystem` 與 suspension 設備狀態 | `LandingGear` |
| `DamageModel` 的結構狀態 | 實際擁有部件 |
| `StartupSystem` | `AircraftSimulation` lifecycle |
| `AerodynamicsSystem` | Simulation Model |
| fallback ground forces | `GroundInteractionModel` |

## Simulation 模型

### SimulationPipeline

**已確認**

`SimulationPipeline` 明確知道且執行固定物理順序。物理 Model 不使用
System 的自動註冊方式，因為：

- 執行順序高度重要。
- Model 數量少。
- Model 不會頻繁新增。
- 明確列出順序較容易理解與驗證。

目前的物理順序概念為：

```text
Aerodynamics
→ Propulsion
→ GroundInteraction
→ MassProperties
→ 統一模擬結果
```

下列概念先留在 `SimulationPipeline` 私有實作：

- Force accumulator。
- 僅在該幀有效的 model context。
- Model result aggregation。
- Simulation result projection。

只有當其實作足夠複雜並形成真正 seam 時才拆出。

每個 Model 讀取完成的 aircraft snapshot、DCS observations，以及由
Pipeline 明確提供的較早 Model 結果。Model 不互相呼叫。每個機輪每幀
只能選擇一個地面力來源：DCS suspension feedback 或
`GroundInteractionModel` fallback，不得同時累加。

目前 fallback 不會因 suspension feedback 自動停用，因此此規則不是單純
搬檔；必須在抽取 `GroundInteractionModel` 時以獨立 commit 與
before/after 測試明確修改。

### 飛機狀態與模擬效果

**已確認**

System 回答：

> 飛機與設備目前處於什麼狀態？

Simulation Model 回答：

> 這些狀態在目前 DCS 世界狀態下產生什麼模擬效果？

例子：

- `Engine` 保存 spool、燃燒、噴嘴與 condition。
- `PropulsionModel` 根據 Engine 狀態、空氣狀態與飛機定義計算推力。
- `LandingGear` 保存輪胎、避震器與煞車狀態。
- `GroundInteractionModel` 計算需要回傳 DCS 的地面力。
- `Fuel` 保存油量與分布。
- `MassPropertiesModel` 計算需要回傳 DCS 的質量、重心與慣量變化。

物理 Model 不直接改寫 System 狀態。DCS 整合力與力矩後，新的位置、
姿態與速度於下一幀重新輸入 Core。

### Configuration ownership

**已確認**

- 各 System 擁有自己的 Config 型別、資料與驗證。
- 各 Simulation Model 擁有自己的 Config、參數表與驗證。
- owner 內部、不需要外部調校的演算法數值可以留在其實作中，但必須以
  鄰近註解說明其物理、調校或 legacy 行為意義；不為了形式一致而全部搬入
  Config。
- 多個 Simulation Model 共用的機體幾何才放入
  `Simulation/Definition/`。
- 最終不保留依賴具體 System 的全域 `Data::AircraftConfig` 聚合袋。
- production composition 留在 `AircraftSimulation` 的 setup/catalog
  路徑；DcsBridge 與 `Fck1cEfm` 不組裝具體 System。

## DCS 與 Core 的狀態所有權

**已確認**

DCS 擁有：

- 世界位置與姿態。
- 世界與機體速度。
- 加速度與角速度。
- 大氣與地表輸入。
- DCS rigid-body integration 的結果。

Core 擁有：

- 飛機設備狀態。
- 控制與設備需求。
- 設備故障及完整度。
- 對目前 DCS 狀態應產生的力、力矩與質量效果。

DcsBridge 負責將 DCS 資料轉換成 Core 使用的統一單位與座標。Core 不
使用 DCS ID、DCS header type 或 DCS 特有座標規則。

System 讀取 DCS-owned observation 時，使用與其他 AircraftData 相同的
typed data reader。`AircraftSimulation` 在每幀開始時更新保留的
observation channels；資料來源不會出現在 System 的查詢路徑中。
所有 System group 在該幀都讀取這份最新 DCS-owned observation；group
snapshot 延遲只套用於 System 在同幀產生的資料，不讓外部 observation
額外延遲一幀。因此高速開始時，`LandingGear` 會在第一幀立即依目前速度
關閉 NWS，不保留舊 baseline 的單幀轉向暫態。
Observation 在飛行開始時由 start mode 與明確的 neutral 值完整初始化；
本幀沒有新 sample 時保留最後值。只有資料 provider/channel 根本沒有
註冊時才 setup 失敗，單幀缺少 callback sample 不算錯誤。
Suspension feedback 的 sample 數值同樣保留，但其 availability 表示
「本幀收到」並在建立 frame snapshot 後消耗，讓每個機輪能在下一幀沒有
新 feedback 時改用 fallback，而不把舊 sample 誤認為本幀來源。

## 每幀資料流

**已確認**

```text
1. DcsBridge 收集並轉換最新 DCS callback 輸入
2. ed_fm_simulate 建立完整 FrameInput
3. Fck1cEfm 將 FrameInput 送入目前 AircraftSimulation
4. AircraftSimulation 更新該幀 DCS-owned observations
5. SystemPipeline 執行 Control group
6. SystemPipeline 統一提交 Control results
7. SystemPipeline 執行 Equipment group
8. SystemPipeline 統一提交 Equipment results
9. SimulationPipeline 讀取完成的 aircraft snapshot
10. Simulation Models 產生力、力矩與質量效果
11. AircraftSimulation 建立一份完成的 FrameOutput
12. DcsBridge 原子發布該完成結果
```

外部 callback 只能看見上一份或新一份完整 frame，不得看見組內提交中
的中間狀態。

Core 在 FrameOutput 回傳該幀的質量效果；DcsBridge 擁有 DCS
`change_mass` callback 的待交付佇列與讀取游標。讀取 callback 不得回頭
清除 Fuel/System 狀態；若 DCS 跨幀未讀取，已完成的質量效果不得遺失，
release 時則明確清空未交付項目。

## 模擬政策

**已確認**

無限燃料、無敵與簡易飛行屬於 Simulation policy，不是飛機 System。

- 無限燃料不移除 `Fuel`，而是阻止消耗效果套用。
- 無敵不移除 integrity 資料，而是在 damage event 進入部件前忽略。
- 簡易飛行不要求 System 知道 DCS 選項；相關輔助由
  `AircraftSimulation` 或適當的 Simulation Model 處理。

## Cockpit 擴充

**已確認**

- 飛機本身邏輯留在 C++ Core。
- Lua device 專注於 cockpit Adapter 與 presentation。
- 未來對 cockpit 輸出的資料，只包含真實飛機儀表能顯示或使用的內容。
- DCS 物理 callback 所需的 `FrameOutput` 不等同於公開 cockpit 狀態。
- 本階段只確保 Core 能在完成 frame 後投影資料，不建立 cockpit
  unavailable 模型。

## 目標檔案結構

```text
Core/
├── README.md
├── Fck1cEfm.h
├── Fck1cEfm.cpp
│
├── Contracts/
│   ├── Commands.h
│   ├── Events.h
│   ├── AircraftData.h
│   └── FrameContracts.h
│
├── Systems/
│   ├── System.h
│   ├── SystemPipeline.h
│   ├── SystemPipeline.cpp
│   ├── FlightControlComputer/
│   │   ├── Entry.cpp
│   │   ├── FlightControlComputerConfig.*
│   │   ├── FlightControlComputer.h
│   │   ├── FlightControlComputer.cpp
│   │   └── ControlLaws.cpp
│   ├── PrimaryFlightControls/
│   │   ├── Entry.cpp
│   │   └── PrimaryFlightControls.*
│   ├── SecondaryFlightControls/
│   │   ├── Entry.cpp
│   │   └── SecondaryFlightControls.*
│   ├── Engine/
│   │   ├── Entry.cpp
│   │   ├── EngineConfig.*
│   │   └── Engine.*
│   ├── Fuel/
│   │   ├── Entry.cpp
│   │   └── Fuel.*
│   ├── LandingGear/
│   │   ├── Entry.cpp
│   │   ├── LandingGearConfig.*
│   │   ├── SuspensionFeedback.h
│   │   └── LandingGear.*
│   └── AirframeStructure/
│       ├── Entry.cpp
│       └── AirframeStructure.*
│
└── Simulation/
    ├── AircraftSimulation.h
    ├── AircraftSimulation.cpp
    ├── AircraftSimulationFactory.h
    ├── AircraftState.h
    ├── ForceMoment.h
    ├── SimulationPipeline.h
    ├── SimulationPipeline.cpp
    └── Models/
        ├── ModelEffect.h
        ├── Aerodynamics/
        │   ├── AerodynamicsConfig.*
        │   └── AerodynamicsModel.*
        ├── Propulsion/
        │   ├── PropulsionConfig.*
        │   └── PropulsionModel.*
        ├── GroundInteraction/
        │   ├── GroundInteractionConfig.*
        │   └── GroundInteractionModel.*
        └── MassProperties/
```

目錄以飛機功能或物理模型分類，不以 class 類型或可見性建立大量
`Interface/`、`Manager/`、`Internal/` 目錄。

`SystemCatalog.g.cpp` 由建置工具產生於 intermediate 目錄，不屬於來源
樹。現階段沒有兩個 Model 共同擁有的幾何，因此不建立空的
`Simulation/Definition/`；未來只有真正跨 Model 的定義才放入該目錄。

## 依賴規則

**已確認**

```text
DcsBridge
  → Fck1cEfm Interface

Fck1cEfm
  → AircraftSimulation

AircraftSimulation
  → Core Contracts
  → SystemPipeline
  → SimulationPipeline

SystemPipeline
  → Core Contracts
  → System Interface
  → System Catalog

Concrete System
  → System Interface
  → Core Contracts
  → Common utilities

Simulation Model
  → Core Contracts
  → completed aircraft snapshot
  → frame observations
  → its own definition and tables
  → Common utilities
```

禁止的依賴：

```text
DcsBridge → Concrete System
DcsBridge → Simulation Model
System → Another concrete System
System → DcsBridge
System → Simulation Model
Simulation Model → Concrete System
Simulation Model → Another concrete Simulation Model
Common → Systems / Simulation / DcsBridge
```

## 完成條件

架構重構完成時，應滿足：

- `Core/` 根目錄只保留 `Fck1cEfm` 入口。
- `Fck1cEfm` header 不包含任何具體 System header。
- DcsBridge 不包含 `Systems/` 或物理 Model header。
- `SystemPipeline` 是唯一管理 System 執行與共享資料提交的位置。
- System 不直接呼叫其他 System。
- 空氣動力、推力、地面力與質量效果都位於 Simulation Models。
- 新增 System 不修改 `Fck1cEfm`、`AircraftSimulation` 或
  `SystemPipeline`。
- 同一輸入與同一 `dt` 產生確定性一致結果。
- command 不會在 `step()` 之外推進連續狀態。
- 外部只會取得完成的 frame snapshot。
- 所有 setup 錯誤明確失敗，不以預設值或靜默 fallback 隱藏。

## 尚待實作時決定的局部細節

下列項目不再改變架構邊界，可在對應施工步驟依測試與效能證據決定：

1. `ISystem` 的確切 C++ 函式簽章與 handler 的 type-erasure 方法。
2. typed key 的 slot 配置與 `SystemResult` 的實體記憶體布局。
3. `Entry.cpp` catalog 的確切 MSBuild 產生與兩個 project 共用方式。
4. setup 與未預期 runtime 例外如何轉成 C ABI 可診斷錯誤。
5. per-frame heap allocation 的實測結果與是否需要進一步消除。

施工切分、行為保留規則與每步驗證方式另見
[`EFM_CORE_REFACTOR_IMPLEMENTATION_PLAN.md`](EFM_CORE_REFACTOR_IMPLEMENTATION_PLAN.md)。
