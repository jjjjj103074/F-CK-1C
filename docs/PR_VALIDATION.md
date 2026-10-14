# PR validation / PR 驗證

## 繁體中文

### 日常操作

Linux 可以編輯程式碼；DLL 與原生測試仍在 Windows 建置。沿用 MSVC、VS 與 VS Code，
不用改用 CMake 或另一套編譯器。正式建置只支援 Release / x64。

```powershell
npm ci
npm run check:efm-toolchain
npm run build:efm
npm run validate:efm
```

`build:efm` 重建並更新工作目錄的 `bin/F-CK-1C_EFM.dll`。既有提交 hook 在需要時
仍會執行它並暫存 DLL。`validate:efm` 只驗證，不覆寫或暫存上傳 DLL；結果在
`tmp/pr-validation/submission/`。驗證不會啟動 DCS。

### PR 執行順序

| 順序 | 檢查 | 責任／失敗處理 |
|---|---|---|
| 1 | 建置與驗證規則變更提醒 | 在 PR 維持一則更新的提醒，列出規則分類、檔案、變更行數、舊／新片段與完整差異連結；不新增人工核准門檻 |
| 2 | PR 排版與工具測試 | 使用鎖定的 clang-format；失敗就停止提交驗證 |
| 3 | PR 來源與工具鏈 | 取得指定提交的乾淨來源；確認 MSVC、連結器、編譯器前後端、靜態執行期函式庫雜湊與 SDK 版本 |
| 4 | PR DLL 重建與 SHA-256 | 完整重建，與保留的上傳副本比對整份 SHA-256；不同就失敗，顯示兩份雜湊 |
| 5 | 上傳 DLL 接口與載入 | 對同一份驗證通過的上傳副本比對完整匯出名稱清單，再於獨立 x64 Windows 程序正常載入、查詢必要接口、卸載；不呼叫 EFM 回呼 |
| 6 | PR 原生測試與檢查器 fixtures | 執行既有原生測試，以及架構／System catalog／DCS ID 產生器的測試 |
| 7 | 合併相容驗證 | 保留 PR 原 DLL，模擬與指定目標提交合併；重建 DLL、核對它仍與 PR DLL 相同、執行原生測試 |
| 8 | 合併前總結 | 分開顯示兩項結果，重新查詢 PR／目標提交是否仍為最新；失敗、跳過或過期都不能作為通過 |

如果目標分支改變了 DLL 的有效輸入，合併階段會要求同步 PR、重建並提交 DLL，
不會偷偷換用合併後產出的 DLL，也不會將差異直接認定為惡意程式碼。
建置與測試不得改變來源、建置設定或上傳 DLL。
提交與合併 checkout 都保留原生測試需要的 `FM/` 配置；`.jsonc` 也納入驗證期間的來源不變檢查。

新原生驗證只在 PR 建立、更新、重新開啟、修改目標等事件執行；Draft PR 一樣檢查。
沒有 PR 的一般分支 push 不觸發它。既有 `main` / `dev` push 排版檢查保留。
同一 PR 更新時取消舊驗證；合併前總結會拒絕過期結果。

### 規則變更與信任邊界

提醒涵蓋 `.github/workflows/`、建置專案／props／targets、工具鏈固定值、建置與來源產生
腳本、提交 hook、來源正規化設定、工具版本、排版設定、測試內容與接口基準。
一般 runtime C++ 功能修改依正常 PR 審查，不額外標成建置規則。
刪除或重新命名規則檔案也會提醒。GitHub 未提供完整檔案清單時會明確指出無法完整判斷。

`PR Build and Validation Rule Notice` 使用可信目標分支的提醒程式，只讀 PR metadata／diff，
並更新自己的同一則提醒；不 checkout、建置或執行 PR 程式碼。
原生驗證使用低權限的 `pull_request` runner，不使用 repository secrets、寫入權限或共享快取。
PR 的建置／測試配方可以改動，會明確提醒人工審查。此設計採取**提醒加人工檢視**，
沒有設定強制 CODEOWNERS 核准；CI 綠燈不能證明新配方沒有繞過驗證。
特別檢視來源選擇、重建動作、DLL 複製、雜湊比較，以及測試是否被跳過。

提醒工作流程須先部署到預設分支 `main`，被驗證的目標分支也須包含可信提醒程式。
GitHub 從預設分支載入 `pull_request_target` 工作流程，再由此流程明確 checkout
PR 的可信目標提交。首次 PR 需人工檢視；只合併到 `dev` 尚未完成提醒流程的部署。
未合併前的隔離測試可使用測試分支專用入口執行相同提醒程式，但不代表正式觸發已啟用。
本機可預覽具體規則差異：`npm run preview:pr-rules -- HEAD`。

### 固定建置配方

