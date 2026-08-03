# 專用 Debug Indicator 與 `debug.csv` 診斷工具計畫

## 0. 文件狀態

- 狀態：通用診斷通道、`debug.csv` 與專用 Debug Indicator 已完成；2026-08-03 DCS 驗證確認兩欄六區塊與全部現有資料均正常顯示。
- 範圍：規劃通用除錯資料通道、專用 Debug Indicator、`debug.csv`，以及既有 `fck1c_state.csv` 的責任整理。
- 本文件不改變飛控、自動駕駛、推進或飛行物理行為。
- 本文件沒有尚待決定的架構問題；DCS 已驗證單一 `ceStringPoly` 可穩定顯示 10 行，但 15 行與完整 59 行字串不顯示，因此正式版採用六個獨立的 10 行文字區塊。

## 1. 已確認的設計原則

### 1.1 Debug Telemetry 是通用工具

Debug Telemetry 不擁有任何飛控或 AP 語意，也不限定開發者只能觀察某一種狀態。

- Core 開發者可在需要的位置主動推送資料。
- 在 commit 前推送，顯示的就是 commit 前資料。
- 在 commit 後推送，顯示的就是 commit 後資料。
- 在一段運算中間推送，顯示的就是該中間狀態。
- 工具不替開發者重新解釋、延後或改寫資料時點。
- 工具只負責接收、保存最新值、顯示最新值與定期留下歷史快照。

因此，Debug Telemetry 不承諾「只顯示完整 FrameOutput」或「只顯示同一批 commit 後的系統狀態」。資料的意義由宣告名稱、單位、說明及實際推送位置共同決定。

### 1.2 同一份資料，兩種觀看方式

```text
Core 開發程式碼
    |
    | declare channel + publish value（任意時點、任意頻率）
    v
DebugTelemetryHub（DcsBridge 擁有）
    |
    +--> Latest Snapshot --> 專用 Debug Indicator（畫面上的最新值）
    |
    +--> Timed Samples ----> debug.csv（固定頻率的歷史值）
```

專用 Debug Indicator 與 `debug.csv` 是同一個工具的兩個輸出端，不建立兩套資料定義：

- Indicator 回答「現在最新收到什麼」。
- CSV 回答「每個固定模擬時間點，當時最新收到什麼」。

### 1.3 診斷資料不參與模擬決策

診斷通道是單向觀察點：

- 不回寫 AircraftData、System state、FrameInput 或 FrameOutput。
- 不作為飛控、AP 或其他系統的輸入。
- 不改變 SystemPipeline 的排程、同時到期批次或 commit 規則。
- 診斷工具故障不得產生新的控制命令或改寫飛機狀態。

這條限制只是在保護模擬權威性，不限制開發者可以推送什麼內容。

## 2. 目前問題

目前診斷責任混在三個位置：

1. `fck1c_state.csv` 同時保存飛機可觀察狀態與大量 FLCC／AFCS 內部狀態。
2. 現行 DCS runtime 會呼叫 `ed_fm_enable_debug_info()`，但不會呼叫 `ed_fm_debug_watch()`，因此不能依賴原生 Watch 顯示診斷文字。
3. 若每次調查都直接新增 FrameOutput 與 StateCsvWriter 欄位，會讓穩定的跨層契約和飛機狀態 CSV 持續膨脹。

這會造成兩個架構問題：

- 為了短期觀察某個內部值，必須穿透 Core、FrameOutput、DcsBridge 與 CSV 多層介面。
- 診斷需求改變時，與正式飛機輸出契約一起改動，局部性差且容易留下過期欄位。

## 3. 目標與非目標

### 3.1 目標

- 提供一個 DCS-neutral 的 Core 診斷發布介面。
- Core 可宣告命名資料通道，並在任意程式位置、任意頻率推送數值。
- 支援浮點數、整數、布林值與文字。
- 專用 Debug Indicator 顯示每個通道的最新值。
- `debug.csv` 以固定模擬時間頻率保存相同通道的最新值歷史。
- 新增或移除 Core 診斷通道時，不必修改 Indicator Lua 或 CSV 的通用實作。
- 讓 `fck1c_state.csv` 回到描述飛機可觀察狀態的主要責任。
- 所有遺失、重複宣告、座艙參數與檔案寫入錯誤都明確可見，不靜默假裝成功。

