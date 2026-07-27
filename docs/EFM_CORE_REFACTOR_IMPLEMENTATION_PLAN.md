# F-CK-1C EFM Core 重構實作計畫

## 結論

目標架構可以實作，但原計畫不是完全沒有問題。審查現有 Core、DcsBridge、
建置檔與 native tests 後，已在架構文件補正五個會影響正確性的缺口：

1. `release()` 後到下一次 `start()` 前的燃油與選項沒有 owner。
2. 「首次 DCS sample 缺失即失敗」與現有 sticky input 行為衝突。
3. 分組提交會改變 gear-to-flap 與 engine-to-fuel 的同幀時序。
4. Simulation Models 是否能使用前一個 Model 的結果沒有定義。
5. Command、Event、Config 的 ownership 與接收者數量沒有完全定義。
6. System 如何取得 start mode 並建立可立即發布的初始值沒有定義。
7. `take_mass_delta()` 會在 `step()` 外改寫 Fuel 暫態，而且跨幀未讀取的
   delta 可能被下一幀覆蓋。

補正後沒有發現阻止重構的架構矛盾。剩餘問題是局部 C++ 表示方式與
MSBuild 掛載方式，可以在不改變責任邊界的情況下用測試決定。

本文件是施工清單；架構邊界與原因以
[`EFM_CORE_ARCHITECTURE_PLAN.md`](EFM_CORE_ARCHITECTURE_PLAN.md) 為準。

## 重構期間不得改變的內容

除了下列四個已隔離的語意修正外，每個步驟都必須保留：

- 步驟 15：無敵時的 damage event 不留下潛伏的 segment damage。
- 步驟 16：分組提交帶來已確認的一幀訊號延遲。
- 步驟 19：有 DCS suspension feedback 時不再額外累加 fallback ground
  force。
- 步驟 21：mass delta 改由 DcsBridge 保留至 DCS 實際讀取，不再因跨幀
  未輪詢而遺失。

其餘不得改變：

- DCS C ABI export 名稱與 callback 行為。
- DcsBridge 的 raw ID、單位與座標轉換責任。
- 現有空氣動力、推力、地面力與質量公式及參數。
- cold、hot ground、hot air start 的結果。
- command 在 `step()` 外只更新離散請求，不推進連續狀態。
- 缺少本幀 callback 時保留上一份 observation 的行為。
- 外部永遠只能讀到完整完成的 frame。

本次不新增 cockpit、液壓/電氣網路、訊號延遲、unavailable、多 writer、
任意型別 registry 或通用物理 plugin framework。

## 共通驗證

每一個 code commit 都要通過下列檢查，不能等到重構結束才一起測：

1. `git diff --check`
2. Release x64 native test project 於 60 秒內完成建置。
3. `F-CK-1C_EFM_Tests.exe` 於 60 秒內全數通過。
4. `tools/build_dll.ps1` 成功建立 DLL。
5. DLL export 名稱清單與步驟 1 保存的 baseline 相同。
6. 該步驟列出的 focused tests 通過。

純文件 commit 不需要重建 DLL，但仍需 `git diff --check` 與連結檢查。
如果一個步驟無法保持上述綠燈，該步驟必須再切小，不得把失敗帶到下一步。

## Phase 0：鎖定現有行為

### 1. 建立可重複的 baseline

變更：

- 保存 DLL export 名稱 baseline，忽略 RVA 與位址。
- 增加多幀 characterization tests，而不改 production code。
- 同一輸入序列重跑兩次，確認每幀輸出完全一致。
- 記錄目前已知的一幀內執行時序。

必要情境：

- command → FBW → control surfaces。
- gear transition → flaps/slats。
- engine spool/fuel flow → fuel consumption。
- DCS suspension feedback 與 fallback ground force。
- damage、invincible、repair。
- 無敵期間 damage 是否留下稍後才生效的潛伏狀態。
- release → preparation setters → start。
- 缺少本幀 atmosphere、surface、body state 等 sample。
- 有 suspension feedback 時目前是否仍同時產生 fallback ground force。
- DCS 未每幀讀取 mass delta 時，現在會保留或覆蓋哪些值。

驗證：

- 所有舊測試不修改預期值仍通過。
- 新增的多幀 golden 測試通過。
- 兩次相同執行逐幀相同。
- DLL export baseline 可由乾淨建置重建並比較。

停止條件：

- 現有測試若不是全綠，先診斷原本問題，不開始搬移。
- 無法用測試描述的既有行為，先補足觀測點，不以推測代替。

