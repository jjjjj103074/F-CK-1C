# Cockpit 來源行為 Baseline

## 1. 這份文件固定什麼

這裡描述目前原始碼「想要做到的行為」，不是新架構設計，也不是宣稱所有行為已在 DCS 成功運作。

共同規則：

- C++ EFM 已經是飛行模擬與飛機系統的目標權威。
- 目前 Lua 中仍有系統狀態、控制器、計時器與 DCS adapter 混合存在。
- 遷移時先保持本文件的輸入／狀態轉移／輸出，再另外討論功能改善。
- 實機已知錯誤集中列在第 10 節，不能偷偷當作期望行為。

## 2. DCS 載入鏈

`device_init.lua` 目前建立：

| Device | DCS class | Lua 入口 | 更新週期 |
|---|---|---|---:|
| Gear | `avLuaDevice` | `Systems/gear_system.lua` | 0.01 s |
| Actuators | `avLuaDevice` | `Systems/actuators_system.lua` | 0.01 s |
| CMS | `avLuaDevice` | `Systems/cms_system.lua` | 0.02 s |
| WEAPON_SYSTEM | `avSimpleWeaponSystem` | `Systems/weapon_system.lua` | 0.05 s |
| RADAR | `avSimpleRadar` | `RADAR/FCK1C_Radar.lua` | 由 DCS class 驅動 |
| RADAR_STATE | `avLuaDevice` | `Systems/radar_state_system.lua` | 0.02 s |
| HMCS | `avLuaDevice` | `Systems/hmcs_system.lua` | 0.05 s |
| AAM_AUDIO | `avLuaDevice` | `Systems/aam_audio_system.lua` | 0.05 s |
| AUTOPILOT | `avLuaDevice` | `Systems/autopilot_system.lua` | 0.02 s |

另外載入：

- Controls Indicator：`ccControlsIndicatorBase`
- 2D HMCS：`ccControlsIndicatorBase`
- VR HMCS：`ccIndicator`

完整且可機器檢查的順序見 [device-load-chain.csv](generated/device-load-chain.csv)。

## 3. Gear 與 Actuators

### Gear

- `gear_system.lua` 保留 DCS device 入口。
- `post_initialize()` 與 `update()` 都沒有系統行為。
- 起落架狀態與動畫由 EFM C++ 管理。

### Actuators

來源意圖：

- `actuators_system.lua` 載入 `actuators.lua`。
- 讀取左右 rudder sensor。
- 把 `85 .. -85` 的 sensor 值映射到座艙 argument。
- 更新左右 rudder draw argument。

目前邊界事實：

- C++ 已經寫入 rudder draw arguments 17、18。
- Lua actuator 不是狀態權威，且與 C++ 呈現責任重疊。
- 實機載入因 argument 名稱不存在而失敗，詳見第 10 節。

## 4. Autopilot／Auto Throttle

### 初始狀態

- 更新週期：0.02 s。
- AP Master：關閉。
- 垂直模式預設：Pitch Hold。
- 水平模式預設：Heading Hold。
- Auto Throttle：關閉。
- `FM_MAXPOWER_SWITCH` 與 `FM_MAXPOWER_READY` 初始化為 1。

### Engage／Disconnect gate

| 功能 | 可接通條件 | 自動解除條件 |
|---|---|---|
| AP Master | IAS ≥ 240 kt、無 Weight-on-Wheels、`abs(roll) ≤ 45°`、`abs(pitch) ≤ 45°` | IAS < 240 kt 或 Weight-on-Wheels |
| Auto Throttle | Mach ≤ 0.95、無 Weight-on-Wheels | Mach > 1.00 |

### 模式與 reference

| 模式／操作 | 目前來源行為 |
|---|---|
| Pitch Hold | 接通時捕捉目前 pitch |
| VS Hold | 接通時捕捉目前垂直速度 |
| ALT Hold | 接通時捕捉目前高度 |
| Heading Hold | 捕捉目前 heading |
| Heading Select | 使用可調 heading reference，跨 0/360 時 wrap |
| NAV Track | 只記錄模式，沒有產生水平控制命令 |
| Vertical Increase／Decrease | ALT 每次 100 ft；VS 每次 1 m/s；Pitch 每次 1° |
| Lateral Increase／Decrease | Heading 每次 1° |
| Speed Increase／Decrease | 每次 5 kt，限制 200–550 kt |

