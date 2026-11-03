# 原生機砲彈量測試

1. 關閉 DCS，將 ZIP 完整解壓到任意資料夾，執行 **Install.cmd**。
2. 開啟 Steam DCS，進入 F-CK-1C 的熱啟動任務。機砲裝填選 100%，進座艙後等 10 秒。
3. 按 **右 Ctrl + M** 開啟主保險，再按 **右 Ctrl + D** 進入 DGFT。
4. 按住 **Space 約半秒**點射，放開後等 5 秒。正常時可看到或聽到機砲射擊。
5. 關閉 DCS，執行 **Collect.cmd**，將產生的 **FCK1C-NATIVE-GUN-RESULTS-日期.zip** 傳回。

我會從記錄判斷原生入口是否相容、剩餘彈量是否下降，以及 MC 是否收到相同數量。讀取失敗時，收集檔會包含當次 CockpitBase.dll，供核對原生介面。讀取成功時只收集版本指紋與記錄。

**Restore.cmd** 可恢復安裝測試包前的模組。先執行 Collect.cmd 保留測試結果。