### 3.2 非目標

- 不把 Debug Telemetry 做成正式的 System 間通訊匯流排。
- 不用它替代 AircraftData、FrameOutput、CockpitSnapshot 或 EventLog。
- 不保證記錄每一次推送；`debug.csv` 是固定頻率快照，不是無損事件追蹤。
- 不重用或修改正式的 `ControlsIndicator/`。
- 不讓專用 Debug Indicator 成為飛機 System、座艙裝置或模擬資料來源。
- 第一版不做分頁、顯示層級、捲動、滑鼠操作或獨立設定畫面。
- 不在本次實作 debug geometry 或受力向量顯示。
- 不在這次修改 FLCC、AP、鍵盤輸入速率或搖桿釋放體感。
- 不為尚未出現的網路串流、外部 GUI 或遠端診斷預先建立框架。

## 4. Module 與 Seam

本設計採用一個小型 Interface，將工具實作留在 DcsBridge，形成一個窄 Seam。

### 4.1 Core Contracts：穩定的小介面

Core Contracts 定義 DCS-neutral 的資料型別與發布能力，建議放在：

```text
Core/Contracts/Diagnostics/
    DebugTelemetry.h
    DebugTelemetryTypes.h
```

介面責任：

- 宣告通道。
- 取得不可偽造、帶型別的 channel handle。
- 在指定模擬時間發布新值。
- 明確回報宣告或發布錯誤。

介面不負責：

- CSV 格式。
- DCS callback。
- 字串排版。
- 檔案路徑與檔案輪替。
- Indicator 版面、可見狀態與 DCS 座艙參數。
- 決定開發者應在哪個演算法位置推送。

### 4.2 DcsBridge：工具實作與 DCS Adapter

DcsBridge `Internal/` 新增下列私有 Module：

```text
DcsBridge/Internal/DebugTelemetry/
    DebugTelemetryHub.*
    DebugCsvWriter.*
    DebugIndicatorExporter.*
```

- `DebugTelemetryHub` 實作 Core 診斷介面，保存通道目錄、按時間排序的更新與最新值快照。
- `DebugCsvWriter` 依固定模擬時間取樣 Hub，產生 `debug.csv`。
- `DebugIndicatorExporter` 讀取 Hub 最新快照，依序打包成六個多行文字區塊與一個獨立狀態列，並寫入專用 Indicator 的座艙參數。

這三個 Module 對外只暴露完成任務需要的小介面。CSV 執行緒、字串 escaping、文字排版、座艙參數寫入與檔案輪替等 Implementation 都留在 Module 內部。

### 4.3 Composition Root 與生命週期

`BridgeContext` 擁有 `DebugTelemetryHub`，而且 Hub 必須比 Core 活得久：

```text
BridgeContext
    +-- DebugTelemetryHub
    +-- Core::Fck1cEfm（建構時注入 DebugTelemetry Interface）
    +-- DebugCsvWriter（讀取 Hub）
    +-- DebugIndicatorExporter（讀取 Hub，寫入專用座艙參數）
```

- 不使用 global、singleton 或 thread-local publisher。
- 正式執行必須由 composition root 明確注入 Hub。
- 測試明確注入 recording adapter 或 disabled adapter；不得用隱藏的空物件掩蓋漏接依賴。
- flight start 清除上一趟飛行的最新值與時間序列，但保留已宣告通道目錄。
- `ed_fm_release` 完成該趟飛行的 CSV flush，並清除 per-flight 資料。
- process tool 的建立與銷毀遵守既有 DLL loader-lock 規則。

## 5. 通道模型

### 5.1 通道宣告

每個通道在 Core setup／建構階段宣告，至少包含：

| 欄位 | 用途 |
|---|---|
| `name` | 穩定且唯一的機器可讀名稱，也是 CSV 欄名 |
| `label` | Indicator 顯示名稱 |
| `value_type` | `double`、signed integer、boolean 或 text |
| `unit` | 可選；顯示與文件用途，不做數值轉換 |
| `description` | 可選；說明觀察點真正代表的時點與語意 |

