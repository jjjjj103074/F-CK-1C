# MC 完整實機測試流程

本輪先做任務 A 的 C01：按右 Ctrl＋J，確認 CHF 減 7、FLR 保持；再做 C03–C06 與 C08 核對前向程序、保險獨立及按住後回中。

## 安裝與開始

SOI 與左右功能對應使用標準 debug（右 Ctrl＋右 Shift＋F12）查看；本輪以真實 F-16 手冊為操作依據。沒有實際 MFD 畫面，功能空殼會顯示 RESERVED。

1. 完全關閉 DCS，解壓縮測試包，在 F-CK-1C 資料夾執行 install.bat；預設安裝到 Saved Games\DCS。使用 DCS.openbeta 或其他資料夾時，將該 Saved Games 路徑作為 install.bat 的第一個參數。
2. 在任務編輯器依下表存好各任務；使用空中或熱機出生、關閉無限彈藥，選 JHMCS＋2D Overlay。全部保持專案預設按鍵。
3. 進任務後看 HMCS 與右上 MC TEST 面板。記錄器會自動將每次任務另存到 Saved Games\DCS\Logs\FCK1C-MC-TEST；視窗／按鍵設定採用現有配置。

## 任務配置

| 任務 | 配置 |
|---|---|
| A：空中空載 | 全部掛點維持空；機砲 100%，反制充足。用來測模式、保險、機砲、反制與畫面。 |
| B：空中八枚 AAM＋兩架目標 | RT／RO／LO／LT：AIM-9X 各一；RI／LI：AIM-7P 各一；MF／MB：AIM-120C 各一；MM：CLEAN。前方兩架不反擊、直飛的敵機，與玩家同高度，距離約 3 km、6 km；靠近至 1–3 km 測 IR。 |
| C：空中混合掛載 | RT：AIM-9X；LT：AIM-9L；RO／LO：Mk-82；RI／LI：330 加侖副油箱；MM：BRU-33＋兩枚 Mk-20；MF／MB：CLEAN。 |
| E：空中全部 CLEAN | 全部九站明確選 <CLEAN>。 |
| F：空中全部維持空 | 全部九站維持未選掛載；與 E 分開存成另一任務。 |
| C-AG：混合掛載的空對地飛彈 | 任務編輯器有可用空對地飛彈時，將 C 的 MM 改成該飛彈並記下型號；其餘相同。 |
| D：友軍機場熱機補給 | 熱機停機位出生、機砲與反制充足，起始外掛維持空；選可提供地勤的友軍機場。 |

可將同一份任務另存並改掛載。A 的機砲耗盡後，機砲相關項目需重新載入有彈任務；B 中標示「重新載入」的項目要補回原八枚配置。

## 面板怎麼看

- MC=1 表示 MC 快照可用；GEN 持續增加。ARM：0=SAFE、1=SIM、2=ON；MODE：0=NAV、1=DGF、2=MSL、3=A-A、4=A-G。
- SUB：0=---、1=HMD、2=VS、3=HUD、4=BVR。UNCAGE 是 MC 保存的解籠選擇，按 C 在 0／1 間切換；TRIG 是第二段扳機的按住狀態。
- GUN OK／AAM OK=1 時，旁邊的 GUN／AAM 是有效存量；GUN 是原生實際存量。DGF 的 WPN 顯示 AAM 並不阻擋機砲。
- SEL 是 Lua 最近完成選站呼叫的紀錄：0 RT、1 RO、2 RI、3 MM、4 MF、5 MB、6 LI、7 LO、8 LT；-1 表示邏輯選擇為空。實際選站以離架掛點再核對。
- RDR 是 DCS 原始雷達模式；RDR=3 且 STT 為正距離表示來源回報單目標追蹤。這版 HMCS 中央圈是固定瞄準圖形，用右上數值確認追蹤。
- IR OK=1、LOCK=1 表示 MC 已接受有效 IR 鎖定回報；鎖定音跟這個值。RDR=3 與 IR LOCK 分開看。
- TX 是目前 DCS 動作視窗數量，ACK 是已執行前綴。停止操作後通常 1–2 秒回 TX=0；停超過 5 秒仍卡住請回報畫面與前一個操作。

## C++ 反制驗證

N01–N09 是可選的既有反制回歸項目。四個原生測試鍵已在本包綁定，與本輪解籠測試使用同一個記錄器與結果收集器。

STAT：1=就緒、2=排隊、3=原生呼叫返回、4=Analog 放開返回、5=保險阻擋、6=具名入口缺少、7=呼叫失敗、8=離開任務、9=佇列已滿。數字 3／4 代表呼叫返回；實際成功還要看 FLR／CHF 減少及離機效果。

若 CMOBS 某一位不可讀，該彈量顯示 -1，直接回報 CMOBS 數值與可見離機效果，原始記錄仍會保存。STAT=7 時停止原生測試，繼續原有 MC 測試；收集包會保留例外碼。

## 預設快捷鍵

| 功能 | 預設鍵 |
|---|---|
| C++ Digital 熱焰彈／干擾絲（本包預設） | 左 Alt＋左 Shift＋Insert／左 Alt＋左 Shift＋Delete |
| C++ Analog 熱焰彈／干擾絲（本包預設） | 左 Ctrl＋左 Shift＋Insert／左 Ctrl＋左 Shift＋Delete |
| 一般主模式 NAV／A-A／A-G | 1／2／3 |
| 超控 DGFT／CENTER／MSL | 右 Ctrl＋D／右 Shift＋D／右 Alt＋D |
| MSL STEP 短按／長按 | Q：短按同型輪站；按住達 0.5 秒換型 |
| 保險 ON／OFF／SIM | 右 Ctrl＋M／右 Shift＋M／右 Alt＋M |
| 第一段扳機／機砲 | 左 Alt＋Space／Space |
| 解籠切換／投放 | C（再按一次關閉）／右 Alt＋Space |
| TMS 上／下／左／右 | I／K／J／L |
| DMS 上／下／左／右 | 右 Shift＋I／K／J／L |
| 標準 Debug Indicator | 右 Ctrl＋右 Shift＋F12 |
| CMS Forward／Aft／Left | 右 Ctrl＋I／右 Ctrl＋K／右 Ctrl＋J |
| CMS Right／Press | 右 Ctrl＋L／右 Alt＋PgDn |
| 手動音效測試 | 右 Shift＋T |
| 座艙／外部視角 | F1／F2 |
| 地勤通訊／離開任務選單 | 反斜線鍵 \／Esc |
| 搖桿預設機砲／投放 | JOY_BTN1／JOY_BTN2 |

