# 2026-07-31 Phase 3 Autopilot 首次驗收

## 結論

Phase 3 首次 DCS 驗收結果為 `FAIL`，唯一新增的阻擋問題是 ALT Hold
持續上下擺盪。其餘已操作的 AP／A/T 模式未由測試者發現明顯異常；
Vertical／Heading Reference 增減因沒有預設按鍵綁定而標記為 `NOT_RUN`。

CSV 證明 ALT Hold 並非輕微晃動：目標高度為 `1913.361 m`，117.45 秒內
實際高度到達 `1912.201..2037.086 m`，pitch command 反覆到達
`-0.6..+0.6`，G 值到達 `-1.772..4.827`。因此本次 run 不得判定為
Phase 3 完成。

## Run identity

| 欄位 | 值 |
|---|---|
| Scenario | COLD-CLEAN、AIR-CLEAN、RWY-CLEAN |
| 測試日期 | 2026-07-31，Asia/Taipei |
| DCS | 2.9.28.26385 x86_64 MT |
| Application revision | 266385 |
| Renderer revision | 26305 |
| Terrain revision | 27850 |
| Recorded repository HEAD | `546f8ccb93eac08cb249b31ae910d5a5ca87a006` |
| Recorded branch | `專案整理` |
| Working tree | dirty；測試當下未固定 status entry 數量 |
| 測試者 | 使用者執行操作；Codex 解析 CSV／log |
| Phase 3 migration result | `FAIL` |
| Blocking result | ALT Hold 明顯上下擺盪 |
| Manual coverage gap | Vertical／Heading Reference 增減 |

## Binary identity

| Artifact | Bytes | SHA256 |
|---|---:|---|
| source `bin/F-CK-1C_EFM.dll` | 579072 | `913AE6FACC10CBC8CA074A708B62A3704568A4279306B2DDBDAAB11BE0BC2644` |
| installed `F-CK-1C_EFM.dll` | 579072 | `913AE6FACC10CBC8CA074A708B62A3704568A4279306B2DDBDAAB11BE0BC2644` |

## 原始測試檔案

原始檔保存在 repository 的 `.tmp/`，不納入 Git；下列 hash 固定本次
分析來源。

| 原始位置 | 修改時間（Asia/Taipei） | Bytes | SHA256 |
|---|---|---:|---|
| `.tmp/fck1c_state.csv` | 2026-07-31 13:27:36 | 35948574 | `DF7B9A32D7A55CA2B7089366E99D36310D66C580775DA2FE2042185653133A88` |
| `.tmp/fck1c_efm.log` | 2026-07-31 13:27:37 | 1439 | `A7448C9677B119C654A75A79B9C0568AEED7BDBFF418E68A9E3737438D44841A` |
| `.tmp/dcs.log` | 2026-07-31 13:27:42 | 157305 | `50049301489553C61708CC946604B1F3DB72D4E3F4A2B61B6E81E06CCBE4EB0D` |

## Scenario 摘要

### COLD-CLEAN

- `cold_ground`，29.928 s，4988 rows。
- AP 全程未接通；engage rejection reason 為
  `BelowMinimumIndicatedAirspeed`。
- A/T 全程未接通；engage rejection reason 為 `WeightOnWheels`。

### AIR-CLEAN

- `hot_air`，163.830 s，27296 rows。
- 已出現 Pitch Hold、VS Hold、ALT Hold、Heading Hold、Heading Select、
  NAV Track 與 bypass；A/T 有 4580 rows 為 engaged。
- 測試者回報除 ALT Hold 外沒有特殊感覺。
- ALT Hold 有 19571 rows，command 到達負向飽和 7517 rows、正向飽和
  2674 rows；808 個零 command rows 對應 bypass。
- AP 與 A/T 最後都有 `Commanded` disengage reason。

### RWY-CLEAN

- `hot_ground`，29.724 s，4955 rows。
- Thrust Cut Test 有 404 rows 為 requested。

## 根因量測

將 ALT Hold command 與稍後的 G 值做延遲比對，最高相關落在約
`0.55..0.60 s`，斜率約為每 1.0 normalized pitch command 產生
`4.1 G` 的變化。原 ALT Hold 比例增益 `0.08` 沒有為這個 FBW 增益與
延遲保留足夠穩定裕度，因此控制器在飛機尚未完成前一次反應前就繼續
加大反向指令。

回歸測試以本次量得的 frame interval、延遲與 command-to-G 反應建立
最小閉迴路；修正前四個 damping 條件全部失敗。ALT Hold 已改用獨立
比例增益 `0.04`，VS Hold 保持原值 `0.08`，積分器、band 與 limit
均未改動。修正後 native tests 為 `2282 checks, 0 failures`。

這是首次實機驗收發現後的必要穩定性修正，不代表 DCS 已通過；仍需
使用新 DLL 重測。

## Log 結果

- DCS 成功載入 installed `F-CK-1C_EFM.dll`。
- `dcs.log` 沒有 `autopilot_system.lua` 載入紀錄。
- 三個 run 都保留 command `2659` unknown warning；這是既有 DCS command，
  沒有證據指向本次 AP route。
- damage model corrupt 與 HMCS VR parent 缺失仍存在，沒有被隱藏。

## 證據

- [結構化觀察](observations.csv)
- [CSV 數值摘要](state-summary.csv)
- [DCS／EFM relevant log 摘錄](dcs-relevant.txt)

## 限制

- 沒有保存畫面或錄影；操控感受依測試者回報。
- Reference 增減沒有預設鍵，因此本次沒有人工操作。
- 回歸測試固定已量得的控制鏈，不取代 DCS 實機複測。
