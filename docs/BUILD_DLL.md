# EFM DLL build

提交用 DLL 統一使用本文件的工具鏈與指令，在 Windows 建置 `Release | x64`。
配方以 [`EfmBuild.props`](../src/efm/EfmBuild.props)、專案檔及共用建置腳本為準。

## Requirements

依 [Microsoft 安裝指引](https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc)
安裝 Build Tools，並在 Visual Studio Installer 匯入 [`.vsconfig`](../.vsconfig)。
VS、VS Code 與 AI 工具共用這套建置工具。

| Component | Version |
|---|---|
| Visual Studio / Build Tools | 2026 **18.10.3**，本配方的參考安裝版本 |
| MSBuild | 檔案版本 **18.10.1.42706** |
| MSVC v142 x64/x86 | 14.29.30133; cl 19.29.30159.0, link 14.29.30159.0 |
| Windows SDK | 10.0.26100.0 |
| PowerShell | Windows PowerShell 5.1；腳本使用 `-NoProfile` |
| Node.js | 22 LTS；提交／排版工具使用 `package-lock.json`，直接執行 DLL 建置腳本不需 Node.js |

從 [Microsoft 版本歷史](https://learn.microsoft.com/en-us/visualstudio/releases/2026/release-history)
取得對應版本的 Build Tools 安裝器；已有符合版本的安裝時，匯入設定補齊元件即可。
操作方式見 [官方 `.vsconfig` 指引](https://learn.microsoft.com/en-us/visualstudio/install/import-export-installation-configurations)。
安裝後執行：

```powershell
npm ci
npm run check:efm-toolchain
```

腳本依 [Microsoft `vswhere` 指引](https://github.com/microsoft/vswhere/wiki/Find-MSBuild)
尋找安裝，確認 MSBuild、編譯器、連結器版本與 SDK。
版本不符會顯示預期值與實際值；依本表安裝對應版本後重跑。

## Build

在儲存庫根目錄執行：

```powershell
npm run build:efm
```

等同 `./tools/build_dll.ps1`。它完整重建、更新 `bin/F-CK-1C_EFM.dll` 並驗證複製結果。

| 使用工具 | 提交用 DLL 的建置方式 |
|---|---|
| Codex／Claude Code | 執行 `npm run build:efm`，完成後執行 `npm run validate:efm` |
| VS Code | 在整合終端機執行相同指令 |
| Visual Studio | 可直接編譯與除錯；提交前在終端機執行相同指令 |
| 無 Node.js | 執行 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/build_dll.ps1` |

### 重現既有 DLL

使用乾淨 checkout，取出相同 commit，保留 `.gitattributes` 指定的換行格式：

```powershell
git status --short
npm ci
npm run check:efm-toolchain
npm run validate:efm
```

`git status` 應為空。`validate:efm` 保留提交 DLL，獨立重建並比對完整 SHA-256。
要驗證既有 DLL，直接執行驗證；`build:efm` 是產生並更新 DLL 的指令。
DLL 雜湊使用 PowerShell 官方 [`Get-FileHash`](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/get-filehash?view=powershell-5.1)。

## Native tests

在根目錄執行，並保留追蹤中的 `FM/` 設定：

```powershell
./tools/build_efm.ps1 -Target Tests
./src/efm/x64/Release/F-CK-1C_EFM_Tests.exe
```

結束碼 0 代表通過。DCS 遊戲內測試另行執行。

## Verification

| Command | Check |
|---|---|
| `npm run validate:efm` | Clean rebuild, whole-DLL hash, exports/loading, tool fixtures and native tests |
| `npm run check:efm-dll` | DLL load, required exports and unload in an x64 process |
| `./tools/check_efm_exports.ps1` | Complete export list against `EfmExports.baseline.txt` |
| `./tools/test_efm_architecture_checker.ps1` | Source dependency rules |
| `./tools/test_system_catalog_generator.ps1` | System discovery and generated catalog |
| `./tools/test_dcs_id_generator.ps1` | Generated DCS identifiers |

驗證報告位於 `tmp/pr-validation/`；CLI 建置紀錄預設位於 `tmp/efm-build/`。
`dll-build.log`／`tests-build.log` 保留實際編譯、連結指令與各項工作耗時，供比較參數及定位效能問題。
接口變更需一起更新來源、接口基準與 DCS 整合。CI 設定見 [PR 驗證](PR_VALIDATION.md)。

## Commit hook

The hook checks staged C++ formatting first. Runtime C++ sources, build settings,
build scripts or the DLL trigger a rebuild and automatic DLL staging.
The exact input list is in [.husky/pre-commit](../.husky/pre-commit).
Documentation and test-only edits skip the runtime build.

Builds read the working tree. Stage all related inputs before committing;
the hook reports unstaged, untracked or ignored build inputs.
Run native tests and the relevant verification commands above before opening a PR.
Formatting setup: [C++ formatting](CPP_FORMATTING.md).

## Build profile

### 固定參數與環境

| 項目 | 固定設定 |
|---|---|
| 建置 | `/t:Rebuild /p:Configuration=Release /p:Platform=x64`，x64 主機工具 |
| C++ | C++17、UTF-8、`/O2 /Oi /GL /EHsc /MT /GS /Gy /fp:precise` |
| 定義 | DLL 使用專案檔的 `WIN32/NDEBUG/_WINDOWS/_USRDLL/F_CK_1C_EFM_EXPORTS` 與 Unicode 定義 |
| 工具功能 | 原生 MSVC；ASAN 與 CLR 停用 |
| 可重現輸出 | `/Brepro /experimental:deterministic`、來源映射為 `D:\efm-repro`、PDB 僅記錄檔名 |
| MSBuild | `/noAutoResponse /p:UseEnv=false /m`；程序重用採預設設定 |
| 初始化 | 使用所選安裝的 `VsDevCmd.bat`，指定 x64、工具集與 SDK，清除 `CL/_CL_/LINK/_LINK_` |
| 匯入 | 停用 Directory.Build.props／targets；專案不匯入 user.props |

初始化由 Microsoft 腳本設定工具環境；共用建置指令清除額外編譯／連結參數，並停用自動讀取預設參數檔。
直接在 VS／MSBuild 建置仍會檢查工具鏈；提交用產物使用共用腳本，以套用相同初始化與建置參數。
各來源檔的 PCH、輸出路徑與其他參數取自已提交的專案檔；完整指令見建置 log。
建置直接使用所選安裝的 Microsoft C++ targets。
工具集與 SDK 透過 MSBuild 的 `PlatformToolset`、`VCToolsVersion`、`WindowsTargetPlatformVersion` 選用，
方式見 [官方命令列建置指引](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170)。

### 更新工具鏈

以獨立 PR 更新配方、安裝指引與 `.vsconfig`。
在開發電腦與 CI 以新配方重建並比對 DLL，再更新提交產物。
配方差異會列入規則提醒；工具鏈更新需通過整套驗證與跨機 DLL 比對。
GitHub runner 缺少指定版本時，安裝對應官方元件或更新工具鏈配方。

Visual Studio 除錯時可能需要指定本機來源路徑，因為產物中的來源映射為 `D:\efm-repro`。

System discovery writes `SystemCatalog.g.cpp` to the intermediate build directory.
To register a System, add `Core/Systems/<Owner>/Entry.cpp`.

## English quick reference

Use the pinned profile and `.vsconfig`; the validated reference is VS/Build Tools
18.10.3, MSBuild file version 18.10.1.42706, v142 14.29.30133 and SDK 10.0.26100.0.
Install the official Build Tools and import `.vsconfig`. The version check verifies
MSBuild, compiler and linker file versions and the requested SDK installation.

For an existing commit, run `npm ci`, `npm run check:efm-toolchain`, then
`npm run validate:efm` against its original DLL. To publish a changed DLL, use
`npm run build:efm`. All editors use this shared command for submitted artifacts.
It initializes the selected tools with Microsoft's `VsDevCmd.bat`, clears extra
compiler/linker flags and ignores automatic response files. Output is streamed;
logs contain the actual compiler/linker commands and task timings.

Builds use Microsoft's installed C++ targets and the toolset/SDK versions from the
project profile. Validation compares the whole DLL using PowerShell `Get-FileHash`,
then checks exports, loading and native tests. Update the profile, installation
instructions and DLL together through a PR, with cross-machine validation.
