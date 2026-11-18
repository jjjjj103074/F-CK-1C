# MC 操作與 HOTAS 參考

## 本輪實作

| 操作 | 鍵盤預設 |
|---|---|
| NAV／A-A／A-G | 1／2／3 |
| DGFT／CENTER／MSL OVRD | 右 Ctrl＋D／右 Shift＋D／右 Alt＋D |
| MSL STEP | Q：短按同型輪站，按住達 0.5 秒切換武器型別 |
| 主保險 ON／OFF／SIM | 右 Ctrl＋M／右 Shift＋M／右 Alt＋M |
| 扳機第一段／第二段 | 左 Alt＋Space／Space |
| 解籠／投放 | 右 Alt＋右 Shift＋M（按住）／右 Alt＋Space |
| TMS 上／下／左／右 | Enter／Backspace／-／= |
| CMS 前／後／左／右／按壓 | 右 Alt＋Delete／Home／Insert／End／Page Down |

三段超控各自選定開關位置；放開按鍵後保持選定位置。CENTER 恢復保存的主模式。
一般 A-A／A-G 重按回 NAV；超控期間保存原主模式。保險獨立。
第一段扳機入口保留，第二段延續機砲使用。解籠、投放、TMS 與 CMS 延續原有操作規則。

玩家控制表使用 MC 語意入口；輸入轉接同時過濾繼承設定中的原生模式、武器循環、
捕獲、發射超控與反制施放入口。原生 ID 與 DcsBridge 作動映射繼續服務內部操作。
IRST 與手動音效測試列於 Developer Tools。機砲／投放的原生輸入別名轉接至 MC。

## MSL STEP 的處理

FC 以輸入觀測時間記錄按下，短按在放開時送出同型輪站；達 0.5 秒送出一次換型，
持續按住保持該次結果。模式切換結束舊配置的按住輸入。主保險選擇與掛點選擇獨立。
所有選擇要求通過 SM FIFO，由 SM 決定有效候選、要求版本與外部執行確認。

短按依本機固定掛點順序選擇目前 SMS 型別的下一站；換型依有效掛載首次出現的型別順序循環。
同型用完時先接續同型的有效掛點，再接續其他可用型別。
選擇型別使用來源完整武器型別識別；AIM-9L／M 的已知標準掛載識別合併為 A-9LM。
相同來源武器型別的其他掛架沿用該合併；其餘飛彈保留各自識別並參與選擇。
對照資料位於 `Common/MissileSelection.cpp`，掛點順序沿用本機 `StationDefinitions.lua`。

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

本機掛點配置與 F-16 不同，使用本機順序。MFD、NWS 共用鍵、A-G STEP、SOI 與額外 TMS 功能見後續討論。
CMS 與解籠議題列於 [待辦](MC_TODO.zh-TW.md)。
