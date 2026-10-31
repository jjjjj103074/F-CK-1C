# MC 模式狀態與同步事件

本次將 ModeManagement 對火控與反制的直接操作改為有序同步事件，保留現有功能。
此為 DCS 專案架構，不代表實機 MC 或硬體保險的實作。

## 第三方依賴

sigslot v1.2.3 原始標頭與 MIT 授權放在
`src/efm/F-CK-1C_EFM/ThirdParty/sigslot/`，版本、完整 SHA 與檔案雜湊見該處 README。
沿用 EfmBuild.targets 的 ThirdParty 路徑，所有 EFM C++ 與原生測試可使用
`<sigslot/signal.hpp>`。無額外 runtime DLL，也不在建置時下載依賴。

採用 `signal_st`、具型別參數與 scoped_connection；只有需要先後的監聽者指定不同 group。
上游標頭不自行修改。格式化已排除 ThirdParty；架構檔案長度檢查只新增該原始標頭的例外，
不放寬自有程式的限制。檢查用的相對路徑統一為反斜線，避免 Linux PowerShell 因分隔符不同
而漏掉依賴規則或誤判 SDK／第三方檔案。使用連線物件解除訂閱，不以函式地址識別回呼。

## 元件與資料所有權

| 元件／資料 | 責任 |
|---|---|
| ModeManagement | ModeData 唯一寫入者；處理主保險與模式操作並發布選擇事件 |
| FireControl | 讀取模式；接收主保險事件停止操作、接收模式事件更新自身程序 |
| CountermeasureControl | 讀取主保險資格；處理 CMS 命令與主保險導致的中止 |
| StoresManagement | 讀取模式；既有掛載與選站工作 |
| ModeData | MC 持有的記憶體資料；其他元件取得 const 引用 |
| ModeEvents | 每一台 MC 持有的兩個同步通道；不是資料庫、System 或排程工作 |

ModeManagement 只依賴 ModeData 與模式事件，不知道 FireControl 或反制的具體實作。
MC 集中連接訂閱，並持有 scoped_connection。連線先於接收者銷毀；通道晚於接收者銷毀。
其他系統可共用 sigslot 套件，但各自決定事件範圍；本次沒有導入全機全域廣播中心。
跨 System 與 Lua 的溝通繼續使用既有資料／Bridge 契約。

原本三個任務元件加四個基礎元件，改成四個任務元件加四個基礎元件。
CountermeasureControl 是命令／事件處理者，沒有新增週期工作或獨立執行緒。
DFLCC 的資料庫先建立、固定參照與單一 Executive 原則保持適用，無須新增對應反制元件。

## 名稱與對外快照

- `FireControlMode` 改為 `OperatingMode`，維持 Navigation=0、Dogfight=1、MissileOverride=2。
- `ModeData.fire_control` 改為 `ModeData.operating_mode`。
- Master Arm、operating_mode 與 dogfight 指示移到 `CombatAvionicsSnapshot.mode` 的 ModeSnapshot。
- FireControlSnapshot 保留目標子模式、武器要求與射擊指示。
- Bridge 映射至原有 HMCS_FC_MODE 等座艙參數，外部名稱、數值與 Lua/HMCS 行為不變。

## 指令與事件順序

```text
駕駛指令 → MC 共用 FIFO
  模式／保險 → ModeManagement → ModeData
                             → MasterArmSelected / OperatingModeSelected
                             → sigslot 同步呼叫訂閱者
  CMS       → CountermeasureControl（唯讀 ModeData）
  火控      → FireControl（唯讀 ModeData）
  每筆命令及通知完成後，才取出下一筆命令
→ 原 Executive → 發布 ModeSnapshot／火控／掛載資料
→ Bridge → 原有 Lua 介面與 HMCS
```

主保險 OFF/SIM：先更新主保險 → 火控停止（group 0）→ CMS 中止（group 1）
→ 選 NAV → 火控處理模式。保留停止、CMS abort、模式動作的原有先後。
事件含選擇前後的狀態副本，不以後續狀態覆蓋當次操作；連續 OFF→ON 仍保留停止。
事件命名為 Selected，重選同一模式仍會執行既有武器循環。
回呼不能遞迴消費 MC 命令；不把各型事件累積至幀末再分組通知。

## 保留的功能規則

32 Hz、原 FIFO、ACK 與外部動作重試機制不變。DGFT/MSL 自動 Master ON、
Master ON 回 NAV、NAV 不強制新增停止規則、OFF/SIM 停止、CMS Right 無條件中止，
以及 CMS Press 保留狀態均維持。反制施放程序仍在 Lua。
不增加 A-G、冷啟動、電源恢復、故障備援或新武器支援。

## 驗證

新增原生回歸涵蓋事件狀態副本、重複選擇、忽略釋放／無關指令、不同 group 的順序、
scoped_connection 解除、不同實例隔離、CMS 與模式的 FIFO、既有 OFF→ON 停止順序。
既有 MC 與 exporter 測試核對新快照及原參數映射；架構測試核對第三方原始標頭例外
不延伸到自有檔案。Windows Release 與 DCS 的實際驗證結果須另行記錄，不能以 Linux
測試推定已完成 Windows DLL 或遊戲內驗證。

## 本次驗證紀錄

基準為 `9eb8ff319e822f11aa0f747a4c6aeb282b7149ef`，最終來源為 `5ca3675a5b31aee58340d77edea7cde19f4085eb`。
Windows 完整驗證與 DLL 憑證見[驗證紀錄](MC_MODE_EVENTS_VALIDATION.md)。

| 項目 | 結果 |
|---|---|
| Linux MC／模式原生測試 | 219 checks，0 failures |
| Linux 相容模式快照匯出測試 | 122 checks，0 failures |
| Lua 5.1.5 介面／顯示回歸 | 6,169 checks，0 failures |
| 完整 C++ 排版檢查 | 276 個檔案通過 |
| 排版及 CI 工具回歸 | 84 passed，5 個 Windows 專用測試 skipped，0 failed |
| EFM 架構檢查 | 241 個來源檔通過 |
| 架構檢查器回歸 | 通過，包含上游標頭例外與自有檔案長度拒絕 |
| code-review：Standards | 0 findings |
| code-review：Spec | 0 findings |
| 固定工具鏈 Windows Release／DLL 完整驗證 | 通過；全專案 3,746 checks，0 failures |
| DCS 遊戲內驗證 | 尚未執行 |

Linux MC 使用 g++ C++17、O2、LTO、Wall/Wextra/Werror/pedantic，僅停用既有
missing-field-initializers 警告。原生主程式執行實際 MC、模式與 Pipeline 回歸。

Linux 匯出測試使用原始 exporter 程式與實際測試，Windows 模組查詢由暫存 stub 提供。
既有產生的 CockpitParams.g.h 常數表被 GCC 拒絕；僅在暫存副本將字串宣告改成 constexpr，
數值與名稱不變，專案 SDK、產生器與產生檔均未修改。此相容測試可核對映射，不能取代
Windows 編譯、ABI、最佳化與 DLL 載入驗證。

Node 22.14.0、clang-format 22.1.8 與 Lua 5.1.5 均在暫存目錄取得並核對完整性；
架構檢查與 fixture 使用 PowerShell 7.4.6。Windows CI 已對上述來源完成完整驗證，
對應的 DLL 與來源憑證已放入 bin；DCS 遊戲內驗證另行記錄。
