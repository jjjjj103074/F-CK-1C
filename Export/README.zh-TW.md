# 原生機砲彈量 Export

`StoresExport.lua` 在 DCS 的 Export Lua 環境執行，以 10 Hz 讀取玩家 F-CK-1C 的
`LoGetPayloadInfo().Cannon.shells`，透過 CMS 裝置命令傳給座艙，再由 DcsBridge 送入 MC。

模組內的 `Export` 資料夾保存執行程式。DCS 的載入入口是：

`Saved Games/DCS/Scripts/Export.lua`（使用 DCS.openbeta 時改用對應資料夾）。

將以下區塊加入該入口；既有其他匯出器的內容一併保留：

```lua
do
    local ok, err = pcall(dofile, lfs.writedir() .. [[Mods/aircraft/F-CK-1C/Export/StoresExport.lua]])
    if not ok and log and log.write then log.write("FCK1C_STORES_EXPORT", log.ERROR, tostring(err)) end
end
```

從原始碼工作目錄也可使用安裝工具：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/install_stores_export.ps1 -SavedGamesRoot "$env:USERPROFILE/Saved Games/DCS"
```

停用時追加 `-Action Remove`；工具只移除自己加入的完整區塊。工具建立既有 Export 的備份，
重複安裝保留單一載入入口。手動加入的區塊可由編輯器移除。

請在 DCS 關閉時設定。第一次安裝此版來源，先完成對應 Windows DLL 重建與驗證。
未啟用 Export 或來源逾時時，MC 保留最近有效數字並標記有效性，HMCS 的機砲欄顯示 `GUN ---`。

資料流與驗證紀錄見原始碼的 `docs/SM_NATIVE_AMMUNITION.zh-TW.md`。
