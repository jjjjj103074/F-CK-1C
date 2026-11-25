# MC 操作與 HOTAS 參考

## 本輪實作

| 操作 | 鍵盤預設 |
|---|---|
| NAV／A-A／A-G | 1／2／3 |
| DGFT／CENTER／MSL OVRD | 右 Ctrl＋D／右 Shift＋D／右 Alt＋D |
| MSL STEP | Q：短按同型輪站，按住達 0.5 秒切換武器型別 |
| 主保險 ON／OFF／SIM | 右 Ctrl＋M／右 Shift＋M／右 Alt＋M |
| 扳機第一段／第二段 | 左 Alt＋Space／Space |
| 解籠／投放 | C（AIM-9 切換；AIM-120 目視發射按住）／右 Alt＋Space |
| TMS 上／下／左／右 | I／K／J／L |
| DMS 上／下／左／右 | 右 Shift＋I／K／J／L |
| Debug Indicator（測試包） | 右 Ctrl＋右 Shift＋F12 |
| CMS 前／後／左／右／按壓 | 右 Ctrl＋I／K／J／L；按壓為右 Alt＋Page Down |

三段超控各自選定開關位置；放開按鍵後保持選定位置。CENTER 恢復保存的主模式。
一般 A-A／A-G 重按回 NAV；超控期間保存原主模式。保險獨立。
第一段扳機入口保留，第二段延續機砲使用。AIM-9 以 C 切換自主追蹤，放開後保存；AIM-120 以按住 C 提供目視發射同意；投放依共用程序判定，CMS 在各主保險狀態下啟用；TMS／SOI 依 [1997 年真機手冊與功能分工](MC_SENSOR_CONTROLS.zh-TW.md) 執行。

玩家控制表使用 MC 語意入口；輸入轉接同時過濾繼承設定中的原生模式、武器循環、
捕獲、發射超控與反制施放入口。原生 ID 與 DcsBridge 作動映射繼續服務內部操作。
IRST 與手動音效測試列於 Developer Tools。機砲／投放的原生輸入別名轉接至 MC。

UNCAGE=1 表示駕駛已選取解籠；再次按 C 回到 0。SM 保存當前選中飛彈的解籠要求；原生 consent 讀回與 IR 鎖定分別發布。
MC 在 ON＋空戰模式及有效選彈下依 AIM-7／9／120 程序判定，原生武器系統處理實際離架；機砲新開始資格為 ON＋DGFT，與解籠選擇獨立。

## MSL STEP 的處理

FC 以輸入觀測時間記錄按下，短按在放開時送出同型輪站；達 0.5 秒送出一次換型，
持續按住保持該次結果。模式切換結束舊配置的按住輸入。主保險選擇與掛點選擇獨立。
所有選擇要求通過 SM FIFO，由 SM 決定有效候選、要求版本與外部執行確認。

短按依本機固定掛點順序選擇目前 SMS 型別的下一站；換型依有效掛載首次出現的型別順序循環。
同型用完時先接續同型的有效掛點，再接續其他可用型別。
選擇型別使用來源完整武器型別識別；已識別的 AIM-9B／E／J／JULI／L／M／P／P3／P5／X 合併為 AIM-9 群組。
相同來源武器型別的其他掛架沿用該合併；其餘飛彈保留各自識別並參與選擇。
型號與程序定義位於 `FM/Weapons/WeaponCatalog.json`，掛點順序沿用本機 `StationDefinitions.lua`。

## F-16 的實體控制與參考範圍

MSL STEP 是側桿頭部右側的一顆瞬時按鈕（16PR14341 圖 1-3），與 NWS／空中加油斷開共用同一個實體按鈕，
由飛行狀態及工作情境決定功能。TMS 是側桿正面的四向帽；游標另由油門上的
RDR CURSOR／ENABLE 控制器操作。TMS 對取得操作焦點的感測器（SOI）提出指定與拒絕等操作。

- F-16 MLU Pilot's Guide Part 1，16PR14341，印刷頁 6–8：實體位置、SOI 與通用操作哲學。
  https://www.airwar.ru/other/manuals/%5Baviation%5D%20-%20%5Bmanuals%5D%20-%20F-16A-B%20MLU%20Pilot%20Guide%20Part%201.pdf
- F-16 MLU Pilot's Guide Part 2，16PR15269，印刷頁 52：短按 <0.5 秒選同型掛點、按住 ≥0.5 秒換型。
  印刷頁 62 說明 AIM-120B 為可獨立選擇的型別。
  https://www.airwar.ru/other/manuals/%5Baviation%5D%20-%20%5Bmanuals%5D%20-%20F-16A-B%20MLU%20Pilot%20Guide%20Part%202.pdf
- Falcon BMS 4.38 TO 1F-16CMAM-34-1-1，印刷頁 602、607：同型輪站、換型及 A-9LM／A-9P／A-9X 對照。
  此項為模擬器文件，補充現代武器型別，與上述飛機原廠指南分別引用。
  https://cdn.falcon-bms.com/docs/4.38/TO%201F-16CMAM-34-1-1%20BMS.pdf

本機掛點配置與 F-16 不同，使用本機順序。目前固定 MFD 對應、SOI 與 TMS 四向／長按行為見 [感測器操作](MC_SENSOR_CONTROLS.zh-TW.md)；NWS 共用鍵與 A-G STEP 保留後續討論。
CMS 與解籠議題列於 [待辦](MC_TODO.zh-TW.md)。

## 驗證與來源

SOI、顯示對應與 TMS 的實作範圍及空殼見 [感測器操作](MC_SENSOR_CONTROLS.zh-TW.md)。
Windows 驗證來源與 DLL 雜湊保存在 `bin/F-CK-1C_EFM.validation.json`；測試包的 `BUILD.json` 記錄同一組來源。
DCS 人工測試見 [MC 飛測說明](MC_FLIGHT_TEST.zh-TW.md)，本輪先執行 D01～D07、T01～T11。

CMS 四向採用右 Ctrl＋I／K／J／L；按下送出 1、放開送出 0，表示開關返回中央。
前向按下啟動固定十輪混合程序，每輪熱焰彈與干擾絲各一發，間隔一秒。放開回中後完成程序。
左向啟動固定程序 6：七發干擾絲，每發間隔 0.5 秒。前向與左向的新要求會接替目前程序。
後向為電子干擾許可、右向為電子干擾待命，兩者保留入口並保持目前施放程序。
CMS 永遠啟用，與主保險及主模式各自獨立。程序 6 的可設定內容與操作介面待後續研究。

程序資格、原廠來源及預留路徑見 [武器程序](WEAPON_PROCEDURES.zh-TW.md)。
