# EFM 重構第二階段藍圖

## 階段目標

第一階段已完成系統邏輯、DCS bridge、IDs 與 Diagnostics 的安全拆分。第二階段要完成真正的 ownership 收斂，使 DCS exported callbacks 只負責轉接，而 `Core::Fck1cEfm` 成為唯一的 EFM 協調物件。

目標呼叫流程：

```text
DCS exported callback
  -> DcsBridge adapter
  -> Core::Fck1cEfm public method
  -> Systems / Data / Core
  -> DCS output snapshot
```

分層規則：

- `Core` 不 include DCS API headers。
- `Systems` 不 include `DcsBridge` 或 DCS API headers。
- `DcsBridge` 可以讀寫 `Core` 提供的 input/output snapshot，但不實作飛行邏輯。
- `Data` 只保存 immutable aircraft config 與 tables。
- exported callback 名稱、signature 與語意保持不變。
- 本階段不調整 FBW、氣動、引擎、燃油或懸吊參數。

## 執行前置

開始第 11 步前建立 Git checkpoint，保存第一階段已通過 DCS 測試的狀態。每一步完成後都必須 build DLL，並通過該步的 DCS 測試重點才進入下一步。

## 第 11 步：Core ownership 與 simulation pipeline

這一步允許大幅搬移，一次消除目前主檔的全域狀態與 reference aliases。

工作內容：

- 建立 `Core/Fck1cEfm.h/.cpp`。
- 由 `Core::Fck1cEfm` 持有：
  - `AircraftState`
  - force/moment accumulator
  - gameplay options 與 cockpit output
  - 所有 Systems state/config
  - startup clock 與 lifecycle state
- 將約 225 行的 simulation flow 搬到 `Fck1cEfm::simulate(dt)`。
- 將 simulation flow 拆成具名 private phases，例如：
  - `begin_frame`
  - `update_controls`
  - `update_fbw_and_aerodynamics`
  - `update_engines_and_fuel`
  - `update_ground_and_suspension`
  - `finish_frame`
- 移除主檔約 86 個 reference aliases。
- `ed_fm_simulate()` 改為單一 `g_efm.simulate(dt)` 呼叫。

完成條件：

- `Core::Fck1cEfm` 不 include DCS API headers。
- 主檔不再保存 aircraft/system globals 或 compatibility aliases。
- force、moment、shake、fuel、engine、gear 與 FBW 輸出與搬移前一致。

DCS 測試重點：

- Cold、Hot Ground、Hot Air。
- 全包線基本操控、FBW CAT、AP 與 autothrottle。
- 引擎啟停、AB、燃油消耗。
- 起落架、煞車、NWS、著地與懸吊。
- damage、repair、unlimited fuel、easy flight。

## 第 12 步：DCS callbacks 與 adapter 收斂

一次整理所有 exported callbacks，讓 DCS contract 與自製 Core 完全分層。

工作內容：

- 建立 `DcsBridge/DcsCallbacks.cpp` 或等價檔案，集中所有 `ed_fm_*` definitions。
- 建立 DCS runtime/adapter context，持有：
  - cockpit `EDPARAM` handles
  - module/config paths
  - autopilot bridge state
  - carrier event state
  - diagnostics path adapter
- atmosphere、surface、mass、world/body state callbacks 只轉交給 `g_efm`。
- command callback 只做 DCS command 到 Core command 的 mapping。
- draw args、`ed_fm_get_param()`、mass delta 與 suspension feedback 使用 snapshot/adapter。
- 清空或移除舊 `F-CK-1C_EFM.cpp` 中剩餘 implementation，只保留必要 metadata 或直接由新 callbacks 檔取代。

完成條件：

- exported callbacks 都是短小 adapter。
- DCS-specific state 不進入 `Core` 或 `Systems`。
- DLL exports 與 callback signatures 完全不變。

DCS 測試重點：

- Input axes、keyboard commands 與 custom commands。
- draw args：控制面、gear、airbrake、AB、nozzle、wheel spin。
- engine/fuel/WOW `ed_fm_get_param()` 輸出。
- carrier event、damage event 與 suspension feedback。
- cockpit Lua parameters 與 Controls Indicator。

