# 座艙

座艙入口為 `Cockpit/Scripts/device_init.lua`。
C++ MC 負責任務模式、掛載策略、武器資格、感測器協調、尋標及武器顯示決策；
Lua 負責 DCS 觀測／操作介面、反制程序、繪圖與播放。
完整元件、資料庫及遷移範圍見 [MC 變更報告](MC_MIGRATION_REPORT.md)。

## Device 與狀態

| Device | DCS class / 腳本 | 目前責任 |
|---|---|---|
| CMS | avLuaDevice / cms_system.lua | 50 Hz 數量 API 觀測、有序 DCS action 執行、反制程序及執行互鎖 |
| WEAPON_SYSTEM | avSimpleWeaponSystem / weapon_system.lua | 20 Hz 原始七站觀測、執行 MC 選站要求及確認 |
| RADAR | avSimpleRadar / RADAR/FCK1C_Radar.lua | DCS 雷達設定與原始 DCS 觀測來源 |
| RADAR_STATE | avLuaDevice / radar_state_system.lua | 50 Hz 維持原有雷達開啟參數介面 |
| HMCS | avLuaDevice / hmcs_system.lua | 64 Hz 原始飛行資料取樣、單位呈現、航向刻度與繪圖參數 |
| AAM_AUDIO | avLuaDevice / aam_audio_system.lua | 20 Hz 依 MC tone 要求選資源、播放及 Sound Test |

MC 外層與內部週期工作為 32 Hz；介面的取樣／播放頻率各自保留。
Controls Indicator、HMCS 2D／VR 與 EFM Debug Indicator 由各自的指示器腳本呈現。
Gear、Actuators 與 Autopilot device ID 保留；目前未建立其 Lua device。
起落架與致動器由 Core Systems 執行，DFLCC 的 AP／A/T 狀態快照不可用。

## 命令與參數

`Input/F-CK-1C/` 定義鍵盤與搖桿綁定。
`DcsIds/CommandIds.json` 定義自訂 ID、路由、DCS action 與參數 writer。

- `efm`：進入 ed_fm_set_command，由 DcsCommandRouter 轉為 Core 命令。
- `bridge`：由 DcsBridge 工具處理，例如 Debug Indicator 開關。
- `cockpit`：送至指定 cockpit device。

MC/HOTAS 指令走 `efm`，由 MC FIFO 在下一次 tick 依序處理。
`mission_weapon_input.lua` 在載入鍵盤／搖桿設定後，將原生開火與投放別名
（含共用設定繼承項目）轉成 MC 的 TriggerSecondStage／WeaponRelease。
原按鍵組合保留，重複指令合併；DCS 原生 actuation ID 留在 MC 核准後的 Lua 呼叫端。
其他既有模式、感測器與反制測試入口保留，不能將其視為全部經過 MC。
AP／A/T 命令目前仍有 ID 與綁定，對應控制功能尚未實作。

模組參數的 direction 為 cpp_to_lua、lua_to_cpp 或 lua_internal。
DCS 自有 radar／weapon 參數另有白名單。
新增或調整 ID 後，依 [DcsIds 指南](../src/efm/F-CK-1C_EFM/DcsIds/README.md)
執行產生器，再執行座艙架構檢查。

HMCS 取得磁航向與氣壓高度，發布 availability 及原始 rad／m。
CockpitBridge 將其轉為 Core 的 degree／ft 觀測；缺失值保持不可用。
單位與正負號見[單位規範](EFM_UNIT_CONVENTIONS.md)。

## 掛載與彈量

外掛清單由 `avSimpleWeaponSystem:get_station_info(0..6)` 取得。
Lua 發布原始紀錄；CockpitBridge 驗證整份 revision 後，轉為具型別觀測送入 MC。
分類、數量來源優先序及重新選站屬於 StoresManagement。
C++ 現在已可使用這份資料；目前附帶 EFM SDK 沒有對等完整查詢，因此仍需 Lua 入口。

`cms_system.lua` 的 `get_base_data()` 彈量函數沿用既有候選名稱與探測順序。
依使用者先前正常運作的經驗保留此路徑；本次沒有證據判定這些函數不存在。
各名稱是否由目前 DCS 物件提供、回傳單位及武器範圍，留待遊戲內驗證。
能力檢查、有效零值及無有效來源時的原有估算保留。

## 驗證

```powershell
npm run test:dcs-id-generator
npm run test:cockpit-architecture
```

`tools/test_mc_lua.lua` 以 Lua 5.1 執行實際介面與輸入設定；測試 fixture 不代表 DCS API 已實測。
來源清冊與參數關係見[座艙來源清冊](cockpit-inventory/README.md)。
Device 載入、DCS 感測、呈現、武器與音效依 [DCS 測試](DCS_TESTING.md) 驗證。
