# SM 原生彈量與掛點狀態

本輪以修改前工作目錄快照為比較基準，保留原有未提交變更。功能依據為
[本輪規格](SM_NATIVE_AMMUNITION_SPEC.zh-TW.md)及 2026-11-02 實機測試。

## 架構與分工

| 位置 | 工作 |
|---|---|
| `Export/StoresExport.lua` | 在 Export 環境讀取玩家機砲原生彈量，10 Hz 發送裝置命令 |
| `Cockpit/Scripts/Stores/GunAmmunition.lua` | 共用正規化數值編碼、解碼及逾時設定 |
| `Cockpit/Scripts/Systems/cms_system.lua` | 接收裝置命令，發布完整批次的機砲觀測參數，判讀接收時效 |
| `Cockpit/Scripts/Systems/weapon_system.lua` | 以既有 20 Hz 提供固定九站的原生掛載資料 |
| `DcsBridge/Internal/CockpitBridge` | 驗證完整批次與整數，將原生掛點翻譯為 Core 配置 |
| `StoresManagement` | 更新逐站庫存與原生機砲狀態，寫入資料後通知 |
| `StoresData` | 保存庫存、彈量及選站狀態，供其他使用者唯讀取得 |
| `FireControl` | 維持既有選站與武器使用程序 |
| `HMCS` | 依 MC 彈量有效性呈現數字或 GUN --- |

機砲資料流：

`Export → CMS.SetCommand → 奇偶批次座艙參數 → DcsBridge → GunAmmunitionObservation → SM → StoresData → 快照 → HMCS`

掛點資料流維持：

`get_station_info → weapon_system.lua → 完整批次座艙參數 → DcsBridge → SM → StoresData`

## 掛點狀態

九個固定站位保留在庫存中。配置分為：

- Unknown：尚無足以建立配置的觀測。
- Empty：觀測為零數量，原生掛載識別及類型均為空。
- Clean：原生 CLSID 為 `<CLEAN>`，與空掛點分開表達。
- Equipped：原生提供掛載數量、識別或類型；數量為零時仍保存來源提供的身分。

觀測有效性與最近有效內容分開保存。配置與數量有變化時更新庫存版本，
更換掛載或重新掛彈更新掛載版本。飛彈查詢透過快照使用分類、用途與識別。

## 機砲狀態

原生彈量為整數；第一次尚未取得時 `count` 為空值。有效零發與未取得資料分別保存。
固定 523 發與每秒 100 發的估算已移除，SM 的建構依賴也移除了火控射擊資料。

新批次更新最近有效時間；同一批次重讀保留時間。半秒沒有更新即過期，保留最近數字，
資料恢復時以新的原生數字更新。彈量、來源或有效性改變才增加機砲版本並發布事件。
`gun_ammunition_changed` 的訂閱者在回呼時可讀取已完成更新的 StoresData。

既有快照數值欄位保留供座艙使用，同時提供完整 `gun_ammunition` 狀態，並額外匯出
`McStoresGunCountAvailable`。HMCS 透過自己的 `HmcsWeaponQuantityAvailable` 決定文字呈現。

## Export 與座艙的位置

| 項目 | Export | 座艙 Lua |
|---|---|---|
| DCS 載入入口 | Saved Games/DCS/Scripts/Export.lua | 模組 Cockpit/Scripts/device_init.lua |
| 本輪主要程式 | 模組 Export/StoresExport.lua | 模組 Cockpit/Scripts/Systems/cms_system.lua |
| 本輪取得資訊 | LoGetPayloadInfo().Cannon.shells | 原生掛點、座艙參數、裝置命令 |
| 執行環境 | DCS Export Lua 環境 | 個別座艙裝置的 Lua 環境 |

兩者在 DCS 程序內使用各自 Lua 環境，變數與可用函式分開；本次實機已證實
LoGetPayloadInfo 在 Export 可用，座艙環境沒有相同函式。
本輪用 GetDevice(...):performClickableAction(...) 的裝置命令銜接，再沿用座艙參數與 C++ 的既有通道。
這個入口方式可參考 [DCS-BIOS 開發文件](https://github.com/dcs-bios/dcs-bios.github.io/blob/master/docs/v0.2.3/developerguide.adoc)。

## 安裝與打包

正式模組安裝與打包均包含 Export 資料夾。啟用外部觀測的方式見
[Export 說明](../Export/README.zh-TW.md)。原始碼提供
`tools/install_stores_export.ps1 -SavedGamesRoot <DCS資料夾>`，以受管理區塊追加既有 Export，
並備份原始內容；`-Action Remove` 移除該區塊，保留其他匯出器。

## 驗證

| 本機檢查 | 結果 |
|---|---|
| MC、ModeManagement、SM、診斷與共用 Telemetry 原生回歸 | 433 checks，0 failures |
| CockpitBridge、MC／座艙快照匯出 | 273 checks，0 failures |
| 既有 MC Lua 5.1 行為回歸 | 6,204 checks，0 failures |
| Export／CMS／HMCS 與三任務彈量重播 | 388 checks，0 failures |
| 原有診斷工具回歸 | 41 checks，0 failures |
| Export 安裝／移除 | 10 checks，0 failures |
| Lua 5.1 語法與本輪 Lua 排版 | passed |
| C++ 排版 | 286 個自有檔案通過 |
| EFM 架構 | 249 個來源通過 |
| 座艙架構 | 129 自訂參數、17 原生參數通過 |
| DCS ID 產生器 fixtures | passed |
| 座艙架構 checker fixtures | passed，暫存 Linux 路徑適配 |
| git diff --check | passed |

本機測試以分離的 Lua 環境串接實際 Export、CMS 接收與 HMCS 程式，並重播三個實機任務的彈量變化。
C++ 測試涵蓋配置、有效零發、半彈、數量增加、逾時與恢復、通知順序及 Bridge 批次一致性。
機砲增加的自動測試模擬原生數字增加；DCS 實際補彈與命令送達仍需實機驗收。

目前執行環境為 Linux。暫存的 Windows API stub 與生成常數適配僅用於 Linux Bridge 回歸，
正式 SDK、產生器與生成規則維持原樣。Windows 指定工具鏈、DLL 載入與 DCS 命令實際送達，
仍依專案 Windows／DCS 流程驗證；本輪來源與既有 bin DLL 的版本分別記錄。

## 本輪 code-review

Review 以修改前工作目錄快照為基準，由獨立 Standards 與 Spec reviewer 檢查本輪差異。
初次 Standards 提出一處工具註解語言，已改為繁中；初次 Spec 核對指出機砲文字的
有效性條件在 Add 註冊之後追加，已改為註冊前建立完整條件。
新增畫面註冊回歸保存 Add 當時的條件，核對有效數字、有效零發、未知文字與 AA 隱藏互斥。
該回歸在修改前順序會失敗，修改後通過；此結果驗證完整註冊條件，DCS 實際呈現仍待實機驗收。
