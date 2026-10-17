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
| 提交檢查 | 排版、工具鏈、C++ 靜態分析、DLL 雜湊與接口、載入／卸載、原生測試 |
| 合併檢查 | 模擬合併、DLL 比對與原生測試；目標分支帶入 C++ 輸入變更時補做分析 |
| 檢查結果 | 彙整提交與合併結果，確認 PR／目標版本仍為最新 |
| 變更提醒 | 列出規則檔案、變更類型與差異連結 |

PR 至 `main`／`dev` 時執行，Draft PR 也適用；更新 PR 會取消舊驗證。
`main`／`dev` 的 push 另有排版檢查。

### 觸發門檻

各項依整個 PR 的累積差異判斷，包含刪除與重新命名。合併檢查也納入目標分支的差異。

| 檢查 | 觸發變更 |
|---|---|
| C++ 排版 | C++ 來源／標頭、排版設定、工具版本 |
| 工具測試 | 排版／PR 工具及其測試、相關 workflow、套件與標題設定 |
| DLL 一致性與接口 | 執行期來源／標頭（含 SDK、第三方與產生檔）、DLL、共用建置設定與腳本、DLL 驗證工具 |
| 原生測試 | 執行期來源、測試來源／專案、共用建置設定與腳本、`FM/*.jsonc` |
| C++ 靜態分析 | 執行期／測試輸入、共用建置設定與腳本、分析規則測試；純 `FM/*.jsonc` 變更跳過 |
| 架構測試 | 執行期來源、架構檢查器及其測試 |
| 系統目錄產生器 | `Core/Systems/`、產生器及其測試 |
| DCS ID 產生器 | `DcsIds/`、Cockpit／Input Lua、產生器及其測試 |
| 驗證工具 | DLL／工具鏈驗證腳本、共用建置工具、工具鏈配方、分析規則及其測試；先重建並比對 DLL |
| 合併與版本 | 每次檢查，確認可合併及 PR／目標版本仍為最新 |
| 標題與規則提醒 | 每次 PR 事件，檢查標題並更新規則清單 |

修改門檻或主驗證流程會執行完整檢查，並列入規則提醒。
未觸發的項目顯示「不適用」；差異讀取失敗則檢查失敗。
C++ 有改動即重建比對，不要求 DLL 一定改動；註解或排版可能產生相同 DLL。
門檻集中於 [`tools/pr-checks.mjs`](../tools/pr-checks.mjs)。本機 `validate:efm` 維持完整驗證。

