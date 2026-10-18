# DCS 測試

DCS 測試驗證載入、回呼順序、座艙參數、呈現與飛行生命週期。
自動檢查見 [DLL 建置](BUILD_DLL.md)；EFM 控制範圍見
[DFLCC](../src/efm/F-CK-1C_EFM/Core/Systems/DigitalFlightControlComputer/README.md)。

## 執行前

確認 DCS 版本與部署內容，並比對來源及已安裝 DLL 的 SHA256。
使用 DCS Mission Editor 建立所需出生狀態與掛載的任務。

## 情境

| 情境 | 操作與觀察 |
|---|---|
| COLD-CLEAN | 冷機地面出生；檢查 device、初始油門／引擎／起落架狀態、NWS 與煞車 |
| RWY-CLEAN | 跑道熱機出生；檢查雙發油門、後燃器／噴嘴、煞車、輪速與 Thrust Cut |
| AIR-CLEAN | 空中空載出生；分別操作三軸、襟翼、縫翼、減速板及起落架，觀察對應狀態與動畫 |
| AIR-AIM9 | 使用實際載入 AIM-9 的任務；檢查武器站、數量、選擇、尋標器與音效 |
| RESTART | 離開任務後重新進入；檢查初始化、輸出清除、日誌及 Debug Indicator 可見狀態 |

每個情境檢查 `dcs.log` 的 device 載入錯誤，以及 EFM log 的錯誤與警告。
固定速度、高度、掛載、輸入幅度與持續時間，以比較行為。
AP、A/T 與回授飛控尚未實作；其狀態應維持不可用。

## 座艙與開發工具

- 切換 NAV／DGFT／MSL、Master Arm 與相關 TMS/CMS 操作，
  觀察 HMCS、武器 gate 及 DCS action。
- 測試雷達電源與目標狀態；有武器的任務才驗證 AIM-9 選擇及音效。
- 以 Developer Tools 的 EFM Debug Indicator Toggle 顯示／隱藏通道，
  比對 debug.csv；每次開始飛行時顯示應為隱藏。
- Engine Thrust Cut 啟用時觀察雙發推力為零，解除後恢復；
  intent 通道為 `propulsion_test_thrust_cut_requested`。

## 診斷

`fck1c_state.csv` 提供物理狀態；`debug.csv` 提供已宣告的開發通道。
CSV sequence 缺口可能來自非同步 writer 覆蓋尚未寫入的資料，
不能用它判定所有操作事件是否發生。
輪替、取樣與錯誤行為見 [DcsBridge](../src/efm/F-CK-1C_EFM/DcsBridge/README.md#runtime-diagnostics)。
