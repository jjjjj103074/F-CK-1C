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

## MC 遷移驗證（尚待 DCS 執行）

以下先使用熱機或空中出生；不把冷啟動、斷電恢復或故障備援列為本次 MC 驗收。
記錄 DCS 版本、來源 commit、DLL SHA256、掛載及測試結果。
MC CI 的 `dcs_runtime = not_run` 在人工驗證完成前保持不變。

| 情境 | 操作與預期觀察 |
|---|---|
| 數量 API | 以 Lua debugger 或暫時的介面診斷，記錄 cms_system.lua 各既有候選函數是否可呼叫及回傳值；確認總數、單站、選定武器及機炮數量的實際語意 |
| 有彈／零彈 | 比對七站清單、MC 數量及 HMCS：有效空清單表示飛彈零枚；機炮 API 有效零值表示零發；機炮來源不可用時仍保留原有估計，不能由空掛站推定機炮零發 |
| 模式與武器 gate | 驗證 NAV／DGFT／MSL、Master ON／OFF／SIM、uncage；維持遷移前行為，含 DGFT／MSL 使 Master ON 的原有耦合 |
| 輸入入口 | 分別測試鍵盤 Space、RAlt+Space、搖桿扳機／投放及共用設定別名；應走 MC，不因合併重複項而遺失預設組合 |
| 短按與停止 | 測試短按、長按與放開；MC 最短保持為觀測到執行 ACK 後 80 ms；Master OFF／SIM 命令消費時不等待保持期就追加停止 |
| 選站與再裝填 | 第一個合格有彈站、耗盡後換站、重新裝填事件；選站 ACK 只表示 Lua 呼叫成功，不等同讀回物理選站 |
| 尋標與音效 | 比對 IR／雷達原始觀測、MC contact／lock／tone 與聲音；確認有效全零回退沿用舊行為 |
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
