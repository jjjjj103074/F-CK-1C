# FireControl 與目標指定

更新：2026-11-10。這是本專案的職責安排，沿用 MC 的 32 Hz 與共用動作傳輸。
本輪基準：`9eebf2663b645ddae7cfc870b8645f23372b1477`。

## 分工與資料

| 位置 | 職責 | 主要資料 |
|---|---|---|
| FireControl.cpp | 組裝、模式事件、武器程序結果與更新順序 | ModeData、FireControlData |
| FireControlTargetInput.cpp | 同一 FireControl 類別的 TMS 解讀、雷達操作及捕獲輸入計時 | acquisition_mode、私有 acquisition_input_ |
| Internal/TargetDesignation | 接受指定／切換／取消意圖，整理雷達追蹤觀測 | target_designation，唯一寫入 |
| Internal/WeaponOperation | 解籠、武器使用資格、射擊／投放與停止程序 | weapon，唯一寫入 |
| Internal/InfraredMissileSupport | 整理既有尋標接觸、鎖定與音調要求 | infrared_missile，唯一寫入 |
| StoresManagement | 掛載、選站、武器指令撤回與執行確認 | StoresData |

TargetDesignation 只持有 TargetDesignationData 的參照，提供 request 與 observe 兩個介面。
FC 是目標操作的單一協調入口；模式、按鍵、保險、掛載條件與操作計時各由表中責任位置處理。
FireControlTargetInput.cpp 是既有 FC 的實作分檔。MC 仍有八個同層元件，DFLCC 維持原有架構。
WeaponOperation 回傳 ReleaseInputEnded 後，由 FC 安排原有的投放結束取消指定流程。
紅外線支援讀取指定意圖與武器解籠狀態，沿用原型推導；兩個武器內部模組的整併留待後續。

## 目標指定流程

```mermaid
flowchart TD
    INPUT[駕駛操作與模式事件] --> FC[FireControl 解讀操作]
    FC -->|指定、切換、取消| REQUEST[TargetDesignation.request]
    REQUEST --> INTENT[更新指定意圖 requested]
    RADAR[雷達觀測] --> OBSERVE[TargetDesignation.observe]
    OBSERVE --> VALID{觀測有效且為 STT？}
    VALID -->|是| TRACK[保存追蹤狀態、方位、仰角與距離]
    VALID -->|否| CLEAR[追蹤狀態為 false，幾何資料歸零]
    INTENT --> DATA[TargetDesignationData]
    TRACK --> DATA
    CLEAR --> DATA
    DATA --> SNAPSHOT[MC 型別化快照與既有指定意圖參數]
    FC --> EXEC[安排捕獲輸入與雷達操作]
    EXEC --> QUEUE[MC 持有的 ExternalActionQueue]
    QUEUE --> DCS[Bridge、Lua、DCS]
```

requested 表示 FC 已接受的指定／切換意圖；取消要求將其設為 false。
radar_tracking 依有效雷達觀測的 SingleTargetTrack 模式更新。
指定意圖、雷達追蹤結果及飛彈尋標器鎖定各自保存；取消指定後，雷達欄位仍反映最近收到的觀測。
幾何資料使用弧度與公尺。目標觀測隨 FireControlSnapshot.target_designation 發布；
原有 AIM9 指定、解籠、音調與 HMCS 參數沿用既有映射。

## FC 操作與更新

```mermaid
flowchart TD
    READ[MC 讀取觀測與執行 ACK] --> COMMANDS[依 FIFO 處理操縱與模式事件]
    COMMANDS --> WEAPON[更新武器程序]
    WEAPON --> RESULT{投放輸入程序結束？}
    RESULT -->|是| CANCEL[FC 取消指定並安排捕獲輸入結束]
    RESULT -->|否| TIMER[推進捕獲輸入保持計時]
    CANCEL --> TIMER
    TIMER --> OBS[TargetDesignation 整理本次雷達觀測]
    OBS --> IR[更新紅外線支援]
    IR --> PUBLISH[MC 派送 SM 後續要求並發布快照]
```

TMS 與模式進入維持既有單次捕獲操作；每秒自動重送已移除。
脈衝／按住捕獲由 FC 的 AcquisitionInput 明確表達，80 ms 最短保持從 MC 首次收到執行確認的時間起算。
此保持策略屬本專案既有操作安排；ACK 記錄轉接層完成呼叫，雷達觀測記錄實際追蹤結果。
捕獲輸入結束保留指定意圖；取消指定則清除意圖並安排輸入結束。
NAV、DGFT、MSL 的雷達操作對應維持原有安排，Hud 保存操作選擇，TMS Left 沿用既有處理。

## 主保險與模式

```mermaid
flowchart TD
    ARM[選擇 ON、OFF 或 SIM] --> ARMDATA[只更新主保險並發布主保險事件]
    ARMDATA --> SAFE{OFF 或 SIM？}
    SAFE -->|是| CANCEL[SM 接受 CancelWeaponStarts，撤回先前武器啟動]
    CANCEL --> STOP[FC 提交必要停止要求]
    STOP --> CMS[CMS 中止]
    SAFE -->|否| DONE[保留目前操作模式與目標指定]
    CMS --> DONE
    MODE[選擇 NAV、DGFT 或 MSL] --> MODEDATA[只更新操作模式並發布模式事件]
    MODEDATA --> SM[SM 處理選站意圖]
    SM --> FC[FC 安排模式對應的雷達與目標操作]
```

保險操作保留目前模式，模式操作保留目前保險。ON 更新武裝資格，OFF／SIM 維持立即停止與 CMS 中止順序。
CancelWeaponStarts 經 SM 單一入口接受，保留選站與停止，也保留取消之後的新要求。
NAV 與 CancelStoresCommands 沿用完整的選站及啟動取消規則。

## 本輪驗證

| 檢查 | 結果 |
|---|---|
| C++ MC／模式／SM／原生讀取回歸 | 790 項，0 失敗 |
| Lua 5.1.5 MC 介面與 HMCS 回歸 | 6,183 項，0 失敗 |
| C++ 排版 | 294 個檔案通過 |
| EFM 架構 | 256 個正式來源通過 |
| 座艙架構 | 126 個自訂參數、17 個原生參數通過 |
| code-review：Standards | 0 findings |
| code-review：Spec | 0 findings |

新增回歸覆蓋指定意圖與雷達觀測獨立、STT 有效性與幾何資料、OFF／SIM 加空載仍可捕獲、
保險切換保留目標及選站、模式保留保險、解籠持續三秒僅保留一次捕獲、
捕獲 ACK／保持時間，以及武器撤回保留選站、來源等待與後續新要求。
GCC 使用 C++17、Wall／Wextra／Werror，保留既有聚合初始化例外。
座艙架構檢查在暫存控制器中使用平台路徑分隔符；專案檢查腳本維持原版。

本輪以 `9eebf26` 為來源比較基準，完成兩軸 code-review 後提交至本機，尚未 push。
bin 的 DLL 保留前次 Windows 產物，尚未包含本輪修改。
固定 Windows 工具鏈建置／分析與 DCS 遊戲驗證待後續 DLL 重建時執行。
