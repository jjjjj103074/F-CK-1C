# MissionComputer

MC 是單一 `System`，以專案選用的 32 Hz 運作；組裝沿用 DFLCC 的資料庫先建立、固定參照
與單一 Executive。這是專案設計，並非實機 F-CK-1C MC 的架構復刻。
本次保留既有操作、計時、尋標推導與畫面行為，只調整職責及資料介面。
需求、驗證與限制見[重構報告](../../../../../../docs/MC_REFACTOR_REPORT.md)，
先前遷移經過見[歷史遷移報告](../../../../../../docs/MC_MIGRATION_REPORT.md)。

## 組裝

```text
MissionComputer : System [32 Hz]
├── Database/
│   ├── ExecutiveTables
│   ├── InputOutputData
│   ├── SystemStatusTables
│   ├── MissionData (ModeData / StoresData / FireControlData)
│   │   └── FireControlData.targeting / weapon / infrared_missile
│   └── ExternalActionQueue
└── Components/
    ├── ModeManagement
    ├── StoresManagement
    ├── FireControl
    │   └── Internal/
    │       ├── Targeting
    │       ├── WeaponOperation
    │       └── InfraredMissileSupport
    ├── Executive
    ├── StartupAndRestart
    ├── SystemMonitor
    └── FailureManager
```

三個任務元件與四個基礎元件，共七個同層元件。
FireControl 內部模組不是獨立 System、執行緒或 Executive 工作。
MC、FireControl、Executive 與 ExecutiveTables 禁止複製及搬移，維持固定參照。

| 元件 | 職責 |
|---|---|
| ModeManagement | 保留 Master ON/OFF/SIM、NAV/DGFT/MSL、模式通知與反制啟停資格 |
| StoresManagement | 原有掛載分類、彈量來源、選站／確認、再裝填與武器循環要求 |
| FireControl | 統一目標操作、武器資格／使用程序及紅外線飛彈支援 |
| Executive | 四個週期工作的固定順序、頻率及時間推進 |
| StartupAndRestart | 原有建構時啟用排程 |
| SystemMonitor | 健康監控預留入口，目前無功能 |
| FailureManager | 故障處置預留入口，目前無功能 |

FireControl 內的 Targeting 保存解籠、指定意圖、捕獲子模式與捕獲計時；
WeaponOperation 保留扳機／投放資格、保持與停止程序；InfraredMissileSupport
整理尋標接觸、鎖定、狀態與音調要求。模式及武裝變更透過 FireControl 公開入口通知。
武器循環、選站與數量更新歸 StoresManagement；雷達模型、DCS API 及操作映射留在外部。

## 資料所有權

| 分區 | 唯一寫入者 | 主要讀取者 |
|---|---|---|
| InputOutputData.input | MC 輸入入口及外殼 | 掛載、火控、輸出投影 |
| InputOutputData.input.control_inputs | MC 依命令 FIFO 記錄原始操作 | 輸出投影；HUD/HMCS 的唯讀觀測 |
| InputOutputData.output | MC publish_outputs | Pipeline、Bridge |
| ModeData | ModeManagement | 火控、掛載及輸出投影 |
| StoresData | StoresManagement | 火控及輸出投影 |
| FireControlData.targeting | Targeting | 武器操作、紅外線支援及輸出投影 |
| FireControlData.weapon | WeaponOperation | 掛載數量估算及輸出投影 |
| FireControlData.infrared_missile | InfraredMissileSupport | 輸出投影 |
| ExternalActionQueue | 任務元件追加動作；MC 確認及取得視窗 | 操作程序查詢已執行序號 |
| ExecutiveTables | Executive | 排程診斷 |
| SystemStatusTables | 各基礎元件的指定分區 | 預留 |

MC 直接持有七個具型別記憶體物件；FireControlData 內有三個功能分區。
Core 不知道 DCS ID、座艙參數或顯示格式。`CombatAvionicsSnapshot` 不再包含
最終 weapon_display；改為任務／掛載／火控資料及 `control_inputs`。
輸入序號與外部動作序號獨立；前者只表示操作順序，後者仍用於執行確認。

## 資料流與指令流

