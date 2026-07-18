# DCSBridge 重構計畫

本文件是目前討論結果的唯一計畫來源。已確認的內容直接寫成規格；目前架構與行為已足以開始實作。實作順序與 commit 拆分留到最後單獨討論。

## 目標

- 讓新貢獻者能從入口與目錄快速理解 DCS、DCSBridge、Core 的責任與依賴方向。
- 讓 DCSBridge 保持精簡，不用大量小目錄或只有轉送功能的抽象層製造複雜度。
- Core 不依賴 DCSBridge，也不認識 DCS SDK 型別、Cockpit handle、檔案或 logger。
- DCSBridge 與 Core 只透過用途明確的 typed inputs、commands 與 outputs 溝通。
- 移除 Core 主動反向呼叫 DCSBridge 的做法。
- 以一個 `.log` 記錄程式事件，以一個 `.csv` 記錄 Core 計算出的飛機狀態。
- 支援 DCS 未來可能增加的 callback 並行，不讓檔案 I/O 阻塞飛行模型運算。
- 重構期間不改變既有飛行行為。

## 本輪範圍

### 納入

- DCS EFM callbacks 的組織與轉換。
- DCSBridge 與 Core 的介面。
- `DcsRuntime` 與 `Fck1cEfmRuntime` 的移除。
- `FrameInputCollector`、`FrameOutput` 與 DCSBridge `OutputStore`。
- Command mapping 與 value rule。
- Cockpit、Carrier 的精簡責任，以及現有常駐 Debug Watch 內容的移除。
- 程式事件 log 與飛機狀態 CSV。
- DCSBridge 邊界的執行緒安全。
- 與上述介面直接相關的測試改寫。

### 不納入

- Core／Systems 的檔案架構重整。
- Config 與 FM Data 的完整重構。
- 把整台飛機的 authoritative state 抽成獨立 module。
- Lua 與 C++ log 的統一。
- 改變飛行模型公式、控制律或物理行為。
- 建立通用 event bus、service locator 或鏡射全部 DCS callbacks 的 dispatcher。

## 依賴與 ownership

依賴方向固定為：

```text
DCS C ABI -> DCSBridge -> Core
```

- Core 擁有唯一的 authoritative aircraft state。
- `FrameInputCollector` 只保存尚未提交或最新收到的 DCS 外部輸入，不是第二份飛機狀態。
- `FrameOutput` 是 Core 完成 start 或 step 後產生的唯讀輸出投影，不是完整 Core snapshot。
- `OutputStore` 由 DCSBridge 擁有，只保存最後一次完整發布的 `FrameOutput`，供 DCS 的輸出 callbacks 查詢。
- CSV writer 收到的是帶有 sequence 的 telemetry record，不是另一個可回寫的 state owner。
- Core 不注入 EventLog、DCS runtime 或其他 DCSBridge interface。

## DCSBridge 與 Core 的主要介面

核心 frame 介面定為：

```cpp
FrameOutput start(StartMode mode);
FrameOutput step(const FrameInput& input);
void dispatch(const Command& command);
```

- `FrameInput` 包含 DCS 傳入的實際 `dt`。不得把 step 寫死成 0.001 秒。
- 專案內附的 DCS SDK 只保證 `ed_fm_simulate(double dt)` 在 simulation step 被呼叫，沒有保證 `dt` 永遠是 0.001。
- `simulation_time` 使用實際 `dt` 累加。
- `start()` 依 cold ground、hot ground、hot air 建立對應的初始值並回傳初始 `FrameOutput`。
- `step()` 完成一次 Core 狀態推進並回傳該次完整 `FrameOutput`。
- 不建立 `StepResult` 或 `StartResult`；目前沒有需要與 `FrameOutput` 分離的結果。
- 不在 `FrameOutput` 中放 log 行為或檔案格式。
- `FrameOutput` 不包含 CSV sequence；sequence 是 DCSBridge telemetry 的責任。

Fuel、damage、gameplay option、refueling、mass delta、repair 與 release 在本輪保留用途明確的 Core 操作，不為了方法數好看而塞進通用 `ExternalInput`。需要立即回傳的 fuel getter 與具消耗語意的 mass delta 仍直接且序列化地查詢 Core。

## C++ language standard 與 DCS ABI

- EFM DLL 與 test projects 都明確設定為 ISO C++17；不依賴 Visual Studio 的預設 language mode。
- Param lookup 使用 `std::optional<double>`；有值代表成功，`std::nullopt` 代表沒有 mapping。
- C++17 只用於 DLL 內部 implementation，以及同一 DLL 內 Core／DCSBridge 的 C++ interface。
- DCS seam 仍完全使用專案內 DCS SDK 宣告的 `extern "C"` exported callbacks 與 SDK types；`std::optional`、`std::string`、STL containers、C++ exceptions 或 ownership-bearing C++ objects 都不能跨越這個 seam。
- EFM 維持 x64 build 與目前的 static runtime (`/MT`)。因此 DCS 不需要理解 `std::optional`，也不需要為它提供特定 C++ standard library runtime。
- 任何 C++ exception 都不能穿過 `extern "C"` callback 回到 DCS；預期的錯誤以既定 EventLog 與 neutral/return 規則處理。
- C++17 build 完成後仍要以 DCS 實際載入 DLL、start、step 與輸出 callbacks 作整合驗證；這驗證 binary/ABI 相容性，不把 DCS 誤當成需要「支援 C++17 語法」的 caller。
- 本輪保留 DLL 目前已提供的全部 exports，不新增 SDK header 中其他 optional exports；重構 `EfmExports.cpp` 只整理既有 capability 的責任與資料流。
- 既有 Debug Watch exports 保留 ABI，但 production build 按「Debug Watch 開發工具」章節回報未啟用。

## DCS 與 Core 的資料流分類

### DCS 提供給下一個 step 的連續輸入

- Atmosphere：altitude、temperature、speed of sound、density、pressure、wind vector。
- Surface：ground height、object height、surface type、surface normal vector。
- Current mass state：mass、center of mass、moment of inertia。
- World kinematics：acceleration、velocity、position、angular acceleration、angular velocity、quaternion。
- Body-axis kinematics：acceleration、velocity、wind velocity、angular acceleration、angular velocity、yaw、pitch、roll、angle of attack、angle of slide。
- Suspension feedback：wheel index 與完整 `ed_fm_suspension_info` 內容。
- DCSBridge 在 step 前主動讀取的 autopilot 與 max-power Cockpit 狀態。
- `ed_fm_simulate` 傳入的實際 `dt`。

這些資料由 `FrameInputCollector` 整理後，以一份 `FrameInput` 交給 Core。

DCS 已提供但 Core 尚未實作的欄位仍要翻譯並放入 `FrameInput`，例如 atmosphere pressure、surface normal、moment of inertia 與目前被忽略的 kinematics。尚未使用不等於不需要；本輪不得因現有 Core 沒有 consumer 就在 DCSBridge 丟棄。

### 不併入 FrameInput 的操作

- `ed_fm_set_command` 經 command table 轉換後呼叫 `dispatch(Command)`。
- Fuel set/get、refueling、damage 與 gameplay options 使用現有語意明確的 Core 操作。
- Cold/hot/hot-air start、repair、release 是 lifecycle 操作。
- Carrier push/pop event 留在 DCSBridge `CarrierBridge`。
- Configure 與路徑整理留在 DCSBridge；Config 的重新設計不在本輪。

### Core／DCSBridge 提供給 DCS 的資料

