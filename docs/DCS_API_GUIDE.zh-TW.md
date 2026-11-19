# DCS API 使用指引

本指引整理本專案的 DCS 介面位置、資料契約及實測依據。更新日期：2026-11-15。

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
  -> MissionComputerExporter -> HMCS
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
| 讀值界線 | 最多探查 32 個 mount 索引；合計 0–9999 發，並在空 mount 結束。有效零發保留 availability |

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
持續數字及有效性由 SM 寫入 MC 快照，經 MissionComputerExporter 提供給 HMCS。

| 狀態 | 意義 |
|---|---|
| available | 完整讀取成功；零發也屬此狀態 |
| waiting_for_module / waiting_for_payload / no_gun_mount | 原生來源目前尚未建立 |
| unsupported_build / missing_payload_export | 版本配置或必要匯出未符合呼叫條件 |
| unreadable_object | 指標鏈讀取失敗 |
| native_call_failed | 原生呼叫失敗；exception_code 記錄 SEH 例外代碼 |
| invalid_count / too_many_mounts | 原生結果超出配置範圍 |

原生取樣與 MC 排程有各自時間；同幀的新原生值可能先於 MC 下一個 32 Hz 更新。
比對資料批次時使用 revision 與 availability。

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
`nil`、零數量空識別與 `<CLEAN>` 的轉譯契約見 [SM 指南](../src/efm/F-CK-1C_EFM/Core/Systems/MissionComputer/Components/StoresManagement/README.md)。

選站使用 `select_station(self, index)`。SM 的具型別指令入口將各筆選站連接到既有外部動作佇列，
每筆動作保存掛點與要求版本。視窗首項為選站時，Exporter 才發布該次要求與全域動作序號；
座艙成功呼叫後回覆 ACK，CMS 完成同一序號的選站屏障，再繼續後續操作。
ACK 表示介面操作完成。SM 分別保存要求與最近完成的回覆，依編號與掛點核對。
掛點選擇及循環由 SM 統一決定，數量由掛點觀測更新；空對空顯示另帶完整性，供 HMCS 呈現數字或未知。`WeaponRearmComplete` 及 `UnlimitedWeaponStationRestore` 事件推進重新掛載版本。

### 資料範圍與來源選擇

掛點介面足以支援目前的逐站庫存、掛載識別、數量統計、候選選站及補彈更新。
SM 以原始類型與識別保存未知型號；飛彈查詢使用類型與用途條件。
目前分類分為 Missile／Other／Unknown，並保存空對空用途；炸彈、副油箱等保留原始識別及數量。
掛架子站、單枚武器備便與副油箱油量需由各自功能的觀測來源補充。
`<CLEAN>` 明確回傳時保存為 Clean；來源回傳 nil 時保存為 Empty。
選站 ACK 記錄座艙呼叫完成，武器備便使用獨立狀態。

過去調查中，座艙測試的 `getGunAmmoCount`、`getCannonsAmmoCount`、`getAmmoCount` 及
`LoGetPayloadInfo` 全域入口皆未取得可用方法；正式路徑已集中為本指引的兩個來源。
Export 的 `Cannon.shells` 已有可核對實作，但屬於 Export 執行環境。

