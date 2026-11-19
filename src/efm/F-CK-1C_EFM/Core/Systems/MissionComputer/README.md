# MissionComputer

MC 是單一 `System`，以專案選用的 32 Hz 運作；組裝沿用 DFLCC 的資料庫先建立、固定參照
與單一 Executive。這是專案設計，並非實機 F-CK-1C MC 的架構復刻。
目前主保險與操作模式各自選擇；目標指定職責及最新流程見 [FireControl](Components/FireControl/README.md)。
目前模式事件、反制職責與驗證限制見[模式事件重構](../../../../../../docs/MC_MODE_EVENTS.md)。
先前元件整併見[重構報告](../../../../../../docs/MC_REFACTOR_REPORT.md)，
遷移經過見[歷史遷移報告](../../../../../../docs/MC_MIGRATION_REPORT.md)。

## 組裝

```text
MissionComputer : System [32 Hz]
├── Database/
│   ├── ExecutiveTables
│   ├── InputOutputData
│   ├── SystemStatusTables
│   ├── MissionData (ModeData / FireControlData)
│   │   └── FireControlData.targeting / weapon
│   ├── StoresData（逐掛點庫存、彈量、選站、IR 觀測與武器執行結果）
│   └── ExternalActionOutbox（功能端操作與回覆）
├── StoresCommandQueue（SM 的單一指令入口）
├── RadarRequestQueue（Targeting 的雷達要求入口）
├── ModeEvents / StoresEvents（MC 專用同步事件通道）
└── Components/
    ├── ModeManagement
    ├── SensorOfInterest
    ├── CountermeasureControl
    ├── StoresManagement
    │   └── Internal/（ObservationProcessing、StationSelection、WeaponExecution）
    ├── FireControl
    │   └── Internal/
    │       ├── Targeting
    │       └── WeaponOperation
    ├── Executive
    ├── StartupAndRestart
    ├── SystemMonitor
    └── FailureManager
```

五個任務元件與四個基礎元件，共九個同層元件。
FireControl 內部模組不是獨立 System、執行緒或 Executive 工作。
MC、FireControl、Executive 與 ExecutiveTables 禁止複製及搬移，維持固定參照。

| 元件 | 職責 |
|---|---|
| ModeManagement | 唯一寫入一般 NAV/A-A/A-G、DGFT/MSL 超控與主保險，發布具型別選擇事件 |
| SensorOfInterest | 讀取 DisplaySnapshot 與任務模式，唯一寫入 SOI 來源及顯示位置，處理 DMS 上下 |
| CountermeasureControl | 接收 CMS 四向指令，前向啟動十輪混合程序，左向程序 6 施放七發干擾絲，後／右保留入口 |
| StoresManagement | 全部固定掛點庫存、彈量、掛點選擇／循環、執行確認與掛載變化通知 |
| FireControl | 目標操作、武器資格與使用程序 |
| Executive | 四個週期工作的固定順序、頻率及時間推進 |
| StartupAndRestart | 原有建構時啟用排程 |
| SystemMonitor | 健康監控預留入口，目前無功能 |
| FailureManager | 故障處置預留入口，目前無功能 |

FireControl 分派操作；MC 依 SOI 分派 TMS，Targeting 解讀雷達 TMS 與操作模式，規劃捕獲策略並保存指定意圖與雷達追蹤結果。
與 MC 並列的 [RadarSystem](../RadarSystem/README.md) 透過 Pipeline 接受要求、推進設備操作與發布觀測。
WeaponOperation 保存解籠與扳機／投放資格，提交使用意圖並讀取 SM 執行結果，供 FC 安排目標接續。
模式及武裝選擇透過 MC 持有的 sigslot 同步事件通知各訂閱者。
StoresManagement 統一負責庫存、選站、武器輸入保持與執行確認，並保存全域武器介面的 IR 來源觀測。
WeaponOperation 只讀 WeaponUseData；Targeting 讀取模式與 RadarSnapshot，透過 RadarRequestQueue 提出要求。
雷達模型、DCS API 及操作映射由外部介面提供；HMCS 與音效 Lua 使用完成快照決定呈現。

## 資料所有權

