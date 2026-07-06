# Contributing to F-CK-1C

[English](#english) | [繁體中文](#繁體中文)

---

## English

### Prerequisites

- **Node.js** — Required for commit tooling (commitlint, husky, czg)
- **DCS World** — Required for testing changes
- **Visual Studio Build Tools / MSBuild** — Required because commits rebuild the EFM DLL automatically
- **VSCode** with the [StyLua](https://marketplace.visualstudio.com/items?itemName=JohnnyMorganz.stylua) extension (recommended)

### Development Setup

```bash
# 1. Clone the repository
git clone https://github.com/jjjjj103074/F-CK-1C.git

# 2. Install Node.js dependencies
#    This automatically installs Git hooks via Husky
npm install

# 3. Make sure MSBuild.exe is available
#    The pre-commit hook runs tools/build_dll.ps1 and stages bin/F-CK-1C_EFM.dll

# 4. Install the module in DCS World to test your changes
#    Run install.bat or follow your usual DCS mod installation workflow
```

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
| `chore` | Tooling, config, maintenance |
| `wip` | Work in progress (not for merging to `main`) |
| `ci` | CI/CD, automation, project infrastructure |

**Scopes:**

| Scope | Area |
|---|---|
| `SFM` | Simplified flight model |
| `EFM` | Enhanced flight model / C++ DLL |
| `Cockpit_device` | Cockpit device scripts |
| `Cockpit_indicators` | Cockpit indicator scripts |
| `entry` | Module entry point |
| `ci` | Project tooling and infrastructure |

**Examples:**

```
feat(SFM): adjust drag coefficients for transonic flight
fix(Cockpit_device): correct AIM-9 tone trigger threshold
docs: update CONTRIBUTING with scope list
chore(ci): add *.old to gitignore
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
- **Visual Studio Build Tools / MSBuild** — commit 時會自動重建 EFM DLL，因此需要 MSBuild
- **VSCode** 搭配 [StyLua](https://marketplace.visualstudio.com/items?itemName=JohnnyMorganz.stylua) 擴充套件（建議安裝）

### 開發環境設定

```bash
# 1. Clone 專案
git clone https://github.com/jjjjj103074/F-CK-1C.git

# 2. 安裝 Node.js 依賴
#    此步驟會透過 Husky 自動安裝 Git hooks
npm install

# 3. 確認 MSBuild.exe 可用
#    pre-commit hook 會執行 tools/build_dll.ps1，並暫存 bin/F-CK-1C_EFM.dll

# 4. 將模組安裝至 DCS World 以測試變更
#    執行 install.bat 或依照你慣用的 DCS 模組安裝流程
```

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
| `chore` | 工具、設定、維護 |
| `wip` | 進行中（不應合併到 `main`） |
| `ci` | CI/CD、自動化、專案基礎建設 |

**Scope 範圍：**

| Scope | 對應區域 |
|---|---|
| `SFM` | 簡化飛行模型 |
| `EFM` | 增強飛行模型 / C++ DLL |
| `Cockpit_device` | 座艙裝置腳本 |
| `Cockpit_indicators` | 座艙指示器腳本 |
| `entry` | 模組進入點 |
| `ci` | 專案工具與基礎建設 |

**範例：**

```
feat(SFM): 調整穿音速飛行的阻力係數
fix(Cockpit_device): 修正 AIM-9 音調觸發閾值
docs: 更新 CONTRIBUTING 的 scope 清單
chore(ci): 將 *.old 加入 gitignore
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
