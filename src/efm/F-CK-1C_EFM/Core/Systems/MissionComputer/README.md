# MissionComputer

MC 是單一 `System`，直接持有同層功能元件與具型別記憶體資料庫。
組裝方式沿用 DFLCC；這是專案設計，並非實機 F-CK-1C MC 的架構復刻。
目前完成模式、HOTAS、武器操作、感測器協調與掛點管理的 Lua 遷移。
完整尋標狀態與武器顯示決策將在後續段落移入。

## 組裝與責任

```text
MissionComputer : System
├── Database/
│   ├── ExecutiveTables
│   ├── InputOutputData
│   ├── SystemStatusTables
│   ├── MissionData (ModeData / SensorData / StoresData / WeaponControlData)
│   └── ExternalActionQueue
└── Components/
    ├── ModeManagement
    ├── StoresManagement
    ├── WeaponControl
    ├── SensorCoordination
    ├── Executive
    ├── StartupAndRestart
    ├── SystemMonitor
    └── FailureManager
```

`Entry.cpp` 只登記 `mission_computer`。內部元件不繼承 System，沒有各自的執行緒。
資料庫先於使用它們的元件建立；MC、Executive 與 ExecutiveTables 禁止複製及搬移。
StartupAndRestart 只啟用 Executive，監控與故障處置仍為空入口。
不增加冷啟動、電源模擬或故障備援。

| 元件 | 責任 | 寫入資料 |
|---|---|---|
| ModeManagement | Master ON/OFF/SIM、NAV/DGFT/MSL、模式切換協調與反制啟停命令 | ModeData |
| StoresManagement | 掛點分類、數量來源權威、選站／重新裝填決策及無觀測時的數量估計 | StoresData |
| WeaponControl | 扳機／投放資格、最短保持及 Master OFF/SIM 立即停止 | WeaponControlData、內部操作計時 |
| SensorCoordination | TMS、uncage、子模式、目標指定與 DGFT 自動鎖定要求 | SensorData、內部鎖定計時 |
| Executive | 固定任務表、順序與時間推進 | ExecutiveTables |
| StartupAndRestart | 啟用 Executive | SystemStatusTables.lifecycle（目前為空） |
| SystemMonitor | 保留健康監控入口 | SystemStatusTables.health（目前為空） |
| FailureManager | 保留故障處置入口 | SystemStatusTables.failure_response（目前為空） |

ModeManagement 協調模式變更對其他元件的影響；WeaponControl 呼叫感測器協調的
投放準備／解除指定及掛載管理的數量更新。MC 外殼只負責組裝、接線與對外資料投影。
各元件使用固定參照；資料唯讀／可寫責任在建構介面中明確區分。
ExternalActionQueue 只處理有序傳遞，沒有模式、發射或目標判斷。

## 資料庫

| 分區 | 唯一寫入者 | 主要讀取者 |
|---|---|---|
| InputOutputData.input | MC 命令入口／read_inputs／consume_commands | StoresManagement、WeaponControl、SensorCoordination |
| InputOutputData.output | MC publish_outputs（投影元件狀態） | Pipeline、DcsBridge |
| ModeData | ModeManagement | 武器操作、感測器協調與輸出投影 |
| SensorData | SensorCoordination | 武器操作與輸出投影 |
| StoresData | StoresManagement | 感測器協調與輸出投影 |
| WeaponControlData | WeaponControl | 輸出投影 |
| ExternalActionQueue | 各元件追加語意操作；MC 接收確認並擷取傳遞視窗 | 各元件查詢已執行序號 |
| ExecutiveTables | Executive | 排程診斷 |
| SystemStatusTables | 對應狀態元件的指定分區 | 目前沒有功能使用者 |

這些是具型別記憶體結構，不是外部資料庫服務。DCS ID 與 API 不進入 Core。
現有 CombatAvionicsSnapshot 作為 `kMissionComputerSnapshot` 的值型別，經
AircraftSimulation 的座艙快照送往 DcsBridge。

## 時間與命令

外層與所有週期入口暫用專案 64 Hz，並非實機 MC 的已知頻率。
同時到期順序是 StoresManagement → WeaponControl → SensorCoordination →
SystemMonitor → FailureManager。ModeManagement 是事件入口，不另建空週期工作。
Executive 保留與 DFLCC 相同的整數分數頻率、依到期時間補跑、同時到期依表列順序、
拒絕時間倒退及無效任務等規則。

MC 命令先依收到順序保存在具型別佇列；下一個 tick 讀取最新觀測後，再依序交給
對應元件更新狀態與追加外部操作。不把同一個 tick 中的多次模式切換折成最後狀態。鍵盤與搖桿的自訂 MC/HOTAS 命令經 EFM 路由。
原有直接 DCS 測試／備用按鍵仍是 DCS 自有介面。

外層流程為 read_inputs → consume_commands → Executive::step → publish_outputs。
扳機、投放與鎖定脈衝的最短保持沿用原 Lua 的 80 ms，從 Lua 確認執行起計；
64 Hz 量化與 Lua 更新可能延長保持。Master OFF/SIM 立即追加停止操作。
`SystemStepContext.input_time` 記錄本次主機幀的觀測時間，避免補跑把新確認倒填到
歷史任務時間。DFLCC 的既有時間與行為不變，不需要修改其元件。

