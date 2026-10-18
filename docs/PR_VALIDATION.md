# PR 驗證 / PR validation

## 繁體中文

### 本機使用

DLL 與原生測試在 Windows 執行，環境見 [DLL 建置](BUILD_DLL.md)。
驗證現有提交時，直接執行：

```powershell
npm ci
npm run check:cpp-format
npm run validate:efm
```

`validate:efm` 保留提交 DLL，重建並比對完整 SHA256。
需要更新 DLL 時先執行 `npm run build:efm`。
本機報告位於 `tmp/pr-validation/submission/`。

### CI 檢查

| 檢查 | 內容 |
|---|---|
| 標題格式 | Conventional Commit 格式 |
| 提交檢查 | 適用的排版、工具測試、工具鏈、分析、DLL、接口、載入及原生測試 |
| 合併檢查 | 模擬合併、DLL 比對與原生測試 |
| 檢查結果 | 提交／合併結果與最新 PR／目標版本 |
| 變更提醒 | 規則檔案與差異，供人工審查 |

main/dev 的 PR 包含 Draft。建立、重開、push 或切換目標分支會執行檢查並取消舊工作。
只改說明會略過；只改標題僅驗證標題，兩者均保留已有的原生檢查與提醒結果。
main/dev push 另有 C++ 排版檢查。

### 檢查範圍

門檻由 [pr-checks.mjs](../tools/pr-checks.mjs) 決定，
使用整個 PR 累積差異，包含刪除及重新命名兩端；合併階段加入目標分支差異。

| 輸入 | 檢查 |
|---|---|
| 自有 C++、排版設定或套件 | 適用的排版與工具測試 |
| 執行期來源／標頭、DLL、共用建置輸入 | DLL 重建、雜湊、接口、載入、原生測試及分析 |
| 測試來源／專案 | 原生測試與分析 |
| FM JSONC | 原生測試 |
| 架構、System catalog、DCS ID 工具與對應輸入 | 對應產生器／規則測試 |
| DLL 驗證工具、工具鏈或分析規則 | DLL 比對及驗證工具測試 |
| 主控制流程／門檻 | 全部檢查 |
| Markdown 文件 | 不觸發原生建置；位於 System 或 DcsIds 目錄時仍觸發對應產生器測試 |

未觸發項目顯示不適用；差異讀取失敗則檢查失敗。
執行期輸入變更會重建比對，即使提交 DLL 沒有變動。
合併階段只在目標分支帶入分析輸入時補做分析，其餘沿用提交結果。
本機 `validate:efm` 執行完整原生驗證，不依 PR 門檻篩選。

門檻依路徑與副檔名判斷；EFM 執行期目錄中除 `.md` 外的檔案，
包含 `ReadMe.txt`，均視為執行期輸入，觸發建置、分析與原生測試。

一般建置為 /W4 /WX，完整驗證在同一輪建置加入 MSVC 靜態分析。
ThirdParty 使用外部標頭排除，SDK 與自有生成碼仍受檢查。
詳細規則見 [C++ warnings and analysis](BUILD_DLL.md#c-warnings-and-analysis)。

摘要顯示每項結果，失敗時保存 JSON、摘要與 log 三天。
DCS 結果依 [DCS 測試](DCS_TESTING.md) 另行記錄。

### 失敗處理

- 排版：執行 format:cpp，檢視並重新暫存。
- 編譯／分析：依 build log 的檔案、行號與規則定位。
- 工具鏈：比對 BUILD_DLL 指定版本。
- DLL 雜湊／合併衝突：同步來源、重建 DLL 並重新驗證。
- 接口、載入或原生測試：檢查對應診斷。
- 版本過期：同步目標分支並重跑。

### 規則提醒

提醒使用目標分支程式讀取 PR 差異，涵蓋 CI、工具鏈、建置、hook、
標題、排版、測試與接口規則。PR 維持一則更新的提醒，提醒本身不屬必要檢查。
本機預覽：`npm run preview:pr-rules -- HEAD`。

### 啟用

將提醒 workflow 部署到預設 main、提醒程式部署到目標分支，
並依 GitHub 事件政策啟用 pull_request_target。
實際執行通過後，設定必要檢查：

- `PR / 提交檢查`
- `PR / 合併檢查`
- `PR / 檢查結果`

要求分支為最新。Repository rulesets／branch protection 以 GitHub 設定為準，
本機檔案無法證明其已啟用。Windows runner 為 `windows-2025-vs2026`。

## English

Use the [build guide](BUILD_DLL.md) to install the pinned Windows toolchain.
For an existing commit, run `npm ci`, `npm run check:cpp-format` and
`npm run validate:efm`. Build updates the DLL; validation preserves it.

PR checks validate the submitted tree, simulated merge and current revisions.
Selection uses the cumulative diff; deleted and renamed inputs remain covered.
Runtime inputs trigger DLL rebuild/hash/export/load checks. Test-only inputs run
native tests and analysis; FM JSONC runs native tests.
Controller changes select all checks. Local validation runs the full native suite.
Markdown does not trigger native builds, but System/DcsIds paths still select
generator checks. Non-Markdown files under the EFM runtime directory, including
ReadMe.txt, select runtime checks.

Description edits preserve results; title edits validate the title only.
Code updates and retargeting cancel older validation.
Failure reports are retained for three days.
Record [DCS verification](DCS_TESTING.md) separately.

The notice uses target-branch code and is reviewed manually.
Deploy its workflow/script and configure the three required checks listed above,
including an up-to-date branch requirement, in GitHub settings.