這些 metadata 是讓工具更好用，不是限制資料內容。

規則：

- `name` 重複、空白或同名不同型別時，setup 明確失敗。
- 通道型別宣告後不得在執行期間改變。
- 通道宣告順序就是 Indicator 與 CSV 的穩定欄位順序。
- `debug.csv` header 建立後不接受動態新增欄位；需要新增通道時，下一次載入 DLL／啟動工具建立新 schema。

最後一條是 CSV 必須有穩定欄位結構的技術需求，不是限制開發者的觀察內容或推送位置。

### 5.2 支援值型別

第一版支援：

- `double`
- signed integer
- `bool`
- UTF-8 text

格式規則：

- `debug.csv` 的浮點值使用可重現且足以 round-trip 的完整精度。
- Indicator 的浮點值使用 6 位有效數字，避免畫面診斷值因不必要的小數位擠出欄位。
- `nan`、`inf`、`-inf` 保留為明確文字，不偷偷改成零。
- 布林值輸出為 `True`／`False`。
- 尚未推送的值輸出為 `-`。
- 文字可包含逗號、雙引號與換行；CSV 必須正確 escaping。
- 通道文字值中的換行轉為可辨識的單行表示，避免破壞 Indicator 每通道一行的版面。

### 5.3 發布語意

概念介面如下，實際命名在實作時依專案 C++ 慣例調整：

```cpp
auto channel = telemetry.declare_channel<double>(descriptor);
channel.publish(simulation_time, value);
```

- 發布是 producer 主動 push，不由 DcsBridge 輪詢 Core getter。
- producer 不需要固定頻率；值改變時、每次演算法 tick、command handler 內或特殊診斷點都可發布。
- 每次發布帶有明確模擬時間。
- 一般 System 內由目前 tick context 提供時間。
- command／lifecycle 等非 System 路徑由 `Fck1cEfm` 的目前模擬時間 context 提供時間。
- 若確實需要觀察自訂時間，使用名稱明確的 publish-at API，不讓預設 API 猜測時間。
- 不使用 wall-clock time 作為 CSV 取樣基準。

同一通道在兩個 CSV sample 之間發布多次時，Indicator 在下一次 DCS frame 顯示最新值；CSV 在取樣點保存該時間點以前最後一個值。

## 6. 時間與取樣

### 6.1 固定 64 Hz

`debug.csv` 使用 64 Hz 模擬時間取樣：

- 這是 **Project-defined** 工具頻率。
- 它不是已證實的 F-CK-1C 診斷頻率。
- 它也不是 Indicator 畫面更新率。
- 選擇 64 Hz 是為了與目前 FLCC reference rate 對齊，並讓診斷結果不受遊戲畫面更新率影響。

### 6.2 重採樣規則

Hub 暫存依模擬時間排序的更新，Sampler 使用 zero-order hold：

1. 取出下一個 64 Hz sample time 以前的所有更新。
2. 每個通道保留最後一個更新值。
3. 寫出該 sample time 的完整 latest snapshot。
4. 將下一個 sample time 前移一個固定週期。

這可處理一個 DCS `ed_fm_simulate` callback 內發生多個內部 System tick 的情況，不會把 callback 結束時的最終值錯誤複製到前面的所有 sample。

### 6.3 明確的資料損失模型

`debug.csv` 故意不是 lossless event log：

- 同一通道在兩個 sample 間的中間值可以被折疊。
- 最後值會延續到下一個 sample，直到收到更新。
- 需要完整事件順序、例外或一次性失敗原因時，使用 `fck1c_efm.log`。
- Debug CSV 若因 writer mailbox 被覆蓋而跳號，必須像現有 state CSV 一樣以 sequence gap 或明確計數暴露。

## 7. 專用 Debug Indicator

### 7.1 獨立且只負責顯示

新增獨立的 Lua 目錄：

```text
Cockpit/Scripts/DebugIndicator/
    DebugIndicator_init.lua
    DebugIndicator_page.lua
```

