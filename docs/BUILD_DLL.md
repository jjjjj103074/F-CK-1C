# EFM DLL build

提交用 DLL 使用 Windows Release/x64。
[共用配方](../src/efm/EfmBuild.props)、專案檔與建置腳本定義工具與參數。

## Requirements

安裝 Visual Studio Build Tools，匯入 [`.vsconfig`](../.vsconfig)。

| 工具 | 版本 |
|---|---|
| 參考 VS／Build Tools 安裝 | 2026 18.10.3 |
| MSBuild 檔案版本 | 18.10.1.42706 |
| MSVC v142 x64/x86 | 14.29.30133 |
| cl／link 檔案版本 | 19.29.30159.0 |
| Windows SDK | 10.0.26100.0 |
| 腳本 | Windows PowerShell 5.1 |
| 提交與排版工具 | Node.js 22 LTS、package-lock.json |

```powershell
npm ci
npm run check:efm-toolchain
```

腳本透過 vswhere 尋找相容安裝，驗證 MSBuild、編譯器、連結器與 SDK。
版本不符時依預期值補齊元件。
安裝方式見 [Microsoft Build Tools](https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc)
及 [vsconfig](https://learn.microsoft.com/en-us/visualstudio/install/import-export-installation-configurations)。

## Build

在儲存庫根目錄執行：

```powershell
npm run build:efm
```

等同 `./tools/build_dll.ps1`。完整重建後更新
`bin/F-CK-1C_EFM.dll`，並確認複製結果。
Visual Studio 可直接建置／除錯；提交產物使用共用腳本。

## 重現既有 DLL

從乾淨 checkout 保留提交中的 DLL，依 .gitattributes 取出來源：

```powershell
git status --short
npm ci
npm run validate:efm
```

驗證獨立重建並比對完整 SHA256，不更新提交 DLL。
產生 DLL 使用 build；重現既有 DLL 直接使用 validate。

## C++ warnings and analysis

DLL 與測試共用 /W4 /WX。
ThirdParty 以 /external:I、/external:W0、/analyze:external- 排除診斷；
SDK 與自有生成碼仍受檢查。JSON 標頭使用 `<nlohmann/json.hpp>`。

完整驗證在同一輪建置加入 /analyze，
採 NativeRecommendedRules.ruleset 與固定工具鏈的 CppCoreCheck.dll。
一般建置與 commit hook 不執行分析。單獨分析：

```powershell
./tools/build_efm.ps1 -Target Dll -Analyze
./tools/build_efm.ps1 -Target Tests -Analyze
```

Visual Studio 的 Run Code Analysis 使用相同規則。
警告使驗證失敗；位置與訊息保留於 build log。

## Verification

| 指令 | 內容 |
|---|---|
| `npm run validate:efm` | 工具鏈、乾淨建置與分析、DLL 雜湊、接口、載入、工具 fixtures、原生測試與來源完整性 |
| `npm run check:efm-dll` | x64 程序內載入、必要接口與卸載 |
| `./tools/check_efm_exports.ps1` | 對照 EfmExports.baseline.txt |
| `./tools/test_efm_architecture_checker.ps1` | 依賴規則 fixtures |
| `./tools/test_system_catalog_generator.ps1` | System catalog 產生器 |
| `npm run test:dcs-id-generator` | 追蹤中的 DCS ID 產物與無效輸入 |
| `./tools/test_cpp_analysis.ps1` | 外部標頭與警告規則 |

只執行原生測試：

```powershell
./tools/build_efm.ps1 -Target Tests
./src/efm/x64/Release/F-CK-1C_EFM_Tests.exe
```

保留追蹤中的 FM 設定。結束碼 0 表示通過。
驗證報告位於 `tmp/pr-validation/`，一般 CLI 建置紀錄位於 `tmp/efm-build/`。
dll-build.log／tests-build.log 保留實際編譯、連結指令與耗時。
CI 見 [PR 驗證](PR_VALIDATION.md)，遊戲內行為見 [DCS 測試](DCS_TESTING.md)。

## 建置與提交規則

共用指令以 /t:Rebuild、Release/x64、x64 主機工具建置。
初始化使用所選安裝的 VsDevCmd.bat，清除 CL/_CL_/LINK/_LINK_，
使用 /noAutoResponse、UseEnv=false，停用 Directory.Build.props／targets。
C++17、UTF-8、/MT 與其他編譯選項由共用配方及專案檔定義。
/Brepro、deterministic 與來源映射 `D:\efm-repro` 支援可重現輸出；
除錯時可能需指定本機來源位置。

Pre-commit hook 先檢查暫存 C++ 排版；執行期輸入或 DLL 變更時重建並暫存 DLL。
文件與測試單獨變更略過執行期建置。
建置讀取工作樹，相關輸入必須一起暫存；未暫存、未追蹤或被忽略的輸入會被拒絕。
原生測試需另行執行。

工具鏈版本、配方、.vsconfig、安裝指引與 DLL 應一起驗證。
工具鏈更新需在開發電腦與 CI 重建比對。
System catalog 由 `Core/Systems/*/Entry.cpp` 生成至中間目錄。

## English quick reference

Install the pinned toolchain and import .vsconfig, then run `npm ci`.
Use `build:efm` to update the DLL or `validate:efm` to verify the committed DLL.
Validation includes MSVC analysis, whole-DLL hash, exports/loading, native tests
and source integrity. Keep FM settings available.
Reports are under tmp/pr-validation; DCS tests are separate.