C 是按一下切換的輸入。放開 C 後 UNCAGE 保持選定值，可再獨立操作右 Alt＋Space；再次按 C 解除選擇。這個值表示 MC 的駕駛選擇，尋標器鎖定另看 IR OK／LOCK。

## A：空中空載

### D01　顯示與SOI診斷

操作：進入任務等 3 秒，按 debug（右 Ctrl＋右 Shift＋F12）。

看哪裡：標準 debug：MC / Display Mode、MFD Left / Source、MFD Right / Source、SOI Source／Display。

正常：NAV／NAV；左HSD (SHELL)／HSD，右SMS／NONE，SOI HUD／HUD，Display Context=READY。

### D02　導航空殼與DMS

操作：按超控中央（右 Shift＋D），按 NAV（1），等 1 秒。按 DMS 下（右 Shift＋K），再依序點按 TMS 上／下／左／右（I／K／J／L）。按 DMS 上（右 Shift＋I），再按 DMS 左／右（右 Shift＋J／右 Shift＋L）。

看哪裡：SOI Source／Display、TMS Receiver／Result、DMS Format Step。

正常：DMS下選HSD／LEFT MFD；TMS接收者HSD、結果RESERVED。DMS上回HUD。左右保存格式輪換要求，固定HSD／SMS對應保持原值。

### D03　空對空SOI與保險獨立

操作：按超控中央（右 Shift＋D）、NAV（1）、A-A（2），等 1 秒；依序按 DMS 上（右 Shift＋I）、DMS 下（右 Shift＋K）、OFF（右 Shift＋M）、SIM（右 Alt＋M）、ON（右 Ctrl＋M）。

看哪裡：左右功能、SOI Source／Display、SYS／MODE。

正常：左右FCR／SMS，SOI保持FCR／LEFT MFD；SMS不接收TMS。保險切換保持A-A與SOI。

### D04　空地FCR與HUD空殼

操作：按超控中央（右 Shift＋D）、NAV（1）、A-G（3），等 1 秒。點按 TMS 上（I）、下（K）、右（L），各次等半秒。按 DMS 上（右 Shift＋I），點按 TMS 上（I）；按 DMS 下（右 Shift＋K）。

看哪裡：TMS Request／Result、SOI Source／Display、Radar Observed。

正常：FCR依序提出GROUND DESIGNATE、GROUND REJECT、CYCLE SIGHTING POINT，均RESERVED。DMS上選HUD，HUD的TMS為RESERVED；DMS下回FCR。

### D05　超控恢復顯示配置

操作：按超控中央（右 Shift＋D）、NAV（1）、A-G（3），等 1 秒；按 DGFT（右 Ctrl＋D）、MSL OVRD（右 Alt＋D）、超控中央（右 Shift＋D），每次等 1 秒。

看哪裡：MC / Display Mode、左右功能、SOI、MODE。

正常：依序DGFT／DGFT、MSL OVRD／MSL OVRD、A-G／A-G；左右FCR／SMS，最終恢復A-G空殼與FCR SOI。

### D06　按住TMS時切換模式

操作：按 MSL OVRD（右 Alt＋D），等 1 秒；按住 TMS 上（I），同時按超控中央（右 Shift＋D），再按 NAV（1）；放開 TMS 上（I）。等 1 秒後按 MSL OVRD（右 Alt＋D），再重新點按 TMS 上（I）。

看哪裡：TMS Input／Receiver／Request、SOI、MODE；實際武器。

正常：切模式結束舊操作，放開不對NAV產生捕獲；重新按下後新的FCR指定可運作。沒有開砲或投放。

### D07　任務重入的顯示與SOI

操作：按 DGFT（右 Ctrl＋D）、TMS 右（L）；按 Esc 離開任務並重新載入A，等 3 秒，再按 debug（右 Ctrl＋右 Shift＋F12）。

看哪裡：左右功能、SOI、TMS Input／Request。

正常：回NAV的HSD／SMS與HUD SOI；TMS初始NONE，沒有上次ACM或未完成操作殘留。

### M01　載入與待機

操作：進入空中任務後等待 5 秒。

看哪裡：HMCS；右上 MC TEST 面板。

正常：初始 ARM／NAV／---；MC=1、GEN 持續增加。沒有自行開砲、投放或釋放反制。

### N01　原生反制測試準備

操作：重新載入 A 任務，按超控中央（右 Shift＋D）後按 NAV（1），按保險 OFF（右 Shift＋M），等待 2 秒。

看哪裡：右上 NATIVE、API、CMOBS、FLR、CHF、REQ、CALL。

正常：NATIVE=1、REQ=0、CALL=0，FLR／CHF 穩定。API：1=Digital、2=Analog、3=兩者、0=皆未找到。CMOBS=3 表示兩種實際彈量可讀；1=只有 FLR、2=只有 CHF、0=兩者皆不可讀，無讀值顯示 -1。記下 API 與 CMOBS；後續仍各試一次以記錄缺少的入口。

### N02　C++ Digital 熱焰彈

操作：按一次 Digital 熱焰彈（左 Alt＋左 Shift＋Insert），放開並等 1 秒。要看離機效果可按外部視角（F2），返回座艙按 F1。

看哪裡：右上 REQ、CALL、STAT、FLR、CHF；機尾離機效果。

正常：REQ 增 1。Digital 可用時 CALL 增 1、STAT=3、FLR 恰減 1、CHF 不變，實際釋放一發熱焰彈。API 未含 Digital 時 STAT=6、CALL 與彈量不變；回報「入口缺少」。只有 CALL 增加而彈量不變，回報「呼叫返回但未施放」。

### N03　C++ Digital 干擾絲

操作：按一次 Digital 干擾絲（左 Alt＋左 Shift＋Delete），放開並等 1 秒。

看哪裡：右上 REQ、CALL、STAT、FLR、CHF。

