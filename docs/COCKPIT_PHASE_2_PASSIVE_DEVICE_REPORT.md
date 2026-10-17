# Cockpit Phase 2：被動 Device 遷移報告

## 結論

Gear 與 Actuators 的 Lua devices 已移除。ID 1、2 保留但不註冊，
模型動畫繼續由既有 EFM C++ draw-argument 路徑單獨輸出。

## 移除範圍

- `Systems/gear_system.lua`
- `Systems/actuators.lua`
- `Systems/actuators_system.lua`
- `device_init.lua` 中對應的兩個 `avLuaDevice` creators

未變更起落架、前輪轉向或飛控系統的 domain 邏輯，也未重新分配 device ID。

## Draw-argument 契約

Native tests 固定下列 C++ 輸出：

| 類別 | Draw arguments | 驗證 |
|---|---|---|
| 起落架 | 0、3、5 | 同步位置與 0..1 clamp |
| 前輪轉向 | 2 | 正負方向與 -1..1 clamp |
| 升降舵 | 15、16 | 同步方向與 clamp |
| Flaperon | 11、12 | flap 反向視覺量與左右差動 |
| Rudder | 17、18 | 左右同步；-1、中立、+1 與 clamp |
| Airbrake | 21、182、184 | 同步位置與 clamp |
| Slat | 9、10 | 同步位置與 clamp |
| Afterburner | 28、29 | 左右引擎獨立值與 clamp |
| Nozzle | 89、90 | 已固定的左右引擎對應與 clamp |
| Wheel spin | 76、101、102 | nose、left、right 對應 |

`ed_fm_set_draw_args` 的既有 boundary validator 仍會拒絕 null 或長度不足的
buffer，不以部分寫入掩蓋錯誤。

## 行為差異

- Gear Lua 原本沒有有效行為，移除不改變輸出。
- Actuators Lua 原本與 C++ 重複負責 rudder，而且真實 DCS 證據顯示它在
  初始化時因缺少 `angle_of_draw_left_rudder` 而載入失敗。
- 移除後，該 Lua 載入錯誤不再存在；這是已記錄缺陷的移除，不是新增飛行功能。

## 自動驗證

- Native EFM tests 覆蓋完整 draw-argument 投影、方向、中立、極值與 clamp。
- Cockpit architecture checker 禁止重新加入兩個 creators 或三個已刪檔案。
- Baseline generator 固定 22 個 Cockpit 檔案、7 個 DCS devices 與 3 個 indicators。
- Release x64 DLL、EFM exports、產生器與 architecture checks 必須全部通過。

## DCS 驗收證據

Phase 0 的真實 DCS run 已證明舊 Actuators Lua 沒有成功載入，因此該次 runtime
的 rudder draw arguments 已由 C++ 路徑提供。

Phase 2 新版本已在 2026-07-31 完成 AIR-CLEAN、COLD-CLEAN、RWY-CLEAN
三次獨立 DCS run。證據保存在
[`cockpit-baseline/evidence/2026-07-31-phase2-passive-devices/`](cockpit-baseline/evidence/2026-07-31-phase2-passive-devices/)。

- 三次都載入與 source artifact 相同 hash 的 Phase 2 DLL。
- 三次都沒有 Gear／Actuators Lua 載入嘗試或缺檔錯誤。
- AIR-CLEAN 覆蓋飛控、flap／slat、airbrake、gear、afterburner 與 nozzle。
- COLD-CLEAN 覆蓋 cold-ground、gear-down 與 NWS gate。
- RWY-CLEAN 覆蓋 NWS、三輪 wheel-spin 與左右煞車。

鼻輪 3D 外觀仍不隨 NWS 轉向；CSV 中的 C++ NWS 輸出已正確到達
`-0.75..+0.75`，因此沿用既有
[Issue #21](https://github.com/jjjjj103074/F-CK-1C/issues/21) 追蹤，
不判定為本次 Lua device 移除造成的 regression。Phase 2 遷移驗收為 `PASS`。
