# 2026-07-31 Phase 2 被動 Device 驗收

## 結論

Phase 2 遷移範圍驗收結果為 `PASS`。

三次獨立 DCS run 分別覆蓋 AIR-CLEAN、COLD-CLEAN 與 RWY-CLEAN。
EFM DLL 均成功載入，log 不再出現 Gear／Actuators Lua 的載入嘗試或缺檔錯誤。
飛控、襟翼／縫翼、減速板、起落架、雙發後燃器／噴嘴、煞車與三輪轉速
均在 CSV 中留下完整狀態變化。

唯一由測試者回報的外觀問題是鼻輪不隨 NWS 轉向。CSV 顯示 C++ NWS 輸出
已到達 `-0.75..+0.75`，而且開關 gate 有效，因此此問題位於後續 3D 模型
呈現鏈，沿用既有 [Issue #21](https://github.com/jjjjj103074/F-CK-1C/issues/21)
追蹤，不列為本次 Gear／Actuators Lua 移除造成的 regression。

## Run identity

| 欄位 | 值 |
|---|---|
| Scenario | AIR-CLEAN、COLD-CLEAN、RWY-CLEAN |
| 測試日期 | 2026-07-31，Asia/Taipei |
| DCS | 2.9.28.26385 x86_64 MT |
| Application revision | 266385 |
| Renderer revision | 26305 |
| Terrain revision | 27850 |
| Recorded repository HEAD | `546f8ccb93eac08cb249b31ae910d5a5ca87a006` |
| Recorded branch | `專案整理` |
| Working tree | dirty；60 個 status entries |
| Deployed source association | `UNPROVEN` at commit level；source 與 installed DLL hash 相同 |
| 測試者 | 使用者執行操作；Codex 解析 CSV／log |
| Phase 2 migration result | `PASS` |
| Excluded known issue | NWS 3D 外觀不轉向；Issue #21 |

## Binary 與 mission identity

| Artifact | Bytes | SHA256 |
|---|---:|---|
| source `bin/F-CK-1C_EFM.dll` | 563712 | `CBC07217D58A738F3120DBF7114D90B336D290C33D0C32562866526A46829EFE` |
| installed `F-CK-1C_EFM.dll` | 563712 | `CBC07217D58A738F3120DBF7114D90B336D290C33D0C32562866526A46829EFE` |
| AIR-CLEAN `f-ck-1c_air.miz` | 6562 | `2D5A30848B1DF9D6CE78D8864EF1314FB26C4D23135D65965A372679519D3E67` |
| COLD-CLEAN `f-ck-1c_cold.miz` | 6605 | `C1B14BB6170361E2DBD3361A9788529A85B200A7DBCB648AD32D0A174D96F67B` |
| RWY-CLEAN `f-ck-1c.miz` | 6638 | `2DE5B29092DC0C8D5EE74AEB4E11DDA4FB42CA30330A4A6088894286AF3ABB85` |

## 原始測試檔案

原始檔保存在 repository 的 `.tmp/`，不納入 Git；下列 hash 固定本次分析來源。

| Scenario | Artifact | Bytes | SHA256 |
|---|---|---:|---|
| AIR-CLEAN | `dcs_1.log` | 106372 | `C190BEC0A9BE38076A5427017E1BC6C3201F0200FCA2EAB9BE9AA31B0B8EE408` |
| AIR-CLEAN | `fck1c_efm_1.log` | 478 | `E9CB72F1817EA6DB4E8843CA955BDFC5BD7220D5CA3717ADF4C3682CA7F8D3C3` |
| AIR-CLEAN | `fck1c_state_1.csv` | 11237148 | `B3EA2A357D7930D7714EF612903652F2815709FD92BCDFDE7D375497097D24F8` |
| COLD-CLEAN | `dcs_2.log` | 108030 | `6458502A0BDAEAAFDE365916A71134A291383DB7FA1E61F73995FBF74CD1EB78` |
| COLD-CLEAN | `fck1c_efm_2.log` | 482 | `D67D0045425980A4C9AC663293270346C4560B02A39E2FC8BFAEE04F6BB24CAA` |
| COLD-CLEAN | `fck1c_state_2.csv` | 11861291 | `324D6C1299AA6E7087CE24CF90B98B86BA303EF7C17621DD8B814A5561E106A8` |
| RWY-CLEAN | `dcs_3.log` | 106452 | `822745A95EED2AA51DF18A1AB47F61C9A161573ED49510F9942EC9B2CEE919D0` |
| RWY-CLEAN | `fck1c_efm_3.log` | 481 | `F7ECED491F0CA930C04A6E81ECF73497AD4473B34A36872DFA3794921C601DF3` |
| RWY-CLEAN | `fck1c_state_3.csv` | 6099464 | `4C7780AEE47748489AFCF529B0410429C423E4AD0D7CA126DC942C0C12309C8A` |

## Scenario 摘要

### AIR-CLEAN

- `hot_air`，87.486 s，14579 rows。
- pitch、roll、yaw input 均到達 `-1..+1`。
- elevator、aileron、rudder command 均有雙向輸出。
- flaps、slats、airbrake、gear 均到達 `0..1` 並回到初始狀態。
- 左右 throttle、afterburner ratio、nozzle aperture 的最大差值都是 0。
- 左右 afterburner 都有 1020 rows 為 lit。

### COLD-CLEAN

- `cold_ground`，71.334 s，11890 rows。
- gear 全程為 1，三輪承重。
- NWS 輸出到達 `-0.75..+0.75`。
- yaw 有效時，同時存在 859 rows 的 NWS 動作與 363 rows 的 NWS 關閉狀態，
  證明 NWS gate 有效。
- 測試者回報鼻輪 3D 外觀不轉向；列為 Issue #21 `KNOWN_FAIL`。

### RWY-CLEAN

- `hot_ground`，34.398 s，5734 rows。
- NWS 輸出到達 `-0.75..+0.75`。
- nose／left／right wheel spin 最大值分別為
  `0.999878`、`0.999989`、`0.999989`。
- 左右主輪轉速最大差值為 0。
- 左右煞車均到達 `0..1`，同時有效 1022 rows。

## 已知、非本 Phase 2 問題

三次 run 都保留並重現：

- damage model 缺少 `lineFG`、`lineLG`、`lineRG`，並回報 corrupt。
- `autopilot_system.lua` 超過 60 upvalues，載入失敗。
- HMCS `hmcs_vr_root` 找不到 `hmcs_vr_tfov` parent。
- EFM 每次都收到 command `2659` 兩次並記為 unknown；三個 start mode 都一致，
  沒有證據顯示它與 Phase 2 Gear／Actuators 路徑相關。

這些錯誤沒有被隱藏，也不是本次 Gear／Actuators Lua 移除所引入。

## 證據

- [結構化觀察](observations.csv)
- [CSV 數值摘要](state-summary.csv)
- [DCS relevant log 摘錄](dcs-relevant.txt)

## 限制

- 沒有保存畫面或錄影；外觀結果依測試者回報。
- 原始 CSV／log 位於 ignored `.tmp/`，repository 只保存 hash、摘要與 relevant log。
- working tree 在測試時不乾淨，因此不能把部署內容宣稱為某一個 commit 的可重建產物；
  但 source 與 installed DLL hash 已證明相同。
