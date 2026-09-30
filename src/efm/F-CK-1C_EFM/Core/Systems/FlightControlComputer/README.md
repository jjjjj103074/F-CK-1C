# FlightControlComputer TDD 基準

目前 FLCC 只保留可被 `SystemPipeline` 建立的外殼與已確認的資料邊界。
舊控制律、模式、濾波、診斷投影及設定實作已移除；後續功能必須先用測試定義行為，再逐步加入。

## 根目錄三個 `.cpp` 的職責

| 檔案 | 職責 |
|---|---|
| `Entry.cpp` | 登記 `flight_control_computer` 系統 ID，並提供每架飛機建立 `FlightControlComputer` 的工廠。 |
| `FlightControlComputer.cpp` | 管理 Pipeline 生命週期，宣告輸入輸出，把單次輸入交給 Executive。 |
| `FlightControlExecutive.cpp` | 保留未來 FLCC 內部運算的協調入口；目前沒有控制邏輯或跨週期演算法狀態。 |

根目錄只保留上述三個實作檔、兩個標頭與本文件。內部模組會在有明確需求與測試時才重新建立。

## 輸入邊界

`FlightControlComputer` 以 64 Hz 從 `AircraftDataView` 讀取以下具型別資料：

| 資料鍵 | 型別 | 內容 |
|---|---|---|
| `kFlightControlObservation` | `FlightControlObservation` | 已轉成 Core 單位的飛行狀態。 |
| `kPilotControlSignal` | `PilotControlSignal` | 三軸與配平的正規化駕駛輸入。 |
| `kThrottleLeverSignal` | `ThrottleLeverSignal` | 左右油門桿正規化位置。 |
| `kLandingGearData` | `LandingGearData` | 起落架、煞車、輪組與接地狀態。 |
| `kFlightControlActuatorState` | `FlightControlActuatorState` | 控制面位置、速率與限制回授。 |

這些資料會組成 `FlightControlComputerStepInput`，其中 `dt_s` 的單位為秒。

## 輸出邊界

未來一次完整運算會產生 `FlightControlComputerResult`：

| 資料鍵 | 型別 | 內容 |
|---|---|---|
| `kFlightControlActuatorCommand` | `FlightControlActuatorCommand` | 三軸控制面需求，單位為弧度。 |
| `kEngineThrottleCommand` | `EngineThrottleCommand` | 左右引擎正規化油門命令。 |
| `kAutomaticFlightControlSnapshot` | `AutomaticFlightControlSnapshot` | 自動飛行狀態快照。 |
| `kFlightControlComputerSnapshot` | `FlightControlComputerSnapshot` | FLCC 狀態與診斷快照。 |

## 目前可執行行為

- `setup()` 可完成，讓整個 Pipeline 建立成功。
- 建立時發布中立控制面需求、初始油門桿值，以及 `available == false` 的兩種快照。
- 目前不登記任何 FLCC 指令。
- 第一次 `step()` 會拋出 `FLCC internal computation is not implemented.`，明確表示內部運算尚未建立。

初始值只用來滿足 Pipeline 建立時的資料契約，不代表 FLCC 已可運作。舊實作仍可由 Git 歷史中的 `54989bd` 查閱。
