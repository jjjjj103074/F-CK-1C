# DCSBridge 重構實作歷史

本文件保存 [DCSBridge 重構設計](DCSBRIDGE_REFACTOR_DESIGN.md) 已完成步驟的簡要紀錄。現行行為、資料契約與未完成驗收以設計文件為準；詳細程式差異以 git history 為準。

## 第一輪重構（步驟 0–16）

第一輪原則上每步一個 commit，每次執行 native tests，DLL 由 `tools/build_dll.ps1` 建置，並維持 DCS export surface。

| 步驟 | 已完成結果 |
|---:|---|
| 0 | 建立重構前 tests、DLL、exports 與 SHA256 基準 |
| 1 | EFM 與 tests 明確啟用 C++17 |
| 2 | 建立 `StartMode`、`FrameInput`、availability 與 `FrameOutput` contracts |
| 3 | 把 autopilot／max-power Cockpit 讀取移到 DCSBridge |
| 4 | 移除 Core 反向通知、舊 Debug Watch implementation 與 suspension diagnostics |
| 5 | 移除 `Fck1cEfmRuntime` 與 Core 對 DCSBridge 的依賴 |
| 6 | 建立 thread-safe `FrameInputCollector` |
| 7 | Core production 入口切換為 `start()`／`step()` |
| 8 | 建立 `OutputStore`，集中 frame output callbacks |
| 9 | 建立 process-lifetime、可即時讀取的 `EventLog` |
| 10 | 建立 latest-only、非阻塞 simulate 的 `StateCsvWriter` |
| 11 | 拆出精簡 `CockpitBridge`、`CarrierBridge` 與路徑工具 |
| 12 | 建立唯一 production ownership/composition root `BridgeContext` |
| 13 | 建立 command binding、`std::optional<double>` param lookup 與 numeric boundary errors |
| 14 | 刪除 legacy ownership、runtime 與 snapshot 路徑 |
| 15 | 整理 DCSBridge/Internal、EfmExports 入口與 contributor 文件 |
| 16 | 完成自動檢查與第一輪 DCS 多 flight 實機驗證 |

## 實機驗證後修正（步驟 17–20）

步驟 17–20 均完成相關 native tests、完整 native suite、`tools/build_dll.ps1`、code review 與 commit；DCS 驗證集中在步驟 21。

### 17. 固定規格與 Command 分類資料

- `CommandIds.json` 成為 custom command route 與 known ignored DCS command 的單一資料來源。
- Generator 驗證 route、ID、重複值與 reason，並可重複產生相同結果。
- 同步提交 Lua、generated files、說明文件與重建 DLL。

### 18. EventLog counted warnings 與 Command routing

- EventLog 以動態「warning kind + ID」計數，第一次即時輸出，release 時輸出總數並清除。
- Generator 把 Cockpit route 與 ignored IDs 寫入既有 C++ generated header；runtime 不解析 JSON，也不維護第二份表。
- EFM route 正常 dispatch；Cockpit route 與已申報 DCS-owned IDs 靜默 no-op；unknown ID no-op 並 counted WARN。

### 19. Param export mappings 與分類

- 補齊已申報 compatibility mappings；已實作 mapping 繼續讀 latest `FrameOutput`。
- Start 後第一份 atmosphere／suspension sample 前使用 per-param compatibility value；known compatibility 不寫 log，unknown index 回 `0.0` 並 counted WARN。
- 第一次步驟 21 實測發現測試只重複 production symbols，沒有鎖定實機 raw numeric IDs。後續修正 `2015`／`2025` wheel yaw、`2123` pitch force center，以及 `2132` 高度表壓力 `760.0 mmHg` compatibility output，並增加獨立 raw-ID regression tests。

### 20. Release 與 next-flight preparation

- Release 結束當前 flight 並使 OutputStore invalid，但保留 process-lifetime `BridgeContext`、Core、logger 與 CSV writer。
- Operational/output callbacks 在 release 後維持 lifecycle ERROR；fuel、gameplay 與 mass-delta preparation 可準備下一 flight。
- 第一次步驟 21 實測確認 DCS 也會在 start 前傳入 continuous inputs。後續修正為 release 清掉上一 flight collector、release 後保留下一 flight samples、start 不 reset，且 continuous input 不被 Core execution mutex 阻塞。
