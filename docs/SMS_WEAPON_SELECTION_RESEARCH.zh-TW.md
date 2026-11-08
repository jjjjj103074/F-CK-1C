# SMS 武器與掛點選擇的責任分工

查核日期：2026-11-08。用途：決定本專案 StoresManagement（SM）整理時的選擇責任。本輪新增研究紀錄；執行程式維持原狀。

## 結論

公開 F-16 文件直接將武器與掛點選擇列為 SMS 軟體責任。F-16 MLU 又將 SMS 等功能整合到 MMC 執行。因此，對本專案最合適的解讀是：**選擇功能屬於 MC 內的 SM；MC 是容納與協調這些功能的整體。**

這是有公開證據支持的設計選擇。所引用的是歷史機型／版本的功能描述，現代各批次的來源程式與本專案類別界線仍需分別看待。

## 公開文件與定位

### F-16C/D：直接描述 SMS 軟體

來源：Computing Devices Company／Control Data Corporation，1991 年 9 月，**ASD-TR-91-5009／DTIC ADA255714，MIL-STD-1760 Application Guidelines**。文件核准公開發布；§1.5 說明資料蒐集截至 1986 年 6 月。以下引用 §6 對既有 F-16C/D 的案例。

[政府報告掃描件與書目](https://archive.org/details/DTIC_ADA255714)；[同件全文 OCR](https://archive.org/stream/DTIC_ADA255714/DTIC_ADA255714_djvu.txt)。本次透過 Internet Archive 鏡像核對，OCR 有辨識誤差，請依印刷頁碼與節號定位。

| 印刷頁碼／節號 | 支持的事實 |
|---|---|
| p.59，§6.2.4.1.8，SMS Software | SMS 作業軟體負責選擇、監控、整備、釋放與庫存；同節介紹軟體元件分組。 |
| p.62，§6.2.4.1.8 c(1) | Stores Accounting Component 管理掛載數量、種類與位置。 |
| **p.62，§6.2.4.1.8 c(2)** | **Stores Selection Component 負責武器及掛點的選擇，供顯示與釋放使用。** |
| p.63，§6.2.4.1.8 c(5) | 另有 Weapon Delivery Mode Component 處理主模式與 SMS 子模式。 |

這份資料足以支持選擇責任歸屬 SMS；本專案的換站順序及操作規則另行定義。

### F-16A/B MLU：實體整合到 MMC，功能仍稱 SMS

來源：Lockheed Martin，**16PR14341，F-16 A/B Mid-Life Update, Production Tape M1, The Pilot’s Guide**，1998-11-15。[原廠文件的鏡像 PDF](https://www.mycity-military.com/uploads2/154453_760102325_F-16A-B.pdf)。

- **印刷 p.1／PDF 第 11 頁**：MMC 接替 XFCC、SMS CIU 與 HUD 電子單元的功能。
- **印刷 p.223／PDF 第 233 頁**：描述 AIM-9 支援時，把武器選擇與狀態列為 SMS 功能。

因此，實體電腦由 MMC 執行，軟體功能仍歸 SMS，兩者可以同時成立。

### F/A-18：MC 與 SMS 的分工依機型而異

來源：NAVAIR **A1-F18AC-NFM-000**，1997 年版本 NATOPS 掃描件，**印刷 I-2-70，§2.13.2 Master Modes**。[軍方手冊鏡像 PDF](https://airwar.ru/other/bibl/A1-F18AC-NFM-000%20%2815_01_1997%20%2B%20perf%20data%29.pdf)。

該節將主模式選擇交給 SMS，再由 SMS 通知 MC。它說明模式責任有機型差異；本專案仍維持已決定的 ModeManagement 分工。

F-20 本輪找到的公開原廠資料提供 MC 整體能力描述，細部選擇責任的證據以以上 F-16 文件為準。

## 對本專案的設計建議

以下為工程建議，與上述真機文件分開記錄。

| 部分 | 職責 |
|---|---|
| MC 組裝與輸入協調 | 安排更新順序，傳遞飛行員意圖及模式資料。 |
| ModeManagement | 維持本專案既定模式狀態與有序事件通知。 |
| SM | 統整掛點觀測與原生機砲彈量，發布唯一庫存；依庫存及選擇規則管理候選掛點、選擇結果與執行回覆。 |
| FireControl | 使用 SM 發布的資料處理瞄準、使用許可及既有武器操作流程。 |
| DcsBridge／座艙 Lua | 讀取 DCS 資料、執行選站及操作命令、回傳執行結果。 |

分工重點是由 SM 管理「選哪個掛載／掛點」，FCC 管理既有火控處理。獨立的唯讀資料與語意請求介面讓兩者各自維護自己的狀態。

### 已接受的資料規則

1. **SM 是唯一庫存權威**：DCS 仍是模擬世界的原始資料來源，其他 MC 元件統一讀取 SM 發布的庫存；舊有初始飛彈數量及 FCC 手動扣量由觀測資料取代。
2. **掛點資料 0.5 秒新鮮度**：依最後收到新觀測批次的時間判斷；超時保留最後資料並標為 Stale。新批次抵達後依觀測結果更新有效性。有效空載與資料過期各有自己的狀態。

### 重構時需處理的既有橋接差異

目前 `CycleWeapon` 交給 DCS 自行循環，而明確 `select_station` 的執行回覆記錄在另一條路徑。現有選站回覆代表 Lua 呼叫完成；實際 DCS 選中掛點的回讀能力須另外核對。

整理選擇責任時，需區分「切換武器種類」和「同型武器切換掛點」。本輪研究確定職責歸屬；本輪已接受依固定掛點順序輪換空對空飛彈掛點，規則見 [SM 整理規格](SM_REFACTOR_SPEC.zh-TW.md)。庫存狀態、選擇意圖、執行回覆分別表達，可讓這項差異在介面中清楚可見。