- Total local force、作用位置與 total local moment 從 `OutputStore` 讀取。
- Draw arguments、parameter export 與 shake 從同一份最新 `FrameOutput` 產生；日後若臨時啟用 Debug Watch 工具，也只能讀這份輸出。
- `ed_fm_get_param` 只查詢最新輸出，不要求 Core 重新計算。
- Internal/external fuel getter 與 mass delta 依其即時或消耗語意直接查詢 Core。
- Carrier event 由 `CarrierBridge` 管理。
- 現有 component force/moment callbacks 回傳 `false`，本輪不建立未被使用的 component arrays。

### 非 frame Core 操作的 OutputStore 更新時機

已確認把 OutputStore 的 commit 點限制為 start 與成功 step：

| 操作分類 | Core 是否立即處理 | 是否立即發布 OutputStore | DCS 如何取得立即結果 |
|---|---:|---:|---|
| Command dispatch | 是 | 否，等下一個 step | 不需要立即輸出 |
| Set internal/external fuel、refueling | 是 | 否，等下一個 step | Fuel getter 直接且序列化地查詢 Core |
| Damage、gameplay options、repair | 是 | 否，等下一個 step | DCS callback 本身不要求輸出 |
| Mass delta | 立即 consume | 否 | `ed_fm_change_mass` 直接取得一次性結果 |
| Start | 是 | 是 | 發布 StartMode 對應的初始 `FrameOutput` |
| Release | 是 | 使 OutputStore 失效 | Release 後不應再有正常 output read |
| Carrier push/pop | 由 CarrierBridge 立即處理 | 不適用 | CarrierBridge 回傳 DCS event |
| Configure／module path | 由 DCSBridge 處理 | 不適用 | 不進入 Core frame output |

主要理由是簡化程式碼，以及 DCS setter callback 本身沒有要求立即輸出。其他 mutation 可以立即改變 Core，但不為了刷新快取增加額外更新路徑；需要立即回傳的 fuel 與 mass delta 使用各自明確的 Core query。只有 start 與成功 step 建立新的 `FrameOutput`，release 使 OutputStore 失效。

## FrameInputCollector 的一致性規則

- 每一類 callback 先在區域變數中完成 DCS raw data 到 Core type 的翻譯，再一次發布完整資料。
- 同一類資料不能出現一半新、一半舊的 torn value。
- 不要求 atmosphere、surface、mass、kinematics、suspension 等不同分類來自同一個 DCS 時刻。
- `simulate` 開始時取得當下最新資料；snapshot 前完成發布的資料用於本 step，之後完成的資料自然留到下一個 step。
- 不建立強制跨分類配對、等待或 deadline barrier。
- Start/reset 時依 StartMode 建立已知預設值；尚未由 DCS 提供而無法合理推定的欄位標成 invalid。

### 邊界數值驗證

- Atmosphere、surface、mass、world/body kinematics 與 suspension callback 的任一必要 numeric value 若是 NaN 或 Infinity，拒絕該 callback 的整筆 sample、記錄包含 callback 名稱與欄位的 ERROR，並保留該分類上一筆完整 sample。
- Fuel setter、command value 或其他單一立即輸入若是 NaN 或 Infinity，記錄包含 callback 名稱與 value 的 ERROR，並忽略該次操作。
- `ed_fm_simulate` 的 `dt` 若不是有限值或 `dt <= 0`，明確記錄 ERROR 並跳過該次 step，不以假 dt 繼續。
- 除 SDK 已定義的 index／buffer 邊界外，本輪不為有限數值自行增加推測性的範圍 clamp。

### Suspension