### Bypass

- `APBypass` command 按下且 AP Master 已接通時進入 bypass；放開 command 時離開。
- Bypass 期間 pitch／roll command 設為 0，不輸出 AP 控制。
- Auto Throttle 仍繼續。
- 離開 bypass 時，比較進入前後的 pitch、roll、heading；任一變化超過 1° 才視為 meaningful change。
- 只有 meaningful change 才依目前 hold mode 重新捕捉 pitch、VS、altitude 或 heading reference，避免突然跳回舊目標。
- `override detection` 函式目前是空的；stick 輸入不會自動進入 bypass。

### 控制器常數

| 控制器 | 常數／限制 |
|---|---|
| Pitch | `kp=2.5`、`kd=0.3`、輸出 ±0.6 |
| VS | `kp=0.08`、`ki=0.02`、integrator ±5 |
| ALT | fine band 50 ft、hold band 500 ft、capture band 1000 ft、VS 最大 40 m/s、comfort 0.6 g |
| Heading outer | `kp=2.5`、`ki=0.1`、integrator ±0.5、bank limit 60° |
| Heading inner | `kp=1.8`、`kd=0.2` |
| Auto Throttle | base 0.5、`kp=0.015`、`ki=0.003`、integrator ±30、輸出 0–0.95 |

Mach 高於 0.95 時，Lua 每次 update 額外減少 0.01 throttle command。這是綁定更新頻率的現況；遷移到 C++ 時應先用 0.02 s reference step 做 characterization，再改成真正的 `dt` 等價形式。

### 輸出 parameter

- `AP_MASTER_ENGAGED`
- `AP_VERT_MODE`
- `AP_LAT_MODE`
- `AP_AT_ENGAGED`
- `AP_PITCH_CMD`
- `AP_ROLL_CMD`
- `AP_THROTTLE_CMD`
- `AP_BYPASS_ACTIVE`
- `AP_TARGET_ALT_FT`
- `AP_TARGET_HDG_DEG`
- `AP_TARGET_SPD_KTS`
- `AP_TARGET_PITCH_DEG`
- `AP_TARGET_VS_FPM`

目前 C++ `CockpitBridge` 讀取 AP master、pitch、roll、throttle、bypass、A/T engaged，再送入 C++ Core。這代表現況是「Lua 算控制，C++ 套用」，不是目標中的 C++ 唯一權威。

## 5. Fire Control 與 Countermeasures

### 初始狀態

- 更新週期：0.02 s。
- Master Arm：ON。
- Fire Control mode：NAV。
- AAM submode：NONE。
- AIM-9 數量 fallback：1。
- CMS connected：true。

### Master 與主模式

| 輸入 | 狀態轉移與 DCS action |
|---|---|
| Master OFF／SIM | 停止 gun／pickle、abort CMS、回 NAV |
| Master ON | 允許目前模式的後續 gate |
| NAV | 清除 designation、unlock、DCS cannon mode |
| DGFT | 強制 Master ON、Helmet mode、radar on、切換 weapon、送 auto-lock pulse |
| MSL OVRD | 強制 Master ON、先 BVR 再 FI0、清除 designation、unlock、radar on、切換 weapon |

### AIM-9 狀態

| 條件 | `AIM9_MISSILE_STATUS` |
|---|---:|
| 非 AAM mode 或數量 0 | OFF |
| uncage 且已 designation | TRACK |
| uncage | READY |
| 其他 AAM 狀態 | COOL |

### Gun／Pickle gate

- Gun：Master ON、DGFT、沒有 uncage，minimum hold 0.08 s。
- Pickle：Master ON、AAM mode、uncage；同時送 launch-permission override 與 pickle，minimum hold 0.08 s。
- Release 使用成對的 DCS action 結束持續狀態。

### TMS

| 模式 | TMS | 行為 |
|---|---|---|
| DGFT | Up | Helmet mode + lock pulse |
| DGFT | Down | VS mode + lock pulse |
| DGFT | Right | HUD mode保留，尚未實作 |
| 一般 | Up hold | lock start／finish |
| 一般 | Down | unlock |
| MSL | Left | IFF 保留，尚未實作 |
| MSL | Right | switch target |

