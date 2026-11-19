# MC 七元件重構驗證紀錄

- 架構／顯示遷移來源：`db73b19a018495bf3797c2042f2050e004f2105f`。
- 最終執行期來源（含診斷容量修正）：`29d53000ea56f01a6e116168955c6ff1f597af60`。
- 最終來源 tree：`6892b8a7a93a511d7ce88f9b0d3b8684d960ab5e`。
- 比較基準：`d7dcccbad59f0dd36dbacb640e9b1a66f39ce3f9`。
- 完整變更與資料流見 [MC 重構報告](MC_REFACTOR_REPORT.md)。

## 本機驗證

| 項目 | 結果 |
|---|---|
| MC C++17 行為回歸 | 172 checks，0 failures |
| Lua 5.1.5 實際介面／輸入設定 | 6,169 checks，0 failures |
| 舊顯示規則參考重播 | 8,192 幀，包含跳過 HMCS 更新；讀取時類別／數量全部一致 |
| 實際 C++→HMCS 回歸 | d7 與 db73b19 各 4,096 幀；任務／掛載／尋標／動作及最終畫面類別／數量一致 |
| Exporter 暫存可攜 fixture | 119 checks，0 failures；67 筆錯誤／恢復可完整回報 |
| Node.js 22 工具回歸 | 84 passed、0 failed、5 項環境／Windows 限定跳過 |
| DCS ID 產生器及 fixtures | passed |
| clang-format 22.1.8 | 完整範圍 272 檔，0 需排版；提交前另核對 index |
| git diff --check | passed |

實際 C++ 混合輸入比較包含 19 種命令、按下／放開、有效性、掛載與再裝填、ACK 延遲及排程補跑。
共同狀態軌跡 SHA256：`e0d373602afa4748c76088724a6dde39342bcf96b98fb568985c986308f0de99`。
29d5300 只再修改 Bridge 診斷容量、其測試及文件；MC 與 HMCS 程式和 db73b19 相同。

可攜 Exporter fixture 只在 /tmp 適配既有 MSVC 生成常數語法及 Windows 載入入口，
測試使用真實 Exporter 與專案 FakeCockpitParameters；專案 SDK／產生器未修改。
這些本機測試不代替指定 Windows 工具鏈或 DCS。

## Standards

完整重構：0 hard violations、0 heuristic findings。補充診斷修正與最終四檔產物差異：各 0 findings。
審查確認 System 邊界、資料寫入責任、固定參照、排程、單位與生成 ID 所有權。
初次完整凍結 patch SHA256：`f3194c6df8a2d5db10890a7464de848128a89f3124ed2a328f10e72d350f5d35`。

## Spec

完整重構、補充診斷修正及最終四檔產物差異：各 0 findings。
確認七個同層元件、FireControl 三個內部模組、原操作／畫面行為及完整 generation 讀取。
補充修正凍結 patch SHA256：`232ce3b1ac2446ad7fd3d24b37060ce8639854ec2cf7fffd161adf80d151a11d`。

最終產物凍結 patch SHA256：`6b552e2320e581166752e07974142778b1204de18bbffcfb5ae23d4f933af0fd`。

Standards：0 findings，無最嚴重項目；Spec：0 findings，無最嚴重項目。

## Windows 完整驗證與 DLL

[Windows CI 完整驗證](https://github.com/jjjjj103074/F-CK-1C/actions/runs/41025667883) 已通過，對應上述最終執行期來源與 tree。
全專案原生測試：**3,696 checks，0 failures**；DLL 與測試均完成 MSVC 靜態分析。

| 檢查 | 結果 |
|---|---|
| 提交版本 | passed |
| 編譯工具 | passed |
| DLL 編譯／分析 | passed |
| DLL 雜湊 | passed |
| DLL 接口 | passed |
| DLL 載入／卸載 | passed |
| 架構測試 | passed |
| 系統目錄產生器 | passed |
| DCS ID 產生器 | passed |
| 座艙架構 | passed |
| 座艙架構測試 | passed |
| 驗證工具 | passed |
| 分析規則測試 | passed |
| 測試編譯／分析 | passed |
| 原生測試 | passed |
| 來源完整性 | passed |

工具鏈為專案指定 v142：MSVC 19.29.30159.0、link 14.29.30159.0、SDK 10.0.26100.0，
MSBuild 18.10.1.42706。CI 首次建置的 DLL 經完整乾淨重建比對，雜湊相同。

- DLL SHA256：`9ba2ee88bbeaf763633d1044aca5d629850e4eb4b79369aa12c7dcf748dccb0e`。
- 已核對產物 ZIP SHA256：`9167f7332b8b46854de451f31321488dc13dd412b55c2d4cc31ccaf2a8017938`。
- 驗證證據 ZIP SHA256：`b2f324bd12bc3d447a80cedcdc27317944e251b0eccf33b562951e8588850b25`。
- 已將該 DLL 複製至 [bin/F-CK-1C_EFM.dll](../bin/F-CK-1C_EFM.dll)，
  憑證為 [bin/F-CK-1C_EFM.validation.json](../bin/F-CK-1C_EFM.validation.json)。

產物提交只更新 DLL、憑證及驗證文件；執行期來源保持上述已驗證版本。
來源提交與產物提交分開，避免將舊 DLL 誤稱為重構後已驗證產物。


## DCS 狀態

尚未執行 DCS 遊戲內測試。CI 驗證來源、工具鏈、DLL 與專案測試，
不代表實機載入、SDK 回傳或遊戲內發射／顯示已驗證。
人工步驟見 [MC 重構報告](MC_REFACTOR_REPORT.md#dcs-人工驗證)。
