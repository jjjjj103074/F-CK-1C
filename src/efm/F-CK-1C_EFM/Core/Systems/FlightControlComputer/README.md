# FlightControlComputer 架構

`FlightControlComputer` 代表一台飛控電腦，直接持有七個元件與五個獨立資料庫。
FLCC 負責組裝及 Pipeline 輸入輸出邊界；`Executive` 集中維護排程表並呼叫任務。

**尚未實作控制律、監控、故障、啟動與 AMUX 運算。**
目前骨架可以執行排程，兩種對外狀態快照仍維持 `status.available == false`。
本階段不處理備援。

## 檔案結構

```text
FlightControlComputer/
├─ Entry.cpp
├─ FlightControlComputer.h
├─ FlightControlComputer.cpp
├─ README.md
├─ Components/
│  ├─ Executive/
│  │  ├─ Executive.h / Executive.cpp
│  │  └─ TaskFrequency.h / TaskFrequency.cpp
│  ├─ SystemMonitor/
│  ├─ SelectorMonitor/
│  ├─ FailureManager/
│  ├─ StartupAndRestart/
│  ├─ AmuxProcessor/
│  └─ ControlLaws/
└─ Database/
   ├─ ExecutiveTables.h / ExecutiveTables.cpp
   ├─ ControlLawData.h
   ├─ InputOutputData.h
   ├─ SystemStatusTables.h
   └─ AmuxData.h
```

其餘六個元件各有同名的 `.h` 與 `.cpp`。`Entry.cpp` 只登記一個
`flight_control_computer` System；七個內部元件不繼承 `System`，也不加入外層系統目錄。

## 組裝與固定任務表

1. FLCC 先建立五個資料庫，再建立七個元件。
2. 六個受排程元件透過建構子綁定固定的資料庫參照。
3. FLCC 將 ExecutiveTables 與六個元件的參照交給 Executive。
4. Executive 在建構時一次建立完整任務表，驗證名稱與入口，保持填表列順序。
5. 外層呼叫 `FLCC::step`，由 FLCC 直接呼叫 `Executive::step`。

頻率、順序及入口統一維護於 [Executive.cpp](Components/Executive/Executive.cpp)
的建構子。元件只提供 `step`，不宣告排程設定；FLCC 不收集或註冊任務。
Executive 不將自己登記為任務。

六個元件的入口統一為：

```cpp
void step(std::chrono::nanoseconds scheduled_time, double dt_s);
```

兩個參數分別是本次預定模擬時間與此任務的固定週期，直接按值傳入。
`std::chrono::nanoseconds` 保存整數奈秒，表示從本架飛機建立時的零時刻起算的預定模擬時間。

每列填入名稱、精確 Hz、元件實例與成員函式指標，例如：

```cpp
{"control_laws.step", TaskFrequency(64), control_laws, &ControlLaws::step}
```

`&ControlLaws::step` 類似尚未綁定實例的方法參考。TaskDefinition 的建構子將它
綁定至指定實例，以 `std::function` 保存不同型別元件的入口；填表處不需撰寫 lambda。
同一元件可以出現在多列，分別綁定不同入口及 Hz，不需共同的虛擬元件基底類別。

目前六列都是 **64 Hz 骨架任務**，並非已確認的實機元件頻率或完整資料依賴：

| 列順序 | 元件 | 尚未實作 |
|---:|---|---|
| 1 | StartupAndRestart | 啟動與重新啟動程序 |
| 2 | AmuxProcessor | AMUX 傳輸及資料處理 |
| 3 | SystemMonitor | 系統與輸入監控 |
| 4 | SelectorMonitor | 資料選擇與有效性判斷 |
| 5 | FailureManager | 故障處理及降級策略 |
| 6 | ControlLaws | 控制律與控制需求計算 |

啟動元件目前也只有週期性空白入口；尚未定義真正的啟動狀態機或一次性啟動工作。

C++ 成員依宣告順序建立、反向銷毀，因此資料庫宣告在元件之前，
Executive 宣告在受排程元件之後。FLCC、ExecutiveTables 與 Executive 禁止複製和搬移，
使資料庫參照與任務綁定的目標地址保持有效。所有元件及資料庫由 FLCC 直接持有。

## 同一列的設定與狀態

ExecutiveTables 不使用彼此平行的設定陣列與進度陣列。取得一個任務列後，
就能同時查看該任務的所有設定與執行狀態：

```text
ExecutiveTables
├─ tasks[]
│  └─ ExecutiveTask
│     ├─ const TaskDefinition definition
│     │  ├─ id
│     │  ├─ frequency
│     │  └─ 綁定的 step 入口
│     └─ TaskRuntimeState state
│        ├─ completed_ticks
│        └─ next_due
└─ ExecutiveSchedulerState
   └─ advanced_through
```

- `definition` 是真正的 const 值物件，在任務列建構時完成設定，之後不可修改。
- `completed_ticks` 是成功返回的任務呼叫次數，為進度依據。
- `next_due` 是由 `completed_ticks + 1` 與頻率推導的快取，不以週期累加。
- `advanced_through` 是最近一次成功完成的外層目標模擬時間。
- `time_until_next(reference_time)` 查詢剩餘模擬時間；負值表示相對於該查詢時間已到期。

