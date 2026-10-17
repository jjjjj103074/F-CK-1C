# Cockpit DCS 手動驗收規範

## 1. 目的

這份規範固定「用哪個 mission、做哪些操作、保存什麼證據」。它不自動宣稱 DCS 已測過；只有真的執行並保存證據的 run 才算完成。

## 2. 目前 Mission 清冊

| ID | 外部檔案 | SHA256 | Spawn | Loadout | 狀態 |
|---|---|---|---|---|---|
| AIR-CLEAN | `Saved Games/DCS/Missions/f-ck-1c_air.miz` | `2D5A30848B1DF9D6CE78D8864EF1314FB26C4D23135D65965A372679519D3E67` | Air start；Caucasus；2000 m；220.972222 m/s | 9 個 pylon 全空；fuel 2111；flare 90；chaff 90；gun 100 | 已有 mission |
| COLD-CLEAN | `Saved Games/DCS/Missions/f-ck-1c_cold.miz` | `C1B14BB6170361E2DBD3361A9788529A85B200A7DBCB648AD32D0A174D96F67B` | TakeOffParking；airdrome 22；parking 4 | 9 個 pylon 全空；fuel 2111；flare 90；chaff 90；gun 100 | 已有 mission，尚未存證執行 |
| RWY-CLEAN | `Saved Games/DCS/Missions/f-ck-1c.miz` | `2DE5B29092DC0C8D5EE74AEB4E11DDA4FB42CA30330A4A6088894286AF3ABB85` | TakeOff；airdrome 24 | 9 個 pylon 全空；fuel 2111；flare 90；chaff 90；gun 100 | 已有 mission，備用 |
| AIR-AIM9 | 尚未建立 | 尚無 | 以 AIR-CLEAN 複製 | station 1、9 各裝 `{AIM-9L}`；其餘保持空載 | `NOT_RUN` |

現有三個 mission 都是空載，不能驗證 AIM-9 count、station scan、station selection、uncage 或 tone。建立 AIR-AIM9 時必須透過 DCS Mission Editor 正常儲存，不手工偽造 `.miz`。

## 3. 每次 Run 前固定資料

每個 run 建立：

```text
docs/cockpit-baseline/evidence/YYYY-MM-DD-<scenario>/
  RUN.md
  observations.csv
  dcs-relevant.txt
```

`RUN.md` 必須包含：

- Scenario ID。
- 測試日期與 Asia/Taipei 時間。
- DCS version、application revision、renderer revision、terrain revision。
- Git commit 與 working tree 是否乾淨。
- 測試用 `.miz` 的 SHA256。
- 實際載入 EFM DLL 的 SHA256。
- 完整 `dcs.log` 的 SHA256。
- 部署內容是否可追溯到該 Git commit；不能證明時寫 `UNPROVEN`。
- 測試者與測試結果：`PASS`、`FAIL` 或 `NOT_RUN`。

`observations.csv` 固定欄位：

```csv
Scenario,Area,Step,Expected,Observed,Result,Evidence
```

`dcs-relevant.txt` 只保存與 F-CK-1C、Cockpit、mission spawn、錯誤相關的行；`RUN.md` 同時保存完整 log hash，避免節錄內容被誤認成完整 log。

## 4. 共通操作

每一個 scenario 都先做：

1. 關閉 DCS。
2. 部署準備驗證的 mod 與 EFM DLL。
3. 計算 DLL 與 mission SHA256。
4. 把舊 `dcs.log` 移到 run 專屬備份位置，或確認新 log 的啟動時間。
5. 啟動 DCS，載入指定 mission。
6. 等待座艙穩定至少 10 秒。
7. 按 scenario 執行操作。
8. 正常離開 mission、關閉 DCS。
9. 計算完整 log SHA256，摘錄 relevant log，填寫 observations。
10. 執行 baseline 自動檢查，確保測試時的 source contract 沒有未記錄漂移。

## 5. AIR-CLEAN

目的：確認空中啟動、Cockpit device 載入、基本 HMCS、CMS、Radar fallback 與 Audio 資源生命週期。