- 在 `device_init.lua` 以獨立的 `ccControlsIndicatorBase` 顯示宿主註冊；這是 DCS 的 COMMON screen-space 顯示宿主，不共用正式 Controls Indicator 的頁面、狀態或操作。
- 不修改 `ControlsIndicator/`，也不共用右 Ctrl + Enter 的正式控制指示器開關。
- 不建立 `avLuaDevice`，不模擬任何飛機裝置。
- 只使用 `SCREENSPACE_INSIDE_COCKPIT` 呈現目的與既有字型，不增加模型、材質或貼圖資產。
- Indicator Lua 建立一個半透明背景、兩欄三區塊的六個文字元素，以及獨立狀態列；背景明確表示工具已開啟。
- 版面由欄數、每欄區塊數、邊界比例與 DCS 畫面 aspect 計算，不為每個 channel 寫死座標。
- Lua 不解析 channel payload、不持有系統語意、不保存飛機狀態，也不向 Core 回傳資料；它只負責通用顯示幾何。

刪除這個 Lua 目錄與註冊後，只會失去遊戲內診斷畫面；Core、飛行行為、正式 Controls Indicator 與 `debug.csv` 都不受影響。

### 7.2 最小顯示 Interface

使用下列專用座艙參數：

| 參數 | 用途 |
|---|---|
| `DEBUG_INDICATOR_VISIBLE` | `0` 隱藏、`1` 顯示 |
| `DEBUG_INDICATOR_TEXT_1` … `DEBUG_INDICATOR_TEXT_6` | 每個最多 10 個 channel 的已排版多行文字 |
| `DEBUG_INDICATOR_STATUS` | telemetry、CSV 錯誤與畫面容量溢位訊息；不占用 channel 行數 |

預設每個通道一行：

```text
AP MASTER: TRUE
AP LATERAL MODE: HEADING HOLD
AP TARGET HEADING: 1.5708 RAD
```

`DebugIndicatorExporter` 不硬編碼 FLCC、AP 或其他系統欄位，內容只依 descriptor、宣告順序與最新值生成。每 10 個 channel 形成一個獨立字串，依序填滿六個區塊；目前 59 個 channel 可完整放入。Indicator 顯示時，每個 `ed_fm_simulate` frame 讀取一次 Hub 最新快照；只有內容改變的文字參數才重新寫入 DCS。隱藏時不進行文字排版，也不增加獨立顯示時鐘。

不做分頁與顯示層級。畫面容量固定為 60 個 channel；超出時狀態列明確顯示 `+N CHANNELS NOT SHOWN - SEE DEBUG.CSV`，完整資料仍保存在 `debug.csv`。這個容量是已驗證的 DCS 字串呈現邊界，不是靜默截斷。

### 7.3 DCS runtime 顯示證據

- 同一套 C++ 動態字串路徑顯示 10 行時正常。
- 15 行、20 行與完整 59 行放在單一 `ceStringPoly` 時，背景仍顯示但文字完全消失。
- 實際 59 個 channel 依每 10 行切分後，各區塊字串約為 236 至 415 個字元；因此採用六個獨立字串，不假設未公開的精確 byte 上限。
- 不加入自動縮短 label、任意 byte 截斷或舊值 fallback；未來單一區塊若因極長文字失敗，應明確診斷根因。

### 7.4 唯一操作：顯示／隱藏

- 新增一個可綁定但不預設組合鍵的 `Toggle EFM Debug Indicator` command。
- command 與八個座艙參數都加入既有 DCS ID catalog，再由現有產生器同步 C++ 與 Lua 名稱；不手寫重複字串常數。
- command 由 DcsBridge 的診斷工具路徑處理，不送進 `PilotControls`、Core System 或 SystemPipeline。
- 可見狀態是開發工具的介面狀態，由 DcsBridge 持有；每次 DLL／flight 啟動預設隱藏。
- 不新增下一頁、上一頁、顯示層級或其他操作。

### 7.5 原生 Debug Watch 相容處置

DCS runtime 探針已確認：`ed_fm_enable_debug_info()` 會被呼叫，但 `ed_fm_debug_watch()` 呼叫次數為零。因此：