執行結果記錄於
[`EFM_CORE_PHASE0_BASELINE.md`](EFM_CORE_PHASE0_BASELINE.md)。

## Phase 1：先建立穩定邊界

### 2. 搬移 Core frame contracts

變更：

- 將 Core/DcsBridge 交換型別搬到 `Core/Contracts/FrameContracts.h`。
- 只改 include 與 project source；不改欄位、預設值或轉換。

驗證：

- FrameInputCollector 與 OutputStore 測試全數通過。
- 完整單幀與多幀 golden 不變。
- DcsBridge 不新增對具體 System 的 include。
- 共通驗證全數通過。

### 3. 建立 semantic Commands 與 Events

變更：

- 將 Core command/event 型別移入 `Core/Contracts/`。
- raw DCS command ID 與 damage area ID 保留在 DcsBridge。
- 先保持現有中央 dispatch；本步只建立語言與邊界。

驗證：

- 每一個 raw command 都映射到預期 semantic Command ID。
- 未知 raw ID 的既有行為不變。
- damage area 映射到預期 semantic component/area。
- Core Contracts 不 include DCS headers。
- 共通驗證全數通過。

### 4. 建立一次飛行的 `AircraftSimulation`

變更：

- 將目前的 per-flight 狀態與 frame orchestration 搬入
  `Simulation/AircraftSimulation`。
- `Fck1cEfm` 只保留穩定 façade 與目前飛行 instance。
- 本步仍依現有順序呼叫邏輯，不導入分組時序變化。

驗證：

- cold、hot ground、hot air start 結果與 baseline 相同。
- 未先 `release()` 的重複 `start()` 仍建立全新飛行。
- `release()` 後沒有 active simulation。
- 多次 start/release loop 不殘留上一飛行暫態。
- 每個 System 由 immutable FlightSetupContext 取得相同 start mode。
- start 回傳前，所有必要 initial publications 已統一提交。
- BridgeContext process-lifetime reuse 測試通過。
- 共通驗證全數通過。

### 5. 補上 `FlightPreparation`

變更：

- `Fck1cEfm` 私有保存下一次飛行需要的燃油、外部油箱與模擬選項。
- setter 在無 active flight 時更新 preparation；有 active flight 時也
  更新目前模擬。
- start/release 依架構文件同步應延續的資料。

驗證：

- release 後設定燃油，再 start 時數值正確。
- active flight 中設定燃油後，當前與下次飛行都符合既有規則。
- infinite fuel、invincible、easy flight 跨生命週期行為不變。
- force、moment、frame counter、單次 mass delta 不會跨飛行。
- release 後的普通 command、damage 與 repair event 不保留到下一飛行。
- 共通驗證全數通過。

## Phase 2：建立 System 深模組

### 6. 建立最小 `SystemPipeline` 與測試用 Systems

變更：

- 建立 `System.h`、`SystemPipeline`、明確 typed AircraftData schema。
- 實作 setup read/publish 宣告、單一 writer 驗證、兩組快照與
  pending/committed 分離。
- Setup 先收集全部宣告，再統一驗證並提交初值；不得按 Entry 順序邊
  setup 邊讀值。
- 先只用測試 System 驗證，不接入 production 飛機邏輯。

驗證：

- 缺少 provider、型別不符、未初始化必要資料會明確 setup 失敗。
- 同一 key 重複 writer 會明確 setup 失敗。
- 同一組看不到同組未提交資料。
- Equipment 看得到剛提交的 Control 結果。
- 沒有新值時保留上次提交值。
- pending storage 重複使用時不洩漏上一組未提交資料。
- catalog 順序不同時，相同組的結果仍相同。
- 反轉 setup 順序時初值與驗證結果不變。
- start 前缺少必要 initial publication 時明確失敗。
- 共通驗證全數通過。

### 7. 建立 command/event handler 註冊

變更：

- System 在 setup 註冊自己擁有的 handler。
- Command 採唯一接收者。
- Damage 依 semantic owner 路由；Repair 允許多個訂閱者。
- 暫不建立獨立 Router module。

驗證：

- 重複 Command ID 註冊會 setup 失敗。
- 未註冊 command 的錯誤/忽略行為有明確測試並保持現有外部契約。
- command handler 不會在 `step()` 前推進連續狀態。
- damage 只到正確 owner；repair 到所有已註冊 repairable owners。
- handler 發生錯誤時不會發布半完成 frame，且錯誤可見。
- 共通驗證全數通過。

