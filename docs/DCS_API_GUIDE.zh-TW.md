# DCS API 使用指引

本指引整理本專案的 DCS 介面位置、資料契約及實測依據。更新日期：2026-11-08。

## 選擇執行環境

| 執行位置 | 本專案用途 | 主要入口 |
|---|---|---|
| EFM C ABI | DCS 飛行回呼、物理輸入／輸出與生命週期 | `DcsBridge/EfmExports.cpp`、SDK `include/FM/` |
| 座艙 Lua device | DCS 掛點觀測、裝置操作、顯示與音效 | `Cockpit/Scripts/device_init.lua`、`Systems/` |
| 模組內 C++ 原生橋接 | 經版本核對的機砲彈量讀取 | `DcsBridge/Internal/GunAmmunition/` |
| Saved Games Export Lua | 玩家資料匯出、外部工具 | DCS 的 `Scripts/Export.lua`；本模組彈量由 C++ 提供 |
| Mission Lua | 任務與世界物件操作 | 任務執行環境；與座艙、Export 分別提供全域函式 |

DcsBridge 將原生數值轉成 `Core/Contracts`。Core 與 SM 使用型別化觀測。
新增原生識別碼先更新 `DcsIds/CommandIds.json`，再執行 `tools/generate_dcs_ids.ps1`。
生成的 C++／Lua 對照表由產生器維護。

## 原生機砲剩餘彈量：已實測路徑

### 使用入口與資料流

```text
ed_fm_simulate
  -> BridgeContext::gun_ammunition_source().observe(simulation_time_s)
  -> CockpitBase.dll payload / gun
  -> GunAmmunitionObservation
  -> MC (32 Hz) -> StoresManagement -> StoresData
  -> MissionComputerExporter -> HMCS / debug.csv
```

`GunAmmunitionSource::observe()` 每次回傳最近完整觀測，原生取樣間隔為 0.1 秒。
實際取樣時刻落在下一個 EFM frame；本次 6 ms frame 的紀錄間隔為 0.102 秒。
`status.available=true` 且 `count=0` 表示有效零發。來源未取得時 availability 為 false，
SM 保存最近有效數字及來源狀態；顯示端以 availability 決定數字是否有效。
來源每次觀察遞增自身 revision；MC 依新 revision 更新觀測時間。

每次取樣重新取得 payload、mount、gun 指標，只在這次讀取期間使用；物件所有權屬於 DCS。
飛行開始／釋放時，BridgeContext 清除取樣狀態；下一次飛行重新核對程式庫。
等待 module／payload／mount 建立時持續取樣；原生呼叫失敗會停止該次飛行的原生呼叫，
並發布無效觀測。再次開始飛行會重新建立讀取狀態。

### Windows x64 原生呼叫契約

這是以程式指紋選擇的私有 ABI；ED SDK 的公開 EFM 契約與本節分開維護。
呼叫配置集中於 `NativeGunAccess`，Windows 記憶體檢查及 SEH 集中於 `GunAmmunitionSource`。

| 步驟 | 契約 |
|---|---|
| module | 已載入的 `CockpitBase.dll`；先核對 PE AMD64、PE32+、timestamp、image size 與程式指紋 |
| payload | `GetProcAddress`：`?c_payload@cockpit@@YAPEAVIwHumanPayload@@XZ`；x64 參數為空、回傳 payload 指標 |
| gun mount | payload vtable 位元組位移 `560`；以 `(payload, uint8 index, uint8 255)` 呼叫 |
| gun object | mount 物件位元組位移 `8` 的指標 |
| remaining count | gun vtable 位元組位移 `88`；以 `(gun)` 呼叫，回傳 unsigned count |
| 多組機砲 | index 由 0 遞增；第一個空 mount 結束。第一站即空時為 no_gun_mount |
| 讀值界線 | 最多 32 組 mount、合計 0–9999 發。有效零發保留 availability |

所有指標經 `ReadProcessMemory` 讀取；函式位址須為可執行記憶體，原生呼叫使用 SEH 記錄例外。
物件指標在本次取樣內使用。合法位址及 SEH 是執行保護，配置相容性由版本比對與實機證據確認。

正式配置目前啟用本專案已實測的下列版本：

| 項目 | 數值 |
|---|---|
| DCS | Steam、2.9.31.29434、Windows x64 MT |
| CockpitBase.dll SHA256（實測檔案） | `fb0a09be70627c7790dea1527cedfee43082bb8b112484d5c85be1d99d33de95` |
| PE timestamp | `0x6adbad09` |
| image size | `0x68e000` |
| 指紋 RVA／長度 | `0x2fe650`／49 bytes |
| 指紋演算法／值 | FNV-1a 64-bit／`0xdbd7d715791c239f` |

啟用條件使用 timestamp、image size、局部程式指紋及必要匯出符號；表中的整檔 SHA256 是實測版本識別證據。
新增版本時先取得該版 module 資訊與指紋，核對物件配置及函式契約，再執行射擊與生命週期測試。

### 狀態與診斷

