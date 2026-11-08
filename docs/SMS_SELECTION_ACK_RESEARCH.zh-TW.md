# 選站執行確認與武器備便

本紀錄補充 SM 整理中的選站／投放先後問題；輸入保留政策仍待討論。

## 本專案已核對的 ACK 來源

`Cockpit/Scripts/Systems/weapon_system.lua` 的 `apply_selection_request()` 讀取 MC 發布的要求編號與掛點，呼叫 `WeaponSystem.select_station`。呼叫成功後，保存 `selected_station` 與 `acknowledged_request`，隨下一批掛點觀測回傳。

因此 ACK 的語意是「座艙轉接層已完成此選站呼叫」。它記錄指令執行結果，與飛彈本身回報的備便或健康狀態分開。SM 產生要求時已完成邏輯選擇；等待這個 ACK 的對象是 DCS 轉接層。

MC 32 Hz、掛點裝置 20 Hz、CMS 動作裝置 50 Hz 各自更新。即使各部分正常，新的選站要求也可能尚未被掛點裝置執行。重構需確保選站先於使用該選擇的投放操作。

## 真機公開資料

1. **Lockheed Martin 16PR14341**，1998-11-15，《F-16 A/B Mid-Life Update, Production Tape M1, The Pilot’s Guide》，印刷 **p141–142／PDF 第151–152頁**。手冊分開描述選中掛點、掛點狀態、飛彈 RDY／SIM／MAL／REL 及 BIT 結果。這支持將選擇與備便／健康分開表達。[原廠文件鏡像 PDF](https://www.mycity-military.com/uploads2/154453_760102325_F-16A-B.pdf#page=151)
2. **MIL-STD-1760E**，2007-10-24，附錄 B 表 B-VIII，印刷 **p101／PDF 第121頁**。狀態監看區分收到的要求狀態與儲載物目前狀態。它提供數位介面上的概念例證；各機型及武器使用的具體介面另行確認。[DLA 官方 PDF](https://quicksearch.dla.mil/WMX/Default.aspx?token=606932)

上述資料支持區分「選擇／命令處理」與「實際狀態」。本次查閱沒有找到足以將「短按放開後保留投放，直到未來備便才執行」認定為 F-16 通用規則的公開證據。

## 對本次整理的含意

- 選站 ACK 用於確保 DCS 執行順序。
- 武器備便、飛彈通訊與健康狀態屬於另外的模型與資料來源。
- 80 ms 是本專案現有的動作保持時間。
- 保留短按、持續按住與取消條件須作為明確的模組操縱規則討論，與選站 ACK 的意義分開。

這份研究保留在 docs；目前尚未變更投放等待行為。