DGFT 的自動鎖定間隔沿用 1 s；沒有歷史觀測時，不補造過去的外部操作。
HUD 掃描與 IFF 仍為預留；投放仍沿用現有模式與 uncage 資格，未新增武器導引或彈道。
NAV 切換沒有新增強制停止已開始操作的規則；保留既有行為，待使用者決定。

## Lua 與 DCS 邊界

`cms_system.lua` 保留 DCS dispatch、get_base_data 的原始數量探測與反制程序排程。
反制 Forward／Left／Aft 的原有次數與間隔未改，MC 發出啟動或中止命令。
關閉後立即再開啟 Master 仍會保留中間的中止操作。
`mc_io.lua` 只負責數值參數傳遞，不做任務判斷。

MC 保留未確認操作，最多一次發布 16 個連續序號；Lua 成功執行後確認已完成前綴。
重複視窗不重播已確認操作；缺號、未知值、失敗回呼及不完整視窗不向前確認。
Lua 回呼失敗後可能重試，DCS 本身沒有交易／恰好一次執行的保證。
輸出使用奇／偶 generation：寫入中為奇數，全部欄位成功寫入後才提交偶數。
缺少參數時不提交可消費視窗。序號受 double 可精確表示的正整數範圍限制。
雷達開啟是狀態要求；MC 避免對同份關閉觀測重複要求，Lua 在映射為 DCS toggle 前
再確認目前狀態，避免延後傳遞反而關閉已開啟的雷達。

`weapon_system.lua` 以 20 Hz 讀取七個掛點，發布原始觀測並執行 MC 的選站要求。
DcsBridge 將 DCS wsType 數值轉為具語意的 WeaponCategory／WeaponRole；
Core 接收 present、count、分類與不透明武器識別字串，不接觸 SDK。
每筆邊界記錄格式為 `present;count;level2;level3;identifier`；只切前四個分號，
識別字串內的分號保持原樣。數量限 0–9999，字串須完整且短於 511 bytes。
整份觀測使用奇／偶 revision，七筆讀取前後的 revision 必須相同且為正偶數。
任一 API 失敗或資料不合法時，整份清單不可用，不將殘留數量當成新觀測。
API 回傳 nil 表示該掛點不存在；七站皆成功讀取的空清單是有效的零枚觀測。

StoresManagement 的來源優先序為有效掛點清單 → get_base_data 數量 API → 估計。
零枚是有效資料；估計從原有預設 1 枚或最後有效數量延續，僅在沒有有效數量來源時
依已完成的投放操作遞減。StoresCountSource 隨快照保留來源。
分類沿用舊 Lua：Missile + AirToAir，或識別字串包含 AIM-9／AIM_9／CATM-9。
這仍是原型的廣泛分類，尚未引入精確武器定義資料庫。

AAM 模式選擇第一個仍有彈的合格掛點；模式切換、掛點耗盡與重新裝填可發出新要求。
要求的 station／revision 與最後成功要求的 station／ACK 分開保存；Lua 只確認成功呼叫，
失敗保留要求並重試，MC 拒絕超前或與要求不符的確認。重新裝填只由 Lua 發布事件序號，
重新選站由 MC 決定。NAV 的空要求清除 Lua 的最後選站紀錄；SDK 在此沒有取消選站 API，
因此不宣稱已讀取或取消 DCS 的實際物理選站。掛點資料不可用時不自行猜測新選站。

HMCS 的繪圖、單位轉換及 DCS 觀測，音效播放與 DCS 感測器模型仍屬於 Lua／DcsBridge；
尋標狀態與武器顯示決策尚待後續遷移。

## 生命週期與 DFLCC 對齊

沿用 DFLCC 的單一 System、固定參照、資料庫先建立與單一 Executive。
MC 功能元件依責任拆開，不引入 DFLCC 飛控專屬元件，也不跨電腦引用內部類別。
狀態資料分區比整表可寫參照更窄；本次不修改 DFLCC。
兩者排程仍為分開的對齊實作，沒有先抽泛用框架。

`set_powered(bool)` 仍只是沿用骨架的測試開關，沒有接到任何電源系統。
本次不支援飛行中斷電／復電或故障恢復；每次飛行重新建立 MC、操作序號與 Lua 介面。

## 驗證與分段提交

[MC 原生測試](../../../../F-CK-1C_EFM_Tests/MissionComputerTests.cpp) 涵蓋排程、
Pipeline、操作序列、延後確認、補跑時鐘、短按與立即中止。
[Lua 5.1 測試](../../../../../../tools/test_mc_lua.lua) 涵蓋操作視窗、重試與實際 CMS 對應。
DcsBridge 測試涵蓋無效確認／數量、不完整參數輸出、原始掛點記錄與讀取中變更。
掛點測試另外覆蓋來源優先序、零枚、分類、選站確認、耗盡、重新裝填與 API 失敗。

每段先做 Standards／Spec code review 與本機可執行測試，再 commit／push。
[CI](../../../../../../.github/workflows/mc-windows-validation.yml) 先執行固定 Lua 5.1
測試，再用固定 Windows 工具鏈執行完整 validate_efm.ps1。
只取用成功 run 的 DLL，核對來源 commit／tree 及 SHA256，再另作產物 commit。
[DLL 收據](../../../../../../bin/F-CK-1C_EFM.validation.json) 保留對應來源與 CI run。
完整驗證通過後才進下一段；不自動建立或合併 PR。
遊戲內 DCS 行為仍須另行驗證，CI 不代表已完成 DCS 飛行測試。