正常：REQ 增 1。Digital 可用時 CALL 增 1、STAT=3、CHF 恰減 1、FLR 不變。入口缺少為 STAT=6。干擾絲外觀不明顯，以實際 CHF 減少核對；CALL 增加本身只表示函式返回。

### N04　C++ Analog 熱焰彈

操作：按一次 Analog 熱焰彈（左 Ctrl＋左 Shift＋Insert），放開並等 1 秒。可按 F2 看離機效果，返回按 F1。

看哪裡：右上 REQ、CALL、STAT、FLR、CHF。

正常：REQ 增 1。Analog 可用時 CALL 增 2（按下與放開）、STAT=4、FLR 恰減 1、CHF 不變。入口缺少為 STAT=6。兩次 CALL 對應一次測試脈衝，正常只施放一發。

### N05　C++ Analog 干擾絲

操作：按一次 Analog 干擾絲（左 Ctrl＋左 Shift＋Delete），放開並等 1 秒。

看哪裡：右上 REQ、CALL、STAT、FLR、CHF。

正常：REQ 增 1。Analog 可用時 CALL 增 2、STAT=4、CHF 恰減 1、FLR 不變。入口缺少為 STAT=6。

### N06　按住與重複操作

操作：對已找到且未出錯的入口依序測：Digital 熱焰彈（左 Alt＋左 Shift＋Insert）、Digital 干擾絲（左 Alt＋左 Shift＋Delete）、Analog 熱焰彈（左 Ctrl＋左 Shift＋Insert）、Analog 干擾絲（左 Ctrl＋左 Shift＋Delete）。每鍵按住 2 秒後放開、等 1 秒，再各按放 5 次，每次間隔約半秒。

看哪裡：右上 REQ、CALL、FLR、CHF；是否自行連續施放。

正常：每個按住操作僅 REQ＋1、該類彈量減 1；放開後不再施放。後續 5 次按放僅 REQ＋5、該類減 5。Digital 每要求 CALL＋1、Analog＋2；另一類彈量不變。入口缺少的種類填未測；STAT=7 時停止本輪原生按鍵測試、繼續原有 MC 項目。

### N07　原生反制與主保險獨立

操作：按 OFF（右 Shift＋M），各按放一次四個測試鍵：Digital 熱焰彈（左 Alt＋左 Shift＋Insert）、Digital 干擾絲（左 Alt＋左 Shift＋Delete）、Analog 熱焰彈（左 Ctrl＋左 Shift＋Insert）、Analog 干擾絲（左 Ctrl＋左 Shift＋Delete），各等 1 秒。按 SIM（右 Alt＋M）再做同一組四鍵。最後按 ON（右 Ctrl＋M），等 2 秒。

看哪裡：ARM、REQ、CALL、STAT、FLR、CHF。

正常：OFF／SIM 下各入口依 N02–N05 相同規則執行；API 可用時 CALL 與對應實際施放數增加，主保險保持選定狀態。STAT=5 表示 MC 觀測尚未就緒；STAT=7 時停止本輪原生按鍵測試。

### N08　快速交錯兩種原生派送

操作：ON（右 Ctrl＋M）下，快速依序按放 Digital 熱焰彈（左 Alt＋左 Shift＋Insert）、Digital 干擾絲（左 Alt＋左 Shift＋Delete）、Analog 熱焰彈（左 Ctrl＋左 Shift＋Insert）、Analog 干擾絲（左 Ctrl＋左 Shift＋Delete），之後等 2 秒。

看哪裡：REQ、CALL、FLR、CHF。

正常：兩種 API 可用時 REQ＋4、CALL＋6、FLR 減 2、CHF 減 2，停止操作後沒有繼續釋放。只找到一種 API 時，只該種的兩個要求可實際施放，其餘記為入口缺少。

### N09　重新進入任務

操作：按 Esc 離開任務，再進入原 A 任務，等待 5 秒。

看哪裡：右上 REQ、CALL、STAT、FLR、CHF；實際飛機。

正常：REQ／CALL 重設為 0、STAT=1；彈量依任務重新載入且保持穩定，沒有自動補發前一任務的要求。接著繼續 M02 起的原有 MC 測試。

### M02　三個主模式

操作：依序按 DGFT（右 Ctrl＋D）、MSL（右 Alt＋D）、超控中央（右 Shift＋D）後按 NAV（1）；各次放開並等 2 秒。

看哪裡：HMCS 的 SYS、MODE、SUB。

正常：依序 ARM／DGF／ACM、ARM／MSL／BVR、ARM／NAV／---。

### M03　保險獨立於模式

操作：按 DGFT（右 Ctrl＋D），依序按 OFF（右 Shift＋M）、SIM（右 Alt＋M）、ON（右 Ctrl＋M）。

看哪裡：HMCS 的 SYS、MODE、SUB。

正常：SYS 依序 SAFE、SIM、ARM；MODE 全程 DGF、SUB 全程 ACM。

### M04　SAFE 下切換模式

操作：按 OFF（右 Shift＋M），依序按 DGFT（右 Ctrl＋D）、MSL（右 Alt＋D）、超控中央（右 Shift＋D）後按 NAV（1）。

看哪裡：HMCS 的 SYS、MODE、SUB。

正常：SAFE 保持；模式依序 DGF／ACM、MSL／BVR、NAV／---。

### M05　SIM 下切換模式

操作：按 SIM（右 Alt＋M），依序按 DGFT（右 Ctrl＋D）、MSL（右 Alt＋D）、超控中央（右 Shift＋D）後按 NAV（1）。

看哪裡：HMCS 的 SYS、MODE、SUB。

正常：SIM 保持；模式依序 DGF／ACM、MSL／BVR、NAV／---。

### M06　主保險重選

操作：按 DGFT（右 Ctrl＋D）；OFF（右 Shift＋M）連按 3 次；SIM（右 Alt＋M）連按 3 次；ON（右 Ctrl＋M）連按 3 次。

看哪裡：HMCS、實際武器；右上 TX。

正常：每次選擇保持該 SYS 值與 DGF；沒有意外開火，停止操作後 TX 最終回 0。

### G01　第一段扳機

操作：按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D）；按住第一段扳機（左 Alt＋Space）1 秒後放開。

看哪裡：實際砲火、右上 GUN 與 TRIG。

