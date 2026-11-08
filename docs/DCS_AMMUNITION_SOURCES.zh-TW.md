# DCS 開源模組彈量來源調查

> 歷史研究紀錄。2026-11-08 已取得本專案 C++ 原生機砲讀取的實機證據；目前介面、版本與資料流以 [DCS API 指引](DCS_API_GUIDE.zh-TW.md) 為準。以下保留當時的調查與來源。


調查日期：2026-11-02。範圍為開源飛機模組、開源匯出腳本與 ED 官方介面說明。本文記錄來源核對結果與本專案的實作方向；本輪變更為研究文件。

## 結論

目前已有原始碼依據的原生機砲彈量入口是 **Export 環境的 `LoGetPayloadInfo().Cannon.shells`**。開源 DCS-ExportScripts 的 A-10A 與 F-15C 匯出程式都讀取此欄位。這項證據確認了介面的實際使用方式；F-CK-1C 的回傳數值仍須在相同環境取得。

掛點庫存則由武器裝置的 `get_station_info(index)` 取得，讀取 `count`、`CLSID` 與武器類型。A-4E、A-29B、SK60 均有這類實作。本專案已使用此路徑，且使用者的 DCS log 已證實呼叫成功。

機砲餘彈與掛點數量有不同的來源。A-29B 座艙顯示的機砲數量採用射擊時間估算，不能把其顯示參數當作已取得原生彈量的證據。

## 開源實作比較

| 專案／裝置 | 掛載或彈量取得方式 | 對本專案的意義 |
|---|---|---|
| Community A-4E／`avSimpleWeaponSystem` | `GetSelf()` 取得武器裝置，再讀取各掛點資料與數量 | 與目前掛點轉接層使用相同介面 |
| A-29B／`avSimplestWeaponSystem` | 掛點讀取原生資料；內建機砲初始化為每側 250 發，再按射擊時間扣除 | 機砲為估算，與遊戲實際初始裝填及消耗是兩套資料 |
| SK60／`avSimpleWeaponSystem` | 依掛點索引讀取 CLSID 與火箭數量 | 提供另一個掛點介面的實作案例 |
| DCS-ExportScripts／A-10A、F-15C | 在 Export 腳本讀取 `LoGetPayloadInfo` 的 `Cannon.shells` | 有實際使用原生機砲彈量欄位的程式碼 |
| DCS-ExportScripts／Ka-50 | 取得 Ka-50 的武器裝置，再呼叫 `get_selected_gun_ammo_count()` | 屬機型專用裝置介面；需與本專案的裝置類別分開評估 |

### A-4E

核對版本：`34c1ef439581234ee24d4bde2e13a54999ebaa5a`。