- 每個 wheel 的 `acting_force`、`acting_force_point`、`integrity_factor`、`struct_compression` 與 `wheel_speed_X` 是一筆完整輸入 sample，由 `FrameInputCollector` 一起發布並放入 `FrameInput`。
- Suspension vector 採用 DCS local/body 右手座標系：`+x` 向機鼻、`+y` 向上、`+z` 向右翼；作用位置原點是模型中心。
- `acting_force` 是 local/body force，單位 N；CSV 展開為 `acting_force_x_N`、`acting_force_y_N`、`acting_force_z_N`。
- `acting_force_point` 是該力的 local/body 作用位置，單位 m；`wheel_speed_X` 按 DCS 的 SI 慣例視為 local x 方向線速度，單位 m/s；`integrity_factor` 是無因次結構完整度。三者目前沒有 Core、DCS output callback 或 CSV consumer，只保留在完整 `FrameInput`，不放入 `FrameOutput`，也不建立空白 CSV 欄位。
- `struct_compression` 單位 m，`FrameOutput` 與 CSV 使用 `compression`／`compression_m`。
- 上述座標與單位依專案附帶 SDK 對 body coordinate 的宣告、[DCS EFM API reference frames](https://modding.caffeinesimulations.com/Aircraft/EFM/EFM_API/#dcs-reference-frames) 與 [Eagle Dynamics basic types](https://www.digitalcombatsimulator.com/en/support/faq/1256/)；Core／CSV 不做額外座標或單位轉換。
- 初始/reset 的 Core 計算狀態為無 acting force、compression 0，懸吊回到預設伸展位置；在第一筆 DCS suspension sample 到達前，該輪 raw sample 的 availability 為 false，CSV 對應欄位輸出 `-`。
- 收到 feedback 後保留最後一筆完整 sample，直到下一筆更新或 lifecycle reset。
- 本 step 沒收到新 feedback 不代表 force 變成 0，也不清除 compression。
- `FrameOutput` 與 CSV 每次都輸出 Core 目前使用或計算的 suspension state，不加入 `updated_this_step`。若日後 `acting_force_point`、`integrity_factor` 或 `wheel_speed_X` 出現實際 output consumer，再連同測試與 CSV schema 一起加入，不預先保留 placeholder。
- 移除 DCSBridge 既有 suspension diagnostics、startup probe、periodic probe、ground probe、相關設定與 `fck_susp_debug.log`。

### Cockpit continuous state

- Autopilot 與 max-power 是 step 前主動讀取的 continuous inputs，不是 Core 反向 callback，也不是 engine event。
- Cockpit parameter 缺失視為明確錯誤：EventLog 必須記錄缺少的 parameter 名稱。
- 該次 step 使用明確的 neutral input 繼續，不得略過整個 step。
- Parameter 恢復時記錄 recovery，避免只有失敗而無法確認恢復時間。

## Command mapping

- 使用單一 command binding table，把 DCS command ID 對應到 Core `Command` 與 value rule。
- 新增受支援 command 時只新增或修改一筆 binding，不建立第二份平行 map。
- Value rule 固定為 `PassThrough`、`Constant`、`PressOnly`。
- `PassThrough` 直接傳遞 DCS value，主要用於 axis 與連續控制。
- `Constant` 使用 binding 中指定的固定 value。
- `PressOnly` 只在 DCS value 大於 0 時 dispatch；release callback 忽略。
- `PressOnly` 不快取 pressed state、不做 edge detection。
- 不建立通用 toggle rule。需要 toggle 的語意由明確的 Core command action 表達。
- 已知 axis 與 non-axis command ID 不重疊，不為不存在的衝突增加額外狀態機。
- 收到 binding table 中不存在的 command ID 時，明確寫入 ERROR，至少包含 command ID 與收到的 value；忽略該次 command，不讓它中止後續 step。
- Binding table 自身若出現重複 ID 或無效規則，同樣視為程式錯誤並明確寫入 ERROR。

## FrameOutput 與 OutputStore

`FrameOutput` 只保存目前 DCS callbacks 與 CSV 真正需要的穩定輸出。未來出現具體 consumer 時再增加欄位，不為假想需求或暫時 debug 預留完整 Core state。初版預計分組包含：

- Simulation time 與必要 flight state。
- Total force、moment 與作用位置。
- 左右 engine 的當前輸出，包括實際 thrust。
- Controls 與 DCS draw/parameter export 所需狀態。
- Fuel。
- Landing gear。
- 三組 suspension 完整狀態。
- Shake。
- 各分組必要的 validity，讓 start 初期尚未取得的資料可以表達為 unavailable。

不加入完整 Core implementation state、所有中間計算、autopilot/max-power 輸入副本或未被 DCS 使用的 component force/moment arrays。

`OutputStore` 規則：

- DLL 建立後、第一次合法 start 前尚無 output；DCS 不應在此階段要求 draw args 或其他飛行輸出。
- Start 與每個成功 step 都發布一份完整 immutable `FrameOutput`。
- StartMode 已能決定所有 draw args 依賴的 engine、control、gear 等初始值；continuous input 的個別 unavailable 不會讓整份 `FrameOutput` 失效。
- 讀取者只能取得完整的上一版或下一版，不能看到發布中的半成品。
- Core 正在計算時，DCS output callback 可以繼續讀取上一份已完成輸出。
- Release 後 OutputStore 失效。理論上 DCS 不應再讀取；若仍被呼叫，EventLog 記錄錯誤並依該 ABI callback 回傳明確 neutral value。
- Draw args callback 是 void 且不需要強迫產生回傳值；若它在 start 前或 release 後違反 lifecycle 被呼叫，記錄 ERROR 並保持 DCS 傳入的 array 不變，不把所有 visual values 歸零。

## 多執行緒模型

DCS 公開資訊顯示 DCS 正持續採用多執行緒，但目前專案內的 EFM SDK 沒有保證所有 callbacks 一定同執行緒或一定並行。因此設計支援 callback 並行，但不依賴特定排程。

- `FrameInputCollector` 是 thread-safe latest-known input store。
- `OutputStore` 是 thread-safe immutable latest output store。
- 所有會讀寫 Core 的操作由同一個 execution mutex 序列化，包括 start、step、release、repair、command、fuel、damage、gameplay 與 mass delta。
- Core 本身不因 DCS 多執行緒而散布 mutex；並行責任封裝在 DCSBridge 邊界。
- EventLog 與 StateCsvWriter 各自處理自己的同步。
- 不在持有 input/output/mailbox lock 時執行磁碟 I/O。
- `simulate` 不等待 CSV 格式化、寫檔或 flush。

預期 step 流程：

1. 進入 execution mutex。
2. 從 collector 複製最新 `FrameInput`。
3. 呼叫 Core `step()`。
4. 發布完整 `FrameOutput`。
5. 發布一筆最新 telemetry record。
6. 離開 execution mutex。

輸入 callback 可以在 Core 計算期間繼續更新 collector；這些更新由下一個 step 使用。

## 檔案與目錄

DCSBridge 不提供給其他 module 使用的 umbrella public header。DCS 只看到固定 C ABI；Core contract 留在 Core。DCSBridge 實作依是否為內部細節分類。

```text
F-CK-1C_EFM/
├─ Core/
├─ DcsBridge/
│  ├─ EfmExports.cpp
│  ├─ README.md
│  └─ Internal/
│     ├─ BridgeContext.*
│     ├─ FrameInputCollector.*
│     ├─ DcsCommandRouter.*
│     ├─ DcsDamageMapper.*
│     ├─ CockpitBridge.*
│     ├─ CarrierBridge.*
│     ├─ EventLog.*
│     └─ StateCsvWriter.*
├─ Data/
├─ Systems/
└─ Common/
```

- `EfmExports.cpp` 是 DCSBridge 的固定入口與 orchestration/composition root。它接收 C callbacks、驗證與翻譯 DCS 資料、協調工具、呼叫 Core、發布輸出並回傳 DCS 需要的值。
- `EfmExports.cpp` 不是只有一行轉送的薄殼，但不承擔飛行計算、CSV 格式化或 logger 實作。
- `BridgeContext` 只集中 production ownership、共享狀態與 lifetime，不建立與每個 callback 一一對應的 facade methods。
- 小型純轉換若沒有形成獨立責任，可以留在使用端或既有 header；不為了符合目錄圖強迫每項功能新增 `.cpp/.h`。
- `OutputStore` 若內容很小，可直接成為 `BridgeContext` 的內部型別，不強制獨立檔案。
- 不保留 `DcsModule`、`DcsRuntime` 或新的同義 runtime facade。

## EventLog

事件檔固定為 `fck1c_efm.log`：

- 只記錄程式碼事件、lifecycle、警告與錯誤。
- 一般飛機數值不使用 INFO 或其他 log level 輸出，全部交給 CSV。
- 錯誤需要的 command ID、parameter 名稱、index、檔案路徑與 OS error code 是錯誤內容的一部分，可以隨 message 輸出。
- 格式固定為 `[wall-clock][simulation-time][LEVEL] message`。
- 範例：`[2026-07-16 14:32:05.123][12.34][ERROR] missing cockpit parameter name=...`。
- Simulation time 不加 `simtime=`，並使用與 CSV 相同的浮點格式，方便對齊。
- 尚未 start 或 release 後沒有有效 simulation time 時使用 `[-]`。
- Concurrent callbacks 寫 EventLog 時，simulation time 使用 `OutputStore` 最後一次完整發布的時間；不讀取正在 step 中的 Core state，也不猜測 callback 所屬的未完成 frame。
- 不加入 runtime、sequence、一般 telemetry 或第二份 source 欄位。
- 每個事件完成後 flush，讓其他程式可即時讀取。
- EventLog 必須 thread-safe。
- ERROR 必須明確、可追蹤，並帶上定位問題所需的參數。
- INFO 只用於足夠重要且語意清楚的 lifecycle 或程式事件，不用來輸出一般飛機數據。
- DEBUG 只允許在當下除錯期間暫時加入；問題確認後必須移除，不在完成的重構中保留長期 DEBUG instrumentation。
- EventLog 失敗時不能把 logger 狀態標成成功，但本輪不新增 `dcs.log`、`OutputDebugString` 或其他備援輸出路徑；若實際發生，再根據具體失敗原因處理。

## State CSV

狀態檔固定為 `fck1c_state.csv`：

- 內容是 Core 已計算完成的當前飛機輸出，不是散落在 callbacks 中重新計算的數值。
- 每個 StartMode 先發布 simulation time 0、sequence 0 的初始狀態；這是 start state，不是第一個實際 step。
- 第一個成功 step 使用 sequence 1。
- 每次 start sequence 歸零，但同一個 DCSBridge execution 內不重開 CSV，因此一個檔案可以包含多次飛行。
- Sequence 在每個成功 Core step 時增加，不是在 CSV 寫入成功時增加。
- CSV 固定由 `sequence`、`simulation_time_s` 與後續飛機數據組成。
- `FrameOutput` 的所有飛機輸出欄位都要展平成 CSV 欄位，不從其中挑選子集合；`FrameDataAvailability` 是序列化判斷用的 metadata，不另外輸出 validity 欄位。
- `bool` 固定輸出 `True` 或 `False`。
- 向量固定按 `x`、`y`、`z` 展開成三個欄位；固定集合按既定名稱與 index 展開，不把陣列塞進單一 cell。
- 尚未取得有效資料的欄位固定輸出 `-`，不用空 cell、假 `0`、NaN 或額外 validity 欄位代表未知。
- 新增或移除 `FrameOutput` 飛機輸出欄位時，必須在同一個改動中同步更新 CSV header、row serialization 與 schema 測試。
- 不加入 `schema_version` 或獨立 `dt_s` 欄位；相鄰且未跳號資料的 simulation time 差值可表示該次時間步長。
- 不加入 source 欄位。
- Suspension 寫入完整當前狀態，不加入 `updated_this_step`。
- Header 直接標示已知物理單位；數值保持 Core／DCS 原有單位，不做只為 CSV 的轉換。
- 浮點數使用可 round-trip 的精度與固定 C locale，避免精度損失或小數點受系統語系影響。
- Start 初期 unavailable 的欄位輸出 `-`，不用假 0 代表未知資料。
- CSV 不包含 autopilot 與 max-power input。

### Latest-only 背景寫入

CSV 是 best-effort debug telemetry。每個成功 step 都產生 record，但背景 writer 落後時不允許拖慢 Core 或讓記憶體無限增加。

- 不使用可累積多筆資料的 FIFO queue。
- 使用固定只保存一筆待處理資料的 latest telemetry mailbox。
- Core 發布新 record 時直接覆蓋尚未被 writer 取走的舊 record。
- Writer 有空時取出當時最新 record；已經開始格式化或寫入的 record 不取消。
- Writer 完成後若已有更新，只處理當時最新的一筆。
- Writer 先在記憶體中建立完整 CSV row，再由單一 writer 寫入，避免多執行緒交錯出半行資料。
- 遺失的中間資料以 CSV sequence 跳號表達，不新增 dropped-row 欄位。
- Mailbox 內另有不輸出到 CSV、跨 flight 單調增加的 publish version，避免 sequence 歸零造成同步誤判。
- Start sequence 0 與最後一筆資料沿用相同 latest-only 寫入流程，不為尚未發生的遺失情況建立特殊 acknowledgement 或 drain protocol；若實測真的發生，再依證據處理。
- Release 不產生額外 CSV row，也不等待最後 pending record。
- `ed_fm_release` 是單架飛機的 lifecycle，不等於 DLL unload；同一 DLL lifetime 可以有後續 start。
- CSV writer 屬於整個 DCSBridge/DLL execution，不因 `ed_fm_release` 停止或重建。
- DCS 不會在 process 存活時單獨卸載 EFM DLL，因此不建立額外的 pre-unload 或每 flight thread lifecycle。
- 遊戲 process 正常結束或意外終止時，Windows 會終止 process 內的 worker，回收記憶體與 OS handles；背景執行緒與 mailbox 不會在 DCS process 外形成持續的記憶體洩漏。
- Hard crash 或強制終止時不能承諾執行 cleanup code，也不能保證尚未 flush 的 debug 資料被保存。
- 不在 `DllMain(DLL_PROCESS_DETACH)` 等 Windows loader-lock 路徑等待 worker。

### File lifecycle

- EventLog 與 CSV 都在每個 DCSBridge execution 建立並保持開啟，不由單次 start 控制。
- Active files 為 `fck1c_efm.log` 與 `fck1c_state.csv`。
- Execution 開始時只輪替一次：刪除既有 `.old`、把目前 active 改名為完整檔名後加 `.old`、建立新的 active。
- 只保留 active 與一份 `.old`，不累積 timestamped history。
- 多次 cold/hot/hot-air start 共用同一組 active files。
- 檔案固定放在模組根目錄下的 `log` 資料夾：`<module-root>/log/fck1c_efm.log` 與 `<module-root>/log/fck1c_state.csv`。
- 模組根目錄沿用既有 `ModulePaths`：若第一個 callback 是 `ed_fm_configure`，直接把它的 `cfg_path` 交給初始化；configure 尚未發生時，由目前 DLL 路徑回推模組根目錄。不再解析 DCS Saved Games、`-w` instance 或 `dcs_variant.txt`。
- `DllMain` 與 static constructors 不執行檔案 I/O，也不啟動 writer。`DllMain` 不放任何 EFM operational logic。
- 第一個進入的 EFM exported callback 必須先完成 once-only `BridgeContext` 初始化，再執行該 callback 的驗證、翻譯、Core 呼叫或其他工具操作。
- 初始化順序固定為：解析模組根目錄、建立 `log` 目錄、輪替並開啟 EventLog、輪替並開啟 CSV、寫 CSV header、啟動 CSV writer，最後才處理第一個 callback。如此可讓 EventLog 涵蓋全部 EFM operational code，並在任何 flight start 前準備好 CSV。
- Windows 開檔模式必須允許其他程式在 DCSBridge 寫入期間同時讀取。
- CSV header 建立後可立即讀取；一般資料約每 100 ms flush，使外部工具能近即時看到更新。
- EventLog 是每個完整事件寫入後 `fflush`；CSV 不是每列 `fflush`，而是完整 rows 經 buffered write 後約每 100 ms `fflush`，避免高頻 step 付出逐列 flush 成本。
- `fflush` 只要求資料離開 C/C++ userspace buffer，不等同每次呼叫 Windows `FlushFileBuffers` 強制同步實體磁碟。
- Process crash 時不執行額外 cleanup 或 CSV 修補；已完成並 flush 的 rows 保持可讀，尚在 buffer 中的尾端 debug 資料不保證保存。
- CSV open/write/flush 失敗時明確寫入 EventLog，停止本 flight 的 CSV 寫入並在下一次 start 重試，不在每個 step 重試。
- EventLog 本身失敗時不轉寫 `dcs.log`；其他一般錯誤也不重複寫兩份 log。

## Debug Watch 開發工具

- 移除目前常駐的 Debug Watch 內容，以及它對完整 `FBWControllerState`、suspension probe 與其他 Core implementation state 的依賴。
- 正常 production build 不提供 Debug Watch 內容：`ed_fm_enable_debug_info()` 回傳 `false`，`ed_fm_debug_watch()` 將可寫 buffer 設為空字串後回傳 `0`。這是明確未啟用的能力，不是錯誤或 silent fallback。
- 本輪不為未發生的 debug 需求建立空殼 class、永久 interface、預留欄位或常駐 formatter。
- 日後真的需要 DCS 畫面內即時觀察時，再加入一個 developer-only Debug Watch 工具，並由開發者明確加入 debug build；production build 不編譯它。
- 該工具只能格式化最新 `FrameOutput` 與 build metadata，不持有 authoritative state、不重新計算 Core、不寫檔，也不讓 Core 依賴工具。
- 若某次 debug 必須觀察 Core 內部中間量，應在當次工作中加入明確且暫時的 instrumentation，問題結束後移除；不能因此把完整 implementation state 永久加入 `FrameOutput`。

## 介面細化決策

本節記錄 `FrameOutput`、非 frame Core 操作與 DCS neutral return 的具體介面決策。

### 1. `FrameOutput` 初版欄位

`FrameOutput` 是 Core 對外發布的不可變 frame 結果，不是把整份 Core state 複製出來，也不是第二個 authoritative state owner。Core 仍是唯一真實狀態持有者；`OutputStore` 只保存最後一份完成的發布結果供 DCS callbacks 讀取。

建議的頂層形狀如下：

```cpp
struct FrameOutput
{
    double simulation_time_s;
    FrameDataAvailability availability;
    FlightOutput flight;
    ForceMomentOutput force_moment;
    EngineOutput engines[2];
    ControlOutput controls;
    LandingGearOutput landing_gear;
    SuspensionOutput suspension;
    FuelOutput fuel;
    double shake_amplitude;
};
```

- `sequence` 不放進 `FrameOutput`，由 DCSBridge 發布 CSV 時加入。
- `availability` 只描述依賴 DCS continuous input 的整類資料是否已有真實 sample，例如 atmosphere、surface、mass、world/body kinematics 與各輪 suspension；不為每個數值建立一個 bool。
- StartMode 自己能決定的 engine、control、gear、fuel 等初始狀態仍是有效值；尚未收到 DCS continuous input 的欄位在 CSV 輸出 `-`。
- 固定兩具 engine 與三個 wheel 的集合，實作時優先使用固定長度 array，不建立動態容器。

初版欄位建議：

| 分組 | 欄位 | 目前 consumer／理由 |
|---|---|---|
| `FlightOutput` | `altitude_asl`、`altitude_agl`、`position_world_z`、`mach`、`g_load`、`angle_of_attack`、`angle_of_slide`、`atmosphere_temperature` | CSV，以及 param engine temperature；只發布已經在 Core 使用或計算的飛行狀態，不複製完整 DCS input |
| `ForceMomentOutput` | `force`、`moment`、`center_of_mass` | `ed_fm_add_local_force`、`ed_fm_add_local_moment` 與 CSV |
| 每具 `EngineOutput` | `switch_on`、`throttle_input`、`throttle_output`、`power_readout`、`thrust_force`、`afterburner_ratio`、`afterburner_lit`、`nozzle_aperture` | draw args、param、carrier launch 與 CSV |
| `ControlOutput` | `pitch_input`、`roll_input`、`yaw_input`、`elevator_command`、`aileron_command`、`rudder_command`、`flaps_position`、`slats_position`、`airbrake_position` | draw args、param 與 CSV |
| `LandingGearOutput` | `gear_position`、`nose_wheel_steering`、`brake_left`、`brake_right`、三輪 `wheel_spin` | draw args、param 與 CSV |
| 每輪 `SuspensionWheelOutput` | `acting_force`、`compression`、`force_magnitude`、`weight_on_wheel` | 只發布 Core 目前使用或計算、且供 CSV debug 的 suspension 結果；完整 DCS raw sample 留在 `FrameInput`，是否已有 sample 由 `FrameDataAvailability::suspension` 表達，不新增 CSV validity 欄位 |
| `SuspensionOutput` | 三輪輸出、`any_weight_on_wheels`、`on_ground` | param 與 CSV |
| `FuelOutput` | `internal_fuel`、`external_fuel`、`total_fuel` | param 與 CSV；DCS fuel getter 仍走立即查詢的非 frame 介面 |
| top-level | `shake_amplitude` | `ed_fm_get_shake_amplitude` 與 CSV |

#### `FlightOutput` implementation-ready contract

前面的討論已確定資料範圍；這裡尚缺的是把模糊名稱與單位凍結成不需要實作者自行猜測的 contract。建議：

```cpp
struct FlightOutput
{
    double altitude_asl_m;
    double altitude_agl_m;
    double position_world_z_m;
    double mach;
    double g_load;
    double angle_of_attack_deg;
    double angle_of_slide_deg;
    double atmosphere_temperature_k;
};
```

- AoA/AoS 建議輸出 Core 已經計算並供 aerodynamics 使用的 `alpha`／`beta` degrees，不由 CSV writer 重新轉換；若要保留 DCS 原始 radians，欄位必須明確改名，不能使用沒有單位的 `angle_of_attack`。
- CSV 依上述順序使用 `flight_altitude_asl_m`、`flight_altitude_agl_m`、`flight_position_world_z_m`、`flight_mach`、`flight_g_load`、`flight_angle_of_attack_deg`、`flight_angle_of_slide_deg`、`flight_atmosphere_temperature_k`。
- Start row 時這些欄位都輸出 `-`；後續收到對應 continuous input 後才輸出數值。
- 後續 availability：ASL/temperature 需要 atmosphere；AGL 需要 atmosphere + surface；world Z 需要 world kinematics；Mach 需要 atmosphere + world kinematics；G/AoA/AoS 需要 body kinematics。

建議的 availability 不使用每個 numeric field 一個 `std::optional`，也不輸出額外 CSV validity columns，而是用固定來源類別：

```cpp
struct FrameDataAvailability
{
    bool atmosphere;
    bool surface;
    bool mass;
    bool world_kinematics;
    bool body_kinematics;
    std::array<bool, 3> suspension;
};
```

CSV writer 依來源類別決定 start/unavailable cells 是否輸出 `-`；`FrameOutput` 的 engine、controls、gear、fuel 等由 StartMode 決定的資料不受這些 flags 影響。

目前不建議放入：

- 完整 `AircraftState`、`Fck1cEfmSystems`、`FBWControllerState` 或 config。
- Suspension fallback probe、fallback force diagnosis 等已決定移除的 DCSBridge 診斷欄位；總 force 已經包含真正參與物理計算的結果。
- `MassDeltaResult`；它是只能消費一次的事件資料，不是可重複讀取的 frame state。
- Carrier launch phase；它是 `CarrierBridge` 自己的協定狀態。
- version/date；它們是 DCSBridge build metadata，不屬於 Core。

Debug Watch 不再是決定 `FrameOutput` 欄位的常駐 consumer。Developer-only 工具若日後啟用，只能使用當時既有的穩定輸出；暫時 debug 所需的 FBW 中間量不升級成永久公開介面。

### 2. Core 非 frame 操作 signatures

建議保留語意明確的函式，不建立 `apply(ExternalInput)`、通用 operation map 或一個包辦所有功能的大 request variant：

```cpp
class Fck1cEfm
{
public:
    FrameOutput start(StartMode mode);
    FrameOutput step(const FrameInput& input);
    void dispatch(const Command& command);

    MassDeltaResult take_mass_delta();

    void set_internal_fuel(double fuel);
    double internal_fuel() const;
    void set_external_fuel(const ExternalFuelInput& input);
    double external_fuel() const;
    void add_refueling_fuel(double fuel);

    void set_infinite_fuel(bool enabled);
    void set_easy_flight(bool enabled);
    void set_invincible(bool enabled);

    void apply_damage(const DamageEvent& event);
    void repair();
    void release();
};
```

分工與同步規則：

- DCSBridge 先把 raw DCS 資料驗證、翻譯成 typed input；invalid station/index/element 等資料在 boundary 記 ERROR，且不把無效資料送進 Core。
- 對已驗證且在 Core 內沒有合理失敗分支的 setter 使用 `void`。不為所有函式新增一個實際上永遠成功的通用 `OperationResult`；若某項操作日後真的有可恢復失敗，再為該操作定義專用 result。
- `set_internal_fuel`／`set_external_fuel`／`add_refueling_fuel` 是 DCS 明確要求的立即操作；對應 getter 在同一把 Core execution mutex 下直接讀 Core，因此 set 後立刻 get 可得到新值。
- Param、draw args、shake、carrier 及 CSV 只讀 `OutputStore`；即使 fuel、damage、gameplay 或 repair 剛更新 Core，也等下一次成功 step 才看到新 frame output。
- `take_mass_delta()` 是 consume-once 操作，不能改成讀取 `FrameOutput`，否則 DCS 重複 callback 會重複取得同一筆 mass change。
- 三個 gameplay callbacks 各自保留明確 bool setter；把它們合成 `GameplayOptions` 反而需要 read-modify-write，容易把未包含的設定蓋掉。
- `apply_damage` 接受 DCSBridge 已映射完成的 `DamageEvent`；DCS element ID 不進入 Core。
- `repair()` 立即修改 Core 狀態，但已發布 output 等下一 step 更新。
- `release()` 與 step、command、fuel 等所有 Core 操作共用 execution mutex；完成後 DCSBridge 將 `OutputStore` 標為 released/invalid。CSV writer 不隨它停止。
- `config()`、`snapshot()`、`force_moment_output()`、`shake_amplitude()`、continuous setters、`max_dry_thrust_at()` 都不再是 Core 對 DCSBridge 的公開介面。Carrier 需要的固定 launch thrust 由 composition root 從既有 config 提供給 `CarrierBridge`。
- `ed_fm_configure` 仍由 DCSBridge 處理模組路徑與 bridge 工具；Config 架構本輪不重構，也不讓 Core 反向依賴 DCSBridge。

### 3. DCS callbacks 的 neutral return

Neutral return 只用在 DCS ABI 必須得到回傳值、但沒有可用結果時。它不能取代 ERROR：若狀況違反我們認定的 callback/lifecycle contract，必須先把 callback 名稱、index/ID 與狀態寫進 EventLog。

| 情況 | 建議回應 | 是否 ERROR |
|---|---|---|
| force／moment callback 在 start 前或 release 後讀 output | 所有 force、moment、position 寫 `0` | 是；避免把未初始化或舊資料送回 DCS |
| active flight 的 draw args callback | 從 `OutputStore` 讀取上一份完整 output，更新本模組擁有的 draw args | 否；Core step 期間舊 output 仍保持有效，不存在暫時空窗 |
| start 前或 release 後仍收到 draw args callback | 不修改 DCS 傳入的 draw args，直接 return | 是；這是 lifecycle contract 違反，不製造一份假的 neutral visual state |
| draw args buffer 是 null 或 `size` 不足以容納最大使用 index | 不寫 buffer，直接 return | 是，記錄 pointer 狀態、實際 size、要求 size |
| `ed_fm_get_param` 查詢本模組明確支援的 index | 從最新 output 計算並回傳 | 否 |
| 查詢不存在或未映射的 param index | 回傳 `0.0` | 是；DCS 不會主動遍歷查詢，實際 callback 代表已有 consumer 要求該 param，記錄 index |
| 已映射的 param 卻缺少必要資料 | 回傳 `0.0` | 是，記錄 index 與缺少的資料類別 |
| fuel getter 在 release 後呼叫 | 回傳 `0.0` | 是 |
| shake callback 沒有有效 output | 回傳 `0.0` | start 前／release 後是 ERROR |
| `ed_fm_change_mass` 沒有 pending delta | 將輸出 references 清為 `0`，回傳 `false` | 否，這是正常的「沒有下一筆」 |
| force/moment component callbacks 未實作 component 模式 | 輸出 references 清為 `0`，回傳 `false` | 否，這是固定能力選擇 |
| carrier pop 沒有 event | 清空 `out`，回傳 `false` | 否 |
| carrier pop 在 release 後被呼叫 | 清空 `out`，回傳 `false` | 是 |
| Production build 的 Debug Watch 未啟用 | `ed_fm_enable_debug_info()` 回傳 `false`；可寫 buffer 清空，`ed_fm_debug_watch()` 回傳 `0` | 否，這是明確的 capability 選擇 |
| Developer-only Debug Watch 已啟用，但 buffer 無效或 release 後被讀取 | 回傳 `0`；buffer 可寫時清空 | 是，記錄 buffer/lifecycle 狀態 |
| invalid suspension index 或 null info | 不更新該輪 latest sample，Core 仍可用既有/default sample 執行 step | 是，記錄 index 與 null 狀態 |
| unknown command ID | 不 dispatch，step 可繼續 | 是，記錄 ID 與 value |
| Cockpit 必要 parameter 缺失 | 本次使用已定義的 neutral input，step 可繼續 | 是，並在恢復時記 recovery |
| `ed_fm_simulate` 收到非有限值或 `dt <= 0` | 不執行 Core step，保留最後 output | 是；此情況理論上不應發生，用假 dt 繼續會破壞 simulation time 與積分結果 |
| 除新一次 start／configure 外，callback 在 release 後嘗試讀寫 Core | 不進入 Core；有 return 的回 neutral，void callback 直接 return | 是；下一次合法 start 會重新建立 active flight state |

「回傳 `false`」有兩種完全不同的意思，必須在程式碼與測試中分清楚：沒有下一筆資料／未提供 component 模式是正常協定結果；release 後或 invalid lifecycle 才是錯誤。正常的 `false` 不寫 ERROR。

依本專案實際使用方式，DCS 不會主動遍歷 `ed_fm_param_enum`。因此只要 `ed_fm_get_param(index)` 確實被呼叫，就代表已有 Lua 儀表或其他 consumer 要求該 index；mapping 不存在或資料無法產生都視為 ERROR。Neutral `0.0` 只用來滿足 C ABI，不能讓錯誤保持沉默。本輪不先加入 log 去重；若同一錯誤被高頻重複呼叫，完整重複記錄可以直接暴露錯誤頻率，之後再依實測決定是否需要明確的抑制策略。

Param lookup 的內部結果不能只回傳 `double`，因為 `0.0` 可能是合法值。已確定使用 `std::optional<double>`：有值代表成功，`std::nullopt` 代表 mapping 不存在，不使用 nullable pointer 或 NaN sentinel。`EfmExports.cpp`／param adapter 在沒有 value 時記錄包含 index 的 ERROR，再向 DCS 回傳 neutral `0.0`。

目前 EFM 與 test 的 Visual Studio projects 使用 v142 toolset，但沒有明確設定 `<LanguageStandard>`。本輪要在兩個 projects 都加入 C++17 language standard 設定，避免依賴開發者機器的預設值。

## 明確移除的舊設計

- `DcsRuntime`。
- `Fck1cEfmRuntime` 與 Core 對 DCSBridge 的全部反向 callbacks。
- `on_first_frame`。
- `on_engine_shutdown`；engine shutdown 是 Core 內部狀態變化，以 CSV 前後資料觀察。
- `on_thrust_updated`。
- `on_ground_diagnostics`。
- `on_release` reverse callback。
- 完整 production `Fck1cEfmSnapshot` 與 DCSBridge 對完整 Core implementation state 的依賴。
- DCSBridge suspension diagnostics 與 `fck_susp_debug.log`。
- 每個 callback 打開、寫入、關閉一次檔案的做法。

## 測試策略

測試重點是輸入與可觀察輸出，不測私有函式、mutex 實作或不必要的 class 結構。

### 自動測試

- 對不同 StartMode 輸入，驗證初始 `FrameOutput`。
- 對 `FrameInput + dt` 執行 step，驗證 force、moment、engine、fuel、controls、gear、suspension、shake 等必要 `FrameOutput`。
- 驗證 `dt` 使用 DCS 實際輸入，不假定 0.001。
- 驗證 command table 的 PassThrough、Constant、PressOnly 與 release ignored。
- 驗證 Cockpit parameter 缺失會產生可追蹤錯誤、使用 neutral input 並允許 step；恢復後產生 recovery。
- 驗證 suspension 初始值、完整 sample 更新與 sticky latest-known 行為。
- 驗證 concurrent input update/read 只得到完整舊值或完整新值。
- 驗證 concurrent OutputStore publish/read 只得到完整舊 frame 或完整新 frame。
- 驗證 step 與 release 被序列化，release 後讀取會明確報錯。
- 驗證 EventLog 行格式、simulation time、必要錯誤參數、路徑與 rotation。
- 驗證 CSV header、欄位順序、單位、精度、`True`／`False`、向量展開、unavailable `-`、sequence reset 與多 flight append。
- 暫停 CSV writer 後連續發布多筆 record，驗證 writer 只寫最新資料且 sequence 出現跳號。
- 改寫現有依賴 `Fck1cEfmSnapshot`／`TestRuntime` 的測試，使其改驗證新介面的輸入輸出。

### DCS 內手動／整合驗證

- 實測 `ed_fm_simulate` 的 `dt` 與 callback cadence，不把觀察值升級成固定契約。
- 驗證 DCS callbacks 的實際並行與順序不會破壞 collector、Core 或 OutputStore。
- 驗證外部程式能在 DCS 執行中持續讀取 `.log` 與 `.csv`。
- 驗證 suspension callback 提供的 compression 與 force 在實機中的語意與單位。
- 驗證 cold、hot ground、hot air、release 與同 execution 多次飛行。
- 驗證 CSV writer 落後時飛行運算不被檔案 I/O 阻塞，且 sequence 跳號可見。

## 實作準備狀態

目前沒有尚未回答、會阻擋架構實作的問題。

- CSV 的逐欄 header 與順序由最終 `FrameOutput` 宣告機械式展開，並以 schema 測試鎖定；這是實作工作，不再建立另一份人工挑選清單。
- Suspension 的座標系與單位已依 DCS reference 固定；`acting_force_point`、`integrity_factor` 與 `wheel_speed_X` 目前只保留在完整 `FrameInput`，沒有 output consumer，因此不阻擋本輪，也不加入 `FrameOutput`／CSV。
- 效能與 DCS callback cadence 在實作後用整合測試確認；真正需要觀察的是 `simulate` 未被檔案 I/O 阻塞，以及 CSV 落後時可由 sequence 跳號看見。
- 實作與 commit 順序已列在下一節，等待審核。
- 若實作時發現現有程式碼或 SDK 與本文件矛盾，必須帶著具體檔案、symbol 與影響回來討論，不預先為假想情況增加 abstraction 或 fallback。

## 實作與 commit 計畫（待審核）
### 共通執行規則
- 除「實作前基準」外，一個編號原則上對應一個 commit；若該步實際 diff 過大，只能沿該步已寫明的責任邊界再拆小，不能把後續責任提前混入。
- 每個 commit 完成後都要重建並執行 native tests；DLL 一律透過專案的 `tools/build_dll.ps1` 重建與複製。測試失敗、腳本失敗或 export surface 改變時，不進入下一步。
- 測試只驗證輸入與可觀察輸出，例如 Core result、callback return、log／CSV 內容與 lifecycle；不測 private helper、mutex 型別或 class 數量。
- 本輪不順手修改飛行公式、控制律、Config/FM Data 架構或 Lua。
- DLL export 名稱與 signatures 以重構前產物為基準；每次整理入口檔後都比較 export 清單，不新增 SDK optional exports。

### 0. 實作前基準（不建立 commit）
- 從乾淨 source 重建 Release/x64 tests，並以 `tools/build_dll.ps1` 重建 DLL，保存測試數量、結果、DLL export 清單及腳本輸出的 SHA256。
- 執行現有 cold、hot ground、hot air、simulate、force/moment、draw args、param、fuel、mass delta、suspension 與 carrier 測試，確認重構前基準。
- 現有已編譯 test binary 的基準是 443 checks、0 failures；正式開始實作前仍須用當時 source 重新 build，不能只依賴舊 binary。

### 1. 明確啟用 C++17
- EFM DLL 與 native test projects 都設定 ISO C++17，保留 Release/x64、v142 與 `/MT`。
- 不修改任何執行邏輯。
- 驗收：tests 與 DLL 重新 build，export 清單不變。

### 2. 建立 Core frame contracts，但暫不切換 production data flow
- 在 Core 定義 `StartMode`、完整 `FrameInput`、`FrameDataAvailability`、`FrameOutput` 及其固定分組。
- `FrameInput` 納入所有 DCS 已提供的 atmosphere、surface、mass、world/body kinematics、三輪完整 suspension、autopilot、max-power 與實際 `dt`，即使部分欄位目前尚未被公式使用。
- `FrameOutput` 只包含已確認的 DCS/CSV consumer 所需輸出，不包含 logger、CSV sequence 或完整 implementation snapshot。
- 先增加由現有 Core state 產生 `FrameOutput` 的單一 projection；既有 production callbacks 暫時仍走舊路徑，方便比較新舊結果。
- 步驟 2 暫時以 `frame_output(const FrameDataAvailability&) const` 暴露唯讀 projection 供新舊輸出比較；它不推進 Core，也不接上 production callbacks。步驟 7 的 `start()`／`step()` 接管發布後，projection 轉為其內部實作，不保留第二條公開輸出路徑。
- 驗收：用相同 start/input 驗證新 `FrameOutput` 與既有可觀察輸出一致，並驗證所有 StartMode 初始結果。

### 3. 把 autopilot 與 max-power 讀取移出 Core
- DCSBridge 在每次 simulate 前主動讀取兩組 Cockpit parameter，轉成 typed input 後傳給 Core。
- Core 的 autopilot、engine 與 thrust 更新函式改為接收明確輸入，不再呼叫 `runtime.read_autopilot()` 或 `runtime.read_max_power()`。
- Cockpit parameter 缺失暫時沿用既有可觀察處理；正式 EventLog 錯誤格式在後續步驟統一。
- 驗收：相同 cockpit input 產生相同 controls、engine power 與 thrust output；缺失 input 使用 neutral value且 step 仍完成。

### 4. 移除 Core 反向通知與舊 Debug Watch／suspension diagnostics
- 移除 `on_first_frame`、`on_engine_shutdown`、`on_thrust_updated`、`on_ground_diagnostics`、`on_release` reverse callbacks。
- 移除 startup/periodic/ground suspension probe、`fck_susp_debug.log`、舊 Debug Watch formatter 與其完整 snapshot 依賴。
- Production Debug Watch exports 保留，但固定回報未啟用；不新增替代 debug framework。
- Engine shutdown 不建立事件 callback，之後由每 step CSV 前後狀態觀察。
- 驗收：Core 的 start/step/release 輸出不因移除 diagnostics 改變；Debug Watch ABI return 符合既定規則。

### 5. 移除 `Fck1cEfmRuntime` 注入
- 刪除 Core runtime interface、reference member 與 constructor parameter。
- Core construction 只需要 aircraft config；Core headers 不再 include 或 forward declare DCSBridge/runtime 型別。
- 更新既有 tests，移除 `TestRuntime`／`NullRuntime`，直接從 Core 輸入與輸出驗證行為。
- 驗收：以 dependency search 確認 Core 不再依賴 DCSBridge；全部 Core tests 通過。

### 6. 建立 `FrameInputCollector`
- 在 DCSBridge/Internal 新增 thread-safe latest-known collector；每個 DCS callback 先在 local variable 完成驗證與翻譯，再一次發布整類 sample。
- StartMode reset 建立既定預設值與 availability；suspension 保存三輪完整 raw sample，未更新的 step 沿用上一筆。
- Collector snapshot 不強制不同 callback 分類對齊，只保證每一分類不會出現 torn value。
- 驗收：測試完整 sample publish/read、sticky suspension、reset、缺失分類及並行讀寫只得到完整舊值或完整新值。

### 7. 將 Core 入口切換為 `start()`／`step()`
- `start(StartMode)` 建立初始 state、將 simulation time 設為 0，並回傳初始 `FrameOutput`。
- `step(const FrameInput&)` 套用最新輸入、使用實際 `dt` 完成一次既有計算，再回傳完整 `FrameOutput`。
- DCS continuous callbacks 改為只更新 collector；`ed_fm_simulate` snapshot collector 後呼叫一次 Core step。
- 刪除 Core 對外的 continuous setters、舊 `simulate(double)` 及 cold/hot 三套重複入口。
- 驗收：用相同 callback input 重播，比較重構前後 force、moment、engine、fuel、controls、gear、suspension 與 shake；invalid dt 明確跳過且不發布 output。

### 8. 建立並接上 `OutputStore`
- Start 與每個成功 step 將 immutable `FrameOutput` 一次發布到 thread-safe latest store。
- Force/moment、draw args、param 與 shake callbacks 全部只讀 `OutputStore`；Core 計算中可以繼續讀上一個完整 frame。
- Fuel getter 與 consume-once mass delta 保留直接且序列化的 Core query；release 使 store invalid。
- 驗收：測試 start 前、active flight、Core step 中、release 後與下一次 start 的 callback 行為，以及並行 publish/read 不出現半新半舊 frame。

### 9. 實作 `EventLog`
- 實作 `<module-root>/log/fck1c_efm.log` 的目錄建立、active/`.old` 輪替、其他 app 可同時讀取的 share mode、thread-safe write 與每事件 flush。
- 固定格式為 `[wall-clock][simulation-time][LEVEL] message`；沒有 simulation time 時使用 `[-]`。
- 只加入既定 lifecycle、INFO 與 ERROR；不把飛機數值寫成 INFO，不留下 DEBUG instrumentation。
- 驗收：以 temporary directory 驗證路徑、輪替、格式、simulation time、必要錯誤參數與寫入期間可讀。

### 10. 實作 `StateCsvWriter`
- 實作完整 `FrameOutput` schema、固定 header 順序、已知單位 suffix、`True`／`False`、vector `x/y/z` 展開、unavailable `-` 與 round-trip 浮點格式。
- 實作 `<module-root>/log/fck1c_state.csv` 與 `.old` 輪替、header 立即 flush、約 100 ms flush，以及允許其他 app 同時讀取的 share mode。
- 實作容量一筆的 latest-only mailbox；新 record 覆蓋尚未開始處理的舊 record，不阻塞 simulate，以 sequence 跳號顯示遺失。
- 驗收：測試完整 schema、start row、每成功 step record、sequence reset、多 flight append、latest-only 覆蓋、外部讀取與完整 row 寫入。

### 11. 把剩餘 DCS 工具從 `DcsRuntime` 拆出
- 將 Cockpit handle/parameter 讀取整理為精簡 `CockpitBridge`；只負責 DCS Cockpit API 與 typed values。
- 將 carrier push/pop state 整理為精簡 `CarrierBridge`；保留既有事件與 reference thrust 行為。
- `ModulePaths` 保留 DCSBridge 路徑責任；ConfigReader 只保留本輪仍被實際使用的內容。
- 驗收：沿用與改寫現有 param、simulation event 與 start tests，確認工具拆分不改變 callback output。

### 12. 建立 execution-level `BridgeContext`
- `BridgeContext` 成為唯一 production ownership/composition root，持有 ModulePaths、EventLog、StateCsvWriter、FrameInputCollector、OutputStore、CockpitBridge、CarrierBridge 與 Core。
- 第一個 exported callback 先完成 once-only context 初始化；`ed_fm_configure` 若為第一個 callback，直接使用其 `cfg_path`，否則由 DLL path 回推 module root。
- 初始化先開 EventLog，再準備 CSV/writer，最後才建立並處理其他 operational objects；所有 Core 操作用同一把 execution mutex 序列化。
- Context 與 writer 採 process lifetime；不在 `DllMain` 做 I/O、啟動或 join thread，也不隨 `ed_fm_release` 重建。
- Start 與每個成功 step 同時把帶 sequence 的最新 record 交給 CSV writer；callback 不等待格式化、write 或 flush。
- 驗收：測試 first callback initialization、同 execution 多 flight、release/start、並行 callback，以及 EventLog/CSV 在 flight start 前已可用。

### 13. 統一 command、param 與 numeric boundary errors
- Command binding 固定為單一 table 與 `PassThrough`、`Constant`、`PressOnly`；PressOnly release 不 dispatch，也不保存 pressed state。
- Unknown command、重複 binding、invalid index/pointer 與 NaN/Infinity 按已定規則記錄 ERROR；完整 callback sample 無效時整筆拒絕並保留上一筆。
- Param lookup 改為 `std::optional<double>`；不存在時 EfmExports 記錄包含 index 的 ERROR，再向 DCS 回 neutral `0.0`。
- 驗收：逐一驗證正常 mapping、PressOnly press/release、unknown ID、missing param、invalid sample 與 recovery 的外部結果和 log。

### 14. 刪除 legacy ownership 與 snapshot 路徑
- EfmExports 全面改用 `BridgeContext`，再刪除 `DcsModule`、`DcsRuntime`、`DcsSnapshots`、`Fck1cEfmSnapshot` 與已無 caller 的 diagnostics/runtime helpers。
- 確認 Core 只有 typed input/command/output 與少數具明確立即語意的操作；DCSBridge 不再讀完整 Core implementation state。
- 驗收：dependency search 不再找到 legacy types；tests 與 DLL build 通過，exports 不變。

### 15. 完成目錄、入口與 contributor 文件
- 將 DCSBridge 實作依既定結構收進 `Internal`；不為小型 helper 強迫新增一檔一類。
- 將現有 callback implementation 整理為 `EfmExports.cpp` composition root；它負責驗證、翻譯、協調工具與 ABI return，不承擔物理計算或檔案格式化。
- 更新 Visual Studio projects 與 DCSBridge README，說明入口、資料流、ownership、如何新增 command/FrameOutput/CSV 欄位，以及哪些 headers 不提供外部 include。
- 驗收：新貢獻者只看 README、EfmExports 與 Core frame contracts 即可追蹤主要資料流；所有 project references 正常。

### 16. 最終自動與 DCS 整合驗證
- 重建並執行完整 native tests 與 DLL，比較重構前後 export 清單；在 DCS 驗證 cold、hot ground、hot air、simulate cadence、force/moment、draw args、param、fuel、mass delta、suspension、carrier、release 與同 execution 第二次 start。
- 執行時用外部程式持續讀取 active log/CSV，確認 EventLog 即時可見、CSV 約 100 ms 可見、row 完整，且 writer 落後時 simulate 不等待、sequence 會跳號；比較重構前後相同輸入下的飛行輸出，任何差異都不能用放寬 tolerance 掩蓋。

## 重構後另案討論
- Config／FM Data 的責任與檔案結構，以及是否把整台飛機 state management 抽成獨立 Core module。
- 完成 DCS 實測後重新審視 autopilot、max-power、suspension 與 `FrameOutput` 的分組是否仍合理。
