# Flight Control Refactor Plan

## 文件狀態

本文件已取代先前的
`F16_INSPIRED_FLIGHT_CONTROL_ARCHITECTURE_PLAN.md`。

目前狀態是「宏觀範圍與 System scheduler 架構待使用者最後檢查」。先前文件中的
AP 模式表、控制器形式、參數與詳細 commit 規劃仍然失效，不得當成後續實作
依據。

本文件目前固定：

1. 已由可靠公開來源確認的事實。
2. 已由專案明確決定的產品方向。
3. 重構包含與不包含的宏觀範圍。
4. 現有 `SystemPipeline` 改為時間排程的架構決策。
5. scheduler 完成並驗證後，才繼續討論 AP 與飛控架構。

控制律數學、AP 模式細節、參數與詳細施工順序，必須在後續討論重新建立。

## 1. 重構目標

建立一套以 F-CK-1C 為模擬對象的飛行控制架構：

- 已確認的 F-CK-1 資料優先於任何參考機資料。
- F-CK-1 公開資料不足時，可以使用 F-16 提供飛控架構參考。
- F-16 參考不得被描述成已確認的 F-CK-1 行為。
- 無公開證據的數值、模式與裝置不得偽裝成真實資料。
- 專案擴充、相容功能與開發工具必須明確標示身分。
- 真實飛機裝置之間使用明確 Interface 傳遞訊號。
- 同一飛機裝置內的演算法以內部 Module 組織，不因 C++ 類別不同而偽裝成不同
  飛機裝置。
- Lua 只負責 DCS 座艙輸入與呈現；飛機狀態與飛控邏輯以 C++ 為唯一權威。

這不是 F-16 複製計畫，也不是只把現有 AP 重新調參。

## 2. 證據分級

後續所有飛控決策都必須標示以下其中一種身分。

### 2.1 Confirmed F-CK-1

由中華民國空軍、漢翔、中科院或同等級第一方資料明確支持。這類資料可以定義
F-CK-1 模擬目標。

### 2.2 Reference

由 NASA、美國空軍或其他可靠資料支持，但描述的是 F-16、AFTI/F-16 或一般
飛控工程。這類資料可以協助選擇架構，不可直接宣稱為 F-CK-1 事實。

### 2.3 Project-defined

公開資料不足，由本專案為了形成可操作模擬而定義。必須記錄理由、可觀察行為
與未來替換條件。

### 2.4 Developer-only

只供測試、診斷或飛行模型開發。不得出現在真實功能清單，也不得在正常模擬中
默認啟用。

## 3. 已確認的 F-CK-1 資料

### 3.1 數位線傳飛控

中科院公開資料明確描述 F-CK-1 具備 digital fly-by-wire flight control
systems。

來源：