- 裝置註冊為 `avSimpleWeaponSystem`，腳本使用 `GetSelf()`；槍莢處理讀取掛點資料及 `count`。來源：[裝置註冊](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/device_init.lua#L54)、[掛點處理](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/weapon_system.lua#L907-L915)。
- 作者保留的武器裝置方法清單含掛點、干擾絲及熱焰彈操作，未列出機砲餘彈讀取。這是該份原始碼的介面紀錄，適用範圍限於其記錄的裝置與版本。來源：[方法清單](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/weapon_system.lua#L1838-L1861)。
- C++ 有 `getGunShells` 成員宣告，但對應綁定仍為註解，符號名稱留空。因此它沒有形成可直接採用的讀取實作。來源：[宣告](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/include/Cockpit/ccParametersAPI.h#L81)、[綁定狀態](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/include/Cockpit/ccParametersAPI.h#L172)。

### A-29B

核對版本：`1f632f004e28ef13a5f3404528de4c3fc982a0e2`。

- 左右機砲分別初始化為 250 發；射擊期間依經過時間與射速扣除。來源：[初始值](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L53-L54)、[射擊扣除](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L894-L905)。
- 收到 `WeaponRearmComplete` 時，兩側重設為 250。來源：[補彈處理](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L1218-L1223)。
- 掛點使用 `get_station_info(i)` 及原生 `count`；顯示用的機砲參數則接收另行保存的估算值。來源：[掛點與顯示參數](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L147-L193)。

### SK60

核對版本：`ce578058da225ae8cf00b1a2a8cdd1dee48edac5`。

裝置使用 `avSimpleWeaponSystem`。腳本透過零起算的掛點索引取得槍莢識別及火箭數量。本次核對的程式沒有內建機砲餘彈讀取。來源：[裝置註冊](https://github.com/BAAS-Dynamic/DCS-SK60-Mod/blob/ce578058da225ae8cf00b1a2a8cdd1dee48edac5/Cockpit/Scripts/device_init.lua#L22)、[掛點處理](https://github.com/BAAS-Dynamic/DCS-SK60-Mod/blob/ce578058da225ae8cf00b1a2a8cdd1dee48edac5/Cockpit/Scripts/Systems/weapon_system.lua#L151-L169)。

### DCS-ExportScripts

核對版本：`0df634fb4edb72450e67f2a6502ead38e54f218b`。

- A-10A 先呼叫 `LoGetPayloadInfo()`，再將 `Cannon.shells` 作為機砲數量輸出。來源：[讀取 payload](https://github.com/s-d-a/DCS-ExportScripts/blob/0df634fb4edb72450e67f2a6502ead38e54f218b/Scripts/DCS-ExportScript/ExportsModules/A-10A.lua#L394)、[輸出彈量](https://github.com/s-d-a/DCS-ExportScripts/blob/0df634fb4edb72450e67f2a6502ead38e54f218b/Scripts/DCS-ExportScript/ExportsModules/A-10A.lua#L428)。F-15C 也有同一欄位的讀取。來源：[F-15C](https://github.com/s-d-a/DCS-ExportScripts/blob/0df634fb4edb72450e67f2a6502ead38e54f218b/Scripts/DCS-ExportScript/ExportsModules/F-15C.lua#L1280)。
- 其框架使用 `LuaExportAfterNextFrame` 等 Export 回呼；這些腳本的執行位置與飛機內部座艙裝置不同。來源：[Export 回呼](https://github.com/s-d-a/DCS-ExportScripts/blob/0df634fb4edb72450e67f2a6502ead38e54f218b/Scripts/DCS-ExportScript/ExportScript.lua#L105-L113)。
- Ka-50 透過 `GetDevice(12)` 取得該機的武器裝置，使用 `get_selected_gun_ammo_count()`。這證明機型專用裝置確實可以提供機砲彈量方法；並未證明 `avSimpleWeaponSystem` 有相同方法。來源：[Ka-50 裝置與讀取](https://github.com/s-d-a/DCS-ExportScripts/blob/0df634fb4edb72450e67f2a6502ead38e54f218b/Scripts/DCS-ExportScript/ExportsModules/Ka-50.lua#L671-L686)。

## 為什麼本專案先前沒有讀到

目前 `cms_system.lua` 在 `get_base_data()` 的物件上嘗試 `getGunAmmoCount`、`getCannonsAmmoCount` 與 `getAmmoCount`。使用者四次任務的實際 log 均回報三者為 `nil`；本次查核的開源模組也沒有提供這三個方法的使用證據。

`LoGetPayloadInfo` 的探測則只查看座艙腳本的全域函式，結果同樣為 `nil`。此結果說明目前座艙執行位置沒有提供該全域入口；Export 腳本有其自己的執行環境與介面。

因此，應修正的重點是資料來源與執行位置，而非繼續增加候選函式名稱。

另外，ED 官方文件提供任務腳本的 `Unit:getAmmo()`，可取得單位彈藥資料。它屬於任務腳本介面，適合作為獨立交叉核對來源；接入座艙與 MC 仍需跨環境傳遞。來源：[ED Unit API](https://www.digitalcombatsimulator.com/en/support/faq/1262/)。

## 對本專案的建議（依據上述來源的設計推論）

1. **掛點資料沿用目前來源。** 每站識別、數量與類型繼續由 `get_station_info` 提供。
2. **機砲原生來源優先查證 Export API。** 在 Export 環境取得 `Cannon.shells`，連同本機身分、時間與讀取結果記錄。這樣可以直接核對初始裝填、射擊消耗與重新裝填的原生結果。
3. **接回 MC 的通道另外明確定義。** 來源確認後，以轉接層送入現有機砲彈量輸入，保留來源有效性與任務生命週期；MC 的掛載管理仍只處理彈量資料。本文尚未選定跨環境傳輸方法。
4. **估算數值清楚標示來源。** 診斷與對外資料應區分原生讀值與射速估算，讓回報結果能直接判讀。

這是有原始碼依據的實作方向，而非 F-CK-1C 原生讀取已完成的驗收宣告。先前提供的 log 沒有 Export 環境的 payload 讀值；現有四個任務足以證實舊有候選函式讀取失敗。

## 調查邊界

本次檢查的 A-4E、A-29B、SK60 沒有建立可套用至本專案的座艙內建機砲 getter。VSN F-104 與 F-4 公開儲存庫主要提供發行內容，未找到可核對的相關座艙原始碼。上述結果限定於本輪檢查來源。

掛點索引為零起算；內建機砲的索引及 API 必須由對應來源確認。本文沒有把第一掛點視為內建機砲。

歷史補充：SimShaker 作者曾於 2021 年在 Mi-24P 的錯誤區回報 `Cannon.shells` 固定為 1000。該紀錄適用於當時的回報，不能直接套成目前 F-CK-1C 的結果；它說明介面回傳數字後仍應核對實際變化。來源：[作者原始回報](https://forum.dcs.world/topic/274756-dcs-export-cannon-shell-is-always-1000/)。
