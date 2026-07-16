# DCSBridge 重構步驟 0 基準

本文件保存 `DCSBRIDGE_REFACTOR_DESIGN.md` 步驟 0 的可重現基準。後續每個 commit 都以此比較測試結果、DLL exports 與新增 compiler warnings。

## Source 基準

- Git commit：`e733e52e206b272827f4ac825301374af1be3e4e`
- `src/efm` 沒有 tracked diff，也沒有 untracked source/test files。
- 執行步驟 0 前的非 source 變更只有重建產生的 `bin/F-CK-1C_EFM.dll` 與尚未追蹤的設計文件。
- Configuration：`Release|x64`
- MSBuild：`18.7.8.30822`
- MSVC tools：`14.51.36231`

## 重建命令與結果

Native tests 從 source 重新建置後執行：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' `
  '.\src\efm\F-CK-1C_EFM_Tests\F-CK-1C_EFM_Tests.vcxproj' `
  /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m /v:minimal
.\src\efm\x64\Release\F-CK-1C_EFM_Tests.exe
```

結果：

```text
EFM tests: 443 checks, 0 failures
```

DLL 只使用專案腳本重建與複製：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\build_dll.ps1
```

腳本完成，source DLL 與 runtime DLL hash 相同。

此 toolchain 的不同次完整 rebuild 不保證產生相同 DLL SHA256；下列 hash 是步驟 0 該次建置紀錄，只用來確認同一次 build 的 source/runtime copy 相同。後續 ABI 穩定性以完整 export surface 比較為準。

## 產物 SHA256

| 產物 | Bytes | SHA256 |
|---|---:|---|
| `src/efm/x64/Release/F-CK-1C_EFM.dll` | 291328 | `F06D79C397F10EF1AF594BEC95DC96694DCF07D571144290E98FE4DD2B58BE74` |
| `bin/F-CK-1C_EFM.dll` | 291328 | `F06D79C397F10EF1AF594BEC95DC96694DCF07D571144290E98FE4DD2B58BE74` |
| `src/efm/x64/Release/F-CK-1C_EFM_Tests.exe` | 294400 | `51D68F53E8A96613BF8F75E2F9AA0A540878010CE0981F0646117C7A750B8271` |
| `F-CK-1C_EFM.h` export declarations | 7446 | `C69A3CA0C39E320ECAB53C9394CCB3C5C049D7BAC71C95D195710B8110E83AC7` |
| `include/FM/API_Declare.h` export declarations | 13703 | `3BCBB6840030460B5D16144E37564FC2D630B58FAE0C2968BFB9548036238F85` |

## DLL export surface

`dumpbin /exports` 回報 37 functions、37 names。以下依 ordinal 保存；後續重構不得新增、移除或重新命名：

```text
 1 ed_fm_add_global_force_component
 2 ed_fm_add_global_moment_component
 3 ed_fm_add_local_force
 4 ed_fm_add_local_force_component
 5 ed_fm_add_local_moment
 6 ed_fm_add_local_moment_component
 7 ed_fm_change_mass
 8 ed_fm_cold_start
 9 ed_fm_configure
10 ed_fm_debug_watch
11 ed_fm_enable_debug_info
12 ed_fm_get_external_fuel
13 ed_fm_get_internal_fuel
14 ed_fm_get_param
15 ed_fm_get_shake_amplitude
16 ed_fm_hot_start
17 ed_fm_hot_start_in_air
18 ed_fm_on_damage
19 ed_fm_pop_simulation_event
20 ed_fm_push_simulation_event
21 ed_fm_refueling_add_fuel
22 ed_fm_release
23 ed_fm_repair
24 ed_fm_set_atmosphere
25 ed_fm_set_command
26 ed_fm_set_current_mass_state
27 ed_fm_set_current_state
28 ed_fm_set_current_state_body_axis
29 ed_fm_set_draw_args
30 ed_fm_set_easy_flight
31 ed_fm_set_external_fuel
32 ed_fm_set_immortal
33 ed_fm_set_internal_fuel
34 ed_fm_set_surface
35 ed_fm_simulate
36 ed_fm_suspension_feedback
37 ed_fm_unlimited_fuel
```

## 現有 compiler warning 基準

這些是步驟 0 重建即可重現的既有 warnings；步驟 0 不修改 production source，也不把修正擴入本輪。後續不得新增 warning，刪除舊程式時可自然減少。

| Build | C4996 | C4244 | C4715 | Total warning lines |
|---|---:|---:|---:|---:|
| Native tests | 6 | 1 | 1 | 8 |
| DLL | 21 | 3 | 1 | 25 |

- `C4996`：現有 `fopen`／`getenv` calls；header 被不同 translation units include，所以 DLL build 有重複 warning lines。
- `C4244`：`SimulationEvents.h` 的 carrier thrust `double` 轉 `float`。
- `C4715`：`Common::rescale` 並非所有控制路徑都有 return。這是既有問題，本基準只記錄，不在步驟 0 修正或掩蓋。

## 現有自動測試涵蓋盤點

| 行為 | 現有自動測試證據 | 基準限制 |
|---|---|---|
| Cold／hot-ground／hot-air start | `Fck1cEfmTests` lifecycle tests | 測 Core lifecycle，未直接載入 DLL ABI |
| Simulate | `test_simulation_pipeline` | 驗證時間、controls 與 runtime calls，未完整 assert final force/moment |
| Force／moment | Aerodynamics、suspension 與 Core system tests | 沒有直接測 `ed_fm_add_local_force/moment` ABI output |
| Draw args | `DcsSnapshotsTests::test_draw_arg_snapshot` | 測 projection，未直接測 ABI buffer write |
| Param | `ParamExportTests` 與 param snapshot test | 目前 unknown param 只回 `0.0`，沒有 ERROR 可觀察性 |
| Fuel | Core internal fuel 與 param projection assertions | 沒有完整 external fuel setter/getter callback test |
| Mass delta | `test_pending_mass_delta_is_consumed_once` | 已涵蓋 consume-once 語意 |
| Suspension | feedback value 與 fallback split tests | 只涵蓋現有 compression/force input，未涵蓋完整 raw DCS sample |
| Carrier | simulation event push/pop phase tests | 測純 bridge state，未直接載入 DLL ABI |
| Release | Core release lifecycle test | 未涵蓋 release 後 DCS output callback 規則 |

上述限制不是步驟 0 失敗，也不在步驟 0 新增測試。它們正是後續 `FrameInput`、`FrameOutput`、`OutputStore`、EventLog 與 CSV commits 必須補上的可觀察輸入／輸出測試。

## 步驟 0 結果

- Source/test baseline 可重現。
- Native tests 全部通過。
- DLL script 成功，source/runtime DLL hashes 相同。
- 37 個 exports 已完整保存。
- 既有 warnings 與自動測試缺口已明確保存，沒有默默修改或偏離重構計畫。
