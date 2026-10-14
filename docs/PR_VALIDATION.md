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
| 3 | PR 來源與工具鏈 | 取得指定提交的乾淨來源；確認 MSVC、連結器、編譯器前後端雜湊與 SDK 版本 |
| 4 | PR DLL 重建與 SHA-256 | 完整重建，與保留的上傳副本比對整份 SHA-256；不同就失敗，顯示兩份雜湊 |
| 5 | 上傳 DLL 接口與載入 | 對同一份驗證通過的上傳副本比對完整匯出名稱清單，再於獨立 x64 Windows 程序正常載入、查詢必要接口、卸載；不呼叫 EFM 回呼 |
| 6 | PR 原生測試與檢查器 fixtures | 執行既有原生測試，以及架構／System catalog／DCS ID 產生器的測試 |
| 7 | 合併相容驗證 | 保留 PR 原 DLL，模擬與指定目標提交合併；重建 DLL、核對它仍與 PR DLL 相同、執行原生測試 |
| 8 | 合併前總結 | 分開顯示兩項結果，重新查詢 PR／目標提交是否仍為最新；失敗、跳過或過期都不能作為通過 |

如果目標分支改變了 DLL 的有效輸入，合併階段會要求同步 PR、重建並提交 DLL，
不會偷偷換用合併後產出的 DLL，也不會將差異直接認定為惡意程式碼。
建置與測試不得改變來源、建置設定或上傳 DLL。

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

第一次加入這套流程時，目標分支尚未有可信提醒程式，因此需人工檢視首次 PR。
合併部署後，後續 PR 才會由目標分支版本自動提醒。
本機可預覽具體規則差異：`npm run preview:pr-rules -- HEAD`。

### 固定建置配方

`src/efm/EfmBuild.props` 是共用來源：MSVC v142 / 14.29.30133、cl 19.29.30159.0、
link 14.29.30159.0、Windows SDK 10.0.26100.0，以及四個編譯／連結工具檔案的 SHA-256。
VS IDE 可更新；固定工具組件仍須安裝。工具不符時明確報環境錯誤，不自動換版本。

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

- Require status checks：`PR 提交驗證`、`合併相容驗證`、`PR 驗證總結`。
- Require branches to be up to date before merging，避免目標更新後沿用舊結果。
- 規則變更提醒是審查資訊，不是新的人工核准限制。

原生工作使用 `windows-2022`。只在失敗時保留小型 JSON、摘要與文字 log，保存三天，
不額外上傳 DLL 或專案來源。紀錄工具版本、兩份雜湊、步驟結果及耗時。

自動檢查通過不等於 DCS 遊戲內驗證成功。DCS 留給人工實測，安排於發佈前，或影響
回呼整合、飛行行為、座艙參數、生命週期的改動。PR 應另外記錄人工結果。

**目前驗收限制：** 本機可驗證同一電腦不同路徑的完整 DLL 雜湊；雲端一致性仍須在你
檢查、提交並部署後實際執行。在成功前不要將新檢查設成強制合併門檻。

## English

Windows/MSVC remains the compiler; Linux is for editing. The shared profile in
`src/efm/EfmBuild.props` pins the v142 toolchain, Windows SDK and compiler/linker
fingerprints and enables deterministic time/path handling. Install the v142
x64/x86 tools and Windows SDK 10.0.26100.0 through Visual Studio Installer.
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

The independent target-branch notice job never executes PR code. It maintains
one PR comment listing exact changed rule files, line ranges, categories, old/new
diff excerpts and links. Renames/deletions and incomplete API responses are covered.
Build/validation rules in the PR may change; this is an advisory policy requiring
human scrutiny, not an immutable builder or enforced approval policy. Green checks
are not proof that a modified recipe has not bypassed validation. The initial
deployment requires manual review because the trusted notice is not yet on the
target branch. Preview locally with `npm run preview:pr-rules -- HEAD`.

Do not make the three validation checks required until a real cloud run produces
the same whole DLL hash. Then require `PR 提交驗證`, `合併相容驗證` and
`PR 驗證總結`, plus up-to-date branches on dev/main. Rule notices do not add required
reviewers. Failed runs retain only small diagnostics for three days. DCS is never
launched; in-game flight/cockpit/lifecycle tests remain separate manual evidence.
