# StoresManagement 掛載管理

SM 統整固定掛點、原生機砲及全域 IR 觀測，管理武器掛點選擇與武器操作執行，並發布唯讀資料及同步變更通知。
本元件是 MC 的掛載與數量權威；DCS 提供模擬世界的原始觀測。

## 分工與檔案

| 位置 | 功能 |
|---|---|
| `StoresManagement` | 接受原始指令，管理模式意圖、停止／取消／重新選站規則，安排觀測、執行與通知順序 |
| `Database/StoresCommandQueue` | 所有 SM 傳入指令的單一具型別入口，保存原始 FIFO、SM 指令編號及執行回覆 |
| `Internal/ObservationProcessing` | 接收觀測批次、維護掛載身分／數量／有效性、判斷過期 |
| `Internal/WeaponExecution` | 每條機砲／投放程序的開始確認、80 ms 保持、取消與正常結束 |
| `StoresManagementWeaponUse.cpp` | 同一 SM 類別的使用要求解讀、操作安排與結果寫回 |
| `Internal/StationSelection` | 依模式意圖與有效庫存決定掛點、保存要求與執行確認 |
| `Database/StoresData` | 保存逐掛點庫存、機砲彈量及選站狀態 |
| `Common/StoresQueries` | 提供唯讀飛彈候選查詢與數量加總 |
| `StoresEvents` | 發布庫存、機砲彈量及選站變化 |

這三個 Internal 檔案組是 SM 的內部實作。MC 頂層仍保有原本八個元件，DFLCC 維持原有架構。

## 指令入口與排程介面

呼叫端僅透過 `StoresCommandQueue::submit` 提交 SM 指令：

- `SelectStoresForMode`：處理模式選站與同模式循環。
- `BeginWeaponUse`：接受火控的機砲／投放開始意圖；LaunchPermission 明確表達火控的超控選擇。
- `EndWeaponUse`：依開始要求編號保存正常放開意圖。
- `AbortWeaponUse`：立即撤回先前啟動並安排必要停止。
- `StoresWeaponAction`：保留既有具型別執行操作入口；火控的使用程序透過上述三種要求協作。
- `CancelStoresCommands`：取消尚待執行的選站與武器啟動要求，保留停止操作。
- `CancelWeaponStarts`：撤回先前待執行的武器啟動，保留選站要求；SM 接受後立即處理撤回。

MC 的模式事件連線將選站意圖送到入口；FCC 武器操作只取得此入口、唯讀模式與使用執行資料。
入口回傳 SM 指令編號，並將該筆要求對應到 DCS 執行確認；完成回覆回收後仍可查詢結果。
SM 是入口的唯一消費者。佇列接收原始要求、檢查合法操作類型、通知及追蹤完成回覆；
SM 以 take_next 取得 FIFO 要求、resolve 登記完成或外部執行序號；佇列自行管理儲存與完成紀錄回收。
SM 接受後保存最新模式意圖及待執行流程，決定撤回、停止優先與重新選站。
外部要求的完成編號保留至原動作執行；SM 內部接續操作由同一流程管理。
觀測由唯讀 MissionInputs 提供，資料查詢使用 StoresData。

MC 在處理當次操縱命令前呼叫 `prepare` 更新觀測與自動掛點接續，
在模式事件與各次指令路由後派送就緒要求；Executive 的 `step` 推進 SM，
火控計時器更新後再由 `dispatch` 派送後續停止要求。
這三個方法是排程介面，操作命令統一由指令入口進入。MC 頂層與 DFLCC 的元件組裝模式維持原有安排。

SM 接受停止命令時，若仍有未完成選站，會撤回前面的待選站與啟動項目，先安排取消及停止，
再依最新模式重新確認掛點。重新確認保留仍有效的選擇；停止命令的 SM 指令編號固定，由入口追蹤其 DCS 執行確認。
這項處理涵蓋一般 GunOff／PickleOff，來源無效時仍可完成停止。
主保險使用 AbortWeaponUse，由 SM 撤回啟動並安排必要停止；NAV 使用完整選站取消規則。