正常：沒有機砲射擊，GUN 數量不減；第一段不等同第二段。

### G02　機砲長按與彈量

操作：按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D）；按住機砲（Space）1 秒後放開。

看哪裡：砲火／砲聲；右上 GUN OK、GUN、TRIG。

正常：按住可射擊、放開後停止；GUN OK=1、數量下降，之後保持。DGF 的 WPN 仍可顯示 AAM-0。

### G03　機砲短按

操作：按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D）；機砲（Space）短按 5 次，每次間隔 1 秒。

看哪裡：每次砲火、砲聲及右上 GUN。

正常：五次均有有限射擊，沒有放開後持續連射；不要求每次只打一發。

### G04　NAV、MSL 開砲資格

操作：按 ON（右 Ctrl＋M）、超控中央（右 Shift＋D）後按 NAV（1），點按機砲（Space）；按 MSL（右 Alt＋D），再點按機砲（Space）。

看哪裡：實際砲火與右上 GUN。

正常：兩次都不開始射擊，GUN 數量不減。

### G05　SAFE、SIM 開砲資格

操作：按 DGFT（右 Ctrl＋D）、OFF（右 Shift＋M），點按機砲（Space）；改按 SIM（右 Alt＋M），再點按機砲（Space）。

看哪裡：SYS、實際砲火與右上 GUN。

正常：SAFE／SIM 均不開始射擊；MODE 維持 DGF。

### G06　DGFT機砲與解籠選擇獨立

操作：按主保險 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D）；確認 UNCAGE=0，點按機砲（Space）；點按 C 使 UNCAGE=1，再點按機砲（Space）；最後按 C 使 UNCAGE=0。

看哪裡：右上 UNCAGE、TRIG、GUN，實際砲火。

正常：UNCAGE=0與1時均有砲火，GUN隨實際射擊減少，放開Space後停止。機砲輸入維持獨立，UNCAGE保持選定值。

### G07　射擊中切 SAFE

操作：按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D）；按住機砲（Space），射擊時按 OFF（右 Shift＋M）；繼續按住 Space，按 ON（右 Ctrl＋M）並等 1 秒，最後放開 Space。

看哪裡：實際砲火、SYS、右上 GUN。

正常：SAFE 後停止；恢復 ON 時不自行復射。重新按機砲（Space）才開始新射擊。

### G08　射擊中切 SIM

操作：按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D）；按住機砲（Space），射擊時按 SIM（右 Alt＋M）；繼續按住 Space，按 ON（右 Ctrl＋M）並等 1 秒，最後放開 Space。

看哪裡：實際砲火、SYS、右上 GUN。

正常：SIM 後停止；恢復 ON 時不自行復射。

### G09　射擊中切主模式

操作：按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D）；按住機砲（Space），射擊時按 超控中央（右 Shift＋D）後按 NAV（1），等半秒後放開 Space。另一次把 超控中央（右 Shift＋D）後按 NAV（1）改成 MSL（右 Alt＋D）。

看哪裡：實際砲火，右上 GUN；另看 MODE。

正常：模式切換成功；放開 Space 後必須停止。本版模式限制只審核開始，已開始射擊在切模式後可能繼續，請記下實際結果。

### G10　機砲耗盡

操作：按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D），按住機砲（Space）直到射擊停止再放開。按 超控中央（右 Shift＋D）後按 NAV（1）查看機砲欄。

看哪裡：實際砲火；右上 GUN OK、GUN；HMCS 的 WPN。

正常：原生讀值降至 GUN 0、GUN OK=1；NAV 顯示 GUN 000。重新點按機砲（Space）不會增加彈量。

### C01　CMS Left 程序 6 七發干擾絲

操作：按 CMS Left（右 Ctrl＋J）後放開，等待 5 秒。

看哪裡：CHF／FLR 存量及機尾反制。

正常：CHF 減少 7，FLR 保持；每發間隔約 0.5 秒，完成後停止。

### C02　CMS Aft 電子干擾許可入口

操作：按 CMS Aft（右 Ctrl＋K）後放開，等待 2 秒。

看哪裡：機尾反制與 FLR／CHF 存量。

正常：電子干擾許可為預留入口，存量維持，沒有施放。

### C03　CMS Forward 固定十輪

操作：按 CMS Forward（右 Ctrl＋I）後放開；按外部視角（F2）觀察約 12 秒，再按座艙視角（F1）。

看哪裡：機尾反制、FLR／CHF 存量與 CMS 紀錄。

正常：完成十輪，每輪熱焰彈與干擾絲各一發，程序間隔約 1 秒；放開按鍵後繼續完成程序。

### C04　CMS Right 電子干擾待命入口

操作：按 CMS Forward（右 Ctrl＋I），約 2 秒後點按 CMS Right（右 Ctrl＋L），再等 12 秒。

看哪裡：機尾反制與 FLR／CHF 存量。

正常：右向為電子干擾待命預留入口；前向程序繼續完成，總計十輪混合施放。

### C05　程序中切換主保險

操作：按 CMS Forward（右 Ctrl＋I），約 2 秒後按 OFF（右 Shift＋M），再過 2 秒按 SIM（右 Alt＋M），再過 2 秒按 ON（右 Ctrl＋M），等待程序完成。

看哪裡：主保險顯示、機尾反制與 FLR／CHF 存量。

正常：主保險依操作切換，反制持續完成同一組十輪；切換不重啟程序。

### C06　SAFE／SIM 啟動反制

操作：按 OFF（右 Shift＋M），點按 CMS Forward（右 Ctrl＋I）後等 12 秒，再點按 CMS Left（右 Ctrl＋J）後等 5 秒；按 SIM（右 Alt＋M）重做同一組操作。

看哪裡：主保險顯示與 FLR／CHF 存量。

正常：OFF、SIM 各完成十輪混合施放及七發干擾絲；每組 FLR 減 10、CHF 減 17，主保險保持選定狀態。

### C07　NAV 可使用 CMS

操作：按超控中央（右 Shift＋D）後按 NAV（1），點按 CMS Forward（右 Ctrl＋I），等待 12 秒。

看哪裡：NAV 模式與 FLR／CHF 存量。

正常：保持 NAV 並完成十輪混合施放。

### C08　CMS 按住與回中