### 8. 建立 build-time catalog

變更：

- 由 MSBuild target 在編譯前掃描 `Systems/*/Entry.cpp`。
- generated catalog 輸出到 project intermediate directory。
- production 與 native test project 共用同一份 Core source/build 規則。
- `tools/build_dll.ps1` 的使用方式保持不變。
- 產生器必須支援尚無 production Entry 的空 catalog，並以 fixture 測試
  非空情況。

驗證：

- 刪除 intermediate directory 後可乾淨重建。
- 連續建置不會產生不必要或不確定的 catalog 差異。
- 測試 fixture 新增/移除一個 Entry 時，catalog 數量自動改變。
- 新增 Entry 不需手改 `Fck1cEfm`、`AircraftSimulation` 或
  `SystemPipeline`。
- 兩個 `.vcxproj` 不再各自維護不同的 Core source 清單。
- 共通驗證全數通過。

## Phase 3：逐一抽取飛機 Systems

本階段每次只搬一個 owner。Production 暫時維持原本明確呼叫順序；
不得同時啟用新的分組提交語意。每個新 System 只有一份權威狀態，不能
保留新舊兩份會同時執行的實作。

為避免建立一套臨時 scheduler，`AircraftSimulation` 在本階段可以明確
直接持有並依舊順序呼叫已抽取的 concrete class；這是可搜尋、會在步驟
16 一次刪除的遷移依賴。Entry 與 setup 介面同時接受 unit test，但
production 不得再執行一份舊函式作為 fallback。

### 9. 抽取 `FlightControlComputer`

變更：

- 搬移 InputSystem 的 Core 邏輯、trim、FBW 與 control demand。
- command 透過 setup handler 註冊。
- 新增第一個 production `FlightControlComputer/Entry.cpp`。

驗證：

- InputSystem tests 與所有 FBW snapshot tests 不變。
- neutral input、trim 與 command mapping 不變。
- 多幀 control demand golden 不變。
- catalog 會自動包含 FlightControlComputer，不需修改中央 source list。
- 共通驗證全數通過。

### 10. 抽取 `PrimaryFlightControls`

變更：

- 搬移 elevator、aileron、rudder 的實際位置與 actuator state。
- 只讀取 FCC 發布的 demand，不持有 FCC。

驗證：

- actuator rate、上下限與 neutral behavior 不變。
- FBW demand 到 control surface 的多幀結果不變。
- System 沒有 include 或 pointer 指向 FCC concrete class。
- 共通驗證全數通過。

### 11. 抽取 `SecondaryFlightControls`

變更：

- 搬移 flaps、slats、airbrake 的設備狀態與命令處理。
- 此時仍保留 baseline 的 gear-to-flap 同幀時序。

驗證：

- AirframeDevice 既有測試不變。
- gear transition、flap schedule、airbrake 的多幀 golden 不變。
- System 只讀 AircraftData，不 include LandingGear。
- 共通驗證全數通過。

### 12. 抽取 `LandingGear`

變更：

- 搬移 gear、brakes、NWS、wheel、tire 與 suspension 設備狀態。
- 地面力公式暫時留在原物理位置。

驗證：

- LandingGear 與 Suspension tests 不變。
- DCS suspension feedback、draw argument、brake/NWS 結果不變。
- LandingGear 不產生回傳 DCS 的 ground force。
- 共通驗證全數通過。

### 13. 抽取 `Engine`

變更：

- 搬移 engine switch、combustion、spool、nozzle、fuel flow 與 condition。
- 推力公式暫時留在原物理位置。

驗證：

- EngineSystem 既有測試不變。
- start/spool/afterburner/max-power 與 fuel-flow 多幀結果不變。
- damage 對 engine state 的結果不變。
- Engine 不直接累加 force/moment。
- 共通驗證全數通過。

### 14. 抽取 `Fuel`

變更：

- 搬移內外油箱、油量、供油與 fuel transfer。
- mass effect 暫時留在原物理位置。

驗證：

- 內部與各外部 station 的設定、取得與消耗不變。
- infinite fuel 不改變設備存在，只阻止消耗套用。
- engine fuel flow 到 consumption 的多幀時序仍與 baseline 相同。
- Fuel 不直接建立 DCS mass callback 結果。
- 共通驗證全數通過。

### 15. 抽取 `AirframeStructure` 並移除 `DamageModel`

變更：

