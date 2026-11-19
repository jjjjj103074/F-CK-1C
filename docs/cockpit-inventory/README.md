# 座艙來源清冊

清冊整理目前來源中的裝置、命令與參數關係，供座艙整合與問題定位使用。
CSV 由工具按需產生於 `tmp/cockpit-inventory/`，不納入 Git。

## 來源清冊

| 檔案 | 內容 |
|---|---|
| `source-manifest.csv` | 來源路徑、大小、行數與 SHA256 |
| `device-load-chain.csv` | Device、indicator 與載入順序 |
| `command-routing.csv` | 自訂命令路由及 Input／Lua 引用 |
| `dcs-action-usage.csv` | Lua 使用的 DCS action |
| `parameter-access.csv` | 參數 reader、writer 與呈現位置 |

## 產生與檢查

在專案根目錄執行：

```powershell
npm run generate:cockpit-inventory
npm run test:cockpit
```

指令使用 Windows PowerShell 5.1。Generate 輸出五份清冊；
`test:cockpit` 執行 DCS ID 產生器測試、座艙架構檢查與檢查器 fixtures。

架構檢查每次直接掃描目前來源，在記憶體中建立參數清冊，
驗證單一 writer、宣告的 writer 與實際來源，以及參數引用和 adapter 責任。
執行檢查前不必產生 CSV。`validate:efm` 與適用的 PR 提交檢查也會執行這組檢查。

清冊記錄來源內容與引用關係，不證明遊戲內行為正確。
現行責任見[座艙指南](../COCKPIT.md)，
遊戲內驗證見 [DCS 測試](../DCS_TESTING.md)。