| Step | 操作 | 要觀察的行為 |
|---:|---|---|
| 1 | 載入 AIR-CLEAN，等待 10 秒 | EFM DLL、CMS、Weapon、Radar State、HMCS、Audio 的載入訊息與所有 error |
| 2 | 切 NAV、DGFT、MSL、NAV | FC mode 與 HMCS mode 轉換；DCS sensor action；不得卡在舊模式 |
| 3 | 在 DGFT hold/release Uncage | uncage parameter 1→0；空載不應被誤判成真實 missile inventory |
| 4 | TMS Up press/release、TMS Down | lock start／finish 與 unlock action 成對 |
| 5 | 執行 Sound Test Cycle 三次 | playlist 向前，聲音可聽，正常 tone 狀態不被永久覆寫 |
| 6 | 檢查 HMCS 2D／VR | IAS、ALT、HDG、mode 顯示更新；無 page load error |
| 7 | 操作 AP Master | 舊版應記錄現況載入失敗；C++ 遷移版依 AP source contract 驗證 |

AIR-CLEAN 的 2026-07-28 run 只證明初始化與 log 觀察，沒有完成上表所有人工操作，因此紀錄狀態是 `CAPTURED_PARTIAL`，不是完整 `PASS`。

### Phase 2 被動 Device 動畫驗收

使用 Phase 2 DLL 與 Cockpit scripts 載入 AIR-CLEAN，另以 COLD-CLEAN
檢查起落架與前輪轉向。每一項都要以外部視角觀察並填入新的 evidence run：

| Step | 操作 | 要觀察的行為 |
|---:|---|---|
| 1 | 載入 AIR-CLEAN | log 不再嘗試載入 Gear／Actuators Lua，也沒有缺檔錯誤 |
| 2 | 左右踩 rudder 並回中 | arguments 17、18 同向到達兩端並回到中立 |
| 3 | 操作 pitch／roll／flap／airbrake | elevator、flaperon、slat、airbrake 方向與 Phase 0 相同 |
| 4 | 改變雙發推力 | afterburner 與左右 nozzle 對應正確 |
| 5 | 載入 COLD-CLEAN，收放起落架 | nose、left、right gear 同步且方向正確 |
| 6 | 地面左右轉向並滑行 | NWS 方向正確，三個 wheel-spin argument 持續更新 |