- 結構 integrity 搬到 AirframeStructure。
- engine 與 landing gear damage 分別由其 owner 保存。
- `LandingGear` 使用單一 semantic area 與 nose、left main、right main
  三個 segment；DcsBridge 將 `WHEEL_F`、`WHEEL_L`、`WHEEL_R` 映射到
  對應 segment。
- nose integrity 按比例影響 NWS；left/right main integrity 分別按比例
  影響對應煞車。本階段不新增爆胎、支柱折斷或收放卡死。
- repair 透過 event fan-out；invincible 在事件進入 Systems 前處理。

驗證：

- damage 會實際改變對應設備狀態或其後的物理效果。
- damage 不會誤傷非 owner。
- 三個起落架 segment 只影響各自的 NWS 或煞車能力。
- repair 恢復所有已損壞且可修復部件。
- invincible 阻止 damage，且關閉 invincible 後不會出現先前被忽略的
  潛伏損壞；這項預期只在本 commit 更新。
- invincible 不移除 integrity channels。
- 舊 `DamageModel` 無剩餘 production caller。
- 共通驗證全數通過。

## Phase 4：一次切換 System 時序

### 16. 啟用 Control/Equipment 分組提交

變更：

- `SystemPipeline` 成為唯一 System 排程與提交位置。
- 刪除 `AircraftSimulation` 中的具體 System 呼叫與暫時遷移順序。
- Control 統一提交後才執行 Equipment；Equipment 同組讀同一快照。

這是整個重構唯一預期改變 System 時序的 commit。

驗證：

- 同組 Entry 順序互換時輸出不變。
- FCC 的本幀結果可供 Equipment 使用。
- SecondaryFlightControls 讀到上一個已提交 gear state。
- Fuel 讀到上一個已提交 engine fuel-flow state。
- 所有 group 讀取本幀最新 DCS-owned observation；已批准高速開始時 NWS
  第一幀由舊 baseline 的短暫轉向改為立即歸零。這不是 System 間提交時序，
  而是外部 observation freshness 的修正。
- before/after 多幀 diff 只能出現已批准的一幀時序差異。
- 所有非時序數值、限制、公式與 DCS exports 不變。
- 更新 golden 預期只能發生在此 commit，並在測試名稱說明原因。
- 共通驗證全數通過。

停止條件：

- 若 diff 出現非預期數值差、累積漂移或其他 callback 變化，退回此
  commit 診斷；不能用放寬 tolerance 掩蓋。

## Phase 5：逐一抽取 Simulation Models

### 17. 抽取 `AerodynamicsModel`

變更：

- 搬移空氣動力 force/moment 公式、表格與其專屬 config。
- 不更改座標、單位、插值或 clamp。

驗證：

- AerodynamicsSystem 既有 unit tests 不變。
- 代表性迎角、側滑角、速度與控制面組合的 force/moment golden 不變。
- easy-flight 對氣動的既有效果不變。
- 共通驗證全數通過。

### 18. 抽取 `PropulsionModel`

變更：

- 根據完成的 Engine snapshot 計算 thrust effect。
- Engine 保留設備狀態，不再擁有物理累加。

驗證：

- idle、military、afterburner、max-power 推力結果不變。
- engine damage/condition 對推力的效果不變。
- carrier/reference thrust 查詢仍取自相同權威定義。
- Engine tests 與 Propulsion tests 可分開執行。
- 共通驗證全數通過。

### 19. 抽取 `GroundInteractionModel`

變更：

- 搬移 fallback ground force 與相關 moment。
- Pipeline 每輪每幀明確選擇 DCS feedback 或 fallback。

驗證：

- 有有效 DCS suspension feedback 時不執行同輪 fallback 累加。
- 無 feedback 時 fallback 結果與 baseline 相同。
- brakes、NWS、wheel contact 與 on-ground 情境不變。
- 增加「不會 double force」的專用回歸測試。
- 由舊的同時累加改成互斥來源所造成的預期值，只能在本 commit 更新。
- 共通驗證全數通過。

### 20. 抽取 `MassPropertiesModel`

變更：

- 根據 Fuel snapshot 產生 mass、CG 與 inertia effects。
- Fuel 不再直接建立 DCS mass callback 結果。

驗證：

- fuel consumption 的 mass delta 數值與位置不變。
- 本階段保留目前 mass delta 的位置與 inertia 模型，不新增外部油箱
  CG/慣量精度。
- unlimited fuel、refuel 與單次 mass delta 消費規則不變。
- Fuel 只發布該幀 mass effect，不再由讀取 callback 清除自身狀態。
- 共通驗證全數通過。

