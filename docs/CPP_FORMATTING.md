# C++ formatting / C++ 排版

[English](#english) | [繁體中文](#繁體中文)

## English

### Scope and style

`package.json` -> `cppFormat.roots` selects authored EFM sources and tests.
SDK headers in `include/`, vendor code in `ThirdParty/` and generated `.g.*`
files follow their upstream source or generator.

`.clang-format` defines tabs at width 4, Allman braces, a 120-column target,
no namespace indentation and type-attached `*`/`&`.
Alignment uses spaces. Include order, comments and string contents are preserved.
Long indivisible values may exceed the column target.
Files use UTF-8 and CRLF; `.editorconfig` supplies basic editor settings.

### Setup and commands

Use Node.js 22 or a newer LTS version:

```powershell
npm ci
npm run setup:cpp-format
```

Setup verifies the pinned native clang-format and prints its path.
Install development and optional dependencies; offline installation requires a
populated npm cache. After `npm ci --ignore-scripts`, run `npm run prepare`.
Supported formatter packages cover Windows x64, Linux x64 and macOS arm64.

| Command | Purpose |
|---|---|
| `npm run format:cpp` | Format changed/new authored files |
| `npm run format:cpp:all` | Format the full authored scope |
| `npm run check:cpp-format` | Check the full current scope |
| `npm run check:cpp-format:staged` | Check Git index content |
| `npm run test:cpp-format` | Verify formatter scope, version and staging behavior |

Format before staging. The formatter writes whole files, so select partial
hunks afterward. The hook checks staged content without editing or staging it,
then performs its runtime DLL checks.

When committing C++ with style/scope settings, stage those settings together.
The staged check rejects mismatched package/format/ignore settings, including
untracked or ignored nested overrides, while allowing partially staged source.
It accounts for Git line-ending normalization.

### Editors and agents

VS Code uses the recommended Microsoft C/C++ extension and repository settings.
On Linux/macOS, override `C_Cpp.clang_format_path` with the setup result.
Lua uses StyLua.

In Visual Studio, enable ClangFormat and select the executable printed by setup.
The path is local to the checkout. Format Document uses the repository rules.

`AGENTS.md` defines Codex instructions; `CLAUDE.md` imports it.
Hooks and CI independently verify the same formatter version.

PR validation checks formatting when applicable; main/dev pushes have the
C++ formatting workflow. Required checks are configured through
[PR validation](PR_VALIDATION.md#啟用).
Rule changes require formatter tests and relevant native/build/export checks.

## 繁體中文

### 範圍與外觀

`package.json` 的 `cppFormat.roots` 指定自有 EFM 來源與測試。
`include/` 的 SDK、`ThirdParty/` 與生成的 `.g.*` 檔由原始來源或產生器管理。

`.clang-format` 定義 Tab 寬 4、Allman 大括號、120 字元目標行寬、
namespace 不額外縮排及靠近型別的 `*`／`&`。對齊使用空白，
保留 include 順序、註解與字串。不可拆的內容可能超過目標行寬。
檔案使用 UTF-8 與 CRLF，基本編輯設定由 `.editorconfig` 提供。

### 安裝與指令

使用 Node.js 22 或更新 LTS，執行 `npm ci` 與 `npm run setup:cpp-format`。
Setup 驗證固定版本並顯示執行檔路徑。
開發依賴與 optional dependencies 必須啟用；離線安裝需要 npm 快取。
以 `--ignore-scripts` 安裝後補跑 `npm run prepare`。

指令對照上表。先排版再暫存；部分暫存時，排版後重新挑選區段。
Hook 只檢查 index，不改寫來源或暫存內容，再執行執行期 DLL 檢查。

同時提交來源與排版／範圍設定時，一起暫存設定。
檢查會拒絕未追蹤、被忽略或與 index 不一致的子設定，並處理 Git 換行正規化。

### 工具設定

VS Code 使用推薦的 Microsoft C/C++ 擴充套件與共用設定；
Linux／macOS 以 setup 輸出覆寫本機 `C_Cpp.clang_format_path`。
Visual Studio 啟用 ClangFormat 並選取 setup 顯示的執行檔，
移動 checkout 後重新選擇路徑。Lua 使用 StyLua。

`AGENTS.md` 與匯入它的 `CLAUDE.md` 共用代理指引。
Hook 與 CI 獨立檢查排版。必要檢查見 [PR 驗證](PR_VALIDATION.md#啟用)。
調整規則時執行排版測試及相關原生測試、建置與接口檢查。