操作：按住 CMS Left（右 Ctrl＋J）約 2 秒後放開，再等待 5 秒；按 CMS Press（右 Alt＋PgDn）後等 2 秒。

看哪裡：機尾反制與 FLR／CHF 存量。

正常：左向按住只啟動一次七發干擾絲程序；放開回中後完成原程序，FLR 保持；預留按壓入口保持存量。

### I03　手動音效入口

操作：按音效測試（右 Shift＋T）三次，每次間隔 2 秒。

看哪裡：耳機聲音、MODE／SYS／武器。

正常：依序播放 AIM9 Seek、Acquire LO、Tone HI 的手動試聽；沒有開火／投放或 MC 模式變化。

### P01　NAV 的武器顯示偏好

操作：按 ON（右 Ctrl＋M）、超控中央（右 Shift＋D）後按 NAV（1）；確認 UNCAGE=0，點按 C 使 UNCAGE=1，再按 C 回 0；按住機砲（Space）1 秒後放開；最後再按 NAV（1）。

看哪裡：HMCS WPN；右上 GUN、AAM。

正常：起始 GUN；Uncage 後偏好 AAM，機砲按住時 GUN、放開後回 AAM，再按 NAV 回 GUN。整段不射擊。

### P02　拒絕的投放與畫面偏好

操作：按 ON（右 Ctrl＋M）、超控中央（右 Shift＋D）後按 NAV（1）；點按投放（右 Alt＋Space）；再按 超控中央（右 Shift＋D）後按 NAV（1）。

看哪裡：HMCS WPN；實際武器。

正常：拒絕投放仍可把顯示偏好改成 AAM；再次 NAV 後回 GUN；沒有離架。

### P03　DGF 機砲與 AAM-0

操作：按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D）；按機砲（Space）半秒後放開；再按 超控中央（右 Shift＋D）後按 NAV（1）。

看哪裡：HMCS WPN；右上 GUN。

正常：DGF 的 WPN 顯示 AAM-0，即使機砲正在射擊；GUN 數量由右上面板核對，回 NAV 後 WPN 顯示機砲存量。

### L02　低 FPS 的短按與停止

操作：在平常與較低 FPS 各做一輪：按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D），短按機砲（Space）；按 CMS Forward（右 Ctrl＋I），再按 CMS Right 預留入口（右 Ctrl＋L）；依序按 MSL（右 Alt＋D）、超控中央（右 Shift＋D）後按 NAV（1）。

看哪裡：砲火停止、反制中止、最後 MODE／SUB、TX。

正常：短按可完成且放開後停止；反制中止；最後 NAV／---，TX 最終回 0。

### L03　任務重入清除

操作：先按 ON（右 Ctrl＋M）、CMS Forward（右 Ctrl＋I）；程序未完成時按 Esc，滑鼠離開任務。放開全部實體按鍵，重新載入 A，等 5 秒。

看哪裡：新任務的 MODE／SYS、砲火、反制、右上面板。

正常：新任務回 ARM／NAV／---，沒有前次未完成反制／開火；新 GEN 與 ACK 流程可正常建立。

## B：空中八枚 AAM＋兩架目標

### S01　八枚混合空對空掛載

操作：載入 B；按 MSL（右 Alt＋D），等待 2 秒。

看哪裡：HMCS AAM；右上 AAM OK、AAM、SEL。

正常：AAM OK=1、AAM=8、SEL=0（RT）。CSV 保存九個掛點的原始數量與識別。

### S02　MSL STEP 同型輪站與換型

操作：按超控中央（右 Shift＋D）、NAV（1）、MSL（右 Alt＋D）。短按並放開 Q 四次；接著每次按住 Q 達 0.5 秒再放開，共三次；每次操作後等 1 秒。

看哪裡：右上 SEL；不用依 WPN 猜選站。

正常：進入時 SEL=0。短按依序 1、7、8、0（AIM-9X）。長按依序 2（AIM-7P）、4（AIM-120C）、0（AIM-9X）。長按放開時保持該站，AAM 保持 8。

### S03　超控重選保持與切換配置

操作：按 超控中央（右 Shift＋D）後按 NAV（1），按 DGFT（右 Ctrl＋D），再重按 DGFT（右 Ctrl＋D）一次，最後按 MSL（右 Alt＋D）。

看哪裡：右上 SEL；HMCS MODE。

正常：SEL 保持 0；重選 DGFT 保持原站及捕獲選擇，切換 MSL 套用該配置。

### S04　NAV 清除邏輯選站

操作：按 MSL（右 Alt＋D），等 1 秒，再按 超控中央（右 Shift＋D）後按 NAV（1），等 2 秒。

看哪裡：右上 SEL、TX；HMCS AAM／GUN。

正常：SEL=-1、TX 最終回 0。機上實體掛載不改變。

### T01　DGFT BORE 指定

操作：按 OFF（右 Shift＋M），按 DGFT（右 Ctrl＋D）並等 1 秒；對準前方目標，按住 TMS 上（I）約半秒，再放開。

看哪裡：標準 debug 的 SOI、Radar Requested／Observed；HMCS SUB；右上 RDR／STT。

正常：SOI=FCR／LEFT MFD；上向選 BORE，放開後提出 DESIGNATE。實際追蹤成功時 RDR=3、STT為正距離；主保險保持 SAFE。

### T02　DGFT 待操作與垂直捕獲

操作：按超控中央（右 Shift＋D），再按 DGFT（右 Ctrl＋D），等 1 秒；點按 TMS 下（K）；再點按一次 TMS 下（K）。

看哪裡：標準 debug 的 Radar Requested；HMCS SUB。

正常：由30×20待操作配置選10×60捕獲；再按下拒絕並回30×20待操作配置。debug標示簡化策略，實際追蹤以Radar Observed為準。

### T03　DGFT WVR／30×20入口

操作：按 DGFT（右 Ctrl＋D），等 1 秒；點按 TMS 右（L）。

看哪裡：Radar Requested、TMS Request；HMCS SUB。

正常：要求30×20捕獲，SUB=ACM；以原生boresight簡化執行；沒有武器投放。

### T04　CRM 按住與放開指定

操作：按 MSL OVRD（右 Alt＋D），等 1 秒；對準前方目標，按住 TMS 上（I）約 1 秒，再放開，等 2 秒。

