# MC 七元件重構報告與驗收規格

審查基準固定為 `d7dcccbad59f0dd36dbacb640e9b1a66f39ce3f9`。
範圍包含基準之後全部未提交變更：先前火控職責重構，以及本次紅外線支援／顯示責任調整。
本文件記錄使用者在討論中確認的需求，作為 Spec 審查來源；本次已授權 commit、push 與 CI 驗證。

## 驗收準則

1. 保留原功能；不新增主模式、發射門檻、火控解算、武器導引、電源或備援功能。
2. 保留 ModeManagement，以及 NAV/DGFT/MSL 與 Master Arm 原連動。
3. WeaponControl、SensorCoordination 收入 FireControl；目標與武器操作分開，對外入口統一。
4. 武器切換、掛載、選站及彈量由 StoresManagement 管理，雷達模型與 DCS 呼叫留在外部。
5. Aim9SeekerProcessing 移入 FireControl.Internal.InfraredMissileSupport；採紅外線飛彈共用邊界，
   目前演算法完整保留。武器能力資料及特殊操作只記錄為待辦，不宣稱已支援所有紅外線飛彈。
6. 完整撤掉同層 DisplayManagement。任務模式歸 ModeManagement、掛載數量歸 StoresManagement、
   武器／尋標狀態歸 FireControl；顯示偏好、類別與數量選擇歸 HUD/HMCS。
7. 輸入處理保留必要按鍵狀態與最近按下序號，包含被拒絕操作；經 MC 快照及 Bridge 交給 HMCS。
   保留短按、同週期 FIFO、原始扳機顯示與原顯示優先順序；這些觀測不成為發射或選站權威。
8. 保留既有 HMCS_WEAPON_CLASS／HMCS_WEAPON_QTY 名稱及數值，writer 改為 hmcs_system.lua。
   Bridge 停止寫入這兩項；HMCS 使用 MC 彈量，不新增獨立估算。
9. 保留奇／偶 generation，HMCS 只呈現同一份完整版本；不可用／未完成讀取不覆蓋既有呈現。
10. 保留 19 個命令入口、32 Hz、80 ms 保持、1 s 自動捕獲、動作 FIFO／視窗／ACK 與補跑語意。
    C++ 內部快照可調整；DCS 命令及既有座艙參數保持相容，新增資料經原 ID 產生器生成。
11. 與 DFLCC 對齊單一 System、資料庫先建立、固定參照及單一 Executive；不修改 DFLCC。
12. 使用指定 code-review 技能審查所有未提交／新增檔案，修正後 commit／push，執行必要驗證。
    沿用已同意的 Linux → 來源提交 → Windows CI → 取回已核對 DLL → 產物提交順序。

## 完成的架構

```text
MissionComputer [32 Hz]
├── ModeManagement
├── StoresManagement
├── FireControl
│   └── Internal/
│       ├── Targeting
│       ├── WeaponOperation
│       └── InfraredMissileSupport
├── Executive
├── StartupAndRestart
├── SystemMonitor
└── FailureManager
```

七個同層元件。MC 直接持有 ExecutiveTables、InputOutputData、SystemStatusTables、
ModeData、StoresData、FireControlData 與 ExternalActionQueue 七個具型別記憶體物件。
FireControlData 包含 targeting、weapon、infrared_missile 三個分區，各由對應內部模組寫入。
FireControl 不可複製或搬移，確保內部參照有效。

週期工作收斂為四項：StoresManagement → FireControl → SystemMonitor → FailureManager。
FireControl 內保持 WeaponOperation → Targeting → InfraredMissileSupport 的順序。
ModeManagement 維持事件入口；四個基礎元件的現有／預留行為保持原樣。

## 資料與指令流

