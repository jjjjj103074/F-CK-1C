# StoresManagement 掛載管理

SM 統整固定掛點與原生機砲觀測，管理武器掛點選擇，並發布唯讀資料及同步變更通知。
本元件是 MC 的掛載與數量權威；DCS 提供模擬世界的原始觀測。

## 分工與檔案

| 位置 | 功能 |
|---|---|
| `StoresManagement` | 安排觀測、選站與通知的更新順序 |
| `Internal/ObservationProcessing` | 接收觀測批次、維護掛載身分／數量／有效性、判斷過期 |
| `Internal/StationSelection` | 依模式意圖與有效庫存決定掛點、保存要求與執行確認 |
| `Database/StoresData` | 保存逐掛點庫存、機砲彈量及選站狀態 |
| `Common/StoresQueries` | 提供唯讀飛彈候選查詢與數量加總 |
| `StoresEvents` | 發布庫存、機砲彈量及選站變化 |

這兩個 Internal 檔案組是 SM 的內部實作。MC 頂層仍保有原本八個元件，DFLCC 維持原有架構。

## 對外介面

- 建構時綁定唯讀 `MissionInputs`、可寫 `StoresData` 與本台 MC 的 `StoresEvents`。
- `on_operating_mode_selected` 保存模式與循環意圖，於週期更新時使用最新庫存處理。
- `step` 依序更新庫存、機砲彈量與選站；全部資料寫入後，再依相同順序發布變更通知。

ModeManagement 寫入 ModeData 並發出事件；MC 組裝處將事件分別連接到 SM 與 FCC。
FCC 讀取 StoresData，使用共用查詢取得已知有效的空對空飛彈數量。

## 資料流

```text
DCS get_station_info (20 Hz) → weapon_system.lua → DcsBridge
                                                  ↓
DCS 原生機砲存量 (10 Hz) → GunAmmunitionSource → MissionInputs
                                                  ↓
                            SM (32 Hz) → StoresData → MC 快照
                                                  ↓
                             MissionComputerExporter → HMCS / debug.csv
```

九掛點定義共用 `Cockpit/Scripts/Stores/StationDefinitions.lua`，座艙索引 0–8 依序為
RT、RO、RI、MM、MF、MB、LI、LO、LT。

Bridge 轉譯 DCS 類型、完整 CLSID、數量與配置。SM 保存所有固定掛點；
飛彈查詢使用分類、用途或識別碼條件，掛載識別以來源內容為準。
空對地飛彈、炸彈、油箱與其他掛載也保留在逐掛點庫存。

## 庫存與有效性

每站分別保存配置、數量、識別、掛載版本與最後有效觀測時間。
配置分為 Unknown、Empty、Clean、Equipped；有掛載而數量為零時，保存 Equipped 與其身分。
新掛載、身分替換及重新掛彈更新掛載版本；同次掛載的數量變化保留版本。

新來源批次更新資料與時間；重複批次沿用已接受的內容與時間。
掛點與機砲共用 0.5 秒新鮮度門檻，超時保存最後讀值並標為 Stale；
收到新有效批次後更新資料及有效性。來源錯誤保留其原因，與有效空載分別表達。

`find_missile_stations` 回傳符合條件且有有效彈量的掛點。
`count_missiles` 回傳已知數量及完整性；完整性同時考慮各站有效性及回答查詢所需的分類。
MC 的空對空顯示數量由同一庫存投影，HMCS 依完整性顯示數字或 `AAM ---`。
原生機砲彈量保存在 `GunAmmunitionState`，HMCS 依有效性顯示數字或 `GUN ---`。

## 選站規則與指令流

1. 進入或切換 DGFT／MSL：依固定掛點順序選第一個有效、有彈的空對空飛彈掛點。
2. 重按同一模式：輪換到下一個候選掛點，末站後回到第一站；同次更新的多次重按累計處理。
3. 週期更新：保留仍可用的選擇；所選掛點耗盡後，改選候選清單第一站。
4. 重新掛彈或所選掛點更換掛載：建立新的明確選站要求。
5. NAV：清除邏輯選站。庫存有效且沒有候選時，選擇為空值。
6. 庫存無效或過期：保存最近要求與尚待處理的選擇意圖，待有效觀測更新後處理；NAV 可直接清除意圖。

```text
模式事件 → SM 保存意圖 → 更新觀測 → StationSelection 決定掛點
  → StoresData.selection → MC 快照 → Lua select_station
  → 要求編號與掛點的執行回覆 → SM 更新 acknowledged 狀態
```

`StationSelectionState` 分別保存 requested 與 acknowledged 掛點及要求版本，
`pending()` 回報最新要求是否仍等待相符回覆。回覆須同時符合要求版本與掛點。
acknowledged 表示轉接層已完成該次呼叫；這個欄位記錄指令完成狀態。
NAV 的空值要求會清除 Lua 保存的最近要求紀錄。

FireControl 維持目標操作、武器資格、射擊／投放計時與解鎖流程。
數量在 SM 收到 DCS 新觀測後更新，武器操作的完成通知只結束該程序。

## 診斷與驗證

`Diagnostics/StoresTelemetry` 將逐站輸入、庫存、完整性、資料年齡及彈量送到既有畫面與 debug.csv。
選站另提供 requested、acknowledged 及 pending 通道，便於核對指令流。
Lua `Stores/ObservationDiagnostics` 保存變更時的原始掛點欄位及重新掛彈事件。

原生機砲來源與版本契約見 [DCS API 指引](../../../../../../../../docs/DCS_API_GUIDE.zh-TW.md)。
本輪規格及驗證見 [SM 整理規格](../../../../../../../../docs/SM_REFACTOR_SPEC.zh-TW.md)
與 [SM 整理驗證](../../../../../../../../docs/SM_REFACTOR_VALIDATION.zh-TW.md)。