## 第 13 步：Data layer 與 Legacy 一次清理

將數值資料改為 immutable ownership，並一次刪除未接入主流程的舊實驗架構。

工作內容：

- 建立：
  - `Data/AircraftConfig.h/.cpp`
  - `Data/AeroTables.h/.cpp`
  - `Data/EngineTables.h/.cpp`
- 將 `FM_data.*` 的有效常數與 tables 搬入 `Data`。
- 由 `Fck1cEfm` constructor/config factory 注入 Systems config。
- 移除 mutable `extern double` tables。
- 刪除未使用的：
  - prototype `MassModel`
  - `vertical_speed_AGL` / `update_agl_rate`
  - `FCS.*`
  - `FM_State.h`
  - `Inputs.h` compatibility facade
  - `Utility.h` compatibility facade
- 新程式碼直接 include 所需的 `Common/*`、`Data/*` 或 `DcsIds/*`。
- 更新 Visual Studio project，移除 legacy compile entries。

完成條件：

- 沒有 `LEGACY_CANDIDATE` 實作留在 EFM build。
- 沒有 mutable global aero/engine tables。
- 數值 tables 搬移前後逐項相同。

DCS 測試重點：

- Mach 全區間升阻力與最大迎角行為。
- 高 G 能量損失與最大速度限制。
- 引擎 spool、AB、RPM、temperature 與 thrust。
- fuel burn 與 DCS mass delta。

## 第 14 步：測試、警告與 build contract

建立可以在不啟動 DCS 的情況下驗證純邏輯的測試層，並清除已知編譯警告。

工作內容：

- 建立 native test target，不連結 DCS runtime。
- 為下列內容加入 deterministic tests：
  - Common interpolation/clamp/units
  - Input 與 throttle arbitration
  - Engine spool/AB/nozzle
  - Fuel burn/mass delta
  - FBW reset、limiters 與 actuator bounds
  - Aerodynamics coefficient/force snapshots
  - Suspension feedback/fallback
  - Startup lifecycle
  - Diagnostics formatting
- 修正 `Common::rescale` 所有控制路徑回傳值。
- 處理 `fopen/getenv` deprecation warning。
- 明確處理 SimulationEvents 的 double-to-float conversion。
- 建立 exported callback/ABI 檢查腳本。
- build script 同時執行 ID generation、tests、DLL build 與 hash check。

完成條件：

- Release/x64 build 無目前已知警告。
- 測試可單獨執行且全部通過。
- source DLL 與 runtime `bin` DLL hash 相同。

DCS 測試重點：

- 完整 smoke test 即可，重點確認測試／警告修正沒有改變飛行手感。

## 第 15 步：最終架構驗收與文件更新

這一步不再大搬程式碼，只做最終邊界稽核與交付整理。

工作內容：

- 更新 `EFM_STATE_INVENTORY.md` 與主要 architecture 文件。
- 移除已失效的 `EFMREF candidate`、compatibility 與 migration 註解。
- 自動檢查：
  - `Core` / `Systems` 不依賴 DCS API
  - input profiles 沒有 custom raw IDs
  - DcsBridge 沒有 raw draw-arg IDs
  - Systems 沒有 raw damage IDs
  - 沒有 compatibility aliases/facades
  - 沒有未使用 legacy source 加入 build
- 記錄完整 DCS regression matrix 與最終 DLL hash。

完成條件：

- `ed_fm_* -> DcsBridge -> Core::Fck1cEfm -> Systems` 邊界清楚。
- 主流程可以從檔案結構直接理解，不需要追蹤全域 alias。
- 第一階段與第二階段文件都能說明每個 state、ID 與 callback 的 owner。

## 預估完成度

開始第二階段前，十步安全拆分已完成，終極架構約 75-80%。

- 第 11 步完成：約 88%。
- 第 12 步完成：約 93%。
- 第 13 步完成：約 97%。
- 第 14、15 步完成：100%。
