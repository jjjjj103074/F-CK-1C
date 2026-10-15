# PR 驗證 / PR validation

## 繁體中文

### 日常使用

Linux 可編輯；DLL 與原生測試在 Windows 執行。
安裝方式見 [DLL 建置](BUILD_DLL.md)。

```powershell
npm ci
npm run build:efm
npm run validate:efm
```

`build:efm` 更新 `bin/F-CK-1C_EFM.dll`；`validate:efm` 比對提交 DLL 與重建結果。
本機報告位於 `tmp/pr-validation/submission/`。

### CI 檢查

| 檢查 | 內容 |
|---|---|
| 標題格式 | 驗證 PR 標題的 commit 格式 |
| 提交檢查 | 排版、工具鏈、DLL 雜湊與接口、載入／卸載、原生測試 |
| 合併檢查 | 模擬合併目標版本，重建並比對 PR 原 DLL，再執行原生測試 |
| 檢查結果 | 彙整提交與合併結果，確認 PR／目標版本仍為最新 |
| 變更提醒 | 列出規則檔案、變更類型與差異連結 |

PR 至 `main`／`dev` 時執行，Draft PR 也適用；更新 PR 會取消舊驗證。
`main`／`dev` 的 push 另有排版檢查。

摘要顯示各項結果；版本、雜湊與差異片段可展開查看。
失敗紀錄含 JSON、摘要與 log，保存三天。
DCS 遊戲內測試由人工執行，結果記錄於 PR。

### 失敗處理

| 項目 | 處理方式 |
|---|---|
| 排版 | 執行 `npm run format:cpp`，重新暫存與提交 |
| 工具鏈 | 依 [建置需求](BUILD_DLL.md#requirements) 安裝符合配方的元件 |
| DLL 雜湊 | 確認工具鏈，重新建置並提交 DLL |
| 合併後 DLL 不同／合併衝突 | 同步目標分支，解決衝突後重新建置並提交 DLL |
| 接口／載入／原生測試 | 查看失敗項目與診斷紀錄，修正後重跑 |
| 檢查版本已過期 | 重新執行檢查 |

### 規則提醒

涵蓋 CI、編譯設定、建置腳本、提交與標題規則、工具版本、排版、測試與接口基準。
PR 留一則更新的提醒：檔案表供略讀，差異片段供審查。
本機預覽：`npm run preview:pr-rules -- HEAD`。

提醒使用目標分支的程式讀取 PR 差異；建置與測試使用 PR 的程式。
規則變更由人工審查，提醒本身不列入必要通過檢查。

### 啟用

1. 將提醒 workflow 部署到預設 `main`，並將提醒程式部署到各目標分支。
2. 依 [GitHub 事件政策](https://docs.github.com/en/actions/reference/security/securely-using-pull_request_target) 啟用 `pull_request_target`。
3. 實跑通過後，在 `main`／`dev` 將 `提交檢查`、`合併檢查`、`檢查結果` 設為必要檢查，並要求分支為最新。

Windows runner 使用 `windows-2025-vs2026`。
本公開專案的標準 runner 計算免費；artifacts 使用帳號儲存配額。
詳見 [GitHub 計費](https://docs.github.com/en/billing/concepts/product-billing/github-actions)。

### 實測紀錄（2026-10-15）

[測試 Draft PR #27](https://github.com/jjjjj103074/F-CK-1C/pull/27)。

| 測試 | 結果 |
|---|---|
| 本機完整驗證 | 通過；37 個接口、8 項驗證工具測試、3,257 項原生檢查，約 104 秒 |
| [完整雲端驗證](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38604973891) | 提交、合併、總結通過；兩階段各 3,257 項原生檢查 |
| [DLL 改動一個位元組](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38604318858) | 雜湊不同，載入停止、合併跳過、總結失敗 |
| [取消舊工作](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38604947711) | 舊工作取消，總結未通過 |
| [規則提醒](https://github.com/jjjjj103074/F-CK-1C/pull/27#issuecomment-6172320654) | 同一則提醒隨 PR 更新 |
| 開發工具依賴 | fast-uri 3.1.8、js-yaml 4.3.2；稽核 0 漏洞 |

完整雲端工作合計約 8 分鐘，另加排隊；錯誤 DLL 約 2 分鐘被攔下。
失敗 artifact 約 3.4 KB。
跨機雜湊一致需使用相同編譯工具與 CRT／UCRT；固定配方見 `src/efm/EfmBuild.props`。

此 PR 使用測試專用提醒入口，保持 Draft。
正式提醒入口與必要檢查依「啟用」步驟部署；DCS 留待人工實測。

## English

### Use

Edit on Linux; build and test on Windows. See [DLL build](BUILD_DLL.md) for setup.

```powershell
npm ci
npm run build:efm
npm run validate:efm
```

`build:efm` updates `bin/F-CK-1C_EFM.dll`; `validate:efm` compares that DLL with a
clean rebuild. Reports are saved in `tmp/pr-validation/submission/`.

### Checks

| Check | Purpose |
|---|---|
| 標題格式 | PR title convention |
| 提交檢查 | Formatting, toolchain, whole-DLL hash, exports, loading and native tests |
| 合併檢查 | Simulated merge, rebuild against the original PR DLL, native tests |
| 檢查結果 | Submission/merge results and current PR/base revisions |
| 變更提醒 | Changed rule files and diffs |

PRs to main/dev, including drafts, run these checks. Updates cancel older runs.
Push formatting checks remain on main/dev. Summaries show results; diagnostics
and patches are collapsed. Failed runs retain JSON, Markdown and logs for three days.
Record manual DCS tests in the PR.

### Rule changes and setup

The target branch's notice script reads PR diffs and updates one comment.
It covers CI, builds, hooks, title rules, tool versions, formatting, tests and exports.
Review rule changes manually. Preview with `npm run preview:pr-rules -- HEAD`.

Deploy the notice workflow to default main and its script to each target branch.
Enable `pull_request_target` under the applicable
[event policy](https://docs.github.com/en/actions/reference/security/securely-using-pull_request_target).
After a successful run, require `提交檢查`, `合併檢查`, `檢查結果` and up-to-date branches.

The Windows runner is `windows-2025-vs2026`. The
[test Draft PR](https://github.com/jjjjj103074/F-CK-1C/pull/27) records successful runs,
tampered-DLL rejection and cancellation. The Chinese evidence table has run links
and timings. Production activation and manual DCS tests are separate steps.