- 移除 `[DEBUG-watch-a4f2]` 臨時 callback 探針及其測試。
- 移除 `DebugWatchFormatter`、brief／normal／full 與 `DebugWatchLevel`；專用 Indicator 顯示不依賴它們。
- 保留 `ed_fm_debug_watch` export 名稱以維持目前 EFM export 契約，但實作只清空有效 buffer 並回傳 `0`。
- `ed_fm_enable_debug_info()` 恢復為 `false`；未來若要製作 force vector／debug geometry，另立功能範圍。
- 不要求使用者建立 `autoexec.cfg`，也不依賴 DCS 的右 Ctrl + Pause 效能資訊畫面。

### 7.6 最終 DCS 顯示驗證

以六個區塊直接驗證獨立 screen-space Indicator 能完整顯示 Hub latest snapshot，不建立第二條固定文字 tracer 路徑。

- 驗證時確認兩欄、每欄三區塊都有資料，59 個 channel 依序出現，並確認 toggle 顯示與隱藏。
- 驗證不同 aspect 時，背景、欄位、區塊與字體不重疊或超出畫面。
- 驗證失敗時保留失敗證據並定位實際區塊或參數，不改用 Controls Indicator 或外部 overlay。
- production、測試及文件均不得保留固定 tracer 常數或雙重資料路徑。

## 8. `debug.csv`

### 8.1 路徑與生命週期

- 檔名固定為 `<module-root>/log/debug.csv`。
- 下一次 DLL 執行時，現有檔案輪替為 `debug.csv.old`，並移除更舊的 `.old`。
- 檔案允許分享讀取，DCS 執行時可由其他工具查看。
- flight 重新開始時 sequence 從 `0` 開始；同一 process 的檔案生命週期遵守既有日誌工具規範。
- 沒有任何宣告通道時，不建立空白的 `debug.csv`。

### 8.2 Wide-format schema

```text
sequence,simulation_time_s,<channel 1>,<channel 2>,...
```

- 一列代表一個 64 Hz sample time 的完整最新值快照。
- 欄位順序依通道宣告順序固定。
- 未曾發布的通道使用 `-`。
- 文字使用標準 CSV escaping。
- CSV 保存完整值，不受 Indicator 畫面大小限制。

Wide format 讓 Indicator 與 CSV 都使用「一組具名通道的最新快照」模型，方便直接畫圖、對齊時間與比較多個控制量。

### 8.3 寫入執行緒

沿用現有 CSV 工具的非阻塞原則：

- 模擬執行緒不等待磁碟 I/O。
- writer 的 queue／mailbox 策略必須有明確容量與可觀察的資料遺失訊號。
- writer 錯誤寫入 EventLog，並在 Indicator 顯示時加入工具錯誤狀態。
- 不因 CSV 開啟或寫入失敗而偽造成功資料。
- 診斷輸出失敗不回寫飛行控制；模擬可繼續，但失敗必須持續可見直到修復或重新啟動。

## 9. `fck1c_state.csv` 責任整理

### 9.1 長期保留的內容

`fck1c_state.csv` 以飛機可觀察狀態為主，包括：

- 位置、速度、姿態、角速度與其他飛行狀態。
- 空速、垂直速度與航向。
- 力與力矩。
- 引擎與燃油的飛機輸出狀態。
- 飛行員輸入與控制面實際位置。
- 起落架、懸吊、損傷、震動等飛機狀態。

目前工作樹新增的 IAS、垂直速度、heading、pitch／roll attitude、p／q／r 屬於這個責任，應保留在 state CSV。

### 9.2 遷移到 Debug Telemetry 的內容

下列詳細內部資料改由 Core 宣告 debug channel：

- `afcs_target_*`
- `afcs_*_reference_*`
- `flight_control_selected_*`
- `flight_control_*_reference_*`
- `flight_control_*_command_normalized`
- `flight_control_constraint_*`
- `afcs_*reason`
- 開發用 override、test cut request 或其他只用於分析實作的旗標

實際遷移時先建立對等通道並驗證，再從 `StateCsvWriter` 移除欄位，避免診斷覆蓋率中斷。

這次只改 CSV projection；CockpitSnapshot 與 Lua 需要的正式呈現資料不因診斷遷移而刪除。

## 10. 初始使用範圍

工具完成後，先註冊目前 FLCC／AFCS 調試所需的通道，至少涵蓋：