```text
DCS 觀測／駕駛操作 → Bridge／SystemPipeline → MC
  read_inputs → consume_commands → Executive → publish_outputs
  → AircraftData → AircraftSimulation 座艙快照 → MissionComputerExporter
  → 同一 generation 的座艙參數 → HMCS 顯示規則 → HMCS 繪圖頁

MC 核准的語意動作 → ExternalActionQueue → Bridge
  → mc_io.lua → cms_system.lua → DCS → ACK → MC

MC 選站要求 → weapon_system.lua → DCS SDK → 選站確認 → MC
```

顯示讀取不會執行武器動作、改變選站或授予發射許可。
HMCS 以既有 64 Hz 更新，但讀取的是 MC 32 Hz 的完整狀態；沒有同步 Lua→C++ RPC。
畫面選擇仍保留原先 AAM 條件優先、原始扳機指示及偏好延續。
相關操作的最後按下序號可保留同週期短按與跨幀順序；不需要讓顯示重播武器動作。
HMCS_WEAPON_CLASS／HMCS_WEAPON_QTY 由 hmcs_system.lua 單獨寫入，數量只取自 MC。

## 時間與原功能

週期順序為 StoresManagement → FireControl → SystemMonitor → FailureManager。
FireControl 內依 WeaponOperation → Targeting → InfraredMissileSupport 推進，保留原先順序。
ModeManagement 是事件入口。外層仍為命令 FIFO、32 Hz、80 ms 最短保持、1 s 自動捕獲；
保持時間仍從 MC 觀測到 Lua ACK 的當前輸入時間起算，補跑不回填新的操作／ACK。
Master OFF/SIM 立即追加停止與 CMS 中止要求；NAV 不新增強制停止規則。
NAV/DGFT/MSL、DGFT/MSL 自動 Master ON、Master ON 回 NAV 的連動保留。

外部動作視窗仍為 16 個連續序號，保留成功前綴確認、缺號拒絕、失敗重試及序號界限。
Bridge 在奇數 generation 寫入，完整成功後提交偶數；HMCS 拒絕中途變動的版本。
現有命令 ID、輸入設定、音效及 AIM9_* 座艙參數的值與名稱保持相容。
新輸入序號與機砲數量參數由 CommandIds.json 管理並經原產生器生成。

## 紅外線支援的實作範圍

名稱與內部資料改用 Infrared；目前仍完整保留原 AIM-9 原型演算法，沒有新增武器型號支援。
直接 IR 鎖定／目標角度優先，其次是原有 IR／雷達推導，再其次是有效全零觀測回退。
readiness 是簡化命令狀態；designation 是操作意圖；contact／locked 是另外推導的狀態。
`lock_range_m` 仍是雷達 STT 距離，並非紅外線尋標器自行測距。
未把鎖定狀態新增為發射條件，也未接手 DCS 的飛彈導引模擬。

掛載分類仍沿用廣泛 AAM 判斷，尚未精確區分紅外線與其他空對空飛彈。
未來優先以武器能力／參數資料共用流程，必要時才增加特殊操作；此部分只留待辦註解。
完整掛載查詢仍由 Lua SDK 入口提供；數量來源保持掛載 → 原有數量 API → 原有估計。
反制程序仍在 Lua，MC 保留啟動資格與中止要求。

## 與 DFLCC 對齊

單一 System、具型別資料、固定參照及單一 Executive 與 DFLCC 一致。
這次調整的是 MC 的任務職責，沒有改動 DFLCC，也不要求它新增對應任務元件。
不新增冷啟動、電源或故障備援；NAV/A-A/A-G、超控恢復及實際火控解算留待後續。

## 驗證

Native 回歸覆蓋命令 FIFO、停止／CMS 順序、ACK／計時、數量及尋標觀測。
Exporter 回歸核對新資料、顯示參數單一 writer 與不完整快照。
Lua 5.1 執行實際 HMCS／介面／輸入設定，並以 8,192 幀原顯示規則重播核對結果。
Windows 固定工具鏈的完整驗證與 DLL 產物依重構報告記錄；DCS 遊戲內驗證另行執行。
