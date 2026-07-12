# EFM State Inventory

這份文件是第一步盤點結果，搭配原始碼中的 `EFMSTATE` 註解使用。此步驟只標記 state ownership，沒有改變模擬邏輯。

## Ownership Map

| Owner | 目前狀態群 | 未來目標 | 測試重點 |
| --- | --- | --- | --- |
| `Core/ForceMoment` | `common_force`、`common_moment`、force/moment accumulator | `Core/ForceMoment.h/.cpp` | 飛機仍能產生力與力矩，無突然無推力或無升力 |
| `Core/AircraftState` | DCS 提供的速度、角速度、wind、AOA/AOS、altitude、mach、g | `Core/AircraftState.h` | debug watch 的 ASL/AGL/Mach/G 值仍合理 |
| `Core/StartupState` | `sim_inititalised`、startup visual state | `Core/Fck1cEfm` 或 `StartupState` | cold/hot/air start 初始化狀態正常 |
| `Core/SimulationClock` | `fm_clock` | `Core/Fck1cEfm` | debug log 時間持續增加 |
| `Common/Units` | `pi`、`rad_to_deg` | `Common/Units.h` | 不應影響行為 |
| `Data/AircraftConfig` | `FM_DATA` 幾何資料、`S`、`wingspan`、`length`、`height` | `Data/AircraftConfig` | 飛行阻力/升力量級未明顯變化 |
| `Data/AeroTables` | `mach_table`、`cx0`、`Cya`、`CyMax`、`Aldop`、`OmxMax` | `Data/AeroTables` | 低速/高速手感、失速前後行為 |
| `Data/EngineTables` | thrust、throttle、engine spool/fuel constants | `Data/EngineTables` | 油門反應、RPM、AB、推力 |
| `Systems/InputSystem` | pitch/roll/yaw、trim、axis/keyboard throttle、brake inputs | `Systems/InputSystem` | 軸/鍵盤輸入、trim、左右煞車、油門切換 |
| `Systems/AutopilotBridge` | cached AP pitch/roll/throttle commands | `Systems/AutopilotBridge` | AP 開關、pitch/roll command、自動油門 |
| `Systems/FBWController` | CAT mode、hold state、integrator、limiter、actuator state、FBW debug values | `Systems/FBWController` | CAT1/CAT3、G limiter、hold/degrade、操縱面輸出 |
| `Systems/AerodynamicsModel` | wing/tail/control-surface force application points | `Systems/AerodynamicsModel` | 起飛、轉彎、配平、高 AOA |
| `Systems/EngineSystem` | engine switch、throttle output、RPM readout、AB ratio/lit、nozzle aperture、thrust | `Systems/EngineSystem` | 左右引擎啟停、spool、AB 點火/熄火、噴嘴動畫 |
| `Systems/FuelSystem` | internal/external fuel、total fuel、pending mass delta | `Systems/FuelSystem` | fuel burn、mass change、unlimited fuel |
| `Systems/LandingGearSystem` | gear、flap/slat、airbrake、NWS、wheel brake、wheel spin、carrier state | `Systems/LandingGearSystem` | 起落架、襟翼/縫翼、減速板、NWS、煞車、輪胎轉動 |
| `Systems/SuspensionSystem` | native suspension feedback、WOW、fallback ground force、probe geometry | `Systems/SuspensionSystem` | 著地、WOW、避震壓縮、滑行、降落 |
| `Systems/DamageModel` | damage element integrity、wing/tail/engine integrity、total damage | `Systems/DamageModel` | damage、repair、invincible |
| `DcsInterface/CockpitBridge` | `EDPARAM interface`、cockpit/Lua param handles | `DcsInterface/DcsCockpitBridge` | cockpit param 仍同步，AP/MaxPower/temperature 可讀寫 |
| `DcsInterface/GameOptions` | `invincible`、`infinite_fuel`、`easy_flight` | `DcsInterface` + systems | DCS option callback 正常 |
| `DcsInterface/CockpitOutput` | `shake_amplitude` | `DcsInterface` 或 `Diagnostics` | cockpit shake 仍有輸出 |
| `DcsInterface/ModelBridge` | suspension node names、draw-arg related mapping | `DcsInterface/DcsDrawArgsWriter` + `DcsIds` | 模型動畫、draw args、輪胎/NWS |
| `Diagnostics` | debug/probe timers、flags、log formatting state | `Diagnostics/DebugLogger` | debug log 可產生，但不應影響飛行 |
| `LEGACY_CANDIDATE` | `FM_State.h`、`FCS.*`、`FM_data.cpp` prototype mass model | 待決定保留/合併/刪除 | 不應接入主流程前改變行為 |

## Current Structural Risks

- `FM_data.h` 已改為 `extern` 宣告，資料定義集中在 `FM_data.cpp`。下一步應整理成 immutable table/config。
- `F-CK-1C_EFM.cpp` 仍然同時持有 DCS callbacks、system state、simulation logic、diagnostics。
- FBW state 很大，應最後拆；它最容易改變飛行手感。
- suspension diagnostics 目前仍有常駐開關與 debug state，之後應集中到 `Diagnostics/` 並可關閉。
- 本步沒有修已知 bug，例如 fuel external/internal 邏輯、temperature unit、throttle table size。

## Test Focus For This Step

因為這一步只新增註解與文件，理論上 DLL 行為不應改變。測試目標是確認「重新 build 後沒有任何意外變化」。

建議測試：

1. DCS 能載入模組，cold start、hot start、hot start in air 都能進入任務。
2. Pitch/roll/yaw、trim、油門軸、鍵盤油門、左右煞車都正常。
3. 起落架、襟翼/縫翼、減速板、NWS、輪胎轉動 draw args 正常。
4. 左右引擎啟停、RPM/聲音、推力、AB、噴嘴動畫正常。
5. 起飛、轉彎、降落、滑行沒有明顯異常。
6. fuel burn 與 `ed_fm_change_mass()` 行為維持原樣。
7. AP pitch/roll command、自動油門、MaxPower switch 行為維持原樣。
8. damage、repair、invincible、unlimited fuel 行為維持原樣。
9. debug watch 與 suspension log 仍可輸出。

若上述任何一項改變，代表註解之外可能被 build/line-ending/檔案編碼流程影響，需要先停在這一步排查。