入口在每筆原始要求入列後同步發布 commands_available，由 MC 組裝處喚醒 SM 接受要求與派送就緒動作。
呼叫端收到指令編號後再執行後續目標操作，維持停止先於解鎖的原有順序。
等待庫存的流程保留在 SM 的待執行紀錄。事件通知期間送入的要求也先由 SM 接受並安排規則，
後續派送階段完成執行。

## 全域 IR 觀測

prepare 將當前 MissionInputs.cockpit.ir_seeker 寫入 StoresData.ir_seeker，保留來源的可用性、版本、失效原因、
lock、要求角度與目標角度，再由 MC 發布至 StoresSnapshot.ir_seeker。
同一觀測與掛點庫存／選站各自保存，來源目前提供全域武器介面資料。
有效全零觀測表示來源回報未鎖定；不可用觀測以 availability 表達未知。
HMCS 與音效 Lua 只採用有效來源的 lock，顯示與音效規則由座艙處理。
變更清單見 [IR 觀測整理報告](../../../../../../../../docs/IR_OBSERVATION_REFACTOR_REPORT.zh-TW.md)。

## 武器使用執行

StoresData.weapon_use 分別保存 gun 與 release。每條程序以開始要求的 SM 編號識別，
執行階段為 Idle、StartPending、StartAcknowledged、StopScheduled 或 Canceled；rejected_request_id 保存最近拒絕的開始要求。
新的拒絕與原有已接受程序各自保留；新開始要求重設該種類的保持與放開意圖。

WeaponExecution 使用既有入口回覆追蹤開始確認，將 MC 首次觀察到該回覆的時間記為保持起點。
正常結束要求滿足 80 ms 後，由 SM 透過既有停止優先及重新選站流程派送停止，再寫入 StopScheduled。
StopScheduled 表示停止已排入共用動作佇列。Canceled 保存取消原因；撤回的開始也保留必要停止收尾。
火控只依正常投放結束接續目標操作，SAFE／SIM 中止保留原有指定意圖。

開始要求在轉接層不可用時記錄拒絕；恢復後由新的操作提出開始要求。
已接受的開始、重按、同週期短按及延遲回覆沿用原有 FIFO 與保持規則。

## 資料流

```text
DCS get_station_info (20 Hz) → weapon_system.lua → DcsBridge
                                                  ↓
DCS 原生機砲存量 (10 Hz) → GunAmmunitionSource → MissionInputs
                                                  ↓
                            SM (32 Hz) → StoresData → MC 快照
                                                  ↓
                             MissionComputerExporter → HMCS
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
2. 重按同一模式：輪換到下一個候選掛點，末站後回到第一站；同次更新的多次重按依 FIFO 分別處理。
3. 週期更新：保留仍可用的選擇；所選掛點耗盡後，改選候選清單第一站。
4. 重新掛彈或所選掛點更換掛載：建立新的明確選站要求。
5. NAV：清除邏輯選站。庫存有效且沒有候選時，選擇為空值。
6. 庫存無效或過期：保存最近要求與尚待處理的選擇意圖，待有效觀測更新後處理；NAV 可直接清除意圖。

```text
模式事件／FCC → StoresCommandQueue::submit → SM 指令編號
  → SM 依序處理就緒要求 → ExternalActionQueue 的 DCS 執行序號
  → CMS 執行前綴 → 首項選站交給 weapon_system.lua
  → 選站呼叫完成 → CMS 確認屏障 → 繼續後續投放／停止命令