DGFT auto-lock 在 Master ON、uncage、AIM-9 count > 0 時每 1.0 s 發送一次。

### CMS programs

| HOTAS | Program |
|---|---|
| Forward | 10 次；每次 flare 1 + chaff 1；間隔 1.0 s |
| Left | 2 次 flare/chaff pair；間隔 0.12 s |
| Aft | 7 次 chaff；間隔 0.5 s |
| Right | abort |
| Press | 保留，現況沒有分支 |

每個 pair 的 action hold 為 0.04 s，gap 為 0.12 s。`TriggerFirstStage` 目前只寫 log，沒有功能狀態轉移。

### DCS action 邊界

CMS Lua 目前呼叫 DCS 擁有的 radar、sensor mode、lock、weapon change、gun、pickle、chaff、flare action。完整 ID 與引用位置見 [dcs-action-usage.csv](generated/dcs-action-usage.csv)。

遷移 baseline 的要求是：

- C++ 決定「要發生什麼」與 one-shot revision。
- Lua adapter 仍可負責實際 `dispatch_action()`。
- 不能因改架構漏發 release action 或把 one-shot 變成每幀重複。

## 6. Weapon Station

- DCS device class：`avSimpleWeaponSystem`。
- 更新週期：0.05 s。
- 目前掃描常數 `NUM_STATIONS=7`，索引 0..6。
- AIM-9 判定優先用 `wsType level2=4`、`level3=7`，再以 CLSID 包含 `AIM-9`、`AIM_9` 或 `CATM-9` 補判斷。
- 進入 DGFT／MSL 時掃描，選第一個找到的 AIM-9 station。
- 把全部找到的 AIM-9 count 寫入 `AIM9_MISSILE_COUNT`。
- 掃描結果為 0 時不會把 `AIM9_MISSILE_COUNT` 清成 0，parameter 可能保留舊值。
- 收到 `WeaponRearmComplete` 或 `UnlimitedWeaponStationRestore` 時重掃。
- 目前選定 station 變空時重掃。
- DCS station API 用 `pcall` 暴露錯誤並記錄 log。

已確認飛機定義有 9 個 pylons，翼尖是 station 1 與 9。`NUM_STATIONS=7` 因此可能漏掉 station 8／9 對應的 DCS 索引；這是已知缺陷，不能在 baseline 中假裝不存在。

## 7. Radar／AIM-9 Seeker

### `avSimpleRadar`

- `device_init.lua` 必須保留 `avSimpleRadar -> RADAR/FCK1C_Radar.lua`。
- 這個 DCS device 提供 radar／weapon 相關 parameter 與 DCS sensor integration。
- C++ 可擁有雷達狀態與判斷，但 Lua 入口仍是 DCS adapter。

### Radar State

- 更新週期：0.02 s。
- 初始化與每次 update 都把 `RADARSTATE`、`RADARPOWER_STATE` 寫成 1。
- Weapon active：FC mode 為 DGFT／MSL、count > 0、missile status 不是 OFF。
- telemetry availability 以多個 radar／WS 數值是否非零判斷，epsilon 為 0.00001。
- DCS IR target azimuth／elevation 任一絕對值大於 0.001 視為 signal。
- STT range > 1 視為有效。

來源優先順序：

1. DCS IR lock／target。
2. Radar telemetry。
3. Legacy fallback。

Fallback 規則：

- contact = weapon active 且 uncage。
- lock = contact 且 target designated。

Telemetry 規則：

- contact = weapon active、uncage、IR az/el 非零。
- lock = weapon active、uncage、STT range > 1。

Seeker state：

| 條件 | 狀態 |
|---|---|
| Missile OFF | OFF |
| Missile TRACK 或有效 lock | TRACK |
| Missile READY | SEARCH_UNCAGED |
| 其他 | SEARCH_CAGED |

Tone state：

| 條件 | Tone |
|---|---|
| Weapon inactive | OFF |
| 有效 lock | LOCK |
| 有效 contact | ACQUIRE |
| COOL／READY | SEEK |
| 其他 | OFF |

## 8. HMCS