2026-07-31 已完成 AIR-CLEAN、COLD-CLEAN 與 RWY-CLEAN 實機 run；結果與
數值摘要保存在
[`evidence/2026-07-31-phase2-passive-devices/`](evidence/2026-07-31-phase2-passive-devices/)。
Phase 2 遷移範圍為 `PASS`。鼻輪 3D 外觀不隨 NWS 轉向仍為
[Issue #21](https://github.com/jjjjj103074/F-CK-1C/issues/21) 的 `KNOWN_FAIL`；
CSV 已證明 C++ NWS 輸出與 enable gate 正常，因此不歸類為本次 Lua device
移除造成的 regression。

## 6. COLD-CLEAN

目的：確認 device 初始化順序、冷啟動生命週期、重載與地面 gate。

| Step | 操作 | 要觀察的行為 |
|---:|---|---|
| 1 | 載入 COLD-CLEAN，停留 10 秒 | 所有 device 只初始化一次；保留所有 load error |
| 2 | 地面操作 AP Master／A/T | Weight-on-Wheels 阻止 engage |
| 3 | 切 Master OFF、SIM、ON | OFF／SIM 終止持續 gun、pickle、CMS program 並回 NAV |
| 4 | CMS Forward 後立刻 CMS Right | program 開始後可 abort，不再釋放後續 countermeasure |
| 5 | 重新武裝一次 | Weapon rearm event 被收到，inventory revision／重掃只發生一次 |
| 6 | 離開並重新進入 mission | 狀態重新初始化；沒有沿用前一 run 的 Lua global state |

## 7. AIR-AIM9

### Mission 建立規格

從 AIR-CLEAN 另存新檔：

- 機型與出生位置不變。
- station 1：`{AIM-9L}`。
- station 9：`{AIM-9L}`。
- station 2–8：空。
- fuel 2111、flare 90、chaff 90、gun 100 不變。
- 儲存後把檔名、SHA256 與 DCS Mission Editor 顯示的 loadout 截圖記入 `RUN.md`。

選 station 1 與 9 是故意的：目前飛機有 9 pylons，但 Lua 只掃描 7 個 station。這個情境可直接暴露漏掃翼尖的問題。

### 操作

| Step | 操作 | 要觀察的行為 |
|---:|---|---|
| 1 | 載入後保持 NAV | AIM-9 不 active；記錄實際 count，不接受無根據的 fallback 1 |
| 2 | 切 DGFT | 掃描 station；記錄每個 DCS index、CLSID、wsType、count |
| 3 | 切 MSL | 選擇規則保持一致，不重複消耗 station |
| 4 | Uncage，無目標 | seeker 進入 uncaged/search；SEEK tone |
| 5 | 取得 IR contact | contact、ACQUIRE tone |
| 6 | 取得 lock／designation | lock、TRACK seeker、LOCK tone |
| 7 | Weapon Release press/release | launch permission 與 pickle 成對；minimum hold 0.08 s |
| 8 | 發射一枚 | count 2→1；若 selected station 空，重掃另一枚 |
| 9 | 發射第二枚 | count 1→0；weapon inactive；tone OFF |
| 10 | 重新武裝 | count 回復；只處理一次 rearm revision |

如果舊 Lua 因 7-station 掃描漏掉 station 9，結果應記為 `KNOWN_FAIL`。C++ 遷移修正成 9-station 後可記為「有意修正」，不能改寫舊證據。

## 8. AP Characterization

AP 有兩組互不取代的判定：

### 舊版 runtime

- 必須捕捉 `more than 60 upvalues`。
- 結果是 `KNOWN_FAIL`。

### C++ 遷移版

以 [BEHAVIOR_BASELINE.md](BEHAVIOR_BASELINE.md) 第 4 節為功能規格，至少測：

- 239／240 kt engage boundary。
- Weight-on-Wheels engage rejection。
- roll／pitch 45° boundary。
- AP 自動 disconnect。
- A/T Mach 0.95 engage guard 與 Mach 1.00 disconnect。
- Pitch、VS、ALT、Heading Hold／Select。
- Bypass freeze 與離開後 recapture。
- NAV Track 仍維持 placeholder，不自行增加新導航功能。
- 不規則 `dt` 下與固定 0.02 s reference 的時間行為。

數值控制器優先使用 C++ characterization test；DCS 手動測試只驗證 command wiring、parameter presentation 與飛行方向是否合理。

### Phase 3 C++ 實機驗收步驟

測試前先關閉 DCS，從 repository 根目錄執行 `install.bat`。不能只建置
repository 內的 DLL，因為 DCS 實際讀取的是
`Saved Games/DCS/Mods/aircraft/F-CK-1C`。

Keyboard 預設綁定：

| 功能 | 預設鍵 |
|---|---|
| AP Master Toggle | `LAlt+A` |
| AP Master OFF | `LAlt+LShift+A` |
| AP Bypass（按住） | `LAlt+LCtrl+A` |
| ALT／Pitch／VS Hold | `LAlt+H`／`LCtrl+H`／`LShift+H` |
| Heading Hold／Select／NAV Track | `LAlt+U`／`LCtrl+U`／`LShift+U` |
| A/T Toggle | `LAlt+T` |
| Speed Increase／Decrease | `LAlt+LCtrl+T`／`LAlt+LShift+T` |
| Thrust Cut Toggle | `RAlt+Y` |

Vertical Ref Increase／Decrease 與 Heading Ref Increase／Decrease 沒有預設
keyboard 組合鍵；測試前必須在 DCS Controls 的 `Autopilot` 分類自行綁定。
不要為了測試修改 Lua route。

#### COLD-CLEAN：地面 gate

1. 載入後等待 10 秒。
2. 按一次 AP Master Toggle，再按一次 A/T Toggle；每次操作後等待至少 1 秒。
3. `fck1c_state.csv` 必須顯示：
   `afcs_master_engaged=False`、`afcs_auto_throttle_engaged=False`、
   `afcs_ap_engage_rejection_reason=2`、
   `afcs_at_engage_rejection_reason=3`。
   COLD-CLEAN 同時不符合 IAS 與 WOW；AP 沿用來源的判斷順序，先回報
   `2 = BelowMinimumIndicatedAirspeed`。A/T 沒有最低 IAS gate，因此回報
   `3 = WeightOnWheels`。
4. Controls Indicator 的 thrust-test 狀態必須是 `NORM`。

#### AIR-CLEAN：command、mode、parameter 與控制方向

1. 載入後等待 10 秒，保持穩定飛行。
2. 按 AP Master Toggle；CSV 必須出現 Master `True`、vertical mode `1`
   （Pitch Hold）、lateral mode `1`（Heading Hold）。
3. 依序切 Pitch、VS、ALT Hold；各等 1 秒。模式必須依序為 `1`、`2`、`3`，
   reference 必須在切換當下捕捉目前值。
4. 用綁定的 Vertical Ref Increase／Decrease 各操作一次；確認 target 依目前
   mode 改變後可回到原值。
5. 依序切 Heading Hold、Heading Select；用 Heading Ref
   Increase／Decrease 驗證 target 可改變並跨 0/360 wrap。
6. 切 NAV Track；CSV lateral mode 必須為 `3`，roll command 必須為 0。
   這仍是明確的 placeholder，不應自行轉向 waypoint。
7. 回到 Pitch Hold 與 Heading Hold。按住 Bypass 至少 2 秒，輕柔改變
   pitch 或 heading 超過 1° 後放開；bypass 期間 pitch／roll command 為 0，
   放開後 reference 重新捕捉目前姿態。
8. 按 A/T Toggle；CSV 的 A/T 為 `True`。短按 Speed
   Increase／Decrease，確認 target speed 變動；不要長按，因為
   `pressed` 綁定會連續送 command。
9. 按 AP Master OFF；Master、vertical、lateral 與 A/T 必須全部解除。
10. 全程觀察飛行方向：目標 pitch 高於目前值時應給正 pitch command；
    目標 heading 在左側時應給左滾 command。若有劇烈反向或持續發散，
    判定 `FAIL`。

#### RWY-CLEAN：獨立 Thrust Cut diagnostics

1. 確認雙發有非零推力並等待 1 秒。
2. 按 `RAlt+Y`；Controls Indicator 顯示 `CUT`，CSV 的
   `propulsion_test_thrust_cut_requested=True`，雙發
   `thrust_force_N` 都必須為 0。
3. 再按一次 `RAlt+Y`；顯示回到 `NORM`，intent 為 `False`，雙發推力恢復。

#### 必交證據

- `dcs.log`
- `fck1c_efm.log`
- `fck1c_state.csv`
- AIR-CLEAN、COLD-CLEAN、RWY-CLEAN 各一行人工觀察摘要

`dcs.log` 不得出現 `autopilot_system.lua`、`more than 60 upvalues`，
也不得有 AP／A/T／Thrust Cut 自訂 command 的 unknown-command 訊息。
CSV 的 mode 與 reason 編碼以 C++ enum 為準：
vertical `0/1/2/3 = Off/Pitch/VS/ALT`；
lateral `0/1/2/3 = Off/Heading Hold/Heading Select/NAV`；
reason `0/1/2/3/4/5/6 = None/Commanded/IAS/WOW/Roll/Pitch/Mach`。

## 9. CMS 計時驗收

Countermeasure 實物數量與 log 同時記錄：

| Program | 預期 |
|---|---|
| Forward | 10 flare + 10 chaff，約 10 個 1.0 s step |
| Left | 2 flare + 2 chaff，pair 間隔 0.12 s |
| Aft | 7 chaff，間隔 0.5 s |
| Right | abort 後不再增加 |

遷移 C++ 後，使用一組固定 0.02 s `dt` 與一組不規則 `dt` 重播相同 command sequence。兩組都必須得到相同釋放次數；時間容許誤差由實際 DCS action sampling 測得後寫入測試，不先猜測一個寬鬆值。

## 10. 判定規則

- `PASS`：所有必要 step 都有對應 evidence，沒有未解釋差異。
- `FAIL`：與來源契約或該版本有意修正的契約不符。
- `KNOWN_FAIL`：baseline 已明確記錄的舊缺陷。
- `CAPTURED_PARTIAL`：有真實資料，但沒有完成 scenario 的所有操作。
- `NOT_RUN`：沒有執行，不得用推測填寫 Observed。

遷移 commit 不可因為「舊版本來就壞」而省略測試。應同時呈現：

- 舊版 runtime：`KNOWN_FAIL`。
- 新版 runtime：`PASS` 或 `FAIL`。
- 差異理由：架構遷移或有意修正。
