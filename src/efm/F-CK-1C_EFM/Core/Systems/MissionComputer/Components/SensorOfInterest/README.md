# SensorOfInterest

依唯讀 ModeData 與 DisplaySnapshot，唯一寫入 SensorOfInterestSnapshot。

`refresh()` 接收相符模式的顯示對應，保留有效來源，並讓來源隨顯示位置變化移動；
模式或來源改變後套用可用的預設焦點。`handle()` 處理 DMS 上下的焦點選擇。

MC 使用選擇結果分派 TMS；Targeting 解讀 FCR 操作，RadarSystem 執行設備要求。
NAV 的 HSD／HUD 與 A-G 的預留入口照樣提供可辨識的來源，供後續功能接入。

操作規則與來源見 [感測器操作](../../../../../../../../docs/MC_SENSOR_CONTROLS.zh-TW.md)。