一般編譯使用 `/W4 /WX`，靜態分析加入同一輪建置。合併檢查在目標分支沒有新增分析輸入時，
沿用提交階段的分析結果。只有 `ThirdParty/` 排除診斷；規則與手動指令見 [DLL 建置](BUILD_DLL.md#c-warnings-and-analysis)。

摘要顯示各項結果；版本、雜湊與差異片段可展開查看。
失敗紀錄含 JSON、摘要與 log，保存三天。
DCS 遊戲內測試由人工執行，結果記錄於 PR。

### 失敗處理

| 項目 | 處理方式 |
|---|---|
| 排版 | 執行 `npm run format:cpp`，重新暫存與提交 |
| 編譯／分析 | 查看 build log 的檔案、行號與規則，修正後重跑；第三方範圍設定變更須審查 |
| 工具版本 | 依 [建置需求](BUILD_DLL.md#requirements) 安裝對應版本的官方元件 |
| DLL 雜湊 | 確認工具鏈，重新建置並提交 DLL |
| 合併後 DLL 不同／合併衝突 | 同步目標分支，解決衝突後重新建置並提交 DLL |
| 接口／載入／原生測試 | 查看失敗項目與診斷紀錄，修正後重跑 |
| 檢查版本已過期 | 同步目標分支，再執行檢查 |

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

### 實測紀錄

[測試 PR #27（已關閉）](https://github.com/jjjjj103074/F-CK-1C/pull/27)。

| 測試 | 結果 |
|---|---|
| 本機完整驗證（2026-10-16） | 通過；工具版本檢查 0.29 秒，37 個接口、10 項驗證工具測試、3,257 項原生檢查，約 1 分 36 秒 |
| 門檻回歸測試 | 43 項工具／流程測試通過，涵蓋來源單獨變更、刪除、重新命名與目標分支差異 |
| 額外編譯參數與預設參數檔 | 清除 `CL/_CL_/LINK/_LINK_`、忽略 `Directory.Build.rsp`；實際編譯／連結指令與 DLL 雜湊相同 |
| C++ 行為改動、DLL 未更新 | 隔離副本實際重建；DLL 雜湊不同，在載入前失敗 |
| 文件／測試單獨變更 | 文件跳過編譯；測試單獨執行 3,257 項檢查，DLL 重建與載入均不適用 |
| [完整雲端驗證（2026-10-16）](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38840473756) | 精簡版的提交、合併與總結皆通過；共 6 分 26 秒 |
| [目標版本過期](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38676692562) | 提醒在取出程式前停止，提示同步目標分支後重跑 |
| [DLL 改動一個位元組](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38604318858) | 雜湊不同，載入停止、合併跳過、總結失敗 |
| [取消舊工作](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38604947711) | 舊工作取消，總結未通過 |
| [規則提醒](https://github.com/jjjjj103074/F-CK-1C/pull/27#issuecomment-6172320654) | 同一則提醒隨 PR 更新 |
| 開發工具依賴 | fast-uri 3.1.8、js-yaml 4.3.2；稽核 0 漏洞 |

工具版本與建置參數見 [DLL 建置](BUILD_DLL.md)。

測試 PR 使用專用提醒入口，驗證完成後已關閉，保留紀錄供查閱。
正式提醒入口與必要檢查依「啟用」步驟部署；DCS 留待人工實測。

### 建置效能實測（2026-10-16）

[同機比較紀錄](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38840470690)：
同一 runner，固定來源、工具配方與版本檢查，每次 `Release|x64` 完整重建。
所有 DLL 雜湊一致，兩版實際編譯／連結指令相同，原生測試各 3,257 項通過。

| 建置方式 | DLL 秒 | 建置內腳本合計秒 |
|---|---:|---:|
| 原 7 分鐘版的建置方式 | 43.69 | 2.70 |
| 精簡前：清空環境 | 122.82 | 82.87 |
| 只開啟程序重用 | 121.45 | 82.92 |
| 只改即時輸出 | 123.02 | 84.21 |
| 只停用詳細紀錄 | 125.53 | 82.93 |
| 恢復環境與官方初始化 | 52.76 | 2.60 |
| 精簡版 | 42.90 | 2.61 |

精簡前與精簡版的 DLL 數字採反轉順序後的重測；其餘各跑一次，完整結果見紀錄。
原生測試建置由 149.44 秒降為 69.19 秒。
主要延遲來自清空環境後的 PowerShell 腳本；四次建置約多 320 秒。
版本、DLL 雜湊、接口、載入與原生測試維持原有驗證。

[環境獨立比較](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38842558604)
固定 PowerShell 路徑並執行相同版本檢查：繼承環境 0.69 秒，白名單環境 17.93–37.05 秒。
單獨還原 PATH 或 PSModulePath 仍約 18 秒；此次定位到清空環境的操作，未逐一辨識其他變數。
精簡版使用正常繼承與官方初始化。

### 後續待辦（依順序）

- [x] MSVC 靜態分析與零警告門檻：自有程式 `/W4 /WX`、Native Recommended Rules，排除 `ThirdParty/`。
- [ ] 測試品質整理：在前項之後進行，開始前討論範圍與方法。

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
| 提交檢查 | Formatting, toolchain, MSVC analysis, whole-DLL hash, exports, loading and native tests |
| 合併檢查 | Simulated merge, original PR DLL comparison, native tests; analysis when the base adds C++ inputs |
| 檢查結果 | Submission/merge results and current PR/base revisions |
| 變更提醒 | Changed rule files and diffs |

PRs to main/dev, including drafts, run these checks. Updates cancel older runs.
Push formatting checks remain on main/dev. Summaries show results; diagnostics
and patches are collapsed. Failed runs retain JSON, Markdown and logs for three days.
If revisions are stale, sync the target branch and rerun.
Record manual DCS tests in the PR.

Each check uses the cumulative PR diff, including deletions and both rename paths.
Merge gates also include target-branch changes. Runtime sources/headers, build inputs
or the DLL trigger a rebuild and hash comparison, even when the DLL is unchanged.
Test/FM-only changes run native tests without loading the DLL. Gate/controller edits
run all checks and appear in the rule notice. Unselected checks show “不適用”;
diff failures fail validation. Local `validate:efm` still runs the full suite.
See the Chinese trigger table and [`pr-checks.mjs`](../tools/pr-checks.mjs).

### Rule changes and setup

The target branch's notice script reads PR diffs and updates one comment.
It covers CI, builds, hooks, title rules, tool versions, formatting, tests and exports.
Review rule changes manually. Preview with `npm run preview:pr-rules -- HEAD`.

Deploy the notice workflow to default main and its script to each target branch.
Enable `pull_request_target` under the applicable
[event policy](https://docs.github.com/en/actions/reference/security/securely-using-pull_request_target).
After a successful run, require `提交檢查`, `合併檢查`, `檢查結果` and up-to-date branches.

The Windows runner is `windows-2025-vs2026`. The
[closed test PR](https://github.com/jjjjj103074/F-CK-1C/pull/27) records successful runs,
tampered-DLL rejection and cancellation. The Chinese evidence table has run links
and timings. Production activation and manual DCS tests are separate steps.

MSVC analysis shares the native builds; normal compilation enforces `/W4 /WX`.
Only `ThirdParty/` is excluded. Merge analysis reuses the submission result when
the base introduces no analysis inputs. See [DLL build](BUILD_DLL.md#c-warnings-and-analysis).
Next: test-quality review; discuss its scope and criteria before implementation.
