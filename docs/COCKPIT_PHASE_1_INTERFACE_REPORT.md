# Cockpit Phase 1 — ID／Interface 完成報告

## 結論

Phase 1 已建立後續遷移需要的穩定邊界，但沒有提前搬移 Autopilot、
Fire Control、Radar、Stores、HMCS 或 Audio 的 domain logic。

目前的資料鏈是：

```text
DCS／Lua parameter
    -> CockpitBridge
    -> typed CockpitObservation
    -> FrameInput
    -> Core step

Core FrameOutput
    -> immutable CockpitSnapshot value
    -> CockpitSnapshotExporter
    -> DCS／Lua parameter
```

## 已完成的六項工作

### 1. Device ID 固定

`Cockpit/Scripts/devices.lua` 已改成明確數字，不再依照宣告順序自動加一。

| ID | Phase 1 名稱 | 後續用途 |
|---:|---|---|
| 1 | `Gear` | 移除後保留，不重用 |
| 2 | `Actuators` | 移除後保留，不重用 |
| 3 | `CMS` | 後續 DCS Action Adapter |
| 4 | `WEAPON_SYSTEM` | 後續 Weapon Station Adapter |
| 5 | `HMCS` | 後續 HMCS Presenter |
| 6 | `AAM_AUDIO` | Audio Adapter |
| 7 | `RADAR` | `avSimpleRadar` Adapter |
| 8 | `RADAR_STATE` | 移除後保留，不重用 |
| 9 | `AUTOPILOT` | 移除後保留，不重用 |

Phase 1 刻意不重新命名現有 Lua key，避免介面準備和 device rename
混成同一個風險。架構檢查會阻止這九個數字被壓縮或重配。

### 2. CommandId／Router 擴充

所有現有 cockpit domain command 都已有對應的 semantic `Core::CommandId`
與 Router binding，包括：

- Trigger／Uncage／Weapon Release 等 hold command。
- Master Arm、TMS、CMS、模式切換等 momentary command。
- Autopilot／Auto Throttle／Thrust Cut Test command。

這些 command 的 catalog route 現在仍是 `cockpit`，所以
`ed_fm_set_command` 仍會忽略它們，現有 Lua 行為不會被提前切換。
`inspect_command_binding()` 只供測試檢查未啟用 binding；正式路由仍只走
`map_command()`。

`SoundTestCycle` 沒有加入 Core，因為它是 Audio Adapter 的資源測試，
不是飛機系統 command。

### 3. Parameter catalog

`DcsIds/CommandIds.json` 現在是完整 source of truth：

- 66 個 module-owned cockpit parameters。
- 17 個 DCS-owned raw Radar／IR／Weapon parameters。
- 每個 custom parameter 都記錄 direction、unit、目前 writer 與
  target owner。
- 需要進入 C++ 的 parameter 明確記錄 `cpp_reader`。
- 每個 raw parameter 都記錄 `dcs_to_cpp` 方向、單位、原生 DCS
  軸向契約、驗證依據與來源連結；DcsBridge 不會自行翻轉正負號。

Generator 會驗證 symbol、值、route、direction、unit、writer metadata
與重複值，並產生 C++／Lua constants。Raw DCS parameter 與 module-owned
parameter 使用不同 namespace，避免把 DCS observation 誤認為本模組狀態。

### 4. Typed Observation

`Core::CockpitObservation` 已加入 `FrameInput`，包含三個明確群組：

| Observation | 資料來源 | availability／revision 規則 |
|---|---|---|
| `RadarObservation` | DCS raw Radar parameters | 全組可讀才 available；成功樣本才增加 C++ revision |
| `IrSeekerObservation` | DCS raw Weapon／IR parameters | 全組可讀才 available；成功樣本才增加 C++ revision |
| `WeaponStationObservation` | `avSimpleWeaponSystem` Lua Adapter | Lua 掃描時增加 revision；API error 明確標成 unavailable |

每個欄位名稱都帶單位，例如 `_rad`、`_m`、`_normalized`。合法的零值仍是
available，不使用「總和非零」或 `0` 代表 unavailable 的推測。

`ObservationInvalidReason` 目前定義：

- `None`
- `NotProvided`
- `ParameterUnavailable`
- `InvalidNumeric`
- `InvalidRevision`
- `StationApiError`

### 5. Cockpit Snapshot／Exporter

`FrameOutput` 已加入 value-type `CockpitSnapshot`。每次 Core step：

- outer snapshot 為 available；
- revision 單調增加；
- snapshot time 與 `FrameOutput.simulation_time_s` 相同。

Phase 1 只輸出 envelope 的三個 parameters：

- `CPP_COCKPIT_SNAPSHOT_AVAILABLE`
- `CPP_COCKPIT_SNAPSHOT_REVISION`
- `CPP_COCKPIT_SNAPSHOT_TIME_S`

AFCS、Combat Avionics 與 HMCS 的 typed payload 已有 contract，但在所屬
遷移 phase 完成前維持 nested `available=false`。這不是假資料，也不會覆寫
目前 Lua domain parameters。

### 6. 單一 writer 檢查

Phase 0 揭露的六組多 writer 已整理成：

| Parameter | 唯一 writer |
|---|---|
| `AIM9_MISSILE_COUNT` | `cms_system.lua` |
| `AIM9_MISSILE_STATUS` | `cms_system.lua` |
| `AIM9_TONE_STATE` | `radar_state_system.lua` |
| `AIM9_WEAPON_ACTIVE` | `radar_state_system.lua` |
| `HMCS_DOGFIGHT_MODE` | `cms_system.lua` |
| `HMCS_MASTER_MODE` | `cms_system.lua` |

`check_cockpit_architecture.ps1` 現在會拒絕：

- 同一 output parameter 有多個 writer。
- catalog writer 與實際 writer 不一致。
- C++ writer 缺少實際 `cockpit_parameter_writer(...)` typed endpoint。
- C++ `.cpp` 直接繞過 endpoint 呼叫 DCS numeric update API。
- 未登錄的 raw parameter literal。
- device ID 改號或新增隱式 ID。
- DCS-only API 出現在錯誤 Adapter。

Checker 和 generator 都有 rejection fixtures，避免檢查器只會通過正常案例。

## 明確沒有在 Phase 1 做的事

- 沒有把任何 cockpit command route 改成 `efm`。
- 沒有讓新增的 Core command 改變飛機狀態。
- 沒有把 Radar／AIM-9 判斷邏輯搬入 C++。
- 沒有讓 Lua 消費新的 C++ domain snapshot。
- 沒有刪除任何 device 或 Lua domain implementation。
- 沒有新增完整 Radar、HMCS targeting 或儀表功能。

## 自動驗證

Phase 1 的完成條件由下列檢查覆蓋：

- DCS ID generator determinism 與 duplicate rejection fixture。
- Cockpit baseline reproducibility。
- Cockpit architecture checker 與 rejection fixtures。
- EFM dependency／file-size architecture checker 與 fixtures。
- Release x64 DLL build。
- Native tests。
- DLL export baseline check與 `git diff --check`。

DCS 實機驗收仍依 `docs/cockpit-baseline/DCS_TEST_PROTOCOL.md` 執行。
Phase 1 沒有刪除舊實作，因此未完成的 armed／cold DCS 情境不會被宣稱通過。