`fck1c_efm.log` 的 `gun_ammunition` 前綴記錄每次飛行的 host 核對及來源狀態變化。
持續數字使用既有 `debug.csv` 的 `mc_stores_gun_api_count`、`mc_stores_gun_count`、
`mc_stores_gun_available`、`mc_stores_gun_age_s`。

| 狀態 | 意義 |
|---|---|
| available | 完整讀取成功；零發也屬此狀態 |
| waiting_for_module / waiting_for_payload / no_gun_mount | 原生來源目前尚未建立 |
| unsupported_build / missing_payload_export | 版本配置或必要匯出未符合呼叫條件 |
| unreadable_object | 指標鏈讀取失敗 |
| native_call_failed | 原生呼叫失敗；exception_code 記錄 SEH 例外代碼 |
| invalid_count / too_many_mounts | 原生結果超出配置範圍 |

原生取樣與 MC 排程有各自時間；同幀的新原生值可能先於 MC 下一個 32 Hz 更新。
比對時沿用 revision、availability 及整段 CSV。

### 本專案實機證據

2026-11-08 回傳的 `FCK1C-NATIVE-GUN-RESULTS-20261108-180021-024.zip`，
來源版本 `c92ad9ae76e64d2412d85bea552fc5f6cef60c12`：318 次取樣全部成功、0 次原生例外，
五次點射 `523 -> 464 -> 407 -> 340 -> 269 -> 212`，MC 初始化後的 2,070 筆 CSV 觀測與庫存一致。
此紀錄驗證原生讀取與 MC 傳遞；正式整理版仍需補足零發、地面補彈及換任務的實機案例。

互通配置的研究來源：
[Ethanw05/DCS-F22，固定版本 fec598b](https://github.com/Ethanw05/DCS-F22/tree/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6)。
其中 `NativeSidewinder.cpp` 提供 payload／gun 配置線索，`NativeSidewinderHost.h` 記錄 host 指紋。
本專案讀取器、版本檢查、SEH、觀測、測試及 MC 整合由本專案實作；實機結果以上述 F-CK-1C 紀錄為依據。

## 座艙掛載介面

`device_init.lua` 的 `avSimpleWeaponSystem` 載入 `Systems/weapon_system.lua`。
`GetSelf()` 取得該裝置；以 `get_station_info(self, index)` 每 0.05 秒讀取固定九站。

| index | 名稱 | index | 名稱 | index | 名稱 |
|---:|---|---:|---|---:|---|
| 0 | RT | 3 | MM | 6 | LI |
| 1 | RO | 4 | MF | 7 | LO |
| 2 | RI | 5 | MB | 8 | LT |

資料包含 `count`、`weapon.level1..level4` 與 `CLSID`。完整原始識別送進 Bridge；
Bridge 依 DCS 類別轉成 Core 的分類與配置，SM 管理庫存。
`nil`、零數量空識別與 `<CLEAN>` 的轉譯契約見 [SM 指南](SM_NATIVE_AMMUNITION.zh-TW.md)。

選站使用 `select_station(self, index)`。MC 發出請求後，座艙成功呼叫會回覆該請求的 ACK；
ACK 表示介面操作完成。`WeaponRearmComplete` 及 `UnlimitedWeaponStationRestore` 事件推進重新掛載版本。

## 指令、參數及顯示

- 機砲操作：MC 的 `GunOn`／`GunOff` 語意 action 由 CMS 映射為生成的 DCS `iCommandPlaneFire`／`iCommandPlaneFireOff`，以 `dispatch_action(nil, command)` 執行。
- `get_base_data()`：取得目前裝置環境的觀測介面；使用已核對的成員，記錄可呼叫性、回傳型別及數值。API 研究紀錄見 [彈量來源](DCS_AMMUNITION_SOURCES.zh-TW.md)。
- `get_param_handle(name)`：取得座艙參數。名稱由本專案 `DcsIds` 產生；參數 handle 的存在與實際資料有效性分別判定。
- 觀測批次使用 revision／availability；輸出快照使用 generation。HMCS 讀取完整 generation 後，依各欄位有效性呈現。
- Export 的 `LoGetPayloadInfo().Cannon.shells` 是先前已驗證的另一個執行位置；目前機砲來源使用本節的模組內 C++ 路徑。

## 開發與升級

1. 先確認要使用的執行環境及擁有資料的模組。
2. 在 DcsBridge 進行原生值檢查與單位轉換；Core 接收型別化資料。
3. 新增／修改 ID 透過資料來源及產生器更新；原生機砲私有 ABI 配置維護於 GunAmmunition 目錄。
4. 執行 Lua、介面／生命週期及完整 Windows 驗證，再依 [DCS 測試](DCS_TESTING.md) 記錄實機結果。
5. 安裝器的 `InstallSupport/RemoveLegacyStoresExport.ps1` 備份並移除舊版 F-CK-1C Export 載入區塊；其餘 Export 內容按原位元組保留。

相關文件：[座艙](COCKPIT.md)、[DcsBridge](../src/efm/F-CK-1C_EFM/DcsBridge/README.md)、
[單位契約](EFM_UNIT_CONVENTIONS.md)、[SDK／DLL 建置](BUILD_DLL.md)。
