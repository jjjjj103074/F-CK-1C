# SM 原生彈量與掛點狀態

更新日期：2026-11-08。介面契約與版本資訊見 [DCS API 指引](DCS_API_GUIDE.zh-TW.md)。

## 架構與分工

| 位置 | 工作 |
|---|---|
| DcsBridge/Internal/GunAmmunition | 核對 CockpitBase.dll，讀取原生機砲存量，提供完整觀測 |
| Cockpit/Scripts/Systems/weapon_system.lua | 以既有 20 Hz 提供固定九站原始資料、執行選站與回覆 ACK |
| DcsBridge/Internal/CockpitBridge | 驗證掛點觀測批次，將 DCS 類型轉譯成 Core 配置 |
| StoresManagement | 更新逐站庫存與機砲數量，寫入資料後發送變更通知 |
| StoresData | 保存庫存、觀測有效性、最近數字、觀測時間與選站狀態 |
| FireControl | 既有選站、射擊許可與武器操作程序 |
| CMS／HMCS | 執行有序原生命令／呈現 MC 快照 |

機砲資料流：`CockpitBase.dll -> GunAmmunitionSource -> GunAmmunitionObservation -> MC / SM -> StoresData -> 快照 -> HMCS`。
掛點資料流：`avSimpleWeaponSystem -> 座艙批次參數 -> CockpitBridge -> MC / SM`。

## 掛點配置

- Empty：原生站資訊為 nil，或數量為零且識別皆空。
- Clean：明確回傳 `<CLEAN>`。
- Equipped：有武器身分或非零數量；零數量保留掛載身分。
- 分類與數量由原生 weapon 類別、數量及識別轉譯。RT、RO、RI、MM、MF、MB、LI、LO、LT 對應固定 0–8。

## 機砲觀測

原生來源每 0.1 秒取樣；MC 保持 32 Hz。有效零發為 count=0、available=true。
SM 以來源 revision 更新觀測時間；距最近有效觀測超過 0.5 秒標記過期。
來源無效時保存最近有效數字，使用者依 availability 判讀。
機砲補彈數量由原生下一次觀測更新，與掛點 rearm_revision 分別管理。

`debug.csv` 保留機砲輸入、SM 數量、有效性、來源與年齡；正常來源狀態在 fck1c_efm.log 變更時記錄。

## 安裝遷移

正常啟動即建立 C++ 機砲來源。安裝器先備份並清除本模組舊版 Export 載入區塊，再替換模組。
手動複製模組時，可執行 `InstallSupport/RemoveLegacyStoresExport.ps1 -SavedGamesRoot <DCS資料夾>` 完成舊入口整理。
該工具保留其他匯出器的原始位元組。

## 驗證

診斷包在 DCS 2.9.31.29434 已完成五次點射與 MC 接收驗證，詳見 API 指引。
正式整理版的驗證紀錄見 [驗證報告](SM_NATIVE_AMMUNITION_VALIDATION.zh-TW.md)，驗收範圍見 [規格](SM_NATIVE_AMMUNITION_SPEC.zh-TW.md)。
