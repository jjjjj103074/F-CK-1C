# 2026-07-28 AIR-CLEAN

## 結論

這是一份真實但不完整的 Air Clean 存證，狀態為 `CAPTURED_PARTIAL`。

它證明 EFM DLL、CMS、Weapon、Radar State 與 AAM Audio 曾在 DCS 中被載入，也證明 Actuators 與 Autopilot Lua 當時載入失敗。該 run 沒有依新規範完成全部人工按鍵操作，所以不能標示為 `PASS`。

## Run identity

| 欄位 | 值 |
|---|---|
| Scenario | AIR-CLEAN |
| DCS log opened | 2026-07-28 12:39:45 UTC |
| Local timezone | Asia/Taipei |
| DCS | 2.9.28.26283 x86_64 MT |
| Application revision | 266283 |
| Renderer revision | 26305 |
| Terrain revision | 27850 |
| Recorded repository HEAD | `9c60eac9191c23b24fdeb01dd34eba0904214d57` |
| Recorded branch | `專案整理` |
| Deployed source ↔ HEAD association | `UNPROVEN`；只知道該次載入 binary hash，不宣稱 binary 一定由該 HEAD 建置 |
| Result | `CAPTURED_PARTIAL` |

## Binary／mission／log identity

| Artifact | Bytes | Modified (Asia/Taipei) | SHA256 |
|---|---:|---|---|
| `Saved Games/DCS/Mods/aircraft/F-CK-1C/bin/F-CK-1C_EFM.dll` | 551936 | 2026-07-28 18:41:24 +08:00 | `7A93F19365253C8D1E110143C1A2AD7CB4F78CD4997F8B477900F9AC136DD253` |
| `AppData/Local/Temp/DCS/tempMission.miz` | 6596 | 2026-07-28 20:40:17 +08:00 | `B423713FCBC02BC8C8A1E05B6F044AFD83A32EAD98E51A07FB2AD2D7655BCAB5` |
| `Saved Games/DCS/Logs/dcs.log` | 108268 | 2026-07-28 20:40:59 +08:00 | `CCBE707B926ACAE9C95741BA97A5184736A71B29C1555F41316DE8AEA1A1C00C` |

Temp mission 已檢查為：

- Caucasus。
- F-CK-1C air start。
- altitude 2000 m BARO。
- speed 220.972222 m/s。
- fuel 2111、flare 90、chaff 90、gun 100。
- 9 個 pylon 全空。

## 證據

- 原始摘錄：[dcs-relevant.txt](dcs-relevant.txt)
- 結構化觀察：[observations.csv](observations.csv)

## 限制

- 沒有保存當時的畫面或聲音錄影。
- 沒有完整執行 NAV／DGFT／MSL、TMS、CMS program、Sound Test。
- 空載 mission 不能驗證 AIM-9 station scan 與發射。
- 部署 binary 與 repository commit 的建置關係沒有可驗證 manifest。

這些限制不補猜測；下一次應依 [DCS_TEST_PROTOCOL.md](../../DCS_TEST_PROTOCOL.md) 產生新的完整 run。
