# FireControl 與 Targeting

FireControl 組裝 Targeting 與 WeaponOperation，分派操作、模式事件及更新順序。MC 依 SOI 將 FCR 的 TMS 操作送進火控。目標取得策略由 Targeting 擁有，雷達設備操作由 [RadarSystem](../../../RadarSystem/README.md) 經 SystemPipeline 執行。

| 位置 | 職責與主要依賴 |
|---|---|
| FireControl | 操作分派、主模式通知及更新順序 |
| Internal/Targeting | 唯讀 ModeData／RadarSnapshot；寫入 TargetingData；透過 RadarRequestQueue 提出雷達任務 |
| Internal/WeaponOperation | 使用資格、UNCAGE 與開始／結束意圖；唯讀 SelectedWeaponState／WeaponUseData；使用 SM 指令入口 |
| Internal/WeaponProcedures | 依已解析程序、主保險／主模式、雷達追蹤及 UNCAGE 按鈕計算共用發射判定 |
| RadarSystem | 設備要求展開、捕獲輸入 ACK／保持／放開、雷達觀測與操作結果 |
| DcsBridge/ExternalActionTransport | 共用 DCS 序號、16 項傳送視窗及執行確認對應 |

Targeting 管理 TMS，以及一般 A-A 與 DGFT／MSL 超控的雷達捕獲策略。WeaponOperation 將武器使用開始／結束交給 SM，並整理 SM 執行結果。

## 目標與武器操作

```mermaid
flowchart LR
    INPUT[操縱與模式事件] --> FC[FireControl 分派]
    FC --> T[Targeting 目標流程]
    FC --> W[WeaponOperation 武器意圖]
    W --> SM[SM 執行]
    T -->|RadarRequests| PIPE[SystemPipeline]
    PIPE --> R[RadarSystem]
    R -->|RadarSnapshot| PIPE
    PIPE --> T
```

主保險 OFF／SIM 由 WeaponOperation／SM 停止武器操作；Targeting 使用操作模式及雷達回報。
模式切換保留保險，並由 WeaponOperation 結束先前配置的武器使用。MSL STEP 的短按送出同型輪站意圖，按住達 0.5 秒送出一次換型意圖；SM 決定實際掛點。超控切換規則及座艙指令見[模式架構](../../../../../../../../docs/MC_MODE_EVENTS.md)。

TargetingData 保存期望的捕獲方式與指定意圖。雷達設備來源的工作模式、幾何及有效性由 RadarSystem 發布，再由 Targeting 整理成目前的目標資料。指定意圖與 STT 觀測各自保存。

WeaponOperation 將解籠切換與雷達視線送入 SM 佇列；SM 依確認選站保存要求與武器能力。
投放共同使用 ON＋空戰模式＋有效武器資格，再由 `WeaponProcedures` 選擇 AIM-7／AIM-9／AIM-120 共用流程。
AIM-9 沿用原生投放條件；AIM-7 正常路徑使用有效 STT；AIM-120 使用有效 STT，或按住 UNCAGE 的目視發射同意。
FC 讀取 SM 已解析的型號與程序，掛載辨識及選站執行由 SM 擁有。
UNCAGE 實體輸入保存按下／放開：AIM-9 按下切換自主追蹤，AIM-120 按住提供目視發射同意。TMS 依 1997 年手冊處理 CRM／ACM 操作與右向 1 秒長按；設備能力由 RadarSystem 回報。投放前由 Targeting 確認模式；投放結束由 SM 完成武器使用，雷達指定與捕獲流程持續運作。CRM 的 TMS 下（K）取消追蹤；ACM 的 I／K／L 拒絕舊目標並捕獲 BORE／10×60／30×20。
A-A／DGFT／MSL OVRD 切換保留有效 STT；武器使用隨配置切換正常結束。
程序來源及本次實作範圍見 [武器程序](../../../../../../../../docs/WEAPON_PROCEDURES.zh-TW.md)。

操作表、來源與預留功能見 [感測器操作](../../../../../../../../docs/MC_SENSOR_CONTROLS.zh-TW.md)。

## 相關說明

- [跨 System 架構、序號與驗證](../../../../../../../../docs/RADAR_TARGETING_ARCHITECTURE.zh-TW.md)
- [MC 組裝](../../README.md)
- [先前 IR 觀測整理](../../../../../../../../docs/IR_OBSERVATION_REFACTOR_REPORT.zh-TW.md)