看哪裡：TMS Input／Request／Result、Radar Observed；右上 RDR／STT。

正常：按住時 SPOTLIGHT 為 RESERVED；放開後提出 DESIGNATE。來源成功追蹤時 Radar Observed=STT、STT距離為正。

### T05　CRM 優先目標輪選空殼

操作：按 MSL OVRD（右 Alt＋D），等 1 秒；快速點按 TMS 右（L）。

看哪裡：TMS Request／Result、Radar Observed。

正常：要求 STEP PRIORITY TARGET，結果 RESERVED；原生追蹤狀態保持來源值，沒有以重新換鎖冒充優先目標輪選。

### T06　取消指定與回NAV

操作：按 MSL OVRD（右 Alt＋D），等 1 秒；點按 TMS 下（K），等 1 秒；按超控中央（右 Shift＋D），再按 NAV（1），等 1 秒。

看哪裡：TMS Request、SOI Source／Display、Radar Observed；MODE／SUB。

正常：下向取消MC指定並結束捕獲輸入；回NAV後SOI=HUD、左HSD空殼／右SMS。原生Radar Observed持續回報來源狀態。

### T07　CRM／ACM目標資料展開入口

操作：按 DGFT（右 Ctrl＋D），等 1 秒；點按 TMS 左（J）。再按 MSL OVRD（右 Alt＋D），等 1 秒；點按 TMS 左（J）。

看哪裡：TMS Request／Result。

正常：兩次均為 EXPAND TARGET DATA／RESERVED，保留1997年手冊的資料展開語意；沒有投放或雷達模式切換。

### T08　解籠切換、長按與放開

操作：按 DGFT（右 Ctrl＋D）；確認 UNCAGE=0，按住 C 約 4 秒後放開，等 1 秒；再點按 C。按 MSL（右 Alt＋D）後重做一次。

看哪裡：右上 UNCAGE、RDR、STT、TX。

正常：第一次按下時 UNCAGE 由 0 變 1，長按與放開後維持 1；第二次按下變 0。操作本身不發射，沒有每秒重新捕獲，TX 最終回 0。

### T09　鎖定後切保險

操作：按 MSL（右 Alt＋D），按住 TMS 上（I）1 秒再放開；成功追蹤後依序按 OFF（右 Shift＋M）、SIM（右 Alt＋M）、ON（右 Ctrl＋M）。

看哪裡：SYS、MODE、SUB、右上 RDR／STT。

正常：SYS 改變，MSL／BVR 保持；主保險不主動重設雷達模式或指定。若目標仍在可追蹤位置，記錄追蹤是否維持。

### T10　SIM 下捕獲

操作：按 SIM（右 Alt＋M）、MSL（右 Alt＋D）；按住 TMS 上（I）1 秒再放開。

看哪裡：SYS、SUB、右上 RDR、STT、TX。

正常：SIM／MSL／BVR 保持；捕獲操作可送出，TX 最終回 0。實際追蹤依 RDR／STT 回報。

### K01　單鍵解籠後發射一枚AIM-9

操作：重新載入 B；按主保險 ON（右 Ctrl＋M），按 A-A（2），等 2 秒。確認 SEL=0、UNCAGE=0；點按 C 並放開，確認 UNCAGE=1；點按右 Alt＋Space 並放開。等 3 秒，再按 C 使 UNCAGE=0。

看哪裡：右上 UNCAGE、AAM、SEL；實際離架；CSV 的投放按下序號。

正常：放開 C 後 UNCAGE 仍為 1；投放輸入可獨立送到 MC。預期 RT 的 AIM-9X 離架一枚，AAM 從 8 變 7；投放放開後停止，TX 最終回 0。記下實際離架數量；本項只確認離架，不驗收導引或命中。

### W01　沒有 Uncage 的投放

操作：重新載入 B；按 ON（右 Ctrl＋M）、MSL（右 Alt＋D）；不碰 Uncage，點按投放（右 Alt＋Space）。

看哪裡：實際掛載、右上 AAM。

正常：沒有離架，AAM 保持 8。

### W02　SAFE／SIM 投放資格

操作：按 MSL（右 Alt＋D）、OFF（右 Shift＋M）；確認 UNCAGE=0，點按 C 使 UNCAGE=1，再點按投放（右 Alt＋Space）；按 SIM（右 Alt＋M），再點按投放（右 Alt＋Space）。最後按 C 使 UNCAGE=0。

看哪裡：SYS、實際離架、AAM；CSV。

正常：SAFE、SIM 均不離架，AAM 不減。

### W03　NAV 投放資格

操作：按 ON（右 Ctrl＋M）、超控中央（右 Shift＋D）後按 NAV（1）；確認 UNCAGE=0，點按 C 使 UNCAGE=1，再點按投放（右 Alt＋Space）；最後按 C 使 UNCAGE=0。

看哪裡：MODE、實際離架、AAM；CSV。

正常：NAV 不離架，AAM 不減；WPN 可以因操作偏好改成 AAM。

### W04　MSL 短按發射與自動換站

操作：重新載入 B；按 ON（右 Ctrl＋M）、MSL（右 Alt＋D）；點按 C 使 UNCAGE=1，再點按並放開投放（右 Alt＋Space）；最後按 C 使 UNCAGE=0；等 3 秒。

看哪裡：實際離架位置、右上 AAM／SEL／TX。

正常：原 RT 離架後 AAM 從 8 變 7，SEL 接續第一個仍有彈候選（RO=1），TX 最終回 0；放開後沒有持續投放。若一次實際離架超過一枚，記下數量與按住時間。

### W05　指定掛點的發射

操作：重新載入 B；按 ON（右 Ctrl＋M）、超控中央（右 Shift＋D）後按 NAV（1）、MSL（右 Alt＋D）；按住 Q 至少 0.5 秒再放開一次，等 SEL=2。點按 C 使 UNCAGE=1，再點按並放開投放（右 Alt＋Space）；最後按 C 使 UNCAGE=0。

看哪裡：右上 SEL=2；外部 RI 的 AIM-7P；AAM。

正常：發射使用 RI；實際離架與 AAM 減量相符。此項確認選站／投放，飛彈導引效果另外記錄。

### W06　另一種空對空飛彈