`src/efm/EfmBuild.props` 是共用來源：MSVC v142 / 14.29.30133、cl 19.29.30159.0、
link 14.29.30159.0、Windows SDK 10.0.26100.0，以及四個編譯／連結工具檔案與
`libcmt.lib`、`libcpmt.lib`、`libvcruntime.lib`、SDK `libucrt.lib` 的 SHA-256。
`PreferredToolArchitecture` 明確固定為 x64，讓實際執行的工具與被核對的 Hostx64 檔案一致；
這與 DLL 的 x64 目標架構是兩項不同設定。
VS IDE 可更新；固定工具組件仍須安裝。工具不符時明確報環境錯誤，不自動換版本。
已驗證 VS／Build Tools 2026 的組件；雲端使用官方 `windows-2025-vs2026`。
舊版 VS 可在相同工具／SDK 版號目錄內帶有不同 CRT／UCRT 內容；只看版號不足以確認一致。

參數包含 `/Brepro`、`/experimental:deterministic`、來源 `/pathmap` 與 `/PDBALTPATH`。
真實 checkout 路徑映射成邏輯 `D:\efm-repro`，不需要實際建立該資料夾或磁碟。
PDB 保留；DLL 只記錄 PDB 檔名。VS 除錯可能要求你指定實際來源位置。
這些參數包含實驗性選項，更新工具鏈時必須重新確認跨路徑／跨 Windows 環境的結果。
正式專案不匯入個人 user.props 或 Directory.Build.props/targets，避免隱藏設定改變產物。

請在 Visual Studio Installer 安裝 v142 x64/x86 C++ Build Tools 與 Windows SDK 10.0.26100.0。
CLI 自動找到安裝位置並設定對應開發工具環境；無須手動開 Developer Shell。
直接用 VS 也讀同一份 props，建置前檢查版本。完整安裝／原生測試指令見
[DLL build guide](BUILD_DLL.md)。

### GitHub 啟用與人工 DCS 驗證

工作流程檔案本身不會變更 GitHub 分支保護。部署並實際跑過後，於 `dev` / `main` 設定：

