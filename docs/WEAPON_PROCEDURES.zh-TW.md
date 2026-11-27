# 空對空飛彈流程查證（2026-11-25）

用途：F-CK-1C 的 DCS／FC 介面設計。以下以實機原廠及軍方文件為準；其他網站僅作原文件的鏡像或轉錄，不引用 DCS／BMS 手冊作實機證據。**武器操作程序、飛機發射互鎖、飛彈有效導引條件是不同層次。** 共用家族流程是本專案的設計選擇，不能描述成所有實機、批次都完全相同。

## 已查證、可用於規格

| 對象 | 文件支持的行為 | 不能據此增加的硬互鎖 |
|---|---|---|
| AIM-9L/M | 可於雷達關閉／待命使用；標準 SLAVE 程序包含解籠、確認自行追蹤，再釋放。印刷 **137／PDF 147**。 | 該段沒有證明「IR lock 或 uncage 未成立即由飛機禁止釋放」。[Lockheed Martin 16PR14341](https://www.amvi.it/upload/2016/files/MLU_M1.pdf#page=147) |
| AIM-9 cage/uncage | 空對空模式、選取 AIM-9、MASTER ARM 為 ARM／SIM 時，每次按壓 UNCAGE 在選定視線與 self-track 間切換。印刷 **1-43**；此次僅查得原文轉錄，未核 PDF 頁碼。 | 不應額外要求先取得雷達鎖定，才允許原生 cage/uncage 命令。[HAF GR1F-16CJ-34-1-1 原文轉錄](https://www.scribd.com/document/784889628/To-GR1F-16CJ-34-1-1-F-16C-D-Block-50-Avionics-and-Nonnuclear-Weapons-Delivery-Flight-Manual-16-Jun-1997) |
| AIM-120 常規 | SLAVE 且有 bugged target 時，WPN REL 即發射。印刷 **141／PDF 151**。 | 「必須 STT」比文件所述的 bugged target 更嚴格。[16PR14341](https://www.amvi.it/upload/2016/files/MLU_M1.pdf#page=151) |
| AIM-120 目視／BORE | 無 bugged target，**或**已選 BORE，須**按住 UNCAGE 同時按 WPN REL**；BORE 啟用飛彈自身雷達。印刷 **140–141／PDF 150–151**。 | 雷達鎖定不是所有發射的必要條件；這裡的 UNCAGE 是持續按住的目視發射同意，不能與 AIM-9 的切換狀態混用。[16PR14341](https://www.amvi.it/upload/2016/files/MLU_M1.pdf#page=151) |
| AIM-7 正常／FLOOD | USAF 說明 AIM-7M 為 CW／pulse-Doppler 半主動導引。NAVAIR 實機雷達文件另明定：雷達未追蹤時，Sparrow launch 可命令 FLOOD 天線提供 RF 照射。印刷 **WP02800 p6，¶49–51**；只有轉錄，PDF 頁碼未核。 | 不能把「SARH 必須照射」改成「所有發射都必須已有 radar lock」。FLOOD 仍需要照射，並非主動飛彈模式。[USAF AIM-7](https://www.af.mil/About-Us/Fact-Sheets/Display/Article/104575/aim-7-sparrow/)、[NAVAIR A1-F18AC-742-100 原文轉錄](https://pdfcoffee.com/a1-f18ac-742-100-radar-system-f-a-18-pdf-free.html) |

AIM-120B 的 HAF 原文另列出：有更新的慣性／主動末導引、無更新的慣性／主動末導引，以及尋標器能取得目標時不依賴機上射控的主動模式；印刷 **1-665**。這支持「機上追蹤對常規發射及更新有用途」，不支持「全程 STT 是物理必要」。[HAF 武器章節原文轉錄](https://www.scribd.com/document/906155674/F-16-NNWD-Extracts-text)

## F-16 ACM／DGFT 的精確對照

以下只確證 **F-16A/B MLU M1（1998）**，不是所有 F-16 軟體版本的通則。

| 輸入／轉換 | 已查證行為 | 原文件頁碼 |
|---|---|---|
| ACM 中 TMS forward | 拒絕現有自動鎖定，選 BORE／slewable BORE。 | 印刷 **8／PDF 18**，圖 1-4。[原廠圖](https://www.amvi.it/upload/2016/files/MLU_M1.pdf#page=18) |
| ACM 中 TMS right | 拒絕現有自動鎖定，選 30×20。 | 印刷 **8／PDF 18**。[原廠圖](https://www.amvi.it/upload/2016/files/MLU_M1.pdf#page=18) |
| ACM 中 TMS aft | 拒絕現有自動鎖定，選 10×60。 | 印刷 **8／PDF 18**。[原廠圖](https://www.amvi.it/upload/2016/files/MLU_M1.pdf#page=18) |
| 進入 ACM／DGFT | 保留已有 bugged target；沒有追蹤目標時進 NO RAD，需 TMS／slew 才重新輻射。 | 印刷 **134／PDF 144**。[原廠說明](https://www.amvi.it/upload/2016/files/MLU_M1.pdf#page=144) |

PDF 第 18、144、151 頁已下載、轉圖並目視核對。原廠文件的 PDF 頁數比印刷阿拉伯頁碼多 10；本筆記的 PDF 頁碼全部為閱讀器的 **1 起算**頁碼。

## 型號界線與未證實項目

- **AIM-9P-3／P-5：**USAF 一級資料可確認 P-3 的版本改良，但此次沒有取得這兩型的完整實機發射互鎖表；不能直接將 L/M 的冷卻氣體、時間或所有操作細節套用。P-3／P-5 共用 IR 操作可列為專案簡化。[USAF Sidewinder](https://www.af.mil/About-Us/Fact-Sheets/Display/Article/104557/aim-9-sidewinder/)
- **AIM-9X：**NAVAIR 明確將 lock-on-after-launch 列為 **Block II** 相對 Block I 的改良。因此既不能全家族增加「發射前已 IR lock」硬互鎖，也不能把 Block II 能力倒灌到未註明批次的 AIM-9X。[NAVAIR，2015-08-31](https://www.navair.navy.mil/node/22731)
- **AIM-7M／P：**NAVAIR 雷達文件的武器識別確有 7F／7M（WP01800 p6 ¶43），但此次未找到逐字列出 7P 的同一套 FLOOD 發射程序。AIM-7P 共用 SARH 介面屬專案映射；不要宣稱其專用資料鏈、導引方式切換或準備時間已查證。[NAVAIR 原文轉錄](https://pdfcoffee.com/a1-f18ac-742-100-radar-system-f-a-18-pdf-free.html)
- **AIM-120B／C：**ACC 的官方資料確認 B/C 導引系統含慣性參考、雷達與資料鏈處理，但不是逐批次座艙互鎖表。共用原廠 MLU 的 SLAVE／BORE 操作可以採用，應標明是介面基準。[ACC 官方 AIM-120 說明，2022](https://www.acc.af.mil/Portals/92/Docs/Fact%20Sheets%20-%202020%20Update/Facts%20Sheets%202022%20Final/AIM-120_final.pdf?timestamp=1689088232584&ver=RWnAtEYmQ2B5EzHUKtk12A%3D%3D)

## 對本專案的落地要求

1. 主軍械開關、主模式、武器家族、雷達追蹤／照射、尋標器 cage 狀態分開建模。模式選取本身不要暗改主軍械開關。
2. AIM-9 的 C 鍵切換應送出實際原生命令；顯示狀態不能替代原生操作。IR tone／track 可提供操作提示，沒有文件和可讀訊號時不要新增猜測的 IR-lock 發射門檻。
3. SARH 正常鎖定與明確選取的 FLOOD／視軸發射應分開。DCS FC 是否能從此模組控制 FLOOD／讀取照射，必須由可用 API 與遊戲內測試另行驗證；沒有支援時明列為未實作。
4. ARH 的常規目標指定與目視發射同意分開；若加入 MLU 目視程序，要保留按住 UNCAGE 的輸入語意。不能只因後端收得到 WPN REL 就宣稱整個真實程序已實作。
5. 本研究沒有驗證 DCS 的 seeker-ready、實際 cage 回讀、FLOOD acknowledgment 或 illumination-ready API。不要新增虛構冷卻倒數、準備確認或每型 capability flags；也不要把估計的 FOV／DLZ／IR lock 指標充作原生感測器事實。

補充：指定的 [HAF 原 PDF](https://falcon.blu3wolf.com/Docs/HAF-F16-34.pdf) 本次下載得到防機器人 HTML，web 工具另回報檔案約 373 MB；因此 HAF 僅引用可讀到的原文轉錄，沒有捏造其 PDF 頁碼。

## 本次實作對照

| 共用程序 | MC 發射條件（另需 ON＋空對空模式＋有效選彈） | 本次介面範圍 |
|---|---|---|
| AIM-9 | 沿用 DCS 正常投放，獨立回報 IR 捕獲 | C 每次按下切換選中 AIM-9 的原生自主追蹤；不另加 IR-lock／UNCAGE 硬互鎖 |
| AIM-7 | 有效原生 STT | 正常 SARH 路徑。FLOOD 照射介面預留，未把無追蹤發射當成 FLOOD |
| AIM-120 | 有效原生 STT，或持續按住 C 的目視發射同意 | STT 是目前唯一可用的機上指定目標來源；其他 bug／TWS 路徑預留 |

這是依可用 DCS 介面落地的程序子集。發射、飛彈導引與目標資料交接仍由原生模擬執行；
MC 沒有新增飛彈飛控、冷卻計時、照射或資料鏈模擬。
AIM-9 B/E/J/JULI/L/M/P/P3/P5/X 暫共用此操作程序；原廠 L/M 文件是介面基準，
不同世代的完整備便、冷卻與尋標器能力須逐型另行整合。
雷達／TMS 跨版本的操作參照在表格中明列；MLU 的 AIM-120 能力不代表原始 Block 25 已配備 AIM-120。

### 共用清單與責任

`FM/Weapons/WeaponCatalog.json` 保存 46 種既有掛載的身分、分類、選擇群組、程序與 DCS 掛載參數。
`python tools/generate_weapon_catalog.py` 從 JSON 更新 `F-CK-1C.lua` 的武器群組區段。
機型註冊以既有的 `wpnGroup` 建立可掛清單；C++ 的 `WeaponCatalogData` 直接讀取 JSON 解析 CLSID／別名。
CI 以 `--check` 核對產生檔與 JSON 同步，並在 `io`、`select` 缺省的註冊環境執行回歸測試。
`<CLEAN>` 是掛點配置指令，另由掛點建構器加入。

- `category`／`role`／`guidance`：掛載種類、任務用途與導引分類。
- `id`／`name`／`family`：確切型號、畫面名稱與系列。
- `selection_group`：短按 Q 的輪站群組；AIM-9 各型共用 `aim-9`。
- `procedure`：`aim7`、`aim9`、`aim120` 或 `unsupported`。其他已登錄系列仍可管理庫存與選站。
- `native_ir`／`radar_cue`：目前專案已整合的尋標器控制與指向介面。L/M/X 啟用雷達指向；其餘先沿用原生指定視線。旗標描述本次軟體支援，並非該型真實硬體能力的完整表。
- `loadouts`／`aliases`：前者產生可掛 CLSID 與原有阻力、直徑、互斥等參數；後者辨識原生回報的相同型號別名。

資料流：JSON → Lua 機型註冊／C++ Catalog → SM 逐站庫存與 SelectedWeaponState → FC 共用程序 → SM 指令入口 → 原生 DCS。
SM 保存待確認選站的預定型號，供 FC 規劃；實際指令仍由 SM 等待相符選站確認後執行。
已確認型號名稱、群組總數及 FC 程序結果由 MC 快照發布，座艙依資料顯示。
未登錄型號顯示 `UNREGISTERED`／`UNSUPPORTED`，不加入型號猜測規則。

### 雷達與顯示修正

A-A、DGFT、MSL OVRD 之間切換保留有效 STT。RadarSystem 等追蹤結束後才下達下一個原生搜尋模式，
並以設備背景動作回報，不把延後動作塞進已完成的要求。
ACM 中 I／K／L 分別為 BORE／10×60／30×20；同一次操作拒絕舊鎖並啟動指定型式的捕獲。
原生取消確認前保存自動捕獲意圖，在觀測回搜尋後接續執行。
目前進入 DGFT 仍顯示 BORE 預選，且未實作實機 NO RAD 的待輻射控制；此處與 MLU 原廠流程有明確差異。

左上角 `RELEASE` 顯示 `IR / NATIVE`、`RADAR TARGET`、`NEED STT`、`HOLD C FOR VISUAL`、`VISUAL` 或狀態原因，
只表示 MC 的程序判定。實際離架、IR 狀態與雷達觀測各自記錄。


武器定義於載入時建立為共用唯讀物件；逐站庫存、選中武器及發布快照保存 `WeaponDefinitionRef`。
快照保有定義的生命週期，原始 Catalog 銷毀後仍可讀取該次觀測的型號與程序。