Database 提供唯讀的任務列與整體狀態參照。這些參照指向目前資料，不複製快照。
只有 Executive 可初始化表格及更新狀態；`friend class Executive` 表示允許 Executive
存取 Database 私有成員，不代表繼承或所有權移轉。排程表沒有公開的新增、排序或重設介面。
Executive 本身不另外保存進度，函式內只保留搜尋與呼叫需要的暫時變數。

## 精確頻率與時間契約

`TaskFrequency(p, q)` 代表 `p / q Hz`，不是相對外層頻率的倍率。
兩個參數皆為正的 32 位元整數，建立時約分；例如 `TaskFrequency(64)` 是 64 Hz，
`TaskFrequency(5, 2)` 是 2.5 Hz。頻率不能超過奈秒解析度的 1 GHz。

```text
第 n 次預定時間（奈秒） = floor(n × 1,000,000,000 × q / p)
```

計算使用整數商與餘數分解，避免中間乘法溢位，不依賴 double 或 long double。
在可表示範圍內，每次向下取整的量化誤差小於一奈秒，不逐次累積。

- 第一次呼叫在一個完整週期後，零時刻不呼叫任務。
- 不同到期時間按時間先後執行；同時到期按表格列順序執行。
- 外層較慢或不規則呼叫時，按時間順序執行所有已到期任務。
- 每個任務收到自身的預定時間及浮點 `dt_s = q / p`；dt 不參與到期判斷。
- 重複傳入相同目標時間不重複執行；負時間與時間倒退會被拒絕。
- 入口成功返回後才增加該列的次數並更新下次時間；超出時間範圍會拋出例外。
- 任務例外直接向上傳遞；尚未實作故障恢復、重啟排程或資料交易回復。

Executive 的 [find_next_due](Components/Executive/Executive.cpp) 找到最早到期的列，
`step` 呼叫入口並更新同一列的狀態，直到目標時間以前沒有待執行工作。

外層 SystemPipeline 目前仍以 64 Hz 呼叫 FLCC。內部排程不依賴此頻率，
但欠期補跑只能使用 FLCC 最近取得的輸入快照；
**尚未實作歷史觀測重建或重新取樣，也未驗證實際 CPU 截止期限。**

## 五個資料庫與固定參照

五個分區是具型別、記憶體內的資料容器，FLCC 分別持有，沒有 FlightControlData 總容器。
多個元件可參照同一分區；單執行緒內的先後關係由排程決定，不使用鎖。
參照不轉移所有權；const 參照限制該使用者的修改權限。

| 元件 | 固定依賴 |
|---|---|
| Executive | ExecutiveTables：建立固定設定、讀寫執行狀態 |
| StartupAndRestart | ControlLawData、SystemStatusTables：讀寫 |
| AmuxProcessor | AmuxData：讀寫 |
| SystemMonitor | InputOutputData：唯讀；SystemStatusTables：讀寫 |
| SelectorMonitor | InputOutputData：唯讀；SystemStatusTables：讀寫 |
| FailureManager | SystemStatusTables：讀寫 |
| ControlLaws | InputOutputData、ControlLawData：讀寫；SystemStatusTables：唯讀 |

上述依賴是目前的骨架，尚未構成完整演算法資料流程。
ControlLawData 與 AmuxData 保留明確的空白結構；元件私有的濾波器、積分器與運算歷史由元件持有。

## Pipeline 邊界與目前行為

FLCC 讀取 FlightControlObservation、PilotControlSignal、ThrottleLeverSignal、
LandingGearData 與 FlightControlActuatorState，存入 InputOutputData。
對外發布 FlightControlActuatorCommand、EngineThrottleCommand、
AutomaticFlightControlSnapshot 與 FlightControlComputerSnapshot。

各元件的 step 目前不修改資料庫。控制面需求維持中立，油門需求維持建構時初值。
尚未實作駕駛輸入至控制需求的運算，也尚未登記 FLCC 指令處理器。
兩種狀態快照保持不可用，不以空白任務的成功呼叫表示飛控功能已完成。

## 驗證

[FlightControlExecutiveTests.cpp](../../../../F-CK-1C_EFM_Tests/FlightControlExecutiveTests.cpp)
涵蓋多入口綁定、同列設定與狀態、64/32/3/2.5 Hz、不規則外層呼叫、
固定列順序、一小時非整奈秒週期、大次數整數精度、溢位、初始化及 Pipeline 骨架輸出。
執行方式見 [DLL 建置與測試指南](../../../../../../docs/BUILD_DLL.md)。

## 設計參考

七個軟體元件與五個資料分區參考
[NASA TP-2857](https://ntrs.nasa.gov/api/citations/19890014956/downloads/19890014956.pdf)
圖 26；圖 30 的分區、資料組件、區段、元素與欄位是資料組織階層。
目前以 C++ 型別表達所需分區，未複製報告的位元配置、硬體介面與備援系統。