也須確認 Actions event policy 允許 `pull_request_target`。GitHub 對部分公開 repository
的預設封鎖政策目前採評估模式，並規劃於 2026-11-02 強制套用；正式啟用前依
[官方事件安全指引](https://docs.github.com/en/actions/reference/security/securely-using-pull_request_target)
確認當下政策。本機或測試分支不會自動變更此政策。

- Require status checks：`PR 提交驗證`、`合併相容驗證`、`PR 驗證總結`。
- Require branches to be up to date before merging，避免目標更新後沿用舊結果。
- 規則變更提醒是審查資訊，不是新的人工核准限制。

原生工作使用 `windows-2025-vs2026`。只在失敗時保留小型 JSON、摘要與文字 log，保存三天，
不額外上傳 DLL 或專案來源。紀錄工具版本、兩份雜湊、步驟結果及耗時。

自動檢查通過不等於 DCS 遊戲內驗證成功。DCS 留給人工實測，安排於發佈前，或影響
回呼整合、飛行行為、座艙參數、生命週期的改動。PR 應另外記錄人工結果。

### 隔離 Draft PR 實測（2026-10-15）

[測試用 Draft PR #27](https://github.com/jjjjj103074/F-CK-1C/pull/27) 保持未合併，
只使用 `codex/ci-test-*` 分支。正式 `main` / `dev` 與分支保護未變更。

| 實測 | 結果／證據 |
|---|---|
| 本機最後工具版本 | 乾淨來源的完整 Submission 通過：整份 DLL SHA-256 相同、37 個接口、載入／卸載、8 項驗證 fixtures、3,257 項原生檢查；約 104 秒 |
| 第一次完整雲端正測 | [提交、合併、總結全部通過](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38602491686)；兩階段各 3,257 項原生檢查，原 PR DLL 保留 |
| 故意修改 DLL 一個位元組 | [雲端按預期拒絕](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38604318858)：顯示不同雜湊，未載入 DLL，合併跳過，總結失敗 |
| 取消舊驗證 | [舊工作取消](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38604947711)，總結未成為通過；最後驗收使用還原後的新 head |
| 最後還原重跑 | [最後驗收](https://github.com/jjjjj103074/F-CK-1C/actions/runs/38604973891) 全部通過：提交、合併、總結均 success，包含新增的 8 項驗證 fixtures；head 為 `aca668e` |
| 規則變更提醒 | [同一則更新的提醒](https://github.com/jjjjj103074/F-CK-1C/pull/27#issuecomment-6172320654)，列出具體規則檔案與差異；測試入口使用可信 base 的正式提醒程式 |
| 開發工具依賴修補 | fast-uri 3.1.8、js-yaml 4.3.2；本機及雲端安裝稽核均為 0 vulnerabilities |

第一輪完整雲端工作的實際執行時間為提交 4 分 41 秒、合併 2 分 35 秒、總結 4 秒，
合計約 7 分 20 秒，另加工作排隊與切換時間。錯誤 DLL 在提交工作的約 2 分 12 秒被拒絕。
這些是本次實測數值，後續會隨 runner 負載與專案規模變動。
本專案為公開 repository，使用標準 GitHub-hosted runner 的計算免費；artifacts 共用
帳號儲存配額，參見 [GitHub 計費規則](https://docs.github.com/en/billing/concepts/product-billing/github-actions)。
錯誤 DLL 的失敗 artifact 實測約 3.4 KB，只含三個文字診斷檔，保存三天。

實測曾找出並修正 shallow clone fixture、FM 配置遺漏、未明確指定 x64 host，
以及多套 VS 並存時選到不合格工具的問題。Windows 2022 / VS 2022 的同版號組件
帶有不同 CRT／UCRT，造成完整 DLL 雜湊不符；改用官方
[VS 2026 runner](https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-VS2026-Readme.md)
後才取得跨機一致結果，沒有放寬或正規化 DLL 比對。
正式配方現在也固定四個靜態執行期函式庫，提早指出這類環境差異。

**啟用限制：** 此測試不部署正式 `pull_request_target` 入口，也不啟用強制合併檢查。
正式入口仍須部署到預設 `main`，依前述方式確認 event policy 與分支保護；
DCS 遊戲內驗證仍未執行。臨時函式庫診斷 workflow 已從最後測試分支移除。

## English

Windows/MSVC remains the compiler; Linux is for editing. The shared profile in
`src/efm/EfmBuild.props` pins the v142 toolchain, Windows SDK and compiler/linker
fingerprints and enables deterministic time/path handling.
Four static CRT/UCRT libraries are also fingerprinted; identical compiler and SDK
version labels can still contain different serviced runtime libraries. The verified
installation family is VS/Build Tools 2026, and PR jobs use `windows-2025-vs2026`.
The profile also pins PreferredToolArchitecture to x64 so actual host tools match
the fingerprinted Hostx64 binaries, independently of the output target architecture.
Install the v142 x64/x86 tools and Windows SDK 10.0.26100.0 through Visual Studio Installer.
`npm run build:efm` rebuilds and publishes the local DLL; `npm run validate:efm`
verifies the working-tree submission without publishing or staging it.

PR submission and simulated-merge compatibility are separate Windows jobs.
Submission checks formatting/tool tests, the pinned environment, whole-file
SHA-256, the original uploaded DLL's exports/load/unload, generator fixtures,
and native source tests. A hash failure prevents DLL execution. Merge validation
preserves the submitted DLL before merging, rejects changed DLL results with an
instruction to update the branch/rebuild, and runs native tests. The final job
rejects failed/skipped jobs and checks both commit identities against current PR
metadata. Existing push formatting is retained; native validation runs only for PRs.
Both sparse checkouts retain the tracked FM configuration needed by native tests;
JSONC configuration is included in the source-unchanged check.

The independent target-branch notice job never executes PR code. It maintains
one PR comment listing exact changed rule files, line ranges, categories, old/new
diff excerpts and links. Renames/deletions and incomplete API responses are covered.
Build/validation rules in the PR may change; this is an advisory policy requiring
human scrutiny, not an immutable builder or enforced approval policy. Green checks
are not proof that a modified recipe has not bypassed validation. The initial
deployment requires manual review. GitHub loads the `pull_request_target`
workflow from the default branch (`main`); it then explicitly checks out the
trusted target commit. Deploy the workflow to main and the notice script to
each supported target branch. A dev-only merge does not activate this trigger.
An isolated test-branch entry can exercise the same notice code before deployment,
but does not prove the production trigger is active. Check the applicable Actions
event policy against GitHub's current official guidance before activation.
Preview locally with `npm run preview:pr-rules -- HEAD`.

Do not make the three validation checks required until a real cloud run produces
the same whole DLL hash. Then require `PR 提交驗證`, `合併相容驗證` and
`PR 驗證總結`, plus up-to-date branches on dev/main. Rule notices do not add required
reviewers. Failed runs retain only small diagnostics for three days. DCS is never
launched; in-game flight/cockpit/lifecycle tests remain separate manual evidence.

An isolated, unmerged [test Draft PR](https://github.com/jjjjj103074/F-CK-1C/pull/27)
has exercised successful submission/merge/summary checks, one-byte DLL tampering
rejection before loading, cancellation of superseded runs, and a single updating
rule notice. The detailed evidence, timings and final acceptance status are in
the Chinese test table above. This does not deploy the production default-branch
notice trigger or configure required checks; those remain separate adoption steps.
