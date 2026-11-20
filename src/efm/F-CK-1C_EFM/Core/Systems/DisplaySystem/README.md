# DisplaySystem

與 MissionComputer 並列，以 32 Hz 經 SystemPipeline 交換具型別快照。

- 讀取 MissionComputerSnapshot 的生效操作模式。
- 唯一寫入 DisplaySnapshot：左右 MFD 功能、各位置的控制來源及 HUD 指定入口。
- 保存 DMS 左／右的格式輪換要求，供後續頁面選擇接入。
- 讀取 RadarSnapshot，由 Internal/RadarPresentation 計算前視捕獲／追蹤符號、回波淡出與右下角雷達圖資料。

目前由模式固定配置：NAV 為 HSD／SMS，其餘模式為 FCR／SMS。這是專案預設，
實際 F-16 的 MFDS 格式可配置。雷達圖形由座艙指示器繪製；完整 MFD 頁面與導航功能保留後續實作。
圖形資料流、預設鍵與驗證見 [雷達顯示](../../../../../../docs/RADAR_DISPLAY.zh-TW.md)。

MC 的 SensorOfInterest 讀取上述對應，保存 SOI 並回傳於任務快照；
DisplaySystem 與 MC 透過 Pipeline 傳遞資料，各自維持唯一寫入責任。

真機文件、操作表、預留功能及 debug 欄位見 [感測器操作](../../../../../../docs/MC_SENSOR_CONTROLS.zh-TW.md)。
