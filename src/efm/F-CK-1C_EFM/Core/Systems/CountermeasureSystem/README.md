# CountermeasureSystem

`CountermeasureSystem` 與 `MissionComputer`、`RadarSystem` 同層，以 32 Hz 透過 SystemPipeline 溝通。

```text
CMS 輸入 → MC FIFO → CountermeasureControl → CountermeasureRequests
→ CountermeasureSystem → CountermeasureSnapshot.dispense
→ DcsBridge CountermeasureExecutor → DcsAircraftInterface → context / human
→ 原生施放與庫存 → FrameInput.countermeasures → CountermeasureSystem
```

MC 保存程序要求至反制系統接受，前向為十輪混合施放、間隔一秒，左向程序 6 為七發干擾絲、間隔半秒。
程序、計時與進度由本系統管理。新程序接替原程序，放開 CMS 後完成已啟動的程序。
每輪等到 Bridge 的相符執行回覆才計算下一輪時間；間隔從實際執行時間起算。
低 FPS 補算系統 tick 時，相同待執行序號保持一筆，Bridge 依序號執行一次。

Bridge 持有共用原生介面，每輪對每個選取的種類各呼叫一次直接施放；並記錄前後庫存、存取狀態及例外碼。
原生呼叫失敗時程序進入 Failed，駕駛下一次明確操作可重新啟動。
配對一輪中的兩種類分別回報結果；其中一類耗盡時，另一類已成功施放的結果仍保留。

接受序號、原生呼叫返回與實際庫存是三種不同資訊：Completed 表示程序要求的原生呼叫已返回；
實際消耗由 inventory 的有效觀測與 `countermeasure_dispense` 日誌核對。
`CPP_CM_*` 座艙參數供儀表與飛測記錄讀取。前向與程序 6 是本專案目前的固定設定。

新飛行由 Pipeline 重建程序與要求序號，Bridge 同時清除執行回覆與原生解析結果。
