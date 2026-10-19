# DigitalFlightControlComputer

DFLCC 是一個 64 Hz System，提供可設定增益與曲線的三軸直接映射。
它擁有七個內部元件與五個具型別資料庫；Executive 管理內部任務表。

回授控制、配平運算、模式、監控、故障處理、AMUX 運算、完整冷啟動與重啟恢復
尚未實作。兩種對外狀態快照的 `status.available` 均為 false。

## Pipeline 介面

| 方向 | 型別 |
|---|---|
| 輸入 | FlightControlObservation、PilotControlSignal、LandingGearData、FlightControlActuatorState |
| 輸出 | FlightControlActuatorCommand、AutomaticFlightControlSnapshot、DigitalFlightControlComputerSnapshot |

有電時，`step()` 讀取輸入、推進 Executive，再發布資料庫中的完整輸出。
ControlLaws 只使用三軸操縱輸入，產生 [-1, 1] 的舵面位置需求；
FlightControlActuationSystem 將需求轉成實際角度並模擬致動器。

Engine 直接讀取 PilotControls 發布的 ThrottleLeverSignal。
手動油門獨立於 DFLCC 供電；自動油門尚未實作。
目前沒有 DFLCC 指令處理器。

## 組裝

`Entry.cpp` 只登記一個 `digital_flight_control_computer`。
內部元件不繼承 System，也不加入外層 catalog。

DFLCC 先建立資料庫與週期元件，再建立 Executive 與 StartupAndRestart。
建構子呼叫 `StartupAndRestart::start()`，由它啟用 Executive。
外層模擬時間透過 DFLCC 的 `step()` 推進排程。

| 元件 | 固定依賴與目前職責 |
|---|---|
| Executive | ExecutiveTables；建立任務表及更新進度 |
| StartupAndRestart | Executive；啟用排程 |
| AmuxProcessor | AmuxData；運算尚未實作 |
| SystemMonitor | 唯讀 InputOutputData、可寫 SystemStatusTables；運算尚未實作 |
| SelectorMonitor | 唯讀 InputOutputData、可寫 SystemStatusTables；運算尚未實作 |
| FailureManager | SystemStatusTables；運算尚未實作 |
| ControlLaws | InputOutputData、ControlLawData、唯讀 SystemStatusTables；三軸映射 |

資料庫包含 ExecutiveTables、ControlLawData、InputOutputData、
SystemStatusTables 與 AmuxData。ControlLawData 保存唯讀設定及可更新運算狀態；
AmuxData 目前為空白資料結構。

資料庫參照與成員函式綁定使用固定地址。
DFLCC、ExecutiveTables 與 Executive 均禁止複製及搬移。

## 任務表與時間

[Executive.cpp](Components/Executive/Executive.cpp) 集中定義名稱、Hz、實例與入口。
目前依序排入 AmuxProcessor、SystemMonitor、SelectorMonitor、FailureManager、
ControlLaws，五列均為 64 Hz；這些內部頻率是骨架設定。

每個入口為：

```cpp
void step(std::chrono::nanoseconds scheduled_time, double dt_s);
```

`TaskFrequency(p, q)` 表示 p/q Hz，使用正的 32 位元整數並自動約分，
最高為奈秒解析度的 1 GHz。第 n 次到期時間為：

```text
floor(n × 1,000,000,000 × q / p) nanoseconds
```

整數商餘數計算避免中間溢位及週期累加漂移。

- `start()` 啟用排程，首個任務在完整週期後執行。
- 不同時間依到期順序執行，同時到期依任務列順序。
- 較慢或不規則的外層呼叫會補跑所有到期任務。
- 重複的目標時間不重複執行；負時間與時間倒退會被拒絕。
- 未啟動即 step、重複 start 或超出可表示時間範圍會拋出例外。
- 入口成功返回後才增加該列次數與更新 `next_due`。
- 任務例外直接向上傳遞；沒有交易回復或故障恢復。

ExecutiveTables 的每列包含不可變 TaskDefinition 與可更新 TaskRuntimeState，
保存完成次數及下次到期時間；整體狀態保存 started 與 advanced_through。
只有 Executive 可以初始化及更新表格，外部取得唯讀參照。
`time_until_next(reference_time)` 可回報負值，表示相對該參考時間已到期。

## 供電與生命週期

DFLCC 預設有電，透過 `set_powered(bool)` 設定，`powered()` 查詢。
目前布林供電狀態尚未連接電源系統。

斷電時直接返回，不讀輸入、不推進 Executive、不發布輸出；
Pipeline 保留上次命令。復電不重設進度，會以最新輸入補跑到期任務。
尚未實作歷史輸入重建、輸出失效訊號、致動器斷電反應與復電重啟。
排程決定模擬時間，沒有實際 CPU 截止期限保證。

每次開始飛行重新建立 DFLCC 與資料庫。
ControlLawData 載入 `FM/DFLCC/ControlLaws.jsonc`，失敗時中止初始化。
映射欄位與錯誤行為見 [ControlLaws](Components/ControlLaws/README.md)；
共用載入規則見[設定檔載入器](../../../Common/Configuration/README.md)。

## 驗證與參考

[Executive 測試](../../../../F-CK-1C_EFM_Tests/DigitalFlightControlExecutiveTests.cpp)
涵蓋任務綁定、列順序、精確頻率、補跑、溢位、啟動及供電入口。
建置指令見 [DLL 指南](../../../../../../docs/BUILD_DLL.md)。

七元件與資料分區參考
[NASA TP-2857 圖 26、30](https://ntrs.nasa.gov/api/citations/19890014956/downloads/19890014956.pdf)；
目前 C++ 型別未複製其硬體配置或備援系統。
