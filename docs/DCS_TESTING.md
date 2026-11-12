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

## MC 遷移與後續驗證

以下先使用熱機或空中出生；不把冷啟動、斷電恢復或故障備援列為本次 MC 驗收。
記錄 DCS 版本、來源 commit、DLL SHA256、掛載及測試結果。
完整逐步操作、預設鍵與正常結果見 [MC 實機測試流程](MC_FLIGHT_TEST.zh-TW.md)。
MC CI 的 `dcs_runtime = not_run` 在人工驗證完成前保持不變。

| 情境 | 操作與預期觀察 |
|---|---|
| 掛載觀測 | 比對任務配置與 HMCS 彈量；確認九掛點對應、混合掛載、掛點耗盡及重新掛彈後的數量 |
| 有彈／零彈 | 比對任務配置與 HMCS：有效空載顯示飛彈零枚；機砲原生讀值為零時顯示零發，來源無效時顯示 GUN --- |
| 模式與武器 gate | 驗證 NAV／DGFT／MSL、Master ON／OFF／SIM、uncage；驗證主模式與保險獨立，切 DGFT／MSL 保留原主保險值 |
| 輸入入口 | 分別測試鍵盤 Space、RAlt+Space、搖桿扳機／投放及共用設定別名；應走 MC，不因合併重複項而遺失預設組合 |
| 短按與停止 | 測試短按、長按與放開；MC 最短保持為觀測到執行 ACK 後 80 ms；Master OFF／SIM 命令消費時不等待保持期就追加停止 |
| 選站與再裝填 | 第一個合格有彈站、耗盡後換站、重新裝填事件；選站 ACK 只表示 Lua 呼叫成功，不等同讀回物理選站 |
| 尋標與音效 | 比對 IR／雷達各自的來源觀測與有效性；鎖定音跟隨有效 IR lock，有效全零表示未鎖定 |
| 反制 | Forward／Aft／Left 的次數與間隔、Right 中止、Master OFF 後立即 ON；確認中止不被後續 ON 抹除 |
| 任務重入 | 離開後重入，確認序號、ACK、模式及未完成操作重新建立，沒有上次任務殘留 |

函數在既有版本曾正常使用，是保留現有呼叫的依據；本次未將「不存在」列為已確認問題。
Lua fixture 通過只證明介面如何處理回傳值，不能代替上述 DCS 實測。
32 Hz 是 MC 更新頻率，掛點介面仍為 20 Hz；介面延遲與 DCS 自身 gate 也會影響實際動作時間。

## 診斷

`fck1c_state.csv` 提供物理狀態；`debug.csv` 提供已宣告的開發通道。
CSV sequence 缺口可能來自非同步 writer 覆蓋尚未寫入的資料，
不能用它判定所有操作事件是否發生。
輪替、取樣與錯誤行為見 [DcsBridge](../src/efm/F-CK-1C_EFM/DcsBridge/README.md#runtime-diagnostics)。

## C++ 原生機砲來源

2026-11-08 已完成診斷包的五次點射與 MC 接收驗證，318 次讀取全部成功。
正式來源的介面、支援版本與數值依據見 [DCS API 指引](DCS_API_GUIDE.zh-TW.md)。
後續補測以下生命週期案例：

| 情境 | 操作與觀察 |
|---|---|
| 零發 | 熱啟動後按右 Ctrl+M、右 Ctrl+D，再按住 Space 至耗盡；觀察 HMCS 顯示 GUN 0，放開後維持零發 |
| 地面補彈 | 完成地勤補彈後觀察 HMCS 機砲彈量回升 |
| 換任務 | 重開不同裝填任務，觀察 HMCS 初始彈量與該任務一致 |

來源狀態變化與讀取錯誤可查閱 fck1c_efm.log 的 gun_ammunition 紀錄；彈量使用 HMCS 核對。
