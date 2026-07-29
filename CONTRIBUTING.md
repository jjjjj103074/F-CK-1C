# Contributing to F-CK-1C

[English](#english) | [繁體中文](#繁體中文)

---

## English

### Prerequisites

- **Node.js** — Required for commit tooling (commitlint, husky, czg)
- **DCS World** — Required for testing changes
- **Visual Studio Build Tools / MSBuild** — Required when committing runtime EFM build inputs; the hook rebuilds the DLL
- **VSCode** with the [StyLua](https://marketplace.visualstudio.com/items?itemName=JohnnyMorganz.stylua) extension (recommended)

### Development Setup

```bash
# 1. Clone the repository
git clone https://github.com/jjjjj103074/F-CK-1C.git

# 2. Install Node.js dependencies
#    This automatically installs Git hooks via Husky
npm install

# 3. Make sure MSBuild.exe is available for runtime EFM changes
#    The pre-commit hook rebuilds and stages bin/F-CK-1C_EFM.dll

# 4. Install the module in DCS World to test your changes
#    Run install.bat or follow your usual DCS mod installation workflow
```

### EFM Architecture

The C++ EFM is divided by responsibility:

- `DcsBridge/` owns the DCS C ABI, DCS value translation, callback lifecycle,
  cockpit/carrier adapters, EventLog, and CSV telemetry.
- `Core/` owns the DCS-neutral aircraft simulation. Flight calculations and
  aircraft-system behavior belong here.
- `DcsIds/` owns DCS boundary identifiers and generated command/parameter
  tables. It is not a Core dependency.

Inside Core, `Systems/` owns aircraft equipment and control behavior, while
`Simulation/` owns the physical effects returned to DCS. Systems exchange
typed state only through `AircraftData`; concrete Systems do not include or
call one another. New Systems register through
`Core/Systems/<Owner>/Entry.cpp`. Physical Models are intentionally fixed and
run in the order Aerodynamics, Propulsion, GroundInteraction, MassProperties.

`DcsBridge/Internal/` is private implementation, not a public include surface.
DCSBridge may enter Core only through `Core/Fck1cEfm.h`, `Core/Contracts/`,
and the structured `Core/Diagnostics/ExecutionError.h` boundary type. Every
DCS export must keep the common exception boundary so that a C++ exception
never crosses the C ABI.

Start with the [EFM overview](src/efm/README.md). It links to the current
DCSBridge, Core, System, and DCS ID contributor guides.

### EFM Verification

Use the canonical commands and hook behavior documented in the
[DLL build guide](docs/BUILD_DLL.md). The pre-commit hook does not replace the
native tests, architecture checks, or export verification.
Changes that depend on DCS callback order, cockpit parameters, or flight
lifecycle also require an in-game test before merging.

### Branch Strategy

| Branch | Purpose |
|---|---|
| `main` | Stable releases only — merged from `dev` via PR |
| `dev` | Integration branch — all features merge here first |
| `feature/your-feature` | Your daily work — branch from `dev`, PR back to `dev` |

### Commit Format

All commits must follow [Conventional Commits](https://www.conventionalcommits.org/).
Use `npm run commit` for interactive guided input (recommended).

**Format:** `<type>(<scope>): <description>`

**Types:**

| Type | When to use |
|---|---|
| `feat` | New feature or capability |
| `fix` | Bug fix |
| `refactor` | Code restructure without behavior change |
| `docs` | Documentation only |
| `chore` | Maintenance, tooling, assets, or config |

**Scopes:**

Scopes are required and must be lowercase.

| Scope | Area |
|---|---|
| `core` | Module entry, aircraft definition, SFM/FM config, weapons, runtime Lua |
| `efm` | C++ EFM project, DLL, MSBuild |
| `cockpit` | Cockpit scripts, devices, indicators, input bindings |
| `assets` | Shapes, Textures, Sounds, Liveries |
| `tooling` | Git hooks, commitlint, GitHub Actions, repo tools |
| `docs` | README, CONTRIBUTING, docs |

**Examples:**

```
feat(cockpit): add radar mode switch
fix(efm): correct landing gear compression feedback
docs(docs): update build instructions
chore(tooling): add pre-commit EFM build
```

### Pull Request Process

- **Target branch:** `dev` for features and fixes; `main` is updated only via `dev` PRs for releases
- **PR title** must follow the commit format above — it is validated automatically by CI
- **Merge method:** Squash only (the PR title becomes the squashed commit message)
- No required reviewers, but review comments are welcome

### Code Style

Lua files are formatted with [StyLua](https://github.com/JohnnyMorganz/StyLua).
If you use VSCode, format-on-save is already configured in `.vscode/settings.json`.

> No need to manually align table entries — StyLua handles all formatting.

### Language Policy

Commit messages, code comments, and PR descriptions may be written in **English or Traditional Chinese** — both are equally accepted.

---

## 繁體中文

### 前置需求

- **Node.js** — 提交工具所需（commitlint、husky、czg）
- **DCS World** — 測試變更所需
- **Visual Studio Build Tools / MSBuild** — 提交 EFM runtime 建置輸入時需要；hook 會重建 DLL
- **VSCode** 搭配 [StyLua](https://marketplace.visualstudio.com/items?itemName=JohnnyMorganz.stylua) 擴充套件（建議安裝）

### 開發環境設定

```bash
# 1. Clone 專案
git clone https://github.com/jjjjj103074/F-CK-1C.git

# 2. 安裝 Node.js 依賴
#    此步驟會透過 Husky 自動安裝 Git hooks
npm install

# 3. 修改 EFM runtime 時，確認 MSBuild.exe 可用
#    pre-commit hook 會重建並暫存 bin/F-CK-1C_EFM.dll

# 4. 將模組安裝至 DCS World 以測試變更
#    執行 install.bat 或依照你慣用的 DCS 模組安裝流程
```

### EFM 架構

C++ EFM 依照責任分成三個主要區域：

- `DcsBridge/` 負責 DCS C ABI、DCS 數值翻譯、Callback 生命週期、
  座艙與航空母艦轉接、EventLog 和 CSV 遙測。
- `Core/` 負責不依賴 DCS 的飛機模擬。飛行計算和飛機系統行為應放在這裡。
- `DcsIds/` 負責 DCS 邊界 ID，以及產生的 Command／Param 表格；
  Core 不得依賴它。

Core 內部由 `Systems/` 負責飛機設備與控制行為，`Simulation/` 負責回傳
DCS 的物理效果。System 只能透過 typed `AircraftData` 交換狀態；
具體 System 不得互相 Include 或直接呼叫。新增 System 時，以
`Core/Systems/<Owner>/Entry.cpp` 在建置期註冊。物理 Model 則刻意維持
固定，依 Aerodynamics、Propulsion、GroundInteraction、MassProperties
順序執行。

`DcsBridge/Internal/` 是私有實作，不是對外 Include 介面。
DCSBridge 進入 Core 時，只能使用 `Core/Fck1cEfm.h`、
`Core/Contracts/`，以及作為結構化錯誤邊界的
`Core/Diagnostics/ExecutionError.h`。每個 DCS 匯出函式都必須保留
共用的例外邊界，避免 C++ 例外穿越 C ABI。

請先閱讀 [EFM 總覽](src/efm/README.md)，再依變更責任前往目前的
DCSBridge、Core、System 或 DCS ID Contributor Guide。

### EFM 驗證

請使用 [DLL 建置指南](docs/BUILD_DLL.md) 中唯一維護的驗證指令與 hook
行為說明。Pre-commit hook 不會取代 Native Tests、架構檢查或匯出介面
驗證。
涉及 DCS Callback 順序、座艙 Param 或飛行生命週期的變更，
合併前還需要進行遊戲內測試。

### 分支策略

| 分支 | 用途 |
|---|---|
| `main` | 僅存放穩定版本，從 `dev` 透過 PR 合併 |
| `dev` | 整合分支，所有功能先合併至此 |
| `feature/你的功能` | 日常開發，從 `dev` 建立，PR 回 `dev` |

### Commit 格式

所有 commit 必須遵循 [Conventional Commits](https://www.conventionalcommits.org/)。
建議使用 `npm run commit` 進行互動式引導輸入。

**格式：** `<type>(<scope>): <描述>`

**Type 類型：**

| Type | 使用時機 |
|---|---|
| `feat` | 新功能或新能力 |
| `fix` | 錯誤修正 |
| `refactor` | 程式碼重構（不改變行為） |
| `docs` | 純文件變更 |
| `chore` | 維護、工具、素材、設定 |

**Scope 範圍：**

Scope 必填，且必須使用小寫。

| Scope | 對應區域 |
|---|---|
| `core` | 模組進入點、飛機定義、SFM/FM config、武器、runtime Lua |
| `efm` | C++ EFM 專案、DLL、MSBuild |
| `cockpit` | 座艙腳本、裝置、指示器、輸入綁定 |
| `assets` | Shapes、Textures、Sounds、Liveries |
| `tooling` | Git hooks、commitlint、GitHub Actions、專案工具 |
| `docs` | README、CONTRIBUTING、docs |

**範例：**

```
feat(cockpit): 新增雷達模式切換
fix(efm): 修正起落架壓縮回饋
docs(docs): 更新建置說明
chore(tooling): 新增 commit 前 EFM 建置
```

### Pull Request 流程

- **目標分支：** 功能與修正 PR 至 `dev`；`main` 僅由 `dev` 的 PR 更新（作為 Release）
- **PR 標題** 必須符合上方 commit 格式，CI 會自動驗證
- **合併方式：** 僅允許 Squash（PR 標題會成為壓縮後的 commit 訊息）
- 不強制要求 reviewer，但歡迎留下 review 意見

### 程式碼風格

Lua 檔案使用 [StyLua](https://github.com/JohnnyMorganz/StyLua) 格式化。
若使用 VSCode，`.vscode/settings.json` 已設定存檔時自動格式化。

> 不需要手動對齊 table 欄位，StyLua 會處理所有格式化工作。

### 語言政策

Commit 訊息、程式碼註解、PR 說明均可使用**英文或繁體中文**，兩者同樣接受。