- AP master 與 vertical／lateral active mode。
- engage、disconnect、constraint 與 degradation reason。
- selected／captured targets。
- vertical／lateral shaped guidance references。
- manual／automatic reference source selection結果。
- coordinated flight-control reference。
- normalized actuator demands。
- 為目前測試有用的實際 heading、pitch、roll、vertical speed、p、q、r。

最後一組即使同時存在於 state CSV 也可註冊，因為 Indicator 需要即時對照；資料重複是兩個工具面向不同用途的刻意選擇。

初始通道清單不是永久規範。未來開發者可直接在 Core 擁有資料的位置增刪宣告，DcsBridge 通用工具不跟著修改。

## 11. 錯誤、併發與可觀察性

### 11.1 錯誤原則

- 宣告錯誤在 setup 階段立即失敗，不延後到第一次輸出。
- 使用錯誤 handle、型別不符或時間倒退時明確報錯。
- 執行期發布錯誤由 Hub 保存並在 Indicator 顯示時輸出、同時寫入 EventLog，不讓例外中斷飛控 commit。
- production 明確選用隔離並回報策略；測試可改用回報後重新拋出，以便嚴格定位錯誤。
- Hub 保留每趟 flight 的首筆發布根因與累計失敗數，避免後續連鎖錯誤覆蓋最初原因。
- CSV 路徑、open、write、flush、rotation 錯誤寫入 EventLog。
- Indicator 座艙參數寫入失敗必須寫入 EventLog，不得用舊文字冒充本次更新成功。
- 文字參數寫入失敗不得連帶隱藏背景；背景必須保留可見，讓失敗狀態與「工具未開啟」能被區分。
- 不以零、空字串或舊值冒充失敗的更新。

### 11.2 併發

- Core execution 仍由既有 `execution_mutex` 序列化。
- Hub 自身保護宣告目錄、ordered updates 與 latest snapshot。
- Indicator exporter 讀取一份不可變 snapshot，不持有 Hub 鎖進行字串格式化或座艙參數寫入。
- CSV sampler 與 writer 不讓磁碟 I/O 阻塞 Core publish。
- 同一 simulation timestamp 的發布維持實際接收順序，後發布值成為該時間點的 latest value。

## 12. 實作順序

### Step 0：保留目前飛機狀態基線

- 保留已加入 FrameOutput／state CSV 的 IAS、垂直速度、heading、姿態與角速度。
- 固定現有 native tests 與 export baseline。
- 不在此步修改控制行為。

### Step 1：建立 Core 診斷契約

- 新增 channel descriptor、value variant、typed handle 與 publisher Interface。
- 定義宣告、發布、時間與錯誤語意。
- 提供測試用 recording adapter 與明確的 disabled adapter。
- 更新架構檢查器，只允許經由 Core Contracts 使用該介面。

### Step 2：依賴注入與時間 context

- 由 `BridgeContext` 建立 Hub，再注入 `Fck1cEfm`。
- 讓 System tick 與非 System command／lifecycle path 都能取得明確模擬時間 publisher。
- 不將診斷通道塞入 FrameOutput 或 AircraftData。
- 不增加 global access point。

### Step 3：實作 `DebugTelemetryHub`

- 通道註冊與驗證。
- typed publish。
- ordered updates。
- immutable latest snapshot。
- flight lifecycle reset。
- 併發與錯誤處理。

### Step 4：實作 64 Hz `DebugCsvWriter`

- zero-order-hold resampler。
- wide-format header 與 row formatter。
- UTF-8 text／CSV escaping／nonfinite values。
- 非阻塞 writer、flush、rotation 與錯誤曝光。

### Step 5：實作專用 Debug Indicator

- 新增獨立 `DebugIndicator_init.lua` 與 `DebugIndicator_page.lua`。
- 加入半透明背景與依畫面 aspect 計算的兩欄三區塊版面。
- 新增 `DEBUG_INDICATOR_VISIBLE`、六個 `DEBUG_INDICATOR_TEXT_n` 與 `DEBUG_INDICATOR_STATUS` 座艙參數。
- 讓既有 cockpit parameter endpoint 明確支援 string write，不讓 exporter 直接繞過 endpoint 呼叫 DCS 函式。
- 新增 DcsBridge `DebugIndicatorExporter` 與唯一的 toggle command。
- 接入 Hub latest snapshot，依宣告順序每 10 行打包，容量超過 60 時明確顯示溢位狀態。
- Indicator 使用 6 位有效數字，CSV 保持完整 round-trip 精度；未改變的區塊不重複寫入 DCS。
- 移除原生 Watch formatter、顯示層級與 callback probe；保留最小 ABI export。

