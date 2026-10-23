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
- Cockpit Lua owns the currently implemented CMS, weapon, radar, HMCS and audio
  paths described in the [cockpit guide](docs/COCKPIT.md).

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

### Branch naming and scope

New work branches use `<type>/<module>-<task>` with a lowercase English,
hyphen-separated description. An issue number may precede the module,
for example `bugfix/123-efm-engine-start`.

| Prefix | Purpose | Example |
|---|---|---|
| `feature/` | New functionality | `feature/cockpit-hud-brightness` |
| `bugfix/` | Bug fixes | `bugfix/efm-engine-start` |
| `refactor/` | Code restructuring | `refactor/efm-engine-model` |
| `docs/` | Documentation | `docs/tooling-build-guide` |
| `chore/` | Tools, configuration or asset maintenance | `chore/assets-texture-cleanup` |

Do not use `codex/`, `claud/`, `claude/`, or other agent-name prefixes.
Existing branches retain their names; `dev` and `main` retain their roles.

One branch represents one coherent objective: a feature, a system change,
a bug fix, or an issue. Include its directly required code, tests, documentation
and assets together. Separate independently reviewable objectives; naming a
system does not permit unrelated work. Do not reuse a completed branch for
another task. `feature/` maps to commit type `feat`; `bugfix/` maps to `fix`.

### PR review and merge

AI creates new PRs as Draft and may automatically mark them Ready for review
when the work is reviewable, its description matches the changes, applicable
automated validation and required CI pass for the current revisions, and no
known blocking defects or hidden unfinished work remain. Otherwise keep Draft
and report outstanding items. Ready does not mean human approval.

Every PR must receive human review and be merged by a human. Solo maintainers
may personally review and merge; no second reviewer or formal Approve is required.
Only humans may close or resolve human review comments and threads. AI may
revise, validate and reply with evidence within task authorization, but must not
approve on a human's behalf or treat CI or AI review as completed human review.

AI must not merge through any mechanism, enable auto-merge, enqueue automatic
merging, or bypass PR review through direct target-branch pushes, protection
changes or weakened checks. Commit frequency and branch collaboration remain
task-specific; this policy does not itself authorize commits, pushes or PR creation.
Follow the [shared PR workflow](docs/ai/pr-workflow.md) and existing
[validation policy](docs/PR_VALIDATION.md). These instructions do not prove
that equivalent GitHub protection settings are enabled.

### Commit and PR titles

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
- Cockpit Lua：目前的 CMS、武器、雷達、HMCS 與音效路徑，
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

### 分支命名與工作範圍

新工作分支使用 `類型/模組-工作描述`；描述使用英文小寫與連字號。
有對應 issue 時，可在模組前加編號，例如 `bugfix/123-efm-engine-start`。

| 前綴 | 用途 | 範例 |
|---|---|---|
| `feature/` | 新增功能 | `feature/cockpit-hud-brightness` |
| `bugfix/` | 修正錯誤 | `bugfix/efm-engine-start` |
| `refactor/` | 整理程式結構 | `refactor/efm-engine-model` |
| `docs/` | 修改文件 | `docs/tooling-build-guide` |
| `chore/` | 工具、設定或素材維護 | `chore/assets-texture-cleanup` |

新分支禁止使用 `codex/`、`claud/`、`claude/` 或其他代理名稱作為前綴。
既有分支不改名、不因本規範刪除；`dev` 與 `main` 維持原有用途。

一個分支只代表一件完整的工作：一個功能、一個系統變更、一項錯誤修正，
或一個 issue。直接相關的程式、測試、文件與素材可以放在同一分支；
能獨立審查的不同目標應拆成不同分支。不得以系統名稱籠統收納無關修改，
也不得將已完成的分支繼續用於其他任務。
分支前綴 `feature/` 對應 commit type `feat`，`bugfix/` 對應 `fix`。

### PR 審查與合併

AI 建立新 PR 一律先為 Draft。工作可供審查、說明符合實際差異、適用的
自動化驗證與必要 CI 對目前版本通過，且沒有已知阻擋問題或隱藏未完成事項時，
可自動轉為 Ready for review；否則維持 Draft 並回報待辦。Ready 不代表人工核准。

每個 PR 都須人工審核並由人工合併。單人維護者可親自審核並合併，
不強制第二人或正式 Approve。人工審查留言與討論串只能由人工關閉或 Resolve。
AI 可在任務授權內修正、驗證及回覆證據，但不得代替人工核准，
也不得將 CI 通過或 AI 審查視為人工已審核。

AI 不得透過任何方式合併、啟用 auto-merge 或加入自動合併佇列，
也不得透過直接推送目標分支、修改保護設定或降低檢查標準來繞過 PR 審查。
Commit 頻率及分支內協作方式依任務決定；本規範本身不授權 commit、push 或建立 PR。
操作依[共用 PR 流程](docs/ai/pr-workflow.md)與既有
[驗證規範](docs/PR_VALIDATION.md)；文件規則不代表 GitHub 已啟用對應保護設定。

### Commit 與 PR 標題

Commit 與 PR 標題使用 `<type>(<scope>): <描述>`。
Type 為 `feat`、`fix`、`refactor`、`docs` 或 `chore`；
Type 與 Scope 使用小寫，描述可用英文或繁體中文。
Scope 對照上表；實際規則由 [commitlint.config.js](commitlint.config.js) 定義。
`npm run commit` 提供互動式輸入。

相關建置輸入應一起暫存。Pre-commit hook 檢查暫存的 C++ 排版，
受執行期輸入影響時重建並暫存 DLL，並拒絕未暫存或未納入追蹤的執行期輸入。
原生測試與遊戲內驗證須另行執行。