- [NCSIST — Ching-kuo IDF High Performance Fighter](https://www.ncsist.org.tw/eng/csistdup/products/product.aspx?catalog=9&product_Id=18)

架構意義：

- 玩家輸入不應直接等同控制面位置。
- 飛控電腦位於玩家輸入與致動器之間。
- 飛機反應需要閉迴路感測與控制計算。

此來源沒有公開實際 command law、limiter、gain schedule、控制頻率或故障邏輯。

### 3.2 F-CK-1C/D 使用數位飛控電腦

漢翔公開資料明確描述 F-CK-1C/D 配備 32-bit digital flight control computer
（32-bit DFLCC）。

來源：

- [AIDC — F-CK-1 C/D](https://www.aidc.com.tw/en/military/fck1)

架構意義：

- `FlightControlComputer` 可以代表一個真實存在的飛機裝置或邏輯裝置群。
- Normal control、基本自動控制、限制器與控制面命令可以合理地在該裝置內以
  軟體 Module 組織。

此來源沒有公開 DFLCC 的內部軟體分區、冗餘數量、Interface 或 AP 模式。

### 3.3 雙發動機

中華民國空軍公開資料明確記載 F-CK-1 使用兩具 TFE1042-70 渦輪扇發動機。

來源：

- [中華民國空軍 — IDF](https://air.mnd.gov.tw/TW/Weapon/Weapon_Detail.aspx?CID=55&ID=89)

架構意義：

- 不得直接複製單發 F-16 的推進與方向動態假設。
- 飛控與測試環境必須能面對左右推力不一致造成的偏航力矩。
- 是否具有真實的自動單發補償仍屬未知，不能由雙發配置自行推導。

### 3.4 機體與 F-16 不相同

中科院公開資料描述 F-CK-1 使用 blended wing-body lifting body；空軍公開資料
提供 F-CK-1 自己的尺寸、重量、速度與雙發配置。

架構意義：

- F-16 可以提供飛控架構參考，不能提供 F-CK-1 的氣動導數。
- F-16 的 control-law gain、限制值、控制面分配與 actuator 參數不得直接套用。
- 最終閉迴路行為必須與本專案的 F-CK-1 airframe、aerodynamics、mass 與
  propulsion plant 一起驗證。

## 4. 已確認的 F-16 參考資料

本節全部是 Reference，不是 Confirmed F-CK-1。

### 4.1 飛控邏輯集中於飛行控制系統

美國空軍 AFRL 公開說明指出，F-16 的操縱輸入經 side-stick 送入飛控系統，
由感測器、電腦、selectors、transducers 與 inverters 共同產生 pitch、roll、
yaw response；電腦持續調整輸入以維持飛行穩定。

來源：

- [USAF/AFRL — AFRL supports Thunderbirds with aerospace technology](https://www.edwards.af.mil/News/AFMC-News/Article/1903051/afrl-supports-thunderbirds-with-aerospace-technology/)

可採用的架構參考：

- Pilot input 是飛控計算的輸入，不是控制面需求。
- 穩定增益、command shaping 與回授控制屬於飛控電腦責任。
- 飛行控制是多個真實裝置形成的訊號鏈，不是缺少裝置 seam 的單一函式。

### 4.2 AFTI/F-16 的公開數位飛控架構

NASA TP-2857 公開的 AFTI/F-16 實驗機架構包含：

- 三套相同 FLCC。
- 一套 Aircraft Interface Unit（AIU）。
- sensor selection、voting 與 monitoring。
- multimode control laws。
- gain scheduling。
- stall-spin prevention。
- aileron-rudder interconnect。
- structural filters。
- 外部 air-data、inertial reference、pilot input 與 actuator 裝置。

來源：

- [NASA TP-2857 — Development and Flight Test Experiences with a Flight-Crucial Digital Control System](https://ntrs.nasa.gov/citations/19890014956)

限制：

- AFTI/F-16 是實驗機，不代表所有量產 F-16 Block。
- 此架構可以證明「真實裝置與 FLCC 內部軟體應分層」，不能證明 F-CK-1 使用
  相同硬體數量或相同控制律。

### 4.3 CAT I／CAT III 會影響 F-16 飛控反應

NASA F-16XL 公開資料顯示 CAT III configuration 會限制最大可命令滾轉率。

來源：

- [NASA — Flight Test Results for the F-16XL With a Digital Flight Control System](https://ntrs.nasa.gov/api/citations/20040040334/downloads/20040040334.pdf)

可採用的架構參考：

- 載荷或飛行構型可以影響 control-law gain、rate 與 authority schedule。
- 構型限制必須同時約束玩家與自動控制來源。

不能由此推導：

- F-CK-1 使用 CAT I／CAT III 名稱。
- F-CK-1 的 CAT 邏輯與 F-16 相同。
- F-CK-1 使用相同限制值。

### 4.4 Manual Pitch Override 是 F-16 特定功能

美國空軍資料將 MPO 描述為深失速／departure recovery 所使用的俯仰覆寫。
操作規範也明確禁止為提升性能而使用 MPO 繞過 flight-control limiter。

來源：

- [USAF — Out of Control in the F-16](https://www.acc.af.mil/Portals/92/Docs/ACC%20SAFETY/COMBAT%20EDGE/TAC87_05.pdf)
- [AFMAN 11-2F-16V3 AETC Supplement](https://static.e-publishing.af.mil/production/1/aetc/publication/afman11-2f-16v3_aetcsup/afman11-2f-16v3_aetcsup.pdf)

目前沒有可靠公開證據證明 F-CK-1 具有相同 MPO 功能，因此不列入已確認的
F-CK-1 重構目標。

## 5. 目前仍未知的 F-CK-1 資料

目前找到的可靠公開來源尚未確認：

- 實際 longitudinal、lateral、directional command law。
- 正負 G、AoA、pitch-rate、roll-rate 與 yaw-rate 限制。
- gain scheduling 的輸入與數值。
- CAT I／CAT III 名稱或 stores-configuration 操作方式。
- 基本 AP 模式、panel 操作與各軸 engage 關係。
- AP 是否位於 DFLCC 內，或另有獨立硬體。
- Auto-throttle。
- Manual Pitch Override。
- Direct／reversion control law。
- DFLCC 冗餘、voting、failure 與 degradation 行為。
- 真實 actuator rate、lag、authority 與 surface feedback。
- 不對稱推力的自動補償行為。

這些項目不能因為 F-16 具有類似功能就改列為 Confirmed F-CK-1。

## 6. 已決定的專案方向

### 6.1 `PilotControls` 保留

`PilotControls` 作為獨立 System 的理由是整合不同 DCS command 對同一操作的
表達，並保存需要跨 frame 的玩家操縱狀態。

允許責任：

- 類比軸與離散按鍵輸入整合。
- 按鍵按住、放開與使用 `dt` 的虛擬軸變化。
- 同一操作的 set、toggle、press、release 語意正規化。
- 發布玩家控制裝置的當前訊號。

禁止責任：

- 保存 AP、CAT、FBW 或其他下游飛機系統的權威狀態。
- 將玩家操縱轉換成 Nz、pitch-rate、roll-rate 或控制面位置。
- 執行 envelope protection 或 AP 仲裁。

Toggle 可以被正規化為一個操作，但最終狀態必須由目標狀態的 owner 決定。

### 6.2 CAT 構型概念在重構範圍內

本次需要保留「飛行／載荷構型影響控制律」的能力。

在取得 F-CK-1 證據以前：

- CAT I／CAT III 是 F-16-derived provisional model。
- 現有名稱可以作為相容輸入，但不得列為已確認真實座艙功能。
- 真實 schedule、門檻、警告與 stores integration 保持未決。

### 6.3 `G-Limiter Override` 保留為 Developer-only

此功能明確不是已確認的 F-CK-1 功能。

不變條件：

- 預設關閉。
- 不得被 normal-law 或 AP 正常操作自動啟用。
- 啟用時必須在 diagnostics、telemetry 與 CSV 明確可見。
- 輸入名稱與文件必須標示 Developer 或 Flight Test。
- 正常擬真測試必須確認它沒有啟用。

### 6.4 現有 Lua AP 不是未來規格

Phase 3 搬移的 Lua 行為只保留為 legacy behavior、測試資料與回歸證據。

它不能決定：

- 真實飛機 System 劃分。
- AP 應輸出的物理 command。
- F-CK-1 應具有的 AP 模式。
- Normal Law、protection 或 control allocation。

參考：

- [Cockpit Phase 3 Autopilot 遷移報告](COCKPIT_PHASE_3_AUTOPILOT_REPORT.md)

## 7. 宏觀重構範圍

### 7.1 包含

- 將 `SystemPipeline` 從固定 group 排程改為依模擬時間執行的 scheduler。
- `PilotControls` 的 System 身分與輸出 Interface。
- `FlightControlComputer` 的真實裝置責任。
- FLCC 內部 Module 與外部 System 的 seam。
- 玩家、AP 與未來自動安全來源的 authority ownership。
- `FlightControlDemand` 與 `PrimaryFlightControls` 的 seam。
- actuator demand、actuator state 與實際控制面位置的 ownership。
- 飛控需要的 typed AircraftData Interface。
- AP 與 manual FBW 的重新整理、修復及重構。
- 與 F-CK-1 simulation plant 的閉迴路驗證。
- Developer-only 功能的明確隔離。

### 7.2 不包含

- Radar、weapon、CMS、HMCS、audio 等非飛控系統重構。
- 再次搬移已完成的 Lua 座艙 seam。
- 完整重寫 aerodynamics、mass、fuel 或 propulsion model。
- 虛構 F-CK-1 的機密或未公開控制律。
- 在沒有證據或需求前實作完整 DFLCC redundancy 與故障投票。
- 在本宏觀階段決定 PID、filter、gain、limit 或 control frequency。
- 在本宏觀階段決定完整 AP 模式表。
- 模擬電纜的奈秒級物理傳播延遲。
- 每條 AircraftData 訊號各自擁有 transport-delay queue。
- 使用隨機啟動 offset、Clock Domain、動態 update rate 或多執行緒執行 System。
- 冷啟動、通電時重設 scheduler epoch 或依電源狀態加入／移除 schedule。
- 在 scheduler 以前建立 AP／FBW 的詳細實作或 commit 順序。

## 8. 重構前 `System` 架構的已確認基線

重構前的 `SystemPipeline`：

- 是 aircraft Systems 的唯一 production scheduler 與 commit point。
- 透過 typed `AircraftData` 傳遞跨 System 狀態。
- 驗證 read、publication、型別、初始值與 single writer。
- 每個 semantic `CommandId` 只允許一個 handler。
- concrete System 不需要直接呼叫另一個 concrete System。
- 每個 System 透過 `setup()` 宣告 Interface，透過 `step()` 跟隨 host frame
  推進。

重構前的執行模型只有兩個 `SystemGroup`：

```text
Control
Equipment
```

當時每個 group 使用同一份 immutable input snapshot 執行。相同 group 的
System 看不到彼此在該 frame 的 pending publication；整個 group 完成後才一次
commit。本節只記錄重構前基線，目前 runtime 已由第 10 節的 ordered time
buckets 取代，`SystemGroup` 不再控制執行順序。

來源：

- [System contributor guide](../src/efm/F-CK-1C_EFM/Core/Systems/README.md)
- [SystemGroup definition](../src/efm/F-CK-1C_EFM/Core/Systems/System.h)
- [SystemPipeline execution](../src/efm/F-CK-1C_EFM/Core/Systems/SystemPipeline.cpp)

## 9. 現有 `System` 架構與飛控邏輯鏈的適配性

### 9.1 保留的深 Module

以下設計與真實飛機裝置鏈相容，必須保留：

- concrete System 擁有自己的連續狀態。
- 跨 System 只交換 passive typed data。
- 一個 AircraftData key 只有一個 owner。
- command 有明確的 semantic owner。
- `FlightControlComputer` 先產生 demand，`PrimaryFlightControls` 再推進
  actuator state。
- `AircraftSimulation` 消費完成的控制面位置，而不直接擁有飛機 Systems。
- `SystemPipeline` 是唯一 production scheduler 與 commit point。

因此本次不是新增另一個 `TimeCoordinator`，也不是推翻 `System` Interface。
時間排程、schedule storage、相同時間批次執行與 commit 全部留在
`SystemPipeline` Implementation，深化既有 Module。

### 9.2 現有固定 group 的問題

目前的 `Control -> Equipment` 順序同時混合：

1. System 是什麼種類的飛機裝置。
2. System 的輸出何時對其他 System 可見。

這違反 scheduler 的單一職責，也使延遲由 DCS frame 與 group 數量決定。將一個
裝置拆成更多 concrete Systems 可能意外增加更多 host-frame 延遲，且不同 DCS
frame rate 會改變延遲秒數。

`SystemGroup` 不再作為 production 執行順序。若未來仍需要裝置分類，只能作為
文件、診斷或測試 metadata，不得控制 scheduler。

## 10. 已選定的時間 scheduler 架構

本節是已由使用者同意並於 2026-07-31 完成實作的架構決策。它先於 AP 與飛控
邏輯重構。

### 10.1 單一時間權威

- `AircraftSimulation` 繼續擁有全機 simulation time。
- 每次 DCS callback 定義一個 frame-start time 與
  `frame-end = frame-start + host dt`。
- `SystemPipeline` 接收要前進到的 target simulation time，不建立第二份全機
  時間權威。
- `SystemPipeline` 只保存每個 System 的排程狀態。

### 10.2 System 的 timing Interface

每個 System 在 `setup()` 必須宣告且只能宣告一個固定 update rate：

- Hz；或
- 明確時間 duration。

實作 Interface 為：

- `SystemSetup::update_rate_hz(std::uint32_t)`；或
- `SystemSetup::update_period(SystemScheduledTime)`。

Interface 不暴露：

- phase。
- 上次執行時間。
- 下次執行時間。
- queue 位置。
- Clock Domain。

`System::step()` 必須能取得 scheduler 提供的 `SystemStepContext`，至少包含：

- 本次 scheduled simulation time。
- 該 System 自己的固定 `dt`。

System 不得再把 DCS `FrameInput::dt_s` 當成自己的積分步長。

### 10.2.1 目前 production rate 與證據標籤

| System | Rate | 證據身分 |
|---|---:|---|
| `FlightControlComputer` | 64 Hz | **F-16XL reference**：NASA DFLCS control laws 為每秒 64 cycles；不是已確認的 F-CK-1C 資料 |
| `PrimaryFlightControls` | 64 Hz | **Project-defined fallback** |
| `SecondaryFlightControls` | 64 Hz | **Project-defined fallback** |
| `LandingGear` | 64 Hz | **Project-defined fallback** |
| `Engine` | 64 Hz | **Project-defined fallback** |
| `Fuel` | 64 Hz | **Project-defined fallback** |
| `AirframeStructure` | 64 Hz | **Project-defined fallback** |
| `PropulsionDiagnostics` | 64 Hz | **Project-defined fallback** |

F-16XL 來源：
[NASA TP-3547](https://ntrs.nasa.gov/api/citations/20040040334/downloads/20040040334.pdf)。
目前沒有找到其他現有裝置可靠的 F-16 或 F-CK-1C specific update rate，因此沒有
把一般引擎模擬頻率或其他飛機資料冒充成 F-16 證據。共用的
`kProjectDefinedFallbackUpdateRateHz` 名稱必須保留這項區別。

### 10.3 Ordered time buckets

`SystemPipeline` Implementation 使用依時間排序、相同時間自動合併的 schedule：

```cpp
std::map<ScheduledTime, std::vector<SystemIndex>>
```

概念不變條件：

- key 是下一次 scheduled simulation time。
- value 是該時間必須完成的完整 System batch。
- `begin()` 永遠提供最早的 batch。
- 插入相同時間會加入同一個 batch，不建立第二個相同時間節點。
- schedule 是 private Implementation，不成為新的公開 seam。

不要求真的使用多執行緒。「同時」是邏輯上的同時取樣。

### 10.4 Batch 執行與 commit

對每個不晚於 target time 的最早 bucket：

1. 取出完整 bucket。
2. 建立該 scheduled time 的 immutable AircraftData snapshot。
3. 讓 bucket 內所有 Systems 只讀該 snapshot。
4. 將全部 publication 保存在 pending result。
5. 全部 Systems 成功後一次 commit。
6. 依各自 period 計算下一次時間並重新插入 schedule。

同一 bucket 內的 System：

- 看不到彼此本次尚未 commit 的 publication。
- 不得因 catalog、vector 或容器順序得到不同 AircraftData 結果。
- 可以依穩定的 catalog index 執行，以保持 log 與錯誤順序可重現。

不同 scheduled time 的 bucket：

- 前一個 bucket 完成後立即 commit。
- 後一個 bucket 讀取包含前面所有已提交輸出的最新 snapshot。

### 10.5 排程時間不能跟隨 host lateness 漂移

下一次時間必須從原 scheduled time 計算：

```text
next = current_scheduled_time + period
```

不得使用：

```text
next = DCS_frame_end + period
```

一次很大的 host `dt` 必須完成期間內所有到期更新，不得默默跳過或重設時鐘。

schedule key 不使用 `double` 相等判斷。公開 Interface 可以使用 Hz 或 duration，
Implementation 必須轉成穩定的整數或等價精確表示。若由 Hz 換算，下一次時間應
從 invocation count 與 epoch 重新求得，避免近似 period 的累積漂移。

### 10.6 初始執行

- `setup()` publication 定義 `t = 0` 初始 AircraftData。
- System 的第一次動態 `step()` 發生在第一個完整 period 結束時。
- 所有 Systems 暫時共用 `t = 0` scheduler epoch。
- 完全相同 scheduled time 一律批次執行。
- 不加入隨機 offset 或 phase。

冷啟動、通電後重新計算 epoch 及 power-dependent scheduling 不在目前範圍。

### 10.7 Command 的時間語意

`SystemPipeline::send()` 仍把 command 路由給唯一 semantic owner，但 command
到達不等於立即執行依賴 Observation 的狀態轉換。

owner 的 command handler 可以立即保存輸入意圖：

- 軸與 set 類輸入保存最新值。
- press、toggle、increase、decrease 等 edge action 必須保存事件或累積次數，
  不得因下一筆輸入覆蓋而遺失。

以下動作在 owner 下一次 scheduled `step()` 才正式發生：

- 依賴目前 sensor／Observation 的 mode transition。
- AP engage 時的 altitude、heading、pitch、speed 或其他 reference capture。
- 使用 `dt` 的 held-key 或虛擬軸積分。

因此 command 可以在任意 DCS callback 到達，但它對飛機狀態的可觀察效果與 owner
的 update rate 對齊。

### 10.8 DCS Observation 的時間語意

在取得更精確的 DCS 時序證據前，採用明確的 Project-defined 因果規則：

- DCS Adapter 在 callback 開始時提供最新可用的 `FrameInput` 與
  `AircraftObservation`。
- 該 sample 從 frame-start time 起可用。
- `SystemPipeline` 將它保持到下一筆 DCS sample 到達。
- callback 內所有 scheduled Systems 只讀當時最新且已經可用的 sample。
- 不使用尚未到達的未來 Observation 進行 look-ahead interpolation。
- 即使該 callback 沒有 System 到期，最新 external sample 仍必須更新，供
  `SimulationPipeline` 與後續 System tick 使用。

此規則固定 scheduler 的因果方向，但無法讓 DCS 30 FPS 與 144 FPS 的完整飛行
軌跡逐位元一致，因為外部 Observation 與 DCS 物理積分仍由 host callback 更新。

### 10.9 與 `SimulationPipeline` 的關係

`SimulationPipeline` 暫時保持 host-driven：

1. DCS Adapter 提供本 callback 最新 input／Observation。
2. `SystemPipeline` 執行所有不晚於 frame-end 的 scheduled batches。
3. `SimulationPipeline` 使用最後 committed AircraftData 計算本次輸出。
4. DCS 消費力、力矩與其他輸出。

本階段不把 aerodynamics、propulsion physics 或 DCS integration 放入 System
schedule。若未來需要物理 substep，必須另行討論，不能藏在 scheduler refactor
中。

## 11. 對新 AP／FBW 架構的影響

### 11.1 不衝突的裝置 seam

暫定最小裝置鏈是：

```text
DCS input Adapter
  -> PilotControls
  -> FlightControlComputer
  -> PrimaryFlightControls
  -> AircraftSimulation
  -> observed aircraft motion
  -> FlightControlComputer
```

其中：

- DCS input Adapter 只轉換並傳入 DCS input。
- `PilotControls` 擁有玩家輸入裝置狀態。
- `FlightControlComputer` 擁有飛控決策。
- AP 若被判定屬於 DFLCC，作為其內部 Module，不是獨立飛機 System。
- AP、FBW、limiter 與 mixer 可以在同一次 FCC tick 內依序執行。
- `PrimaryFlightControls` 擁有 actuator 與實際控制面狀態。
- `AircraftSimulation` 擁有飛機物理狀態。

AP 與 FBW 位於同一 FCC System，因此不會因 batch scheduler 在兩者之間增加一個
System period。未來若 AP outer loop 需要較低更新率，可以在 FCC 內部按整數 tick
分頻並保持 demand，不需要把 AP 暴露成頂層 System。

### 11.2 明確接受的跨 System 延遲

若 FCC 與 `PrimaryFlightControls` 在完全相同時間到期：

```text
FCC[k]      讀取 actuator[k-1]
Actuator[k] 讀取 demand[k-1]
```

這是明確的一個接收端取樣週期延遲，不是意外的 DCS frame 延遲。新的 AP／FBW
控制器與閉迴路測試必須包含它，不得只在「demand 立即成為控制面位置」的理想
plant 下調整。

Auto-throttle demand 與 Engine、Engine 與 Fuel 等獨立裝置也遵守相同規則。

Fuel 另外以 DCS callback 為 mass-effect 交付邊界：callback 開始時清除該次
`consumed_mass` 累計，期間零個或多個 64 Hz Fuel ticks 的消耗會累加，最後由
`SimulationPipeline` 一次交付給 DCS。Infinite Fuel 抑制整個 callback 期間，
而不是只抑制第一個到期 Fuel tick。

### 11.3 scheduler 階段必須先修正的現有 AP 耦合

在重設計 AP 模式與控制律以前，scheduler refactor 必須先移除兩個既有耦合：

1. FCC、AP、Engine、Fuel、LandingGear 等 System 不得使用 host
   `FrameInput::dt_s` 積分，必須使用自己的 `SystemStepContext::dt`。
2. 目前 AP command handler 會立即用上次保存的 Observation 擷取 reference；
   必須改為保存 pending intent，並在下一次 FCC tick 使用當時最新可用的
   Observation 執行 mode transition 與 reference capture。

這兩項是時間架構相容修改，不代表核准現有 AP 邏輯為未來規格。

## 12. Scheduler 驗證基準

scheduler 實作完成後，至少必須從 `SystemPipeline` Interface 驗證：

1. 相同總模擬時間使用 30、60、144 FPS 與不規則 host `dt` 分割時，固定 rate
   System 的 invocation count 與 scheduled times 相同。
2. 64 Hz System 經過一秒模擬時間準確執行 64 次。
3. 相同 scheduled time 的 Systems 全部讀同一份 snapshot。
4. 反轉同一 batch 的 catalog／執行順序不改變 completed AircraftData。
5. 不同 scheduled time 的後續 System 能讀到前一個 bucket 的 publication。
6. 沒有 System 到期時，先前 System 輸出維持不變，最新 DCS external sample
   仍會更新。
7. 一次大型 host `dt` 完成所有到期更新，不得靜默丟棄。
8. System 收到自己的 period，不是 DCS host `dt`。
9. 長時間執行不因浮點累加遺失、增加或重排 tick。
10. command 在兩個 owner ticks 之間到達時，只在下一次 owner tick 形成可觀察
    狀態變更。
11. AP engage 在下一次 FCC tick 使用當時最新可用 Observation 擷取 reference。
12. FCC／actuator 同時間取樣的一週期延遲有明確 characterization test。

上述 1 至 12 已由 native tests 覆蓋；其中長時間測試使用 3 Hz 跑一小時來檢查
invocation count 與最後 scheduled time，64 Hz 則驗證一秒恰好 64 ticks。

不同 DCS FPS 的完整飛行軌跡只能要求在定義容差內接近，不要求逐位元一致；該
容差必須在取得實作測量結果後明確制定，不能用寬鬆 fallback 隱藏差異。

### 12.1 DCS integration 驗證結果

2026-07-31 已使用安裝後的 Release x64 DLL 完成兩次 `hot_air` flight lifecycle
驗證。主要 run 為 157.338 秒，第二次重新進入任務為 7.116 秒；CSV 中 simulation
time 均單調前進，沒有非有限值、Fuel 反向增加、EFM exception 或 scheduler error。
主要 run 已觀察到 pitch／roll／yaw、throttle、flaps、airbrake、AP、A/T、ALT Hold、
Heading Hold，以及連續兩次 vertical／heading reference command 的狀態變化；第二次
flight 從 simulation time 0 與初始 Fuel 重新開始，確認 scheduler epoch 沒有跨 flight
殘留。

使用者沒有執行手動 Unlimited Fuel 情境，因為切換成本高且不是本次 scheduler
重構的主要 DCS integration 風險。Infinite Fuel 對單一 callback 內所有 Fuel ticks 的
抑制，以及同 callback 多 tick 的 consumption 累加，仍由 native regression tests
直接覆蓋。本次 DCS log 只出現既有 baseline 問題：unknown command `2659`、damage
model 缺失與 HMCS parent 缺失；沒有本次重構新增的錯誤。

## 13. Scheduler 之後才繼續決定的內容

- F-CK-1 公開證據不足時，允許採用 F-16 參考到什麼深度。
- 哪些感測器需要成為獨立 System。
- AP 是否確定視為 DFLCC 內部 Module。
- 最小真實 AP 模式表。
- Vertical Speed Hold、Auto-throttle 與 reference stepping 的長期身分。
- 構型模型最終名稱及其與 stores system 的關係。
- manual、automatic、safety command 的 physical command type。
- control allocation 位於 FLCC 內部或獨立真實裝置的證據。
- direct／reversion capability 是否只有未來 seam，或列入本次功能。
- 取得更可靠證據後，哪些 Project-defined 64 Hz fallback 可以替換為真實裝置
  rate。

Air data、inertial reference、stores configuration、AP panel、engine control 與未來
safety source 的 System 身分也仍待後續逐一確認。

## 14. 實作閘門

執行順序固定為：

1. [x] 使用者最後檢查並核准本文件的 scheduler 章節。
2. [x] 只重構 `SystemPipeline` 時間排程與必要的 System timing／command 相容修改。
3. [x] 完成自動化 scheduler 驗證與既有功能回歸。
4. [x] 完成 DCS callback、flight lifecycle、controls、AP／A/T 與 Fuel integration
   驗證；Unlimited Fuel 的 callback 語意由 native tests 覆蓋。
5. scheduler 通過後，重新開始 AP 與飛控裝置、Interface、模式和控制律討論。

在第 1 至 4 項完成以前，不開始新的 AP／FBW 架構實作。
