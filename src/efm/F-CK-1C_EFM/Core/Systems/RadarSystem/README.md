# RadarSystem

RadarSystem 與 MissionComputer 並列，由 SystemPipeline 以 32 Hz 執行。
Targeting 提出模式與目標操作意圖；RadarSystem 執行掃描配置、捕獲與取消流程，發布原生觀測。

## 模式

- CRM／RWS：寬區掃描、姿態穩定，使用原生 TDC 指定後捕獲。
- ACM／BORE：機體參考視線的窄區；明確啟動後持續自動捕獲，直到成功或取消。
- STT：由 avSimpleRadar 回報實際狀態，成功後使用原生追蹤資料與限制。

RWS／BORE 數值集中於 `Internal/SearchProfiles.h`；探測距離與 STT 角限沿用既有載入校準。
參數依據、借用年代與原生條帶映射見 [模式設定](../../../../../../docs/RADAR_MODES.zh-TW.md)。
其他模式入口回報 Reserved，保留當前已支援配置。

## 資料流

輸入為 FrameInput 的雷達觀測、介面可用性、外部動作執行確認，以及 RadarRequests。
輸出為 RadarSnapshot：來源觀測、實際搜尋配置、RadarScanControl、要求結果與有序設備動作。
既有 DcsBridge 匯出器將掃描設定寫入原生參數，再發布命令傳送視窗。
所有 System 間資料經 Pipeline 的具型別快照交換。

每筆要求依來源編號執行一次；要求結果保存至 MC 確認前綴。
RWS 捕獲脈衝維持至少 80 ms，從首次觀測到 DcsBridge 執行確認的 input_time 起算。
BORE 自動捕獲持續等候目標；STT 成功後結束捕獲輸入，並在失鎖回 SEARCH 時依已啟用意圖重啟。
TMS 拒絕／模式退出會結束自動捕獲。STT 退出與捕獲鍵放開採不同原生操作。

完整通訊規則見 [架構說明](../../../../../../docs/RADAR_TARGETING_ARCHITECTURE.zh-TW.md)。