```

`StationSelectionState` 分別保存 requested 與 acknowledged 掛點及要求版本，
另以 canceled_through_revision 保存已取消要求的最高版本。
`pending()` 回報選站是否等待有效庫存，或等待尚待處理且未取消要求的相符回覆。
取消結束該次等待，保留要求內容與真正收到的 ACK；新選站建立較高版本，等待自己的回覆。
回覆須符合已發出的要求版本與掛點。
SM 保存各次要求，因此較早要求的完成回覆可在後續要求已排入後正確記錄。
acknowledged 表示轉接層已完成該次呼叫；取消後抵達的真實回覆也會更新此紀錄。
NAV 的空值要求會清除 Lua 保存的最近要求紀錄。
明確 NAV 操作遇到尚未完成的要求時，建立新的清空版本，由相符回覆完成最新選站狀態。

每筆選站動作保存當次掛點與要求編號。Exporter 僅將執行視窗首項的選站資料發布到 Lua，
CMS 同時核對動作序號、選站要求與完成回覆。後續選站使用自己的資料，前面的操作保持先執行。
SM 等待有效庫存時，流程保留於 SM 的待執行紀錄；雷達／目標控制與反制動作可繼續完成。
已交給 DCS 的取消項目保留原傳輸序號並完成確認。
NAV 與取消指令會取消先前待執行的選站及啟動項目，停止操作保留。

FireControl 維持目標操作、武器資格、使用意圖與目標接續；SM 處理射擊／投放輸入保持與停止派送。
數量在 SM 收到 DCS 新觀測後更新，武器操作的完成通知只結束該程序。

## 架構依據

本專案由 MC 容納 SM，將掛載庫存與選站集中於 SM；FCC 使用唯讀資料，持有許可、目標與使用意圖；SM 持有掛載操作的執行時序。
公開資料支持這個功能分工；本專案的類別、排程與選站順序依自身需求設計。

| 來源與定位 | 架構依據 |
|---|---|
| [ASD-TR-91-5009／DTIC ADA255714](https://archive.org/details/DTIC_ADA255714)，印刷 p59、p62–63，§6.2.4.1.8 | F-16C/D SMS 軟體包含庫存、武器／掛點選擇及投放相關功能；p62 c(2) 明列 Stores Selection Component |
| [Lockheed Martin 16PR14341，1998-11-15](https://www.mycity-military.com/uploads2/154453_760102325_F-16A-B.pdf)，印刷 p1／PDF 11、p223／PDF 233 | F-16 MLU 的 MMC 整合 SMS CIU 等功能；武器選擇仍列為 SMS 功能 |
| 同一 MLU 手冊，印刷 p141–142／PDF 151–152 | 選中掛點、武器備便及健康狀態分別表達，對應本專案對選站 ACK 與備便的區分 |
| [F/A-18 NATOPS A1-F18AC-NFM-000，1997](https://airwar.ru/other/bibl/A1-F18AC-NFM-000%20%2815_01_1997%20%2B%20perf%20data%29.pdf)，I-2-70 §2.13.2 | 模式責任依機型而異；本專案模式仍由 ModeManagement 管理 |
| [MIL-STD-1760E，2007-10-24](https://quicksearch.dla.mil/WMX/Default.aspx?token=606932)，附錄 B 表 B-VIII，印刷 p101／PDF 121 | 數位介面區分收到的要求與儲載物實際狀態 |

32 Hz 排程、依固定順序循環掛點及執行確認後至少 80 ms 保持，均為本專案操作規則。

## 資料呈現與驗證

MC 快照保存 SM 的數量、有效性及武器操作狀態，HMCS 依完整性呈現彈量。
SM 選站狀態保存 requested、acknowledged、取消版本及 pending，供指令流程核對。
原生機砲來源透過既有 EventLog 記錄來源狀態變化與讀取錯誤。

原生機砲來源與版本契約見 [DCS API 指引](../../../../../../../../docs/DCS_API_GUIDE.zh-TW.md)。
需求與驗證見 [SM 整理規格](../../../../../../../../docs/SM_REFACTOR_SPEC.zh-TW.md)
與 [SM 整理驗證](../../../../../../../../docs/SM_REFACTOR_VALIDATION.zh-TW.md)。