| 分區 | 唯一寫入者 | 主要讀取者 |
|---|---|---|
| InputOutputData.input | MC 輸入入口及外殼 | 掛載、火控、輸出投影 |
| InputOutputData.input.control_inputs | MC 依命令 FIFO 記錄原始操作 | 輸出投影；HUD/HMCS 的唯讀觀測 |
| InputOutputData.output | MC publish_outputs | Pipeline、Bridge |
| ModeData | ModeManagement | 火控及輸出投影（唯讀） |
| StoresData | StoresManagement | 火控及輸出投影 |
| StoresData.weapon_use | SM 的 WeaponExecution | WeaponOperation 唯讀執行階段及取消／拒絕結果 |
| FireControlData.targeting | Targeting | 輸出投影 |
| FireControlData.weapon | WeaponOperation | 射擊狀態輸出與診斷 |
| StoresData.ir_seeker | StoresManagement | 輸出投影；HMCS 與音效 Lua 的來源觀測 |
| ExternalActionOutbox | 任務元件提出操作；MC 接收生產端回覆 | DcsBridge 合併傳送；SM 查詢要求完成 |
| RadarRequests | Targeting 提出，MC 發布 | RadarSystem |
| RadarSnapshot | RadarSystem | MC／Targeting、DcsBridge |
| DisplaySnapshot | 與 MC 並列的 DisplaySystem | MC／SensorOfInterest、診斷 |
| SensorOfInterestSnapshot | SensorOfInterest | MC 的 TMS 分派、輸出投影與診斷 |
| ExecutiveTables | Executive | 排程診斷 |
| SystemStatusTables | 各基礎元件的指定分區 | 預留 |

MC 直接持有各功能的具型別資料分區；FireControlData 保存目標指定、捕獲操作方式、武器使用意圖。
Core 以任務功能識別交換顯示對應；DCS ID、座艙參數與符號繪製由對外介面處理。`CombatAvionicsSnapshot` 不再包含
最終 weapon_display；改為任務／掛載／火控資料及 `control_inputs`。
輸入序號與外部動作序號獨立；前者只表示操作順序，後者仍用於執行確認。

## 資料流與指令流

```text
DCS 觀測／駕駛操作 → Bridge／SystemPipeline → MC
  read_inputs → consume_commands → Executive → publish_outputs
  → AircraftData → AircraftSimulation 座艙快照 → MissionComputerExporter
  → 同一 generation 的座艙參數 → HMCS 顯示規則 → HMCS 繪圖頁

MC／RadarSystem 的操作要求 → Pipeline 完成快照 → DcsBridge ExternalActionTransport
  → mc_io.lua → cms_system.lua → DCS → ACK → 生產端回覆 → FrameInput／Pipeline

MC 選站要求 → weapon_system.lua → DCS SDK → 選站確認 → MC
```

顯示讀取不會執行武器動作、改變選站或授予發射許可。
HMCS 以既有 64 Hz 更新，但讀取的是 MC 32 Hz 的完整狀態；沒有同步 Lua→C++ RPC。
畫面選擇仍保留原先 AAM 條件優先、原始扳機指示及偏好延續。
相關操作的最後按下序號可保留同週期短按與跨幀順序；不需要讓顯示重播武器動作。
HMCS_WEAPON_CLASS／HMCS_WEAPON_QTY 由 hmcs_system.lua 單獨寫入，數量只取自 MC。

## 時間與原功能

週期順序為 StoresManagement → FireControl → SystemMonitor → FailureManager。
MC 在操縱命令前準備 SM 觀測；SM 先推進武器執行與保持，再由火控讀取結果，最後派送後續指令；指令入口以 SM 編號追蹤各筆要求及其外部動作確認，掛載等待保留在 SM 內。
FireControl 內依武器執行結果通知 Targeting，再整理 RadarSnapshot 的目標觀測。
捕獲輸入計時由 RadarSystem 執行；新要求在後續 Pipeline tick 由雷達處理。
ModeManagement 是事件入口。外層保持命令 FIFO、32 Hz 及 80 ms 最短保持；每秒自動重送捕獲已移除。
SM 與 RadarSystem 的保持從首次觀測到生產端執行確認的 input_time 起算，補跑沿用當前觀測時間。
Master OFF/SIM 立即追加武器停止要求；CMS 在各主保險狀態下啟用。
一般 NAV/A-A/A-G、DGFT/MSL 超控與 Master ON/OFF/SIM 各自保存；保險切換保留目前模式，模式切換保留目前保險。

外部動作視窗仍為 16 個連續序號，保留成功前綴確認、缺號拒絕、失敗重試及序號界限。
Bridge 在奇數 generation 寫入，完整成功後提交偶數；HMCS 拒絕中途變動的版本。
命令 ID 與輸入設定維持既有對應；IR 來源與座艙契約見[IR 觀測整理報告](../../../../../../docs/IR_OBSERVATION_REFACTOR_REPORT.zh-TW.md)。
新輸入序號與機砲數量參數由 CommandIds.json 管理並經原產生器生成。

## IR 武器介面觀測

DcsBridge 讀取 IR lock、要求方向及目標方向，SM 將當前來源觀測存入 StoresData.ir_seeker，
再隨 CombatAvionicsSnapshot.stores 發布。可用性、來源版本與失效原因一併保存。
此資料代表全域武器介面，掛點庫存、所選掛點與該觀測各自保存。
目標距離由 Targeting 整理 RadarSystem 的觀測提供；武器解籠意圖保存在 FireControlData.weapon。

