# EFM 重構藍圖

這份文件配合原始碼中的 `EFMREF` 註解使用。第一階段的目標不是改飛行行為，而是先把「DCS 必須看到的介面」和「我們自製、可重構的模擬邏輯」分清楚。

## 分類標籤

`DCS_CONTRACT`
: DCS 直接呼叫或依賴的 exported callback/API。包含 `ed_fm_*`、DLL ABI、draw args、`ed_fm_get_param()` index 回傳語意、fuel/mass/state callback。可以把內容搬到物件裡，但函式名稱與 signature 不要隨意改。

`DCS_BRIDGE`
: 介於 DCS/Lua/config 與 EFM 內部狀態之間的轉接層。包含 cockpit param handle、Lua autopilot command、`FM/config.lua` path/config 讀取、draw-arg mapping、command ID mapping。這層應該變薄，最後集中到 `DcsInterface/` 或 `DcsIds/`。

`CUSTOM_SYSTEM`
: 自製飛行模型邏輯。包含 FBW、aero、engine、fuel、landing gear、suspension、damage、input arbitration、force/moment accumulation。這些是物件導向重構的主體。

`COMMON_UTIL`
: 無狀態工具。包含 clamp、units、table interpolation、Vec3、path helper、actuator helper。資料夾可叫 `Common/` 或 `Util/`；我偏好 `Common/`，避免 `Util/` 變成什麼都丟的雜物箱。

`DIAGNOSTICS`
: debug log、probe、watch output、文字格式化。應該可以一鍵關閉，並且不要混進飛行控制律。

`LEGACY_CANDIDATE`
: 尚未接進主流程、重複、或需要決定是否保留的 prototype。重構時先盤點，不急著搬。

## 目標架構

建議最後長成這樣，名稱可以依你的習慣微調：

```text
src/efm/F-CK-1C_EFM/
  DcsInterface/
    DcsCallbacks.cpp
    DcsCockpitBridge.h/.cpp
    DcsParamRouter.h/.cpp
    DcsDrawArgsWriter.h/.cpp

  DcsIds/ 或 Generated/
    DcsCommands.g.h
    DcsParams.g.h
    DcsDrawArgs.g.h
    DamageIds.g.h

  Core/
    Fck1cEfm.h/.cpp
    AircraftState.h
    AircraftConfig.h
    ForceMoment.h

  Systems/
    InputSystem.h/.cpp
    AutopilotBridge.h/.cpp
    FBWController.h/.cpp
    AerodynamicsModel.h/.cpp
    EngineSystem.h/.cpp
    FuelSystem.h/.cpp
    LandingGearSystem.h/.cpp
    SuspensionSystem.h/.cpp
    DamageModel.h/.cpp

  Data/
    AeroTables.h/.cpp
    EngineTables.h/.cpp

  Common/
    Vec3.h
    Units.h
    Clamp.h
    Interpolation.h
    Table.h
    PathUtils.h

  Diagnostics/
    DebugLogger.h/.cpp
    DebugWatch.h/.cpp
```

開源 DCS EFM 參考方向：A-4E-C 把 EFM 拆在 `ExternalFM/FM`，有 `FlightModel`、`AircraftState`、`Engine`、`FuelSystem`、`Input`、`Logger`、`Table`、`Units` 等分工；UH-60L 則是比較典型的 DCS module package/layout 參考，但不是同等級的 C++ EFM 架構參考。

## 逐步重構順序

### 0. 標記與盤點

已完成第一版：

- 在主要函式前加上 `EFMREF` 分類。
- 在主要全域狀態群前加上 `EFMSTATE` owner 分類。
- 新增 `docs/EFM_STATE_INVENTORY.md` 作為 state ownership 盤點表。
- 不移動、不改邏輯。
- 後續可用 `rg "EFMREF|EFMSTATE"` 追蹤拆分進度。

下一步補強：