```text
DCS 觀測／駕駛操作 → Bridge／Pipeline → MC 輸入及命令 FIFO
  ├─ ModeManagement → ModeData
  ├─ StoresManagement → StoresData
  ├─ FireControl → FireControlData
  └─ 原始操作狀態／最後按下序號 → InputOutputData.input.control_inputs

上述資料 → CombatAvionicsSnapshot → AircraftData／座艙快照
  → MissionComputerExporter → 同一 generation 的座艙參數
  → hmcs_system.lua 判斷類別／數量 → 原 HMCS 繪圖頁

核准動作 → ExternalActionQueue → Bridge → mc_io.lua／cms_system.lua → DCS → ACK
選站要求 → weapon_system.lua → DCS SDK → 選站確認 → MC
```

Core 移除 WeaponDisplayData、WeaponDisplaySnapshot 及顯示專用 WeaponClass。
對外 C++ 快照中的 aim9 改為 infrared_missile，數量欄位改名 air_to_air_missile_count，
如實表示既有廣泛 AAM 分類；DCS 邊界仍映射到既有 AIM9_* 參數及 enum 數值。
新參數包括 MC 整理後機砲數量、原始扳機狀態與六項操作的最後按下序號。
CommandIds.json 同步改正兩個 HMCS 顯示參數的 direction／writer／owner，再執行原產生器。

輸入序號依每筆 MC 命令遞增，與外部動作序號分開，受 double 精確整數範圍限制。
HMCS 比較相關最後按下序號，重建原先的顯示偏好；不需要重播所有事件或確認顯示動作。
因此同週期按下又放開、HMCS 漏讀中間幀，以及被拒絕的操作都能保留原顯示結果。
HMCS 仍為 64 Hz，MC 為 32 Hz；這是讀取資料快照，未新增同步 RPC 或另一套 HUD。

## 保留的行為與實作限制

- Master OFF/SIM 的停火 → CMS 中止 → 模式／指定重設順序保留。
- 雷達開啟 → 武器循環 → 子模式要求順序保留；NAV 不新增強制停火規則。
- 直接 IR 觀測、IR／雷達推導及有效全零回退的優先序保持。
- designation 是操作意圖，readiness 是簡化命令狀態，均不能當作實測鎖定。
- lock_range_m 仍為雷達 STT 距離；沒有新增紅外線測距或導引模型。
- 掛載仍按既有廣泛 AAM 分類，尚未精確識別紅外線武器能力；通用名稱不代表新增型號支援。
- CMS 程序仍在 Lua，MC 管理其啟動資格與中止要求。
- DFLCC、輸入綁定、DCS 雷達模型與音效資源未改。

## 本機驗證

| 項目 | 結果 |
|---|---|
| MC C++17 範圍回歸 | 172 checks，0 failures |
| Lua 5.1 實際介面與輸入設定 | 6,169 checks，0 failures |
| 顯示規則參考重播 | 8,192 輸入幀，跳過中間 HMCS 更新；所有讀取時的類別與數量一致 |
| 實際重構前後 MC→HMCS 回歸 | 4,096 幀，任務／掛載／尋標／動作與最終顯示類別／數量一致 |
| Exporter 暫存可攜 fixture | 119 checks，0 failures；不代替 Windows 驗證 |
| DCS ID 產生器及 fixtures | passed |
| Node.js 22 工具回歸 | 84 passed，0 failed；5 項 Windows／環境限定測試跳過 |
| 固定 clang-format 22.1.8 | 272 檔，0 需排版 |
| Exporter、完整架構規則及 MSVC 驗證 | 由後續 Windows CI 執行 |
| DCS 遊戲內 | 未執行 |

Linux 的 MC 範圍測試使用 g++ C++17、Wall/Wextra/Werror/pedantic；
只停用基準已有的 missing-field-initializers 警告。Lua 5.1.5 來源 SHA256 與既有 CI 相同。
本機 PowerShell 7 可執行原 ID 產生器；部分既有 Windows 專用架構規則在 Linux 的路徑處理不相容，
完整架構／Exporter／DLL／接口／載入／分析與全原生測試以固定 Windows 工具鏈結果為準。
這些本機檢查不代表 DCS API 或遊戲內武器操作已實測。

