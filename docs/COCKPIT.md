# 座艙

座艙入口為 `Cockpit/Scripts/device_init.lua`。
EFM 負責飛機系統與物理模擬；Lua 仍負責下表列出的座艙狀態、DCS 操作與呈現。

## Device 與狀態

| Device | DCS class / 腳本 | 目前責任 |
|---|---|---|
| CMS | avLuaDevice / cms_system.lua | Master Arm、Fire Control mode、TMS、武器 gate、CMS 程序與 DCS action |
| WEAPON_SYSTEM | avSimpleWeaponSystem / weapon_system.lua | DCS 武器站、選擇與掛載觀測 |
| RADAR | avSimpleRadar / RADAR/FCK1C_Radar.lua | DCS 雷達設定 |
| RADAR_STATE | avLuaDevice / radar_state_system.lua | 雷達電源、模式、目標／尋標器狀態與 AIM-9 tone 決策 |
| HMCS | avLuaDevice / hmcs_system.lua | HMCS 狀態、DCS 感測取樣與呈現參數 |
| AAM_AUDIO | avLuaDevice / aam_audio_system.lua | 依 tone 狀態選取音效資源、播放與 Sound Test |

Controls Indicator、HMCS 2D／VR 與 EFM Debug Indicator 由各自的指示器腳本呈現。
Gear、Actuators 與 Autopilot device ID 保留；目前未建立其 Lua device。
起落架與致動器由 Core Systems 執行，DFLCC 的 AP／A/T 狀態快照不可用。

## 命令與參數

`Input/F-CK-1C/` 定義鍵盤與搖桿綁定。
`DcsIds/CommandIds.json` 定義自訂 ID、路由、DCS action 與參數 writer。

- `efm`：進入 ed_fm_set_command，由 DcsCommandRouter 轉為 Core 命令。
- `bridge`：由 DcsBridge 工具處理，例如 Debug Indicator 開關。
- `cockpit`：送至指定 cockpit device。

路由宣告描述傳輸位置；實際功能由接收端實作決定。
AP／A/T 命令目前仍有 ID 與綁定，對應控制功能尚未實作。

模組參數的 direction 為 cpp_to_lua、lua_to_cpp 或 lua_internal。
DCS 自有 radar／weapon 參數另有白名單。
新增或調整 ID 後，依 [DcsIds 指南](../src/efm/F-CK-1C_EFM/DcsIds/README.md)
執行產生器，再執行座艙架構檢查。

HMCS 取得磁航向與氣壓高度，發布 availability 及原始 rad／m。
CockpitBridge 將其轉為 Core 的 degree／ft 觀測；缺失值保持不可用。
單位與正負號見[單位規範](EFM_UNIT_CONVENTIONS.md)。

## 驗證

```powershell
npm run test:dcs-id-generator
npm run test:cockpit-architecture
```

來源清冊與參數關係見[座艙來源清冊](cockpit-inventory/README.md)。
Device 載入、DCS 感測、呈現、武器與音效依 [DCS 測試](DCS_TESTING.md) 驗證。
