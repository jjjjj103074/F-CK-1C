# FireControl 與 Targeting

FireControl 組裝 Targeting 與 WeaponOperation，分派操作及程序結果。目標取得策略由 Targeting 擁有，雷達設備操作由 [RadarSystem](../../../RadarSystem/README.md) 經 SystemPipeline 執行。

| 位置 | 職責與主要依賴 |
|---|---|
| FireControl | 操作分派、主模式通知、武器正常結束通知及更新順序 |
| Internal/Targeting | 唯讀 ModeData／RadarSnapshot；寫入 TargetingData；透過 RadarRequestQueue 提出雷達任務 |
| Internal/WeaponOperation | 使用資格、解籠及開始／結束意圖；唯讀 WeaponUseData；使用 SM 指令入口 |
| RadarSystem | 設備要求展開、捕獲輸入 ACK／保持／放開、雷達觀測與操作結果 |
| DcsBridge/ExternalActionTransport | 共用 DCS 序號、16 項傳送視窗及執行確認對應 |

`FireControlTargetInput.cpp` 與 TargetDesignation 類別已移除。TMS、解籠的目標配合、NAV／DGFT／MSL 的捕獲策略及投放正常結束的取消指定規則，由 Targeting 完整管理。

## 目標與武器操作

```mermaid
flowchart LR
    INPUT[操縱與模式事件] --> FC[FireControl 分派]
    FC --> T[Targeting 目標流程]
    FC --> W[WeaponOperation 武器意圖]
    W --> SM[SM 執行]
    W -->|正常投放結束通知經 FC 轉交| T
    T -->|RadarRequests| PIPE[SystemPipeline]
    PIPE --> R[RadarSystem]
    R -->|RadarSnapshot| PIPE
    PIPE --> T
```

主保險 OFF／SIM 由 WeaponOperation／SM 停止武器操作；Targeting 使用操作模式及雷達回報。
模式切換保留保險。SM 的選站、停止、取消與武器保持規則維持原設計。

TargetingData 保存期望的捕獲方式與指定意圖。雷達設備來源的工作模式、幾何及有效性由 RadarSystem 發布，再由 Targeting 整理成目前的目標資料。指定意圖與 STT 觀測各自保存。

解籠操作由 FC 分派給 WeaponOperation 及 Targeting。TMS Left 維持既有空操作；Hud 維持保存捕獲選擇；其餘按鍵、DGFT 進入捕獲、投放前重送模式及投放結束取消指定維持原有規則。

## 相關說明

- [跨 System 架構、序號與驗證](../../../../../../../../docs/RADAR_TARGETING_ARCHITECTURE.zh-TW.md)
- [MC 組裝](../../README.md)
- [先前 IR 觀測整理](../../../../../../../../docs/IR_OBSERVATION_REFACTOR_REPORT.zh-TW.md)