操作：重新載入 B；按 ON（右 Ctrl＋M）、超控中央（右 Shift＋D）後按 NAV（1）、MSL（右 Alt＋D）；連續兩次各按住 Q 至少 0.5 秒後放開，等 SEL=4。點按 C 使 UNCAGE=1，再點按並放開投放（右 Alt＋Space）；最後按 C 使 UNCAGE=0。

看哪裡：MF 的 AIM-120C 離架、右上 AAM／SEL。

正常：發射使用 MF；數量與實際離架相符。命中率與完整導引不作這輪 MC 驗收依據。

### W07　DGFT 飛彈發射

操作：重新載入 B；按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D）；點按 C 使 UNCAGE=1，再點按並放開投放（右 Alt＋Space）；最後按 C 使 UNCAGE=0。

看哪裡：實際 RT 離架、右上 AAM／SEL／TX。

正常：DGFT 可經同一投放入口發射；數量與實際離架相符，放開後停止。

### W08　長按投放後放開

操作：重新載入 B；按 ON（右 Ctrl＋M）、MSL（右 Alt＋D）；點按 C 使 UNCAGE=1，再按住投放（右 Alt＋Space）1 秒後放開，等 3 秒；最後按 C 使 UNCAGE=0。

看哪裡：每枚實際離架、右上 AAM／SEL／TX。

正常：數量跟隨實際離架；放開後完成收尾、TX 最終回 0，沒有持續新增離架。長按實際發射幾枚照實記錄。

### W09　快速選站後投放

操作：重新載入 B；按 ON（右 Ctrl＋M）、MSL（右 Alt＋D），立即短按並放開 Q 一次；接著點按 C 使 UNCAGE=1，再點按並放開投放（右 Alt＋Space）；最後按 C 使 UNCAGE=0。

看哪裡：實際離架掛點、右上 SEL／AAM／TX；CSV。

正常：最後選擇 RO=1；投放先等選站執行，再使用 RO，放開後沒有卡住。

### W10　飛彈耗盡

操作：重新載入 B；按 ON（右 Ctrl＋M）、MSL（右 Alt＋D）；點按 C 使 UNCAGE=1。重複「點按並放開投放（右 Alt＋Space）→等 2 秒」，直到全部離架；最後再投放一次，再按 C 使 UNCAGE=0。

看哪裡：每次實際離架、右上 AAM／SEL、HMCS。

正常：AAM 跟隨實際剩餘量降至 0，SEL=-1；再次投放沒有新離架，已用完彈量保持 0。

### W11　關閉解籠選擇後投放

操作：重新載入 B；按 ON（右 Ctrl＋M）、A-A（2）；點按 C，確認 UNCAGE=1，再點按 C，確認 UNCAGE=0；點按投放（右 Alt＋Space）。

看哪裡：UNCAGE、實際離架、AAM。

正常：第二次按 C 後 UNCAGE=0，新的投放要求被拒絕，沒有離架，AAM 保持 8。

### W12　投放中切 SAFE／SIM（已有獨立控制時）

操作：有搖桿時：按 ON（右 Ctrl＋M）、MSL（右 Alt＋D），點按 C 使 UNCAGE=1，再按住預設投放 JOY_BTN2；投放時按 OFF（右 Shift＋M），再按 ON（右 Ctrl＋M），最後放開 JOY_BTN2。重新載入後將中途 OFF（右 Shift＋M）改成 SIM（右 Alt＋M）再做一次。

看哪裡：實際離架、SYS、AAM；CSV。

正常：OFF／SIM 後停止後續投放，恢復 ON 不自行續發。沒有獨立投放按鈕時填「無此設備」；工程回歸另覆蓋這條停止路徑。

### I01　來源 IR 鎖定音

操作：重新載入 B；對準前方飛機並接近至約 1–3 km；按 MSL（右 Alt＋D），點按 C 使 UNCAGE=1，觀察 4 秒，再按 C 使 UNCAGE=0。

看哪裡：右上 IR OK、LOCK；實際耳機聲音。

正常：IR OK=1 且 LOCK=1 時有鎖定音；有效 LOCK=0 時沒有合成搜索／捕獲音。記下是否出現 LOCK=1，雷達 RDR=3 不代替 IR LOCK。

### I02　IR 脫離及跨模式觀測

操作：曾出現 IR LOCK=1 後，轉離目標直到 LOCK=0；依序按 超控中央（右 Shift＋D）後按 NAV（1）、OFF（右 Shift＋M）、SIM（右 Alt＋M）。

看哪裡：右上 IR OK、LOCK；聲音。

正常：鎖定音跟隨有效 IR LOCK；LOCK=0 或 IR OK=0 時停止。主模式與保險本身不是鎖定音的停止條件。

### L01　快速跨系統操作

操作：按 ON（右 Ctrl＋M）；依序快速點按 DGFT（右 Ctrl＋D）、MSL（右 Alt＋D）、TMS 上（I）、TMS 下（K）、CMS Left（右 Ctrl＋J）、超控中央（右 Shift＋D）後按 NAV（1），每組結束等 2 秒，重做 5 組。

看哪裡：最後 MODE／SUB、右上 TX／ACK；實際武器。

正常：每組最後 NAV／---，沒有意外開火或離架；TX 最終回 0、ACK 可繼續推進，下一組仍能操作。

### L04　射擊／投放後重入與全重啟

操作：完成一次機砲（Space）或投放（先按 C；再點按右 Alt＋Space）後，按 Esc 並用滑鼠離開；再進 B。最後完全關閉並重開 DCS，再進 B。

看哪裡：機上八枚 AAM、初始機砲量、SYS／MODE、TX。

正常：每次都依新任務初始化，UNCAGE=0；彈量與選站依新任務設定，沒有自動發射。

### L05　既有搖桿入口（有搖桿時）

操作：使用原有搖桿預設機砲 JOY_BTN1、投放 JOY_BTN2，以及已配置的 Uncage／TMS／CMS；對照鍵盤機砲（Space）、投放（右 Alt＋Space）、解籠切換（C）、TMS（I／K／J／L）、CMS（右 Ctrl＋I／K／J／L；按壓為右 Alt＋PgDn）。

看哪裡：相同功能的實際效果、放開停止與 CSV。

