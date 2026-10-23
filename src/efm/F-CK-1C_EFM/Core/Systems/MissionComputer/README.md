# MissionComputer

MC 是一個獨立的 `System`，直接持有四個內部元件與三個具型別資料庫。
本階段建立頂層組裝、固定任務表與 Pipeline 接入；不實作任務功能。
這是依專案 DFLCC 方式建立的設計，不是 F-CK-1C 或其他實機 MC 的架構復刻。

## 組裝

`Entry.cpp` 只登記一個 `mission_computer`。
內部元件不繼承 `System`，不加入外層 catalog，也不各自建立執行緒。
來源與 Entry 由現有 MSBuild 規則自動收集，不修改共用 Pipeline 或手寫生成目錄。

```text
MissionComputer : System
├── Database/
│   ├── ExecutiveTables
│   ├── InputOutputData
│   └── SystemStatusTables
└── Components/
    ├── Executive
    ├── StartupAndRestart
    ├── SystemMonitor
    └── FailureManager
```

MC 先建立資料庫，再建立週期元件、Executive 與 StartupAndRestart。
建構子呼叫 `StartupAndRestart::start()` 啟用排程；外層 `step()` 推進模擬時間。
資料庫的生命週期涵蓋所有使用者，元件以固定參照取得需要的資料。
MC、Executive 與 ExecutiveTables 禁止複製或搬移，避免任務入口或資料參照失效。

## 元件

| 元件 | 固定依賴 | 本階段行為 |
|---|---|---|
| Executive | 可寫 ExecutiveTables、週期元件參照 | 建立任務表並推進排程 |
| StartupAndRestart | Executive、可寫 LifecycleStatus | 只啟用 Executive；生命週期分區仍為空 |
| SystemMonitor | 唯讀 InputOutputData、可寫 HealthStatus | 空的週期入口 |
| FailureManager | 唯讀 HealthStatus、可寫 FailureResponse | 空的週期入口 |

SystemMonitor 未來負責產生健康檢查結果；FailureManager 根據結果作處置決策。
完整啟動、重啟、監控、故障紀錄、降級與恢復流程均尚未實作。
排程啟用不代表 MC 的任務功能已就緒。

## 資料庫與寫入責任

資料庫是 MC 持有的具型別記憶體資料結構，不是外部儲存服務。

| 資料庫／分區 | 內容 | 寫入者 |
|---|---|---|
| ExecutiveTables | 任務定義、執行次數、下次到期時間及整體排程進度 | Executive |
| InputOutputData.input | 外部輸入快照；目前為空 | MC 的 read_inputs |
| InputOutputData.output | 待發布輸出；目前為空 | 未來各輸出的指定功能元件 |
| SystemStatusTables.lifecycle | 生命週期狀態；目前為空 | StartupAndRestart |
| SystemStatusTables.health | 健康結果與故障紀錄；目前為空 | SystemMonitor |
| SystemStatusTables.failure_response | 故障處置決策；目前為空 | FailureManager |

狀態元件只綁定上表指定的分區，不取得整份可寫 SystemStatusTables。
不先加入「健康」、「可用」或「完成啟動」等預設值，避免把空骨架解讀成已實作功能。

## Pipeline 與任務表

外層使用 `kProjectDefinedFallbackUpdateRateHz`，目前是 64 Hz。
內部任務表集中在 [Executive.cpp](Components/Executive/Executive.cpp)：

| 順序 | 入口 | 頻率 |
|---|---|---|
| 1 | SystemMonitor::step | 專案 fallback，64 Hz |
| 2 | FailureManager::step | 專案 fallback，64 Hz |

以上均為專案骨架設定，沒有實機 MC 頻率依據；後續按功能需求重新決定。
內部入口與 DFLCC 相同：`step(std::chrono::nanoseconds scheduled_time, double dt_s)`。
StartupAndRestart 不參與週期排程。

Executive 的 TaskFrequency、任務資料與排程規則對齊 DFLCC：

- 使用正整數分數表示頻率，由週期次數推導到期時間，避免累加漂移。
- 首次入口在完整週期後執行；不同到期時間依時間排序，同時到期依任務列順序。
- 延遲呼叫會補跑全部到期任務；重複目標時間不重複執行。
- 拒絕負時間、時間倒退、重複啟動、重複任務名稱與無效設定，並檢查時間溢位。
- 入口成功返回才更新該列進度；例外向上傳遞，沒有交易回復或故障恢復。

