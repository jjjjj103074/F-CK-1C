# Contributing / 貢獻指南

[English](#english) | [繁體中文](#繁體中文)

## English

### Setup

Use Git, Node.js 22 LTS and the Windows toolchain in
[BUILD_DLL.md](docs/BUILD_DLL.md). DCS is required for in-game verification.

```powershell
npm ci
npm run check:efm-toolchain
npm run validate:efm
```

Validation preserves the committed DLL and compares it with a clean rebuild.
After changing runtime build inputs, use `npm run build:efm` to update the DLL,
then validate it. Reports are written to `tmp/pr-validation/`.

### Architecture

- `DcsBridge/` owns DCS callbacks, validation, unit translation and runtime tools.
- `Core/` owns aircraft simulation. Systems exchange typed `AircraftData`;
  physical models run through `SimulationPipeline`.
- `DcsIds/` owns DCS boundary identifiers and generated C++/Lua tables.
- `Core/Systems/MissionComputer/` owns mission modes, stores policy, weapon
  authorization, targeting and infrared missile support through FireControl.
  HUD/HMCS owns weapon presentation using MC state and ordered input observations.
- Cockpit Lua adapts DCS observations and actions, runs CM programs, and renders
  displays/audio as described in the [cockpit guide](docs/COCKPIT.md).

Start with the [EFM overview](src/efm/README.md).
Concrete Systems do not include or call one another. Core remains independent
of DCS identifiers and bridge internals. C++ exceptions must end at the DCS ABI.

### Style and verification

C++ uses the shared [formatting rules](docs/CPP_FORMATTING.md):
tabs at width 4, Allman braces and a 120-column target.

```powershell
npm run format:cpp
npm run check:cpp-format
```

Format before staging. SDK, vendor and generated files follow their owning
source or generator. Lua uses StyLua.

Add tests for observable behavior and relevant lifecycle/error paths.
Use [PR validation](docs/PR_VALIDATION.md) for CI requirements and
[DCS testing](docs/DCS_TESTING.md) for callback order, cockpit parameters,
flight lifecycle and runtime loading.

### Submissions

`dev` is the integration branch; `main` contains releases.
Branch daily work from `dev` and submit a PR back to `dev`.
Release PRs go from `dev` to `main`; use Squash merge.

Commits and PR titles use `<type>(<scope>): <description>`.
Types are `feat`, `fix`, `refactor`, `docs`, and `chore`.
Type and scope are lowercase; descriptions may use English or Traditional Chinese.
`npm run commit` provides an interactive prompt.
[commitlint.config.js](commitlint.config.js) defines the enforced rules.

| Scope | Area |
|---|---|
| `core` | Entry, aircraft definition, SFM/FM configuration, weapons, runtime Lua |
| `efm` | C++ EFM, DLL and MSBuild |
| `cockpit` | Cockpit scripts, devices, indicators and input bindings |
| `assets` | Shapes, textures, sounds and liveries |
| `tooling` | Hooks, CI and development tools |
| `docs` | Project documentation |

Stage related build inputs together. The pre-commit hook checks staged C++
formatting and rebuilds/stages the DLL when runtime inputs are affected.
It rejects runtime inputs left unstaged or outside tracking.
Native tests and in-game verification remain separate requirements.

## 繁體中文

### 環境

使用 Git、Node.js 22 LTS，以及 [BUILD_DLL.md](docs/BUILD_DLL.md) 指定的
Windows 工具鏈。遊戲內驗證需要 DCS。

```powershell
npm ci
npm run check:efm-toolchain
npm run validate:efm
```

驗證保留提交中的 DLL，與乾淨重建結果比對。
修改執行期建置輸入後，以 `npm run build:efm` 更新 DLL，再執行驗證。
報告位於 `tmp/pr-validation/`。

### 架構

- `DcsBridge/`：DCS 回呼、驗證、單位轉換與執行期工具。
- `Core/`：飛機模擬；System 以 typed `AircraftData` 交換狀態，
  物理模型由 `SimulationPipeline` 執行。
- `DcsIds/`：DCS 邊界 ID 與生成的 C++／Lua 表格。
- `Core/Systems/MissionComputer/`：任務模式、掛載策略與 FireControl 火控，
  包含紅外線飛彈支援；HUD／HMCS 依 MC 狀態與有序輸入觀測決定武器呈現。
- Cockpit Lua：DCS 觀測與操作介面、反制程序、顯示及播放，
  詳見[座艙指南](docs/COCKPIT.md)。

由 [EFM 總覽](src/efm/README.md) 進入各模組指南。
具體 System 不互相 include 或呼叫；Core 不依賴 DCS ID 或 bridge 私有實作。
C++ 例外必須在 DCS ABI 邊界處理。

### 風格與驗證

C++ 使用共用[排版規則](docs/CPP_FORMATTING.md)：
Tab 寬 4、Allman 大括號、行寬以 120 字元為目標。

```powershell
npm run format:cpp
npm run check:cpp-format
```

先排版再暫存。SDK、第三方及生成檔由原始來源或生成器管理；Lua 使用 StyLua。

測試應涵蓋可觀察行為及相關生命週期、錯誤路徑。
CI 規則見 [PR 驗證](docs/PR_VALIDATION.md)；
回呼順序、座艙參數、飛行生命週期與載入行為依 [DCS 測試](docs/DCS_TESTING.md) 驗證。

### 提交

`dev` 是整合分支，`main` 用於 Release。
日常工作由 `dev` 分支，PR 回 `dev`；Release 由 `dev` PR 至 `main`，
合併方式使用 Squash。

Commit 與 PR 標題使用 `<type>(<scope>): <描述>`。
Type 為 `feat`、`fix`、`refactor`、`docs` 或 `chore`；
Type 與 Scope 使用小寫，描述可用英文或繁體中文。
Scope 對照上表；實際規則由 [commitlint.config.js](commitlint.config.js) 定義。
`npm run commit` 提供互動式輸入。

相關建置輸入應一起暫存。Pre-commit hook 檢查暫存的 C++ 排版，
受執行期輸入影響時重建並暫存 DLL，並拒絕未暫存或未納入追蹤的執行期輸入。
原生測試與遊戲內驗證須另行執行。