## Standards

No documented-standard violations or actionable baseline smells found in the complete frozen patch.

The changes preserve the documented System boundaries, typed database ownership, fixed references,
scheduling order, unit suffixes and generated-ID ownership. HMCS presentation uses committed MC
observations and stores quantities; Bridge no longer writes the two presentation parameters.

FireControl’s delegation is consistent with the documented unified fire-control entry, so it is not
reported as a Middle Man smell. The retained prototype algorithm and future-capability comments
also follow the explicitly limited scope.

Tooling-enforced formatting and pending Windows CI were excluded as instructed. Review was read-only.
Standards: **0 hard violations; 0 heuristic findings.**

## Spec

Spec：0 項發現。已依本文件驗收準則 1–12，審查完整凍結差異。

- 未發現缺漏／部分實作、範圍擴張或實作錯誤。
- 確認七個同層元件及 FireControl 三個內部模組；搬移後原演算法、模式連動、動作順序、計時與掛載數量行為保留。
- 19 個命令入口、既有命令 ID、座艙參數名稱及數值均保持相容；DFLCC 未修改。
- 原始扳機、最近按下序號與 HMCS 顯示優先順序相符；Bridge 不再寫顯示類別／數量，HMCS 使用 MC 彈量並拒絕不完整 generation。

這是獨立唯讀規格審查。Windows CI 與產物提交依已核准順序後續執行，不列為本次程式缺陷。

兩項審查皆針對完整凍結差異（含新增檔案），SHA256：
`f3194c6df8a2d5db10890a7464de848128a89f3124ed2a328f10e72d350f5d35`。
審查後只在本文件補上驗證與審查結果，程式內容未變。

Standards：0 findings，無最嚴重項目；Spec：0 findings，無最嚴重項目。

## 補充：輸出診斷容量

來源提交後的額外邊界檢查發現：當 generation 可寫、其他輸出欄位大量缺失時，
新增的 MC 資料使一次匯出的錯誤／恢復回報上限達 67 筆，超過原有 64 筆容量。
原先可重現 `Cockpit parameter event capacity exceeded`；容量調整為 128，
並加入全部缺失、重複錯誤抑制、完整恢復及 generation 提交的回歸測試。
此修正維持既有失敗／恢復回報與完整快照要求，不新增任務功能。
補充三檔差異已另行獨立審查：Standards 0 findings、Spec 0 findings，無最嚴重項目。
凍結補充 patch SHA256 為 `232ce3b1ac2446ad7fd3d24b37060ce8639854ec2cf7fffd161adf80d151a11d`。
可攜 fixture 只在 /tmp 對既有 MSVC 特定生成常數語法與 Windows 載入入口做適配，
專案生成器及 SDK 未修改；修正後通過 119 checks，並重新執行固定 Windows CI。

## Windows CI 與產物

來源提交後，由 mission-computer 分支既有 MC Windows workflow 重建 DLL 並完整驗證。
取回產物時核對 source_commit、source_tree、run_id、validation 與 DLL SHA256，
再更新 bin/F-CK-1C_EFM.dll 及 bin/F-CK-1C_EFM.validation.json，另作產物提交。
結果與可下載來源將在任務完成簡報及驗證憑證中記錄。

## DCS 人工驗證

使用配對的來源與 DLL，檢查：NAV/DGFT/MSL 切換；Master ON/OFF/SIM；
扳機與短按投放；按住操作時切 Master OFF；NAV 短按解籠後的 AAM 顯示；
快速 NAV／DGFT／MSL 與解籠組合；AAM/GUN 數量、零彈、再裝填及換站；
尋標／鎖定／音調；HMCS 2D/VR；反制啟動／中止與重新進入任務。
預期功能與原版一致；新增主模式、解算與武器能力留待後續元件內部整理。
