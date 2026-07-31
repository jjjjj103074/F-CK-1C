# Cockpit Phase 0 Baseline

## 結論

這個目錄是駕駛艙遷移的「施工前存證」。Phase 1 之後每次搬動系統，都必須拿新結果與這裡比對。

Baseline 分成兩層，不能混在一起：

1. **來源意圖（Source Contract）**：目前 Lua 原始碼實際寫了哪些狀態、門檻、計時器與 DCS 動作。這是遷移時要還原的功能。
2. **實機觀察（Runtime Evidence）**：目前版本放進 DCS 後真正有執行什麼。這會如實保留載入失敗與缺陷，不把錯誤包裝成正確功能。

例如 `autopilot_system.lua` 有完整控制邏輯，但 2026-07-28 的 DCS log 證明它因為超過 Lua upvalue 限制而沒有載入。因此：

- C++ 遷移要對照「來源意圖」重建 Autopilot 功能。
- 回歸報告要同時註明舊版實機其實沒有成功執行。
- 不要求新 C++ 繼續重現「載入失敗」這個錯誤。

## Phase 0 交付物

| 交付物 | 狀態 | 用途 |
|---|---:|---|
| [來源與介面清冊](generated/source-manifest.csv) | 完成 | 固定目前 21 個 Cockpit 檔案及 15 個飛機定義／輸入／C++ 邊界檔案的 hash |
| [DCS device 載入鏈](generated/device-load-chain.csv) | 完成 | 固定目前 6 個 device 與 3 個 indicator 的 class、script、順序 |
| [自訂 command 路由](generated/command-routing.csv) | 完成 | 固定 command ID、EFM／Cockpit route、Input 綁定與 Lua 引用 |
| [DCS action 使用清冊](generated/dcs-action-usage.csv) | 完成 | 固定由 DCS 擁有、C++ Router 忽略的 action |
| [parameter 讀寫清冊](generated/parameter-access.csv) | 完成 | 列出 Lua／C++ 的 reader、writer 與 presentation reader |
| [逐系統行為真值表](BEHAVIOR_BASELINE.md) | 完成 | 固定目前來源中的狀態轉移、門檻、計時與已知問題 |
| [DCS 驗收規範](DCS_TEST_PROTOCOL.md) | 完成 | 固定 mission、loadout、操作、證據與判定方式 |
| [2026-07-28 Air Clean 實機證據](evidence/2026-07-28-air-clean/RUN.md) | 已存證 | 保存一輪真實 DCS 執行結果 |
| [Phase 2 被動 device 實機證據](evidence/2026-07-31-phase2-passive-devices/) | 完成 | 包含 AIR-CLEAN、COLD-CLEAN 與 RWY-CLEAN；鼻輪外觀問題另列 Issue #21 |
| [Phase 3 AP／A/T 首次實機證據](evidence/2026-07-31-phase3-autopilot-fail/) | `FAIL`，已修正待複測 | command／mode／diagnostics 大致通過；ALT Hold 失穩已由 trace 固定並完成最小修正，尚未取得新 DLL 的 DCS PASS 證據 |
| Air AIM-9 實機證據 | 尚未執行 | 尚無帶武器 mission，不可用空載 mission 假裝通過 |

「尚未執行」不是遺漏的假成功。測試方法與通過條件已經固定，但必須真的進 DCS 操作後才可改成完成。

## 自動清冊怎麼使用

只檢查，不改檔案：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/capture_cockpit_baseline.ps1 -Mode Check
```

或：

```powershell
npm run test:cockpit-baseline
```

來源經過審查、確定 baseline 應該跟著改變後，才更新清冊：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/capture_cockpit_baseline.ps1 -Mode Refresh
```

工具有意採取明確失敗：

- Cockpit 檔案不是 21 個時失敗。
- 6 個 device 或 3 個 indicator 缺少時失敗。
- 必要的輸入／C++ 邊界檔案缺少時失敗。
- 產物不存在或與來源不同時失敗。
- 不會自行接受漂移，也不會產生假資料讓檢查通過。

## 清冊欄位怎麼看

### `source-manifest.csv`

- `Path`：repository-relative 路徑。
- `Bytes`、`Lines`：快速確認結構變動。
- `SHA256`：確認內容是否完全相同。

### `device-load-chain.csv`