### Step 6：遷移現有 FLCC／AFCS 診斷資料

- 在資料擁有者附近宣告並發布對等通道。
- 比對新舊輸出，確認沒有失去目前可觀察資訊。
- 從 state CSV 移除內部診斷欄位。
- 保留 CockpitSnapshot 所需的正式資料。

### Step 7：自動化驗證

- 完成第 13 節測試。
- 執行 native test suite、build、architecture checker 與 EFM export baseline。
- 確認沒有飛控／AP 行為性差異。

### Step 8：Code review

- 以本計畫為 spec，同時檢查 repository standards 與需求符合度。
- 只修正本次架構與實作問題，不順帶調整 AP tuning。
- Review 有未決的產品或架構問題時停止並討論；純實作缺陷直接修正後重跑驗證。

### Step 9：安裝與 DCS 針對性驗證

- 執行 `install.bat` 安裝到實際 F-CK-1C module。
- 依第 14 節完成 DCS 測試。
- 若專用 Indicator 無法顯示 C++ 多行字串，記錄明確 runtime 證據並停止，不自行擴大成複雜 Indicator。

### Step 10：DCS 證據與 commit

- 保存 DCS 驗證結果與必要證據。
- DCS 驗證通過且使用者明確要求後才 commit。

## 13. 自動化測試計畫

### 13.1 Core contract tests

- 宣告並發布所有支援型別，包含含逗號、雙引號與換行的文字。
- 重複名稱、空名稱、同名異型別明確失敗。
- 任意 call site 的 pre-commit、mid-operation、post-commit 發布都依接收順序保存。
- 同時間多次發布時，最後值正確。
- `nan`／`inf` 不被改寫。
- 測試 adapter 可完整觀察發布；disabled adapter 行為明確且不偽裝已記錄。

### 13.2 Hub 與 resampler tests

- latest snapshot 一致且不可變。
- irregular push rate 重採樣成穩定 64 Hz。
- 同一 sample 間多次更新只保存最後值。
- 一次 DCS callback 內多個 simulation tick 不產生錯時複製。
- 暫停、重新開始 flight 與 simulation time reset 正確。
- 相同 timestamp 維持接收順序。
- 併發 publish／snapshot 不出現部分更新或資料競爭。

### 13.3 CSV tests

- header 順序與資料列順序一致。
- 尚未發布值為 `-`。
- bool、integer、double、nonfinite 與文字格式正確。
- CSV escaping 正確。
- sequence、64 Hz sample time、rotation、flush 與 share-read 正確。
- writer 落後或錯誤時有明確 gap／error，不靜默遺失。
- 沒有通道時不建立空檔。

### 13.4 Debug Indicator tests

- exporter 依通道宣告順序產生每通道一行的文字。
- 顯示最新的浮點數、整數、布林值與文字。
- Indicator double 使用 6 位有效數字；`debug.csv` 的完整 round-trip 精度測試保持不變。
- 通道文字值的換行不會破壞一通道一行的版面。
- 61 個通道時，前 60 個分成六個 10 行區塊，第 61 個由獨立狀態列明確報告。
- 沒有通道時六個區塊與狀態列輸出空文字，但可見狀態仍由 toggle 明確控制。
- 隱藏時不進行文字格式化或座艙文字參數更新。
- 連續相同 snapshot 不重寫文字參數；單一區塊改變時只寫入該區塊。
- 顯示期間並行 publish 不產生破碎 snapshot。
- 座艙字串參數寫入失敗會產生明確事件，且不保留偽造的新值。
- `ed_fm_debug_watch()` 對有效 buffer 寫入空字串並回傳 `0`。
- Lua 靜態檢查確認 Debug Indicator 不引用 `ControlsIndicator/`、不建立 `avLuaDevice`、不包含飛行或 AP 邏輯。