正常：同功能經相同 MC 入口處理，放開均能收尾。沒有該實體控制時填「無此設備」。

### T11　CRM右向長按與TWS空殼

操作：按 MSL OVRD（右 Alt＋D），等 1 秒；按住 TMS 右（L）約 2 秒後放開，等 1 秒；再按住同鍵約 2 秒後放開。

看哪裡：TMS Request／Result、Radar Requested／Observed。

正常：第一次要求TWS (SHELL)，SEARCH MODE結果RESERVED；放開保留這個結果。第二次回RWS。原生Radar Observed仍是實際SEARCH或STT。

## C：空中混合掛載

### S05　混合掛載與複合掛架

操作：載入 C，按 MSL（右 Alt＋D），等 2 秒。

看哪裡：外部掛載；HMCS AAM；右上 SEL；CSV 九站。

正常：AAM=2，第一個可用 AAM 站為 RT=0；炸彈、油箱與複合掛架保存於各站資料，沒有混入 AAM 數量。

### S06　混合掛載的型別輪換

操作：按超控中央（右 Shift＋D）、NAV（1）、MSL（右 Alt＋D）。短按 Q，再各按住 Q 達 0.5 秒並放開三次；各次等 1 秒。

看哪裡：右上 SEL；實際掛載。

正常：SEL 由 0 開始；短按保持 0（唯一 AIM-9X）。長按依序 8（AIM-9L）、0、8；略過炸彈／油箱／複合掛架。

## E：空中全部 CLEAN

### S07　全部 CLEAN

操作：載入 E，按 MSL（右 Alt＋D），等 2 秒。

看哪裡：外部各掛點、AAM OK、AAM、SEL；CSV。

正常：外觀與 CLEAN 配置一致；AAM OK=1、AAM=0、SEL=-1；九站均有掃描紀錄。

## F：空中全部維持空

### S08　維持空掛載

操作：載入 F，按 MSL（右 Alt＋D），等 2 秒。

看哪裡：外部掛架、AAM OK、AAM、SEL；CSV。

正常：外觀與任務的空配置一致；AAM OK=1、AAM=0、SEL=-1。若原生 API 對空與 CLEAN 都回 nil，紀錄保持這個實際來源結果。

## C-AG：混合掛載的空對地飛彈

### S09　空對地飛彈觀測（選單有提供時）

操作：將 C 的 MM 複合掛架改成任務編輯器中可選的空對地飛彈，記下名稱；進任務後按 MSL（右 Alt＋D），等 2 秒。

看哪裡：外部掛載、AAM；CSV MM 原始資料。

正常：該飛彈身分／數量保存在 MM，AAM 仍為兩枚翼尖空對空飛彈；循環不選 MM。這項驗證庫存，A-G 發射流程另行實作。

## D：友軍機場熱機補給

### R01　地面機砲補彈

操作：載入 D，按 ON（右 Ctrl＋M）、DGFT（右 Ctrl＋D），點射機砲（Space）3 次；按 OFF（右 Shift＋M）。開通訊（反斜線鍵 \），用滑鼠選地勤／重新裝填，完成後等待 5 秒。

看哪裡：右上 GUN 與機外掛載；CSV。

正常：GUN 補回該任務裝填量。若通訊選單沒有開啟，回報 R01「通訊阻擋」，保留其他項目繼續測。

### R02　地面外掛補彈與換裝

操作：在 D 開通訊（反斜線鍵 \），用地勤把 RT／LT 改成 AIM-9X、RO／LO 改成 AIM-120C；完成後按 ON（右 Ctrl＋M）、超控中央（右 Shift＋D）後按 NAV（1）、MSL（右 Alt＋D）。

看哪裡：外部掛載、右上 AAM／SEL；CSV rearm revision。

正常：AAM=4、SEL=0；已改掛點的識別與數量更新，後續模式循環使用新庫存。

## 測完回傳

1. 完成後關閉 DCS，在解壓縮的測試包執行 COLLECT-RESULTS.bat。若 Saved Games 路徑不同，將該資料夾路徑作為第一個參數。
2. 回傳產生的 FCK1C-MC-RESULTS-日期.zip，並寫「異常代號＋實際情況」及未測／阻擋代號；可選填 RESULTS.csv。將自己建立的 .miz 任務一併附上，供核對掛載、目標與彈量設定。
3. 記錄器會保存各次任務的座艙參數、原始掛點字串、MC 快照、DCS 視窗／ACK、雷達與 IR 來源。收集包另含安裝 DLL 雜湊、DCS 日誌及既有輸入設定。

## 驗證範圍

人工流程共 86 項，涵蓋目前實作的模式、反制、掛載、選站、火控資格、捕獲、原生彈量、IR 狀態、座艙呈現與生命週期。

逐站分類／時效、缺失觀測、錯誤／部分 ACK、80 ms 保持及跨 tick 順序，另由 CI 的原生／Lua 回歸測試注入並核對；人工只核對可操作的結果。原生測試的 C++ 派送使用獨立測試佇列，Lua 僅取樣實際反制存量。後續目標為透過 SystemPipeline 的 C++ 反制系統；本包保留現行 CMS 程序供回歸。座艙記錄是取樣，MC 與掛點的 coherent 欄位各自表達同一份完整來源版本；原始雷達與呈現值各自取樣。

目前 SystemMonitor／FailureManager 是預留入口；冷啟動、供電、故障備援、A-G 發射規劃、HUD 掃描實作及自動 Seek／Acquire 音效留在後續功能範圍。

本輪刻意保留的行為：保險與主模式獨立；機砲在 ON＋DGF 時接受新開始，與UNCAGE選擇獨立；AAM 投放在 ON＋空戰模式＋UNCAGE=1 時接受新開始；已開始操作的主模式切換採原規則，OFF／SIM 才主動中止；鎖定音由有效 IR 來源回報決定。CMS 在各主保險狀態下啟用；前向固定十輪混合施放，左向程序 6 固定七發干擾絲、間隔 0.5 秒；後／右為預留入口。

安裝時會備份並移除舊F-CK-1C掛載匯出與彈量探針入口。回收包包含recording-health.json，可核對每份錄製CSV的資料列。來源commit、DLL SHA256與Windows CI位於測試包根目錄BUILD.json。