### 21. 將 mass delta 交付狀態移到 DcsBridge

變更：

- FrameOutput 攜帶該幀已完成的 mass effect。
- DcsBridge 在發布 frame 時，將 effect 放入待交付佇列。
- `ed_fm_change_mass` 只消費 DcsBridge 佇列，不再呼叫 Core 清除 Fuel
  暫態。
- 不設定 silent queue cap；若未來量測證明需要限制，另行設計可見錯誤
  與背壓政策。

驗證：

- 同一筆 delta 只交付一次。
- DCS 連續數幀未讀取時，各筆 delta 仍依序交付而不遺失。
- 沒有 delta 時 callback 回覆既有的 unavailable 結果。
- release 明確清空上一飛行尚未交付的 delta。
- OutputStore 的 publish/read concurrency tests 通過。
- 共通驗證全數通過。

### 22. 完成固定 `SimulationPipeline`

變更：

- 固定順序為 Aerodynamics → Propulsion → GroundInteraction →
  MassProperties。
- 使用私有 per-frame context 與 accumulator。
- Model 只取得 aircraft snapshot、observations 與明確提供的較早結果。
- 刪除舊物理 orchestration。

驗證：

- 每個 Model 每幀恰好執行一次。
- 目前固定、單線程的直接排程以程式結構及既有單幀、多幀、
  deterministic tests 驗證；不額外加入 call-count instrumentation。
- force/moment aggregation 不因抽取而重複或漏算。
- ground fallback 若需要 propulsion 結果，只能透過 frame context 取得。
- 完整單幀、多幀與 deterministic tests 通過。
- OutputStore 仍只發布完整 frame。
- 共通驗證全數通過。

## Phase 6：收尾與強制架構邊界

### 23. 將 Config 移回真正 owner

變更：

- 各 System/Model 擁有自己的 Config、表格、production values 與驗證。
- owner 內部且不需外部調校的 legacy/演算法數值可留在實作中，但要以
  鄰近註解說明意義，不強制全部提升為 Config。
- 只把跨 Model 的機體幾何留在 `Simulation/Definition`。
- 移除依賴具體 Systems 的全域 `Data::AircraftConfig`。

驗證：

- 每個 owner 對不合法 config 有明確失敗測試。
- config 建立後不被 runtime 任意修改。
- DcsBridge 與 Fck1cEfm 不 include owner-specific Config。
- carrier thrust、幾何、氣動與 engine production values 不變。
- 共通驗證全數通過。

### 24. 刪除舊路徑並檢查依賴

變更：

- 刪除已無 caller 的舊 flat Systems、舊 orchestration、轉送 header 與
  暫時遷移程式碼。
- 加入可重複執行的 architecture dependency check。
- architecture dependency check 同時掃描 `.cpp`、`.h` 與 `.hpp`，避免
  只更換 header 副檔名就繞過邊界。
- 更新 Core README、架構文件與 source tree。

驗證：

- 禁止依賴檢查涵蓋：
  - DcsBridge → Concrete System/Simulation Model。
  - System → another concrete System/DcsBridge/Simulation Model。
  - Simulation Model → concrete System。
- Core 根目錄只保留 `Fck1cEfm.h/.cpp`。
- 沒有 production code include 已刪除路徑。
- 沒有未使用 source、dead compatibility path 或雙重權威狀態。
- 專案初期只強制檔案不超過 700 行；50 行函式與三層巢狀保留為人工
  review 指引，不加入阻擋式自動檢查。明顯責任混雜仍應在 review 時拆分。
- 從乾淨 intermediate 完成 native tests 與 DLL build。
- DLL export 名稱與 Phase 0 baseline 完全一致。
- `git diff --check` 通過。

## 最終完成條件

只有同時滿足下列條件才算完成，不以「可以編譯」代替：

- 架構文件中的完成條件全部成立。
- 24 個步驟的 commit 與其中通過的 focused/common validation 即為驗證
  紀錄，不另外要求逐步報告文件。
- 除步驟 15、16、19、21 明列並經測試批准的差異外，多幀行為與
  baseline 相同。
- 沒有為了通過測試加入 silent fallback、雙路徑或吞錯。
- 新增一個最小測試 System 時，只需新增其目錄與 Entry，不修改中央
  façade、pipeline 或既有 System。
- DCS DLL 可從乾淨 checkout 以既有 `tools/build_dll.ps1` 方式建立。
