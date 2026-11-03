# MC 掛載管理第一版驗證

比較基準：`7e6780a0e2a6cd1732cc57ac4e3fc5ada87e2e9e`。本次來源保持未提交。
本紀錄對應逐掛點庫存重設計；先前模式事件的 Windows DLL 憑證繼續保留。

## 變更範圍

- 機型註冊與座艙共用九掛點定義及名稱，掃描範圍為 0–8。
- 原始掛載紀錄傳遞四層原生類型、掛點名稱與完整 CLSID。
- 每個固定掛點保存掛載、數量、有效性、來源時間與掛載版本。
- 唯讀查詢接受庫存快照，涵蓋全部用途飛彈及資料完整性。
- 內容與有效性變化在資料寫入後發布每台 MC 專用的同步事件。
- 選站策略移至 FireControl，保留原有模式規則及既有射擊程序。
- 既有 HUD 彈量／API 回退／估算與逐掛點觀測分開保存。

函式與資料分工見 [StoresManagement](../src/efm/F-CK-1C_EFM/Core/Systems/MissionComputer/Components/StoresManagement/README.md)。

## 本機結果

| 項目 | 結果 |
|---|---|
| MC／模式／掛載管理原生回歸 | 313 checks，0 failures |
| Bridge／快照匯出相容回歸 | 242 checks，0 failures |
| Lua 5.1.5 實際介面／輸入／顯示回歸 | 6,203 checks，0 failures |
| Lua 5.1 語法 | passed |
| clang-format 22.1.8 | 283 個自有 C++ 檔案通過 |
| StyLua 2.6.0 | 轉接層及共用定義完整檢查通過；機型及舊測試採本次變更區段排版 |
| EFM 架構檢查 | 247 個來源檔通過 |
| DCS ID 產生器 fixtures | passed |
| 座艙架構檢查 | 126 個自訂參數、17 個原生參數通過 |
| git diff --check | passed |

掛載回歸涵蓋固定九站、AA 與非 AA 飛彈、未知分類、非武器名稱比對、查詢完整性、
同批重讀、新批相同內容、同批讀取恢復的時間、數量減少、更換掛載、重新掛彈、
空掛點與零數量掛載、最後有效資料保留、事件順序／隔離／解除，以及第九站選站與索引界限。
既有 MC 回歸繼續核對模式、主保險、FIFO、停止、發射保持、API 彈量與 HMCS 相容資料。

## Linux 相容措施

MC 原生回歸使用 g++ C++17、O2、LTO、Wall/Wextra/Werror/pedantic，
沿用既有測試聚合初始值的 missing-field-initializers 例外。

Bridge／匯出回歸在暫存目錄使用 Windows 模組查詢 stub，並將生成常數的字串宣告適配為 constexpr。
專案 SDK、產生器與生成資料的正式語意維持原樣。此範圍使用 O0 與相同嚴格警告；
GCC 的 O2/LTO 曾對既有 read_mission_adapter 的 optional 判斷回報 maybe-uninitialized，
因此 Windows 最佳化與 MSVC 分析留待正式工具鏈驗證。

座艙架構檢查使用原始 checker。其參數 CSV 由暫存的 inventory generator 產生，
僅將既有 Windows 路徑結尾分隔符改為主機分隔符；正式專案工具未修改。
DCS ID 產生器與其 fixtures 使用原始專案工具執行。

## Windows 與 DCS

本次尚未執行指定 Windows 工具鏈的完整建置、靜態分析、ABI 與載入驗證。
`bin/F-CK-1C_EFM.dll` 與憑證維持先前已驗證版本，尚未對應本次來源。
本次 Lua／C++ 掛載紀錄格式已同步變更，DCS 測試前須先重建並驗證新 DLL。

遊戲內仍需核對九掛點索引與模型位置、原生類型回報，以及重新掛彈／選站的實際結果。
本機 fixture 證明程式處理與契約一致，遊戲內結果另行記錄。

## 掛載觀測診斷補充（2026-11-02）

本輪新增 MC 的 StoresTelemetry 診斷輔助類別，使用既有 DebugTelemetrySink 宣告 43 個通道，
逐站發布 Bridge 解析後的輸入、庫存摘要與完整識別，並發布查詢完整性及機砲來源。
Lua 保留原始掛點 table、讀取結果與實際嘗試的數量 API，內容變化時寫入 dcs.log；
另以 1 Hz 觀測座艙可見的 LoGetPayloadInfo，供機砲資料來源比對。

| 本輪檢查 | 結果 |
|---|---|
| MC／模式／掛載／診斷整合及 DebugTelemetryHub | 388 checks，0 failures |
| Lua 5.1.5 實際介面／輸入／顯示與原始診斷 | 6,223 checks，0 failures |
| Lua 5.1 語法 | passed |
| clang-format 22.1.8 全範圍 | 286 個自有檔案通過 |
| StyLua 2.6.0 | 診斷與轉接層完整檢查通過；測試檔新增診斷區段排版 |
| EFM 架構檢查 | 249 個生產來源檔通過 |

診斷回歸經過實際 MC Pipeline 與 DebugTelemetryHub，涵蓋最後一站、原生識別字元、
未知分類、有效零彈、無效觀測時保存庫存、重新掛彈、來源批次及 64 Hz 取樣。
Lua 回歸涵蓋原生欄位型別、缺欄位、變化記錄、批次標記、診斷失敗隔離、
原有 API 優先序與低頻 payload 觀測；觀測結果保持為診斷資料。

[必要 DCS 測試](MC_STORES_DCS_TEST.zh-TW.md) 以空載、兩種互斥機腹配置及清空涵蓋九站，
再用一次機砲點放判讀實際彈量來源。資料庫內部規則由自動測試涵蓋，人工聚焦原生介面。

Windows DLL 與憑證保持原版本；本輪未執行 Windows／DCS，亦未 commit／push。
人工執行前需由本次完整來源重建並完成 Windows 驗證，再與本次 Lua 一起部署。