| 證據 | 保留的結論 |
|---|---|
| [Community A-4E，固定版本](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/weapon_system.lua#L907-L915) | avSimpleWeaponSystem 的掛點資料與 count 提供庫存觀測案例 |
| [DCS-ExportScripts A-10A，固定版本](https://github.com/s-d-a/DCS-ExportScripts/blob/0df634fb4edb72450e67f2a6502ead38e54f218b/Scripts/DCS-ExportScript/ExportsModules/A-10A.lua#L428) | Export 環境使用 LoGetPayloadInfo 的 Cannon.shells |
| [A-29B，固定版本](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L894-L905) | 射擊時間扣量屬估算；本專案機砲以原生讀值更新 |

## 指令、參數及顯示

- 機砲操作：MC 的 `GunOn`／`GunOff` 語意 action 由 CMS 映射為生成的 DCS `iCommandPlaneFire`／`iCommandPlaneFireOff`，以 `dispatch_action(nil, command)` 執行。
- `get_base_data()`：取得目前裝置環境的觀測介面；成員依裝置與執行環境核對。本專案掛點與機砲分別使用本指引記錄的來源。
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

## 全域 IR 武器觀測（2026-11-11）

CockpitBridge 以來源目錄中的 IrLock、IrDesiredAzimuth／Elevation、IrTargetAzimuth／Elevation 讀取原始欄位，
驗證有限數值後形成 IrSeekerObservation。此型別只表達來源回報，SM 保存並隨 MC 快照發布。
所有角度維持來源契約的弧度；要求方向與目標方向分別保存。
來源欄位、方向、單位與讀取者由 CommandIds.json 及其產生器維護。

目前來源沒有掛點識別及獨立的接觸、冷卻完成、備便回報。
全零數值是有效樣本；availability 表達讀取結果，與 locked 分別使用。
WS_TARGET_RANGE 已自 IR 讀取群組移除，距離資訊由目標／雷達觀測提供。
資料流與已移除推導見 [IR 觀測整理報告](IR_OBSERVATION_REFACTOR_REPORT.zh-TW.md)。

## 雷達與共用操作傳輸

雷達原生觀測由 CockpitBridge 轉譯至 FrameInput，再由 RadarSystem 經 SystemPipeline 發布。Targeting 依 RadarSnapshot 規劃目標操作。

Lua 的 OBS_MC_ACKNOWLEDGED_SEQUENCE 保留既有參數名稱，現在表示 DcsBridge 共用 DCS 視窗的傳送確認；Core 功能取得 ExternalActionFeedback 的各生產端編號。ACK 表示呼叫已執行，STT 及 IR 鎖定仍由來源觀測提供。

詳見 [Targeting／RadarSystem 架構](RADAR_TARGETING_ARCHITECTURE.zh-TW.md)。

## 原生反制派送：候選入口與實測結果

本輪目標是確認 C++ 能直接施放熱焰彈與干擾絲。測試成功後，反制的程序、
狀態及控制將規劃在透過 SystemPipeline 溝通的 C++ System，座艙處理操作與呈現。
本輪測試工具位於 `DcsBridge/Internal/NativeCountermeasures/`，由 BridgeContext
持有；目前的 MC CountermeasureControl、CMS Lua 程序及 FIFO 操作行為保留供回歸。

### 呼叫與觀測路徑

```text
測試包預綁按鍵 -> ed_fm_set_command -> NativeCountermeasureProbe 佇列
 -> ed_fm_simulate（Core 執行鎖以外）
 -> CockpitBase.dll 具名匯出 -> DCS 原生命令 357 / 358

既有 avSimpleWeaponSystem -> 測試包內的唯讀 getter 取樣
 -> FLR / CHF / CMOBS -> 面板與 CSV
```

| 候選具名入口 | 呼叫契約 | 測試 |
|---|---|---|
| `ed_cockpit_dispatch_action_digital` | `void(int command)` | 357 熱焰彈；358 干擾絲，每次呼叫一次 |
| `ed_cockpit_dispatch_action_analog` | `void(int command, float value)` | 357／358；送 1，至少 40 ms 後送 0 |

來源是 [Community A-4E 的原生標頭](https://github.com/heclak/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/cockpit_base_api.h)
中的函式宣告。宣告提供候選簽章；本專案的 DCS 版本及 EFM 呼叫上下文是否會實際施放，
由 N01–N09 實機測試判定。只解析已載入 CockpitBase.dll 的具名匯出，原生呼叫集中於
Windows SEH 包裝；記錄入口可用性、要求／呼叫數、執行緒與例外碼。

測試包使用 `FM/native-countermeasure-probe.enabled` 啟用；一般模組包預設關閉。
四個測試鍵將要求排至 EFM 更新回呼執行。每次按住僅建立一筆要求，類比按下／放開
共同構成一筆脈衝；SAFE／SIM 清除待執行要求並放開進行中的類比輸入。
飛行結束／重入會清除佇列，原生呼叫失敗時記錄 SEH 例外碼並停止該次飛行的派送。

測試觀測使用 `avSimpleWeaponSystem:get_flare_count()`／`get_chaff_count()`，參考
[A-4E 的反制裝置](https://github.com/heclak/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/countermeasures.lua)。
本機模組的裝置能力另由實測確認。取樣器僅附加在測試包的武器 Lua 更新末端，
CMOBS 位元 1 代表熱焰彈有效、2 代表干擾絲有效；有效零值保持為 0，
缺少 getter／無效回傳則顯示 -1。原生呼叫返回與實際存量減少各自記錄，
前者只證明函式返回。完整操作及回報格式見 [MC 實機流程](MC_FLIGHT_TEST.zh-TW.md)。

### 2026-11-14 實測：兩種派送皆返回，熱焰彈存量保持 90

測試環境為使用者日誌回報的 DCS `2.9.31.29434`、Windows x64 MT。
測試包來源 `717ea0d938708d4990884bdd5da4569bcee085bc`；安裝 DLL 與 BUILD.json
皆為 SHA256 `f4c459b0e93733dec485a5d680d4e3672de548123a72c4d3f3e6beba326da279`。
使用者確認無限彈藥關閉，第一輪原生測試結果如下：

| 熱焰彈入口 | 要求次數 | 正常返回的呼叫 | 熱焰彈存量 | 判定 |
|---|---:|---:|---|---|
| Digital | 20 | 20 | 90 → 90 | 施放驗證未通過 |
| Analog | 20 | 40（按下＋放開） | 90 → 90 | 施放驗證未通過 |

C++ 日誌的 `api_mask=3`，兩種具名入口皆可解析；呼叫期間 exception 為 0。
Analog 的按下／放開間隔為 42 ms。Digital 熱焰彈要求落在模型時間
21.960–28.020 秒，30.309 秒的存量仍為 90；Analog 熱焰彈要求落在
53.928–58.284 秒，60.737 秒的存量仍為 90。

佐證來自這次 `dcs.log` 內既有 ammo-probe-r1 的 Export 診斷：
`LoGetSnares().flare` 與裝置 4 的 `get_flare_count()` 皆回報 90；
第一輪有 14 組存量取樣，三輪合計 56 組。此紀錄確認原生派送函式返回
與實際施放各自需要驗證；上述兩個入口的現行呼叫方式維持為待修正的候選路徑。

本次三份座艙 CSV 只有表頭。`Recorder.lua` 對 `file:write()` 的成功回傳值
使用 assert，而本次 DCS 寫入成功後回傳零個值，初始化於第 40 行中止。
記錄器已加入零回傳值相容處理，並用實際紀錄器驗證 header、資料列、定期 flush
及標準 I/O 錯誤回報。已有表頭的三份紀錄以當次 DCS 日誌交叉核對。

### 2026-11-15 原生派送接收端查核

本機 CMS 只派送 357／358，武器轉接層只處理選站與觀測；兩者皆缺少將這些指令
轉成 `drop_flare()`／`drop_chaff()` 的接收及執行端。公開舊版 avSimpleWeaponSystem
樣本與開源 Skyhawk／A-29B 的接線差異支持此缺口是目前失敗的主要解釋。
目前已證實的是專案內接線缺漏；補上接收端後，仍須以同一 C++ 派送入口及實際彈量
作一次對照，確認目前 DCS 版本的完整路徑。兩個全域 API 的搜尋結果以標頭宣告為主，
可追到真實 C++ 呼叫的是 A-4E 的 `to_device`；Analog 的 1／0 脈衝也缺少作為
數位熱焰彈替代入口的公開契約。完整來源、命令編碼及證據界線見
[原生反制 API 研究紀錄](NATIVE_COUNTERMEASURE_API_RESEARCH.zh-TW.md)。


## DCS 玩家飛機共用介面

`DcsBridge/Internal/Aircraft/DcsAircraftInterface` 由現有 `BridgeContext` 持有。
它集中管理 DLL 識別、符號、程式指紋、物件取得及受保護的原生呼叫，
以資料與操作結果提供機砲及反制能力。原生物件所有權屬於 DCS；各次操作重新取得。

| 方法 | 內容 |
|---|---|
| `read_gun_ammunition()` | 機砲總彈量、來源狀態、版本資訊與例外代碼 |
| `read_countermeasures()` | 熱焰彈、干擾絲庫存及來源狀態 |
| `dispense_countermeasure(kind)` | 單發施放與前後庫存；`returned` 表示原生呼叫返回 |
| `reset_flight()` | 清除本次飛行的解析狀態 |

目前機砲資料來源已接入。反制介面採用 2026-11-15 的 DCS 直接施放實測路徑：
`context -> human -> payload` 讀取兩種庫存，`human` 執行單發施放。
該次結果包 `FCK1C-CM-DIRECT-RESULTS-20261115-233433-950.zip` 記錄
16 次要求／16 次呼叫：熱焰彈 90→80、干擾絲 90→84，每次只扣對應的一發，例外碼為零。
共用介面的重構行為以替代執行環境的原生測試及 Windows CI 驗證。

機砲與反制各自核對其功能所需的版本／程式指紋。模組識別改變時重新解析，
施放前再次比對玩家物件及方法。`DcsAircraftBackend` 提供平台與測試替換的接點。

System 經由 `FrameInput`、`AircraftData`、`SystemPipeline` 與 `FrameOutput`
交換觀測、狀態及動作需求。後續原生雷達、掛載等能力可建立各自 Adapter，
共用這一層的環境存取。詳細分工見
[DCS 飛機介面](../src/efm/F-CK-1C_EFM/DcsBridge/Internal/Aircraft/README.md)。

## Lua裝置更新與指示器父節點

使用`make_default_activity()`並由Lua執行`update()`的裝置，透過`need_to_be_closed = false`保留執行狀態。
本專案的雷達通電更新與飛測Recorder採用這個設定。靜態裝置配置與指示器初始化腳本依各自生命週期載入。
驗證裝置時應從初始化推進到實際更新，核對副作用：Recorder產生表頭後持續新增資料列，雷達呼叫`set_power(true)`。

HMCS VR在同一子頁先建立裁切遮罩，再建立以遮罩為父節點的符號。
子頁測試分別核對父節點可見性、元素唯一性與遮罩／瞄準線保留情況。

安裝工具會備份並移除歷史`FCK1C_AMMO_PROBE`與`FCK1C_STORES_EXPORT`載入區塊，並封存已識別的彈量探針檔案。
其他Export內容與自訂檔案按原始位元組保存。未識別原生命令2659維持原有診斷，另以[issue #37](https://github.com/jjjjj103074/F-CK-1C/issues/37)追蹤。