- `Order`：`device_init.lua` 中的宣告順序。
- `Kind`：DCS device 或 indicator。
- `Device`：`devices.lua` 的穩定名稱。
- `Class`：例如 `avSimpleRadar`、`avLuaDevice`。
- `Script`：DCS 實際載入的入口。

### `command-routing.csv`

- `Route=efm`：控制器 command 直接交給 EFM Router。
- `Route=cockpit`：目前由 DCS cockpit device 接收。
- `CockpitTarget`：Input profile 指定的 device。
- `InputReferences`、`CockpitLuaReferences`：`檔案:出現次數`，空白代表目前沒有引用。

### `parameter-access.csv`

- `Read`／`Write`／`ReadWrite`：程式碼透過 handle 讀寫。
- `PresentationRead`：indicator page 只用 parameter 呈現，不擁有狀態。
- `Layer`：Lua 或 C++。
- 動態 heading slot 的 writer 與 presentation reader 都以 `HMCS_HDG_SLOT_TICK_*`、`HMCS_HDG_SLOT_LABEL_*` 表示。

## 清冊直接揭露的現況

Phase 3 程式遷移後，目前自動清冊共有：

- 36 個來源／邊界檔案：21 個 Cockpit 檔案加 15 個飛機定義／Input／C++ contract 檔案。
- 63 個自訂 command：43 個 route 到 EFM，20 個 route 到 Cockpit。
- 74 個不同 parameter 名稱，133 筆 reader／writer／presentation 關係。
- 26 個由 DCS 擁有、EFM Router 明確忽略的 DCS command。

Phase 0 曾記錄下列多 writer；Phase 1 已整理成唯一 writer：

| Parameter | Writer |
|---|---|
| `AIM9_MISSILE_COUNT` | CMS |
| `AIM9_MISSILE_STATUS` | CMS |
| `AIM9_TONE_STATE` | Radar State |
| `AIM9_WEAPON_ACTIVE` | Radar State |
| `HMCS_DOGFIGHT_MODE` | CMS |
| `HMCS_MASTER_MODE` | CMS |

Phase 0 原始結果仍保存在建立 baseline 的 Git commit；目前 generated
清冊則跟隨已審查的介面變更，供自動檢查重跑。

另外 `GearAuto`、`NoseTurnAuto` 已列在 command catalog 且 route 到 EFM，但目前兩份 Input profile 都沒有引用。這些是 baseline 揭露的既有結構問題；Phase 0 不直接修改它們。

Phase 2 已移除 Gear／Actuators 的 Lua creators 與三個被動 Lua 檔；ID 1、
2 仍保留且不得重用。全部模型 draw arguments 由 EFM C++ 單一路徑輸出。

Phase 3 已移除 AUTOPILOT Lua creator 與 `autopilot_system.lua`；ID 9
仍保留且不得重用。AP／A/T command、狀態、控制器與 test-thrust intent
都由 C++ 擁有；Lua 只透過 generated parameter 讀取呈現資料。Phase 3
首次實機證據為 ALT Hold `FAIL`；修正已安裝但尚待 DCS 複測，因此此時
只能宣稱程式與自動驗證完成。

## Baseline 變更規則

Phase 1 之後，任何清冊漂移都要先回答三件事：

1. 是單純搬家，還是有意改變功能？
2. 原本唯一權威是誰，新唯一權威是誰？
3. 對應哪一個自動測試或 DCS 驗收紀錄？

如果只是遷移：

- 使用相同輸入，狀態轉移與輸出必須等價。
- 更新 writer／reader 位置，但不能在 Lua 與 C++ 同時留下兩個權威。
- 已知的載入錯誤可以被修正，但要在差異報告中明說。

如果同時改功能，必須拆成另一個變更。這樣才能知道差異來自「搬家」還是「規則改了」。

## Phase 0 完成定義

Repository 內的 Phase 0 在下列條件下視為完成：

- 自動清冊可重跑且 `Check` 通過。
- [逐系統行為真值表](BEHAVIOR_BASELINE.md) 覆蓋目前所有 Cockpit 子系統。
- [DCS 驗收規範](DCS_TEST_PROTOCOL.md) 定義 Air Clean、Cold Clean、Air AIM-9 的輸入與證據。
- 至少一份真實 runtime evidence 已保存，且錯誤沒有被隱藏。
- 尚未執行的 DCS 情境保持 `NOT_RUN`，不得宣稱通過。

開始遷移某一個特定系統前，該系統相關的 DCS 情境還必須先執行完成；否則只可進行不改行為的介面準備，不能刪除舊實作。
