# C++ formatting / C++ 自動排版

[English](#english) | [繁體中文](#繁體中文)

## English

### Active scope

The approved DFLCC style now applies to authored C++ throughout
`src/efm/F-CK-1C_EFM/` and `src/efm/F-CK-1C_EFM_Tests/`.
SDK headers in `include/`, vendored code in `ThirdParty/`, and generated `.g.*`
files retain the format of their owning source or generator.

The root `.clang-format` owns the style and enables editor formatting.
`package.json` → `cppFormat.roots` defines the active source/test directories
for the CLI, commit hook, and CI. The script and `.clang-format-ignore`
both exclude SDK, third-party, and generated files; the ignore file also
protects those files when an editor invokes clang-format.

### Install

Run from the repository root with Node.js 22 or a newer LTS version:

```powershell
npm ci
```

The existing Husky setup now also verifies the pinned native clang-format
binary. Its exact version is locked by `@polycam/clang-tools` in `package.json`
and `package-lock.json`. npm downloads the matching platform package once and
caches it. Formatting requires Node.js and Git; MSVC remains the DLL compiler.
The dependency also contains other Clang tools; this workflow invokes only
clang-format. Supported packages are Windows x64, Linux x64, and macOS arm64.

To verify an installation or display the path for Visual Studio:

```powershell
npm run setup:cpp-format
```

If installation used `--ignore-scripts`, run `npm run prepare` afterward.
Install with development and optional dependencies enabled. An offline
installation requires the packages to be present in the npm cache. A missing
or mismatched formatter fails with a setup instruction rather than silently
using another installed version.

### Daily workflow

1. Edit C++ manually or with Codex / Claude Code.
2. Run `npm run format:cpp` before staging. It formats changed and new files
   in the active scope, including both staged and unstaged working-tree changes.
3. Review the diff, stage the intended changes, and commit.
4. The pre-commit hook checks the **Git index** for staged C++ files.
   It changes neither source files nor the index. On success, the existing EFM
   DLL build and automatic DLL staging run under their existing rules.

| Command | Purpose |
|---|---|
| `npm run format:cpp` | Format changed/new working-tree files in the active scope |
| `npm run format:cpp:all` | Explicitly format all authored C++ sources and tests |
| `npm run check:cpp-format` | Check all current files in the active scope without edits |
| `npm run check:cpp-format:staged` | Check staged content without edits or automatic staging |
| `npm run test:cpp-format` | Verify scope, partial staging, and formatter version handling |

`format:cpp` writes whole files. When partially staging, format first and then
choose the intended hunks again. The existing DLL hook still rejects dirty
runtime build inputs; passing the formatting check does not bypass that rule.
The formatting check accounts for Git's CRLF-to-LF normalization in the index.
When committing C++ together with formatting settings, stage the intended
settings first. The staged check rejects unstaged changes to `package.json`,
`package-lock.json`, and formatting/ignore settings along the source's directory
path, including untracked or ignored overrides. This ensures its scope and style
match the index while still permitting partially staged C++ source files.

The `PR Native Validation` workflow runs the same pinned formatter and tool tests
for pull requests; `C++ Format Check` retains checks on pushes to `dev` / `main`.
Both run on Windows and check the active scope
without changing or committing files. Repository branch protection must select
this check separately if it should be required for merging.

### Editors and coding agents

**VS Code on Windows:** install the recommended Microsoft C/C++ extension
(`ms-vscode.cpptools`). Open the repository folder and run `npm ci`.
The committed workspace settings explicitly select clang-format, its pinned
native executable, and format-on-save for C/C++. Tab width is 4. Use
**Format Document** (`Shift+Alt+F`) to format manually. Lua retains its StyLua
formatter. If you edit in Linux or macOS, override `C_Cpp.clang_format_path` in
your local workspace settings with the path printed by `setup:cpp-format`.

**Visual Studio:** run `npm run setup:cpp-format` and copy the printed executable
path. In **Tools → Options**, search for **ClangFormat**, enable ClangFormat
support, enable **Use custom clang-format.exe file**, and select that executable.
The path is a personal IDE setting. Set it again after moving the checkout or
switching to another checkout. VS reads the repository `.clang-format` files;
use **Format Document** (`Ctrl+K`, then `Ctrl+D`). Native VS formatting can also
run as you type; this setup does not require a format-on-save extension.

**Codex:** the repository `AGENTS.md` points to this guide and requires the shared
format/check commands after editing files in the active scope.

**Claude Code:** the repository `CLAUDE.md` imports `AGENTS.md` through
`@AGENTS.md`, so both agents share the same instructions. Start a new session
after adding these files; use `/context` in Claude Code to inspect loaded memory
files. The Git hook and CI verify formatting independently of agent instructions.

### Style

The shared rules use indentation tabs displayed at width 4, spaces for alignment,
Allman braces, a 120-column target, and no extra namespace indentation. Long
parameter lists prefer one item per line. `*` and `&` attach to the type.
Parameter continuations align with the opening bracket using spaces;
constructor initializers use one item per line to keep assembly expressions readable.
Include order, comment line breaks, string literals, and namespace comments are
preserved. Consecutive assignments/declarations and trailing comments are not
aligned into columns. Files use UTF-8 and CRLF.

120 columns is a target, not a reason to rewrite long comments, string literals,
or indivisible identifiers. `.editorconfig` supplies matching basic editor
settings; `.clang-format` owns the C++ layout rules.

### Maintaining the rules

This is the project's chosen style, rather than a universal C++ layout.
The C++ Core Guidelines recommend consistent indentation (NL.4) and suggest
K&R/Stroustrup layout (NL.17) when there is no established style. Allman is our
project convention; it is not the Guidelines' default brace layout. Discuss style changes before
reformatting, and keep bulk formatting separate from behavioral edits.
After changing the rules, verify the formatter workflow, native build/tests,
and DLL exports. Keep upstream and generator-owned files excluded.

## 繁體中文

### 啟用範圍

DFLCC 範例的排版規則已確認，現在套用於
`src/efm/F-CK-1C_EFM/` 與 `src/efm/F-CK-1C_EFM_Tests/` 的自有 C++，包含來源與測試。
`include/` 的 SDK、`ThirdParty/` 的第三方程式碼，以及生成的 `.g.*` 檔案，
仍由原始來源或生成器管理。

根目錄 `.clang-format` 集中管理規則並啟用編輯器排版。
`package.json` 的 `cppFormat.roots` 控制指令、提交檢查與 CI 的來源／測試範圍。
腳本與 `.clang-format-ignore` 都排除 SDK、第三方及生成檔；ignore 設定也保護編輯器排版。

### 安裝與各工具設定

使用 Node.js 22 或更新的 LTS，在專案根目錄執行：

```powershell
npm ci
```

這會安裝依賴、設定 Husky 並驗證固定版本的原生排版工具。版本由 `package.json`
與鎖定檔固定，工具透過 npm 下載並快取。排版只需 Node.js 與 Git；DLL 使用 MSVC 編譯。
套件另附其他 Clang 工具，本流程只使用 clang-format。套件支援 Windows x64、
Linux x64 與 macOS arm64。

執行 `npm run setup:cpp-format` 可再次驗證版本並顯示執行檔路徑。
若安裝時用了 `--ignore-scripts`，之後補跑 `npm run prepare`；開發依賴與 optional
dependencies 必須啟用。離線安裝需要先有 npm 快取。工具缺失或版本不符會明確報錯。

| 工具 | 設定方式 |
|---|---|
| VS Code（Windows） | 安裝推薦的 Microsoft C/C++ 擴充套件並開啟專案資料夾；共用設定會指定固定工具、Tab 寬 4 與存檔排版。手動排版用 `Shift+Alt+F` |
| Visual Studio | 執行 `npm run setup:cpp-format`，在 Tools → Options 搜尋 ClangFormat，啟用支援與自訂執行檔路徑，貼上顯示的路徑。手動排版用 `Ctrl+K`、再 `Ctrl+D` |
| Codex | 根目錄 `AGENTS.md` 要求修改自有 C++ 後執行共用排版與檢查指令 |
| Claude Code | `CLAUDE.md` 以 `@AGENTS.md` 匯入相同規範；新工作階段可用 `/context` 確認載入 |

VS 的路徑屬於個人 IDE 設定，移動專案或切換另一份 checkout 後需重新選擇。
VS 可使用內建的鍵入排版與手動排版，不需要額外的存檔排版擴充套件。
Linux／macOS 的 VS Code 請在本機設定覆寫 `C_Cpp.clang_format_path`，使用 setup 顯示的路徑。
Lua 繼續使用原有的 StyLua。

### 日常使用與提交

流程是：**修改 → 排版 → 檢視差異 → 暫存 → 排版檢查 → 原有 DLL 建置 → 提交**。

- `npm run format:cpp`：排版本次修改或新增的自有 C++。
- `npm run format:cpp:all`：明確排版全部自有 C++ 來源與測試。
- `npm run check:cpp-format`：檢查全部自有 C++，不修改檔案。
- `npm run check:cpp-format:staged`：檢查暫存內容，不修改來源或自動暫存。
- `npm run test:cpp-format`：測試範圍限制、部分暫存與固定版本檢查。

排版指令會改寫整個檔案；有部分暫存需求時，先排版，再重新挑選要提交的區段。
Hook 讀取 Git index，處理 Git 將 CRLF 正規化為 LF 的差異。
同時修改排版設定時，請先暫存預計提交的設定。若 `package.json`、`package-lock.json`
或來源目錄路徑上的排版／ignore 設定仍有未暫存差異，檢查會明確拒絕；未追蹤或被忽略的
子設定也包含在內。這能確保檢查採用與暫存區一致的範圍及規則，仍支援 C++ 來源的部分暫存。
格式通過後，原有 DLL hook 才繼續執行；runtime 建置來源存在未暫存修改時，仍會拒絕建置。

PR 的 `PR Native Validation` 與推送至 `dev`／`main` 的 `C++ Format Check` 會用相同版本
跑工具流程測試與全部自有 C++ 檢查。
若要禁止未通過的 PR 合併，另於 GitHub 分支保護設定將此檢查列為必須通過。

### 排版外觀

Tab 顯示寬度 4，對齊使用空白；大括號獨立一行；行寬以 120 字元為目標；namespace
內容不額外縮排。長參數列傾向每行一個參數，續行以空白對齊開括號；建構子的初始化清單
每行一項，讓組裝關係容易閱讀。`*`／`&` 靠近型別。
保留 include 順序、註解原有換行、字串內容及 namespace 註解。
變數宣告、賦值與尾端註解不刻意對齊成整片欄位。使用 UTF-8 與 CRLF。
長註解、字串或無法拆開的識別名稱可能超過 120 字元。

這是專案採用的排版風格。C++ Core Guidelines 的 NL.4 建議一致縮排；NL.17 在沒有
既定風格時建議 K&R／Stroustrup 版面。Allman 是本專案慣例，與該預設大括號版面不同。
日後調整規則先討論，整批排版與功能變更分開；
變更規則後驗證排版流程、原生測試、DLL 建置與匯出介面，並保留上游與生成檔的排除設定。

## Tool references

- [C++ Core Guidelines: NL.4, consistent indentation](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rl-indent)
- [C++ Core Guidelines: NL.17, suggested default layout](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rl-layout)
- [ClangFormat style options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- [Native npm tool package](https://github.com/PolyCam/clang-tools)
- [VS Code C++ formatting](https://code.visualstudio.com/docs/cpp/cpp-ide#code-formatting)
- [Visual Studio ClangFormat support](https://learn.microsoft.com/en-us/visualstudio/ide/reference/options-text-editor-c-cpp-formatting)
- [Codex project instructions](https://learn.chatgpt.com/docs/agent-configuration/agents-md)
- [Claude Code shared instructions](https://code.claude.com/docs/en/memory#share-one-file-with-other-coding-tools)