- 更新週期：0.05 s。
- 顯示預設：Master ON、weapon class GUN、gun quantity 523、AAM fallback quantity 1。
- cockpit argument 509 接近 0，容許範圍 ±0.25 時視為 helmet installed／enabled。
- cockpit argument 510 ≥ 0.5 時切換 display mode。
- heading 優先使用 magnetic heading，否則使用負的 heading。
- heading tape 有 13 個 slot，每 15° 一個 tick，每 30° 顯示 label。
- IAS：m/s × 1.943844 轉 knot。
- Altitude：m × 3.28084 轉 feet。
- Gun count 先探測 DCS sensor；沒有可用 API 時用 523 模擬，開火時每秒減少 100。
- AAM 顯示條件：FC mode 為 DGFT／MSL，或 uncage/contact/lock/status 任一顯示武器正在使用。

目前 HMCS 又直接 listen Master Arm、mode、trigger、uncage、weapon release command，自己保存一份顯示狀態。這與 CMS 的 Fire Control 狀態重複，是遷移時要消除的雙重權威。

2D 與 VR page 只應是 parameter 的 presentation reader。實際 reader/writer 見 [parameter-access.csv](generated/parameter-access.csv)。

## 9. AIM-9 Audio

- 更新週期：0.05 s。
- sound host 優先 `COCKPIT_RADAR_WARN/HEADPHONES`，失敗時嘗試 `COCKPIT/HEADPHONES`。
- 建立 SEEK、ACQUIRE、LOCK 三個 sound。
- 一般 tone gain 1.6，sound test gain 1.4。
- sound test playlist 共 23 個項目。
- tone 變化時才切換 sound；相同 tone 維持播放。

目前 `post_initialize()` 會把 `AIM9_TONE_STATE` 與 `AIM9_WEAPON_ACTIVE` 寫成 0。Audio 因此會修改不屬於自己的武器狀態，這是已知所有權問題。遷移後 Audio adapter 應只讀 requested tone，不再成為 writer。

## 10. 已知 Runtime 事實與缺陷

2026-07-28 Air Clean 真實執行已確認：

| 區域 | 實機結果 | Baseline 解讀 |
|---|---|---|
| EFM DLL | 成功載入 | 可作為該次 run 的 binary 證據 |
| Damage model | 缺 `lineFG`、`lineLG`、`lineRG`，DCS 判為 corrupt | 與本次 Cockpit 遷移不同範圍，但保留記錄 |
| Actuators Lua | argument `angle_of_draw_left_rudder` 不存在，載入失敗 | 來源意圖仍記錄；不能宣稱 runtime 有更新 |
| Autopilot Lua | function 超過 60 upvalues，載入失敗 | C++ 遷移要還原來源意圖，不重現載入錯誤 |
| CMS | 成功載入；Master ON、NAV、AIM-9 fallback count 1 | 只證明初始化，不代表所有 command 路徑通過 |
| Store probes | Gun／AIM-9 candidate methods 全部 missing | 實機沒有取得可靠 inventory |
| Weapon system | 成功初始化 | 空載 mission 未驗證 AIM-9 station 選擇 |
| AAM audio | host 與 3 個 sound 成功建立，playlist 23 | 尚未證明所有 tone 轉換可聽 |
| Radar state | fallback；radar on；無 contact／lock；count 1 | 空載時的 fallback 會造成虛擬 count，需如實保留 |

完整該次證據見 [2026-07-28 Air Clean](evidence/2026-07-28-air-clean/RUN.md)。

## 11. 遷移比對原則

每搬一個系統，至少比較：

1. 相同 command sequence。
2. 相同 observation sequence 與 `dt`。
3. 相同狀態轉移。
4. 相同持續輸出與 one-shot 次數。
5. 相同 DCS presentation，或明確列出刻意修正的舊錯誤。

不能只看「畫面看起來差不多」。特別要檢查：

- press 與 release 是否成對。
- timer 在不穩定 `dt` 下是否仍保持相同秒數。
- parameter 是否只剩一個 writer。
- Lua update 頻率改變時，行為是否仍以時間而不是 frame count 計算。
- DCS API unavailable 時是否明確留下 error／log，而不是靜默使用假成功。