有電時的外層流程為 `read_inputs → Executive::step → publish_outputs`。
本階段沒有 AircraftData 讀取、發布或命令／事件處理器；兩個 I/O 入口為空。
尚未增加 MC 對外狀態契約，也不接管任何現有座艙功能。

## 供電與生命週期

與 DFLCC 一樣，MC 暫定預設有電，以 `set_powered(bool)` 控制。
斷電時直接返回；復電不重設排程，會從上次進度補跑至最新模擬時間。
目前未接電源系統，也沒有歷史輸入重建、輸出失效、斷電恢復或真正的重啟程序。
每次開始飛行重新建立 MC 與資料庫。

這是沿用既有骨架的暫時語意。加入有時效的事件與操作命令前，必須先決定
斷電期間工作的處置、事件保存與過期規則；不能直接將週期補跑當作事件重播。

## 與 DFLCC 對齊及差異

參考 [DFLCC README](../DigitalFlightControlComputer/README.md)。本次沒有修改 DFLCC。

| 項目 | MC 決定與原因 |
|---|---|
| 外層與組裝 | 沿用單一 System、Entry、資料庫先建立及固定參照的方式 |
| 基礎元件 | 沿用 Executive、StartupAndRestart、SystemMonitor、FailureManager 的角色 |
| 飛控專屬項目 | 不帶入 ControlLaws、SelectorMonitor、AmuxProcessor、ControlLawData 或 AmuxData；MC 尚無相應需求 |
| 資料存取 | 比 DFLCC 的整表可寫參照更窄，明確區分生命週期、健康與處置的寫入責任 |
| 頻率來源 | 使用專案 fallback，不引用 F-16XL 飛控頻率作為 MC 規格 |
| 排程實作 | 目前在 MC 內保留對齊的實作，不跨 System 引用 DFLCC 內部類別 |

DFLCC 是否也採用狀態分區的窄參照，留待其監控與故障邏輯設計時一併評估。
兩部電腦的排程目前有重複實作；若後續要共同維護，可另行評估抽到共用位置，
並以既有 DFLCC 排程測試保護行為。本次不先改動 DFLCC 或建立泛用框架。
供電、補跑與重啟語意也是兩者後續需要一起檢視的設計議題。

## 後續擴充

有獨立狀態、輸入輸出或生命週期的任務功能，加入為 MC 直接持有的同層元件。
在 Executive 的任務表明確安排入口、頻率與相依順序。
同一功能內的計算步驟可使用函式或輔助類別；不同頻率不必然代表不同元件。

按實際需求增加功能資料庫，指定各欄位的寫入者與唯讀使用者。
運算暫存留在元件內；不要先建一個收納所有功能的總資料庫或總處理元件。
新增外部資料時，同步定義 Core 契約、setup 宣告及讀取／發布實作。

## 驗證

[MC 架構測試](../../../../F-CK-1C_EFM_Tests/MissionComputerTests.cpp)
涵蓋啟動交接、排程順序、多頻率補跑及透過 Entry 接入 Pipeline。
正式 Windows 建置與原生測試指令見 [DLL 指南](../../../../../../docs/BUILD_DLL.md)。

## 分段遷移與 Windows 驗證

Lua 的 MC 決策依模式與 HOTAS、掛載與武器操作、感測器協調與尋標／顯示狀態
分段遷移。DCS 觀測、操作介面、繪圖與音效播放仍保留在 Lua／DcsBridge。
反制程序的發射排程屬於反制系統；本次不將其視為 MC 管理邏輯搬入。
不增加冷啟動、電源模擬或故障備援。

每段先做 Standards／Spec code review 與本機可執行測試，再提交、推送至
`mission-computer`，由 [Windows 工作流程](../../../../../../.github/workflows/mc-windows-validation.yml)
使用既有固定工具鏈建置，並執行完整 `validate_efm.ps1`。
使用獨立的 SubmittedDllPath 驗證新產物，工作樹內提交的 DLL 不會被改寫；
原有 PR 的提交 DLL 比對與合併檢查維持不變。

只取用成功 run 的 DLL，核對 manifest 的來源 commit、tree 與 DLL SHA256，
再將產物作為獨立 commit 納入。完整 Windows 驗證通過後才進下一遷移段落。
CI 不會代為推送、建立 PR 或合併；遊戲內 DCS 行為仍須另行驗證。