- 對仍未拆分的 global config/table 做更細的 immutable/mutable 分類。
- 依 `docs/EFM_STATE_INVENTORY.md` 決定第一批可移動的 state owner。

### 1. 建立安全網

先確保每一步都能回頭比較：

- 建立 DLL build 驗證命令與紀錄方式。
- 固定一份 debug-watch output baseline。
- 把目前已知不應該順手修的 bug 寫入待辦，例如 fuel external/internal 邏輯、temperature unit、throttle table size。

這階段只建立驗證，不主動改飛行手感。

### 2. 先抽 Common 與 Diagnostics

低風險優先：

- `Utility.h` 拆成 `Common/Units.h`、`Common/Clamp.h`、`Common/Interpolation.h`、`Common/Vec3.h`。
- path helper 拆到 `Common/PathUtils.h/.cpp`。
- `dbg_susp()`、`susp_probe_log()`、`suspension_debug_log()`、`ed_fm_debug_watch()` 的格式化拆到 `Diagnostics/`。

注意：

- `Utility.h` 目前有 non-inline `smooth_lerp()` 定義在 header，之後應該改成 `inline` 或移到 `.cpp`。
- `FM_data.h` 已改成 `extern` 宣告加 `.cpp` 定義；下一步應整理成 immutable config/table object。

### 3. 建立 Core 外殼

新增 `Core/Fck1cEfm`，但一開始只包住舊流程：

```cpp
void ed_fm_simulate(double dt)
{
    g_efm.simulate(dt);
}
```

初期 `simulate()` 可以仍然呼叫舊 helper 或操作現有全域變數。重點是先讓 DCS callback 成為 thin wrapper。

### 4. 搬 AircraftState

依系統把全域狀態搬到 `AircraftState`，一次只搬一群：

- atmosphere / kinematics
- pilot input / command state
- engine state
- fuel and mass state
- landing gear / suspension state
- FBW state
- damage state
- output force/moment

每搬一群都要能 build，並確認 `ed_fm_get_param()`、draw args、debug watch 還能讀到同樣資料。

### 5. 拆 Systems

建議順序：

1. `InputSystem`
2. `AutopilotBridge`
3. `FuelSystem`
4. `EngineSystem`
5. `LandingGearSystem`
6. `SuspensionSystem`
7. `DamageModel`
8. `FBWController`
9. `AerodynamicsModel`

`FBWController` 和 `AerodynamicsModel` 放後面，因為最容易改變飛行手感。前面的 input、fuel、engine、gear 比較適合先練出拆分節奏。

### 6. 集中 DCS ID 與 Lua 同步

第一步不要直接做 generator。建議分三段：

1. 先建立 `DcsIds/`，集中 C++ 目前散落的 command/drawarg/param/damage ID。
2. Lua 端也集中到 `Cockpit/Scripts/generated/` 或 `Cockpit/Scripts/common/`。
3. 最後再決定是否用單一來源產生 `.lua` 與 `.g.h`。

`FM/config.lua` 建議保留給 DCS FM/suspension/mass/inertia contract，不要把所有 command ID 都塞進去。

### 7. 清掉 Legacy 與實驗碼

等主要系統分離後再處理：

- `FM_data.cpp` 裡未接入主流程的 `MassModel`。
- suspension startup probe / fallback ground force 的常駐 debug 開關。
- hardcoded log path。
- `ed_fm_get_param()` 裡容易誤讀的 nested `case` 結構。

## 第一批實作建議

我建議下一個實作 PR/commit 做這些就好：

1. 新增 `Common/`，先搬 `Vec3`、`Units`、`Clamp`、`Interpolation`。
2. 新增 `Diagnostics/DebugLogger`，把三個 log writer 包起來。
3. 不改 `ed_fm_simulate()` 本體。
4. Build DLL，確認 exported callback 還存在。

這樣第一刀很小，但會立刻改善可讀性，並讓後面拆系統不需要一直碰基礎工具。