### 13.5 Architecture tests

- Core 不 include DCSBridge。
- DcsBridge 不 include 具體 Core System／Simulation implementation。
- Debug adapter 只能經 Core Contracts Interface 注入。
- 加入反例 fixture，確保直接從 DcsBridge 讀 FLCC internals 會被拒絕。
- 專用 Indicator Lua 只能讀取 `DEBUG_INDICATOR_VISIBLE`、六個 `DEBUG_INDICATOR_TEXT_n` 與 `DEBUG_INDICATOR_STATUS`，不得成為 Lua→C++ 模擬資料通道。
- Lua 必須用同一個文字元素 factory 與迴圈產生六區塊，並以畫面 aspect 計算兩欄三區塊版面，不逐項寫死 channel。

## 14. DCS 手動驗證計畫

先驗證六區塊完整輸出的顯示路徑，再執行完整資料行為驗證。

### 14.1 顯示路徑驗證

1. 執行 `install.bat`，進入任意 F-CK-1C 座艙任務。
2. 在 DCS Controls 中綁定並執行 `Toggle EFM Debug Indicator`。
3. 確認專用 Indicator 與正式 Controls Indicator 完全獨立。
4. 確認畫面為兩欄、每欄三區塊，59 個真實 channel 全部出現、換行正確，狀態列沒有錯誤，且再次 toggle 後完全隱藏。
5. 以至少兩種 aspect／解析度確認背景、欄位、區塊與文字不重疊或超出畫面。
6. 保存畫面、`fck1c_efm.log` 與 DCS log 作為顯示路徑證據。

顯示路徑未通過時停止；不以其他 Indicator 或 overlay 繞過問題。

### 14.2 完整資料驗證

1. 操作 AP master、vertical mode、lateral mode 與 reference commands。
2. 確認 Indicator 最新值會更新，並與同一模擬時間附近的 `debug.csv` 相符。
3. 隱藏 Indicator 後繼續飛行，確認 `debug.csv` 仍以固定模擬時間寫入。
4. 顯示或隱藏 Indicator，確認飛機操控、AP 狀態與正式 Controls Indicator 都不受影響。
5. 造成一段 frame rate 變化，確認 CSV sample time 仍維持 64 Hz，而不是依畫面 FPS。
6. 結束並重新開始 flight，確認 Indicator 預設隱藏，latest values、sequence 與 per-flight 時間正確重置。
7. 確認 `fck1c_state.csv` 仍有物理飛行狀態，且 AP／FLCC 詳細診斷位於 `debug.csv`。

## 15. 完成條件

- Core 可在任意需要的位置與頻率主動推送四種資料型別。
- 推送位置決定資料時點語意，工具不強迫 commit-only。
- Core 不知道 Debug Indicator、CSV 或檔案路徑。
- DCSBridge 不需要知道 FLCC／AP 的內部 class。
- 專用 Indicator 以兩欄三區塊顯示最多 60 個最新值，超出容量時明確指向 `debug.csv`；`debug.csv` 保存完整 schema 的 64 Hz 歷史快照。
- Indicator double 使用 6 位有效數字，`debug.csv` double 保持完整 round-trip 精度。
- 文字值在 Indicator 與 CSV 都正確。
- 新增／移除通道不需要修改兩個輸出 Adapter。
- `fck1c_state.csv` 聚焦於飛機可觀察狀態。
- 診斷工具不參與模擬決策，不改變控制結果。
- schema、座艙參數、writer 落後與 I/O 失敗都明確可見。
- native tests、architecture checker、build 與 export baseline 通過。
- 安裝後首次顯示路徑與完整 DCS 手動驗證完成。

## 16. 參考

- [DCSBridge contributor guide](../src/efm/F-CK-1C_EFM/DcsBridge/README.md)
- [Core Systems contributor guide](../src/efm/F-CK-1C_EFM/Core/Systems/README.md)
- [Flight-control refactor plan](FLIGHT_CONTROL_REFACTOR_PLAN.md)
- [DCS forum：EFM debug info 在 multi-thread 的回報](https://forum.dcs.world/topic/368950-efm-ed_fm-enable-debug-info-not-working-in-multithread/)