HMCS 依有效 lock、模式及操作意圖決定武器呈現；音效 Lua 依有效 lock 選擇並維持鎖定音。
原型的接觸、備便、尋標狀態及雷達／操作意圖鎖定推導已移除。
完整移除清單、介面與驗證見[整理報告](../../../../../../docs/IR_OBSERVATION_REFACTOR_REPORT.zh-TW.md)。

CountermeasureControl 將前向與左向按下轉為具序號的程序要求，經 SystemPipeline 交由同層 CountermeasureSystem 排程；Bridge 使用共用 DcsAircraftInterface 直接施放並回報原生庫存。

## 逐掛點庫存

九掛點共用機型 Lua 定義；StoresManagement 保存各站的識別、數量、有效性、觀測時間與掛載版本。
庫存快照隨 CombatAvionicsSnapshot.stores.inventory 發布；共用查詢直接讀取唯讀快照。
空對空顯示數量由同一庫存查詢產生，另提供完整性；掛點與機砲觀測超過 0.5 秒時保存最後讀值並標為過期。
資料與函式分工見[掛載管理說明](Components/StoresManagement/README.md)。

## 模式狀態與事件

`ModeData.master_mode` 保存一般 NAV/A-A/A-G，`override_mode` 保存中央/DGFT/MSL。
`operating_mode()` 計算生效配置，模式選擇與超控復歸由 ModeManagement 管理。
對外 `CombatAvionicsSnapshot.mode` 獨立發布 `ModeSnapshot`；FireControlSnapshot
只保留火控資料。Bridge 發布主模式、超控與生效配置，HMCS 顯示目前配置；對照見模式事件說明。

MC 持有自己的 `ModeEvents`，由 ModeManagement 同步發布
`MasterArmSelected`／`OperatingModeSelected`，組裝時集中登記訂閱。
事件含當次選擇前後的 ModeData 副本；超控重選保持原設定，獨立 MSL STEP 處理輪站。
ModeManagement 透過 ModeData 及 ModeEvents 提供模式與通知。

Master OFF/SIM 同步通知火控停止；目標指定與所選模式保持。
CMS 接收共用 FIFO 中的獨立指令，前向與左向啟動各自的固定施放程序。
同一 FIFO 命令的通知全部結束，才處理下一筆，保留同 tick OFF→ON 的停止效果。
監聽者不得遞迴消費命令；訂閱在組裝時建立，不在週期計算中反覆建立。

ModeEvents 先於使用者建立；scoped_connection 最後宣告，先於接收者銷毀。
每一台 MC 有獨立通道，不使用全域事件中心。sigslot 放在共用 ThirdParty，
其他 C++ 系統與測試可直接使用，但不因此改變現有跨 System 資料契約。
導入、範圍及驗證見[模式事件重構](../../../../../../docs/MC_MODE_EVENTS.md)。

## 與 DFLCC 對齊

單一 System、具型別資料、固定參照及單一 Executive 與 DFLCC 一致。
這次調整的是 MC 的任務職責，沒有改動 DFLCC，也不要求它新增對應任務元件。
不新增冷啟動、電源或故障備援；NAV/A-A/A-G、超控恢復及實際火控解算留待後續。

## Targeting 與 RadarSystem

Targeting 與 RadarSystem 依 RadarRequests／RadarSnapshot 透過 AircraftData 交換資料。
DcsBridge 將各生產端要求對應到共用 DCS 序號，維持 16 項視窗與成功前綴確認。
本輪架構、序號及來源驗證見 [跨 System 說明](../../../../../../docs/RADAR_TARGETING_ARCHITECTURE.zh-TW.md)。

## 驗證

Native 回歸覆蓋命令 FIFO、武器停止／獨立 CMS 要求、ACK／計時、數量及尋標觀測。
Exporter 回歸核對新資料、顯示參數單一 writer 與不完整快照。
Lua 5.1 執行實際 HMCS／介面／輸入設定，並以 8,192 幀原顯示規則重播核對結果。
Windows 固定工具鏈的完整驗證與 DLL 產物依重構報告記錄；DCS 遊戲內驗證另行執行。

掛點、原生機砲彈量與指令入口見 [StoresManagement](Components/StoresManagement/README.md)。

## 顯示與控制來源

[DisplaySystem](../DisplaySystem/README.md) 經 SystemPipeline 提供功能對應。MC 透過 SensorOfInterest 管理焦點；Targeting 沿用唯讀 ModeData／RadarSnapshot 及 RadarRequestQueue，顯示依賴集中於 SOI 與 MC 輸入分派。

真機來源、固定模式配置、空殼範圍及 debug 欄位見 [感測器操作](../../../../../../docs/MC_SENSOR_CONTROLS.zh-TW.md)。
