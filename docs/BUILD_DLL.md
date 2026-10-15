# EFM DLL build

提交用 DLL 統一使用本文件的工具鏈與指令，在 Windows 建置 `Release | x64`。
配方以 [`EfmBuild.props`](../src/efm/EfmBuild.props)、專案檔及共用建置腳本為準。

## Requirements

在 Visual Studio Installer 匯入 [`.vsconfig`](../.vsconfig)。只需 Build Tools，不必安裝完整 IDE。

| Component | Version |
|---|---|
| Visual Studio / Build Tools | 已驗證 2026 **18.10.3**；其他安裝需通過相同內容指紋 |
| MSBuild | 檔案版本 **18.10.1.42706**，含建置引擎、C++ targets 與 tasks 的內容指紋 |
| MSVC v142 x64/x86 | 14.29.30133; cl 19.29.30159.0, link 14.29.30159.0 |
| Windows SDK | 10.0.26100.0；標頭、x64 函式庫與工具內容須符合配方 |
| PowerShell | Windows PowerShell 5.1；腳本使用 `-NoProfile` |
| Node.js | 22 LTS；提交／排版工具使用 `package-lock.json`，直接執行 DLL 建置腳本不需 Node.js |

安裝元件 ID 不會鎖定修補版；版本目錄名稱相同也可能有不同內容。安裝後先檢查：

```powershell
npm ci
npm run check:efm-toolchain
```

腳本會選擇符合配方的安裝；明確指定不相符的 MSBuild 時會報錯。
不相符時還原安裝包，不修改配方雜湊來略過檢查。

### 固定安裝包

從 [Microsoft 版本歷史](https://learn.microsoft.com/en-us/visualstudio/releases/2026/release-history)
下載 **18.10.3 的固定版 Build Tools 安裝器**，檔名存為 `vs_buildtools.exe`。
依 [官方離線安裝指引](https://learn.microsoft.com/en-us/visualstudio/install/create-a-network-installation-of-visual-studio)
建立安裝包：

```powershell
$configPath = (Resolve-Path .\.vsconfig).Path
.\vs_buildtools.exe --layout C:\EFM-BuildTools-18.10.3 --config $configPath --lang en-US
C:\EFM-BuildTools-18.10.3\vs_buildtools.exe --layout C:\EFM-BuildTools-18.10.3 --verify
C:\EFM-BuildTools-18.10.3\vs_buildtools.exe --noWeb --config $configPath --installPath C:\EFM-BuildTools --passive --wait --norestart
```

保存整個 layout、安裝器及 `.vsconfig`；封存後不更新該 layout。
首次建立的安裝包需通過 `check:efm-toolchain` 與 DLL 比對，再作為團隊共用版本。
固定版安裝器仍需這項驗證，因為 SDK／獨立元件可能有 servicing 差異。
`--noWeb` 限制本次安裝來源；之後的 Installer／系統更新仍需自行管理。

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
`dll-build.log`／`tests-build.log` 保留實際編譯與連結指令，供比較參數。
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
| 可重現輸出 | `/Brepro /experimental:deterministic`、來源映射為 `D:\efm-repro`、PDB 僅記錄檔名 |
| MSBuild | `/noAutoResponse /nr:false /p:UseEnv=false /p:VcpkgEnabled=false /m` |
| 個人設定 | 使用獨立子程序與白名單環境；重新設定 PATH，再由指定 VsDevCmd 初始化 |
| 匯入 | 停用 Directory.Build.props／targets；使用空的 MSBuild 使用者匯入目錄；專案不匯入 user.props |

呼叫端的 `CL/_CL_/LINK/_LINK_`、工具路徑、INCLUDE／LIB 與 ForceImport 等環境設定不會傳入共用建置程序。
直接在 VS／MSBuild 建置仍會檢查工具鏈；提交用產物使用共用腳本，以套用完整環境隔離。
各來源檔的 PCH、輸出路徑與其他參數取自已提交的專案檔；完整指令見建置 log。

### 內容指紋範圍

配方除了原有編譯器及 CRT 單檔雜湊，也檢查下列群組的相對路徑與內容：

| 群組 | 範圍 |
|---|---|
| MSVC | `bin/Hostx64/x64`、`include`、`lib/x64` |
| Windows SDK | 指定版本的 `Include`、`Lib/ucrt/x64`、`Lib/um/x64`、`bin/x64` |
| MSBuild | `MSBuild/Current/Bin`，含 x64 引擎與相依檔案 |
| C++ targets/tasks | `MSBuild/Microsoft/VC/v180` |
| 開發環境初始化 | `Common7/Tools/VsDevCmd.bat`、`vsdevcmd/` 與 `VC/Auxiliary/Build` |

群組指紋依相對路徑排序，與安裝位置、檔案時間無關。
語系資源 `*.resources.dll` 與數字語系目錄不參與指紋；它們只影響診斷訊息。
Windows／.NET Framework 作業環境並未封存，最後仍以完整 DLL 雜湊一致判定重現成功。

### 更新工具鏈

以獨立 PR 更新配方、安裝指引與 `.vsconfig`，並重新建立、保存通過驗證的 layout。
在開發電腦與 CI 以新配方重建並比對 DLL，再更新提交產物。
配方差異會列入規則提醒；新版本需整套驗證，不能只更換版本字串或接受目前環境的雜湊。
GitHub runner 不提供相符內容時，CI 會明確報錯；先還原固定安裝包或審查工具鏈更新。

Visual Studio 除錯時可能需要指定本機來源路徑，因為產物中的來源映射為 `D:\efm-repro`。

System discovery writes `SystemCatalog.g.cpp` to the intermediate build directory.
To register a System, add `Core/Systems/<Owner>/Entry.cpp`.

## English quick reference

Use the pinned profile and `.vsconfig`; the validated reference is VS/Build Tools
18.10.3, MSBuild file version 18.10.1.42706, v142 14.29.30133 and SDK 10.0.26100.0.
Content fingerprints also cover headers, x64 libraries, SDK tools, MSBuild/C++ tasks
and the developer-shell scripts. Matching version labels alone are insufficient.

For an existing commit, run `npm ci`, `npm run check:efm-toolchain`, then
`npm run validate:efm` against its original DLL. To publish a changed DLL, use
`npm run build:efm`. All editors use this shared command for submitted artifacts.
It rebuilds in a clean child environment, ignores automatic response files and
isolates user imports. Logs contain the actual compiler/linker commands.

Preserve a verified offline layout made with the fixed-version bootstrapper.
The Windows/.NET host is not an archived image; a matching whole-DLL hash remains
the acceptance criterion. Update the profile, installation recipe and DLL together
through a reviewed PR, with cross-machine validation.
