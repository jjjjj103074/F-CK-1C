# F-CK-1C 經國號

[English](README.md)

DCS World 社群模組，重現 AIDC F-CK-1C 經國號。

## 狀態

Release 支援任務中的 AI 飛機。玩家座艙與 EFM 仍在開發，
開發版本位於 [dev](https://github.com/jjjjj103074/F-CK-1C/tree/dev)。

EFM 目前提供可設定的三軸直接映射；回授控制、配平、自動駕駛與自動油門尚未實作。
控制路徑與限制見 [DFLCC 指南](src/efm/F-CK-1C_EFM/Core/Systems/DigitalFlightControlComputer/README.md)。

## 安裝

從 [Releases](https://github.com/jjjjj103074/F-CK-1C/releases/latest) 下載套件，執行 `install.bat`，
或將模組資料夾複製到 `Saved Games/DCS/Mods/aircraft/F-CK-1C/`。

安裝器會刪除並替換既有模組資料夾；執行前先備份其中的自訂內容。

Saved Games 位於自訂位置時：

```bat
install.bat "D:\Saved Games\DCS"
```

## 開發

- [貢獻指南](CONTRIBUTING.md)：環境、流程與提交規則。
- [EFM](src/efm/README.md)：架構與各模組指南。
- [座艙](docs/COCKPIT.md)：Lua device、輸入路由與參數。
- [建置與驗證](docs/BUILD_DLL.md)：工具鏈與本機指令。
- [DCS 測試](docs/DCS_TESTING.md)：驗證情境與觀察項目。

錯誤與功能建議請提交至 [Issues](https://github.com/jjjjj103074/F-CK-1C/issues)。

## 研究

[航空參考資料庫](docs/reference-library/README.md) 提供英文的來源、知識、評級與版權規範。
目前已建立基礎架構；尚未收錄資料或實作自動搜尋。

[控制律模型](research/control-law-diagrams/README.md) 提供獨立的
F-16 Modelica 研究模型與驗證工具。

## 授權

程式碼、工具、文件與生成的執行期二進位檔採 MIT；
非程式素材未另行標註時採 CC BY 4.0。
詳見 [LICENSE](LICENSE) 與保留的 [EFM 授權](src/efm/LICENSE)。
