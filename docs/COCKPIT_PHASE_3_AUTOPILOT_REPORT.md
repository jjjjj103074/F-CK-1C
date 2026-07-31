# Cockpit Phase 3：Autopilot 遷移報告

## 結論

Phase 3 的程式遷移與自動驗證已完成。AP、A/T、bypass、reference、
controller、disconnect reason 與 Engine Thrust Cut Test 的狀態權威都在
C++；`autopilot_system.lua` 與 AUTOPILOT creator 已移除。

DCS 首次實機驗收目前是 `FAIL`：ALT Hold 出現持續上下擺盪。根因已由
該次 CSV 固定為 ALT 外迴路與 FBW 反應增益／延遲不匹配，C++ 修正與
回歸測試已完成，但仍必須使用新 DLL 複測。在飛行控制方向依
[DCS 手動驗收規範](cockpit-baseline/DCS_TEST_PROTOCOL.md#8-ap-characterization)
重新通過前，Phase 3 不標記為完整 `PASS`。

## 1. 邊界結果

### C++ 擁有

- `FlightControlComputer::AutomaticFlightControl`
  - AP／A/T engage 與 disconnect gate。
  - Pitch、VS、ALT、Heading Hold、Heading Select 與 NAV placeholder。
  - 所有 target reference、integrator、bypass 與 recapture。
  - pitch、roll、throttle demand。
  - engage rejection 與 disconnect reason。
- `PropulsionDiagnostics`
  - Thrust Cut Test toggle／enable／disable。
  - `PropulsionTestIntent` 唯一狀態。
- `CockpitSnapshotExporter`
  - AFCS snapshot 的 DCS parameter presentation。
  - Controls Indicator 使用的相容 `FM_MAXPOWER_SWITCH`。

### Lua／DCS 邊界保留

- Input profile 仍定義玩家可綁定的 command 名稱與按鍵。
- AP／A/T／Thrust Cut command 直接 route 到 EFM，不指定 cockpit device。
- Controls Indicator 只讀 parameter，不保存或推導 AP 與 diagnostics 狀態。
- `devices.lua` 保留 AUTOPILOT ID 9，但 `device_init.lua` 不建立 creator。

### 已移除

- `Cockpit/Scripts/Systems/autopilot_system.lua`。
- Lua → parameter → `CockpitBridge` → Core 的 `AutopilotCommand` 繞路。
- `FrameInput` 中的 legacy AP 與 max-power test 欄位。
- 沒有 reader 的 `FM_MAXPOWER_READY`。

## 2. 最終邏輯鏈

```text
玩家 Input command
  → DCS ed_fm_set_command
  → DcsCommandRouter
  → SystemPipeline command handler
  → FlightControlComputer / PropulsionDiagnostics
  → C++ authoritative state
  → FlightControlDemand / EngineControlDemand / PropulsionTestIntent
  → simulation models
  → FrameOutput
```

座艙呈現是另一條單向鏈：

```text
C++ authoritative state
  → CockpitSnapshot
  → CockpitSnapshotExporter
  → generated DCS parameter
  → Controls Indicator / 未來座艙儀表
```

Lua parameter 不再回流成 AP 控制輸入。

## 3. 功能還原

| 行為 | Phase 3 結果 |
|---|---|
| AP engage gate | IAS 240 kt、WOW、roll 45°、pitch 45° 邊界已固定 |
| AP 預設模式 | engage 時捕捉 Pitch Hold + Heading Hold |
| 垂直模式 | Pitch／VS／ALT capture、reference step 與 controller 已遷移；ALT 首次 DCS 驗收失敗後已修正，待複測 |
| 水平模式 | Heading Hold／Select、0–360 wrap 已遷移 |
| NAV Track | 保留 mode 與零 roll command，沒有新增導航功能 |
| Bypass | pitch／roll 輸出凍結；超過 1° 才 recapture |
| A/T | Mach 0.95 engage gate、Mach 1.00 disconnect、200–550 kt target |
| Master OFF | AP 與 A/T 一起解除，符合舊 `disengage_all()` |
| Thrust Cut Test | 已獨立成 diagnostics system，不再屬於 AFCS |

控制器 band、limit 與 Phase 0 來源相同；Pitch、VS、Heading 與 A/T gain
未改。ALT Hold 首次 DCS 驗收量得 FBW 約 `0.55..0.60 s` 延遲與
`4.1 G / normalized command` 反應，原本與 VS 共用的 `0.08` 比例增益
造成 pitch command 反覆飽和。ALT 因此拆出獨立比例增益 `0.04`，VS
仍使用 `0.08`；這是實機失敗後的必要穩定性修正，不是全面重新調校。
舊 Lua 的 `check_override()` 本來是空函式，Phase 3 沒有新增 stick
override 功能。

## 4. 明確且必要的等價調整

- 舊 Lua Mach guard 每 0.02 s 減少 0.01；C++ 改成每秒減少 0.5，
  使用實際 `dt`。0.02 s reference step 的結果相同，不規則 frame interval
  不再改變每秒控制量。
- IAS 由 EFM true airspeed 與空氣密度換算；其他 AP observation 直接使用
  EFM 已有的高度、垂直速度、Mach、姿態與角速度。
- C++ 內部一律使用 SI 單位；只在 `CockpitSnapshotExporter` 轉成
  ft、deg、kt 與 ft/min。
- Heading Select 使用明確的 initialized flag，不再用 heading `0.0`
  同時表示合法北向與「尚未初始化」。
- 舊 runtime 的 `more than 60 upvalues` 是被修正的載入缺陷，不是需要
  重現的功能。

## 5. 自動驗證

目前已通過：

- DCS ID generator 與 fixture。
- Cockpit baseline reproducibility。
- Cockpit architecture check 與負向 fixture。
- EFM architecture check 與 fixture。
- System catalog generator fixture。
- EFM export contract。
- Release x64 native tests：`2282 checks, 0 failures`。
- Release x64 DLL build；`bin/F-CK-1C_EFM.dll` SHA256：
  `74A87A86D48A6F3BF474D945A23B844472F18A9CEA729F5663FC2134FFBE7B3A`。
- EFM export baseline：37 個名稱一致。

Native tests 涵蓋 gate 邊界、模式捕捉、reference、controller 數值、
heading wrap、NAV placeholder、bypass threshold、disconnect reason、
Mach guard 的不規則 `dt`、snapshot export、command route、CSV telemetry
與 diagnostics thrust cut。另新增使用首次 DCS trace 所量得 FBW
反應的 ALT Hold 閉迴路 damping regression；修正前 4 個條件均失敗，
修正後全部通過。

## 6. DCS 首次驗收與尚待完成

2026-07-31 首次 run 的完整判定與 hash 見
[Phase 3 Autopilot FAIL 證據](cockpit-baseline/evidence/2026-07-31-phase3-autopilot-fail/RUN.md)：

- COLD-CLEAN 的 AP／A/T rejection 通過。
- AIR-CLEAN 已觀察所有 AP lateral／vertical mode、bypass 與 A/T；
  除 ALT Hold 外，測試者沒有發現明顯問題。
- ALT Hold 117.45 秒內對 `1913.361 m` 目標飛到最高 `2037.086 m`，
  pitch command 到達 `-0.6..+0.6`，G 值到達 `-1.772..4.827`，判定
  `FAIL`。
- Vertical／Heading Reference 增減因沒有預設鍵綁定而 `NOT_RUN`。
- RWY-CLEAN Thrust Cut Test 通過。
- `dcs.log` 沒有載入 `autopilot_system.lua`。

修正版已建置並透過 `install.bat` 安裝；source 與 installed DLL hash
同為
`74A87A86D48A6F3BF474D945A23B844472F18A9CEA729F5663FC2134FFBE7B3A`。
目前只需重測 AIR-CLEAN ALT Hold。新證據回收並判定 `PASS` 前，本報告
保持 `FAIL`，不建立假成功紀錄。
