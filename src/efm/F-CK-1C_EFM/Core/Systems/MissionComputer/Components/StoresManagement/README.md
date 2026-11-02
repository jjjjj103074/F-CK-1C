# StoresManagement

StoresManagement 管理固定掛點的觀測庫存、既有彈量與選站要求；FireControl 決定選站及武器使用程序。
MC 以 32 Hz 更新；Lua 以 20 Hz 讀取 DCS 掛點。相同觀測可以重複處理，收到新批次時更新觀測時間，
內容或有效性改變時才發布通知。

## 固定掛點與來源

`Cockpit/Scripts/Stores/StationDefinitions.lua` 是機型註冊與座艙轉接層共用的九掛點定義：
RT、RO、RI、MM、MF、MB、LI、LO、LT。機型掛點編號為 1–9，座艙依既有零起算介面掃描 0–8。
Lua 回歸核對共用定義、C++ 容量與最後一站傳遞；實際 DCS 掛點映射仍需遊戲內驗證。

Lua 每站發布 `present;count;level1;level2;level3;level4;station_name;CLSID`。
CLSID 是最後一欄，原樣保留其中的分號。Bridge 驗證整批資料，將原生類型轉成 Core 分類，
並將四層類型組成不透明識別。Core 以識別比對掛載；型號未知的飛彈仍可依分類盤點。
本次直接使用 DCS 觀測與既有掛點定義，未增加獨立的武器型號資料庫。

## 資料與 Interface

- `Database/StoresData.h`：MC 持有，StoresManagement 唯一寫入。
- `StoresData.inventory`：九個固定掛點的識別、種類、數量、來源有效性、最後有效觀測時間及掛載版本。
- `Common/StoresQueries.h`：唯讀 `find_missile_stations`、`count_missiles`，接受庫存快照及查詢條件。
- `StoresEvents`：每台 MC 的同步 `InventoryChanged` 通道；資料與舊介面摘要寫入後通知。
- `CombatAvionicsSnapshot.stores.inventory`：對外發布完整逐掛點庫存。

首次有效掛載、更換掛載或重新掛彈會更新掛載版本；同一次掛載的數量變化保留該版本。
有掛載但數量為零，與空掛點分別保存。觀測暫時無效時，保留最後一次有效內容及時間，
並標記目前有效性。查詢回傳有效資料中的候選站與數量，數量結果另帶資料完整性。

## 函式分工

| 位置／函式 | 功能 |
|---|---|
| `StoresManagement::step` | 更新庫存、機砲彈量及相符的選站確認，最後發布變化 |
| `update_inventory` | 記錄來源批次，逐站處理觀測並整理內容變更版本 |
| `Internal::update_station` | 比對掛載身份、數量與有效性，更新單站狀態及掛載版本 |
| `update_legacy_summary` | 維持既有空對空總數來源優先序與選站確認 |
| `request_station` | 接受指定站位要求、驗證索引並遞增要求編號 |
| `find_missile_stations` | 依分類、用途或識別碼回傳有彈的有效候選站 |
| `count_missiles` | 計算符合條件的數量，回報資料是否完整 |

辨識直接使用 Bridge 提供的分類與識別；不另以 AIM-9 名稱判斷。
內部資料處理位於 `Internal/InventoryProcessing`；查詢只依賴共用快照型別。

## 火控與相容行為

FireControl 在週期更新開始時，依原 NAV／DGFT／MSL 規則選站，透過管理器提出要求。
掛載管理器不讀取 ModeData。選站確認仍比對要求編號與掛點，代表 Lua 選站呼叫已執行。
本次維持原發射資格、80 ms 保持、停止、解鎖與 Lua 命令執行流程。

既有 HMCS 空對空總數仍使用掛點 → API → 估算的來源順序，原估算初值與機砲算法保留。
這份相容摘要與逐掛點觀測庫存分開；火控通知估算扣彈時不改寫已觀測的掛點數量。
全用途飛彈盤點不代表已新增各型飛彈的導引或使用程序。

## 觀測診斷

`Diagnostics/StoresTelemetry` 由 MC 在 setup 宣告 43 個共用通道，並於每次 MC step 完成後發布。
每站提供庫存摘要、完整識別及 Bridge 解析後的輸入；全域欄位提供觀測批次、有效性、
重新掛彈版本、查詢完整性與機砲來源。它使用既有 DebugTelemetrySink，畫面與 debug.csv 共用資料。

Lua 的 `Stores/ObservationDiagnostics.lua` 記錄原始 table 的欄位與型別，內容變化時寫入 dcs.log。
掛點記錄附帶即將發布的批次；CMS 記錄實際嘗試的數量 API，並每秒觀測可見的 LoGetPayloadInfo。
原始診斷保留轉接前的缺欄位、容器與轉接架資訊；數量來源優先序及武器程序維持既有行為。

人工驗證步驟見[必要 DCS 測試](../../../../../../../../docs/MC_STORES_DCS_TEST.zh-TW.md)。

## 驗證狀態

來源保持未提交。Windows DLL 尚未由本次來源重建，隨附 bin 仍對應先前的模式事件版本；
本次變更需完成 Windows 完整驗證與 DLL 更新後，再進行 DCS 遊戲內驗證。
本機測試結果與暫存相容措施見[驗證紀錄](../../../../../../../../docs/MC_STORES_VALIDATION.md)。
