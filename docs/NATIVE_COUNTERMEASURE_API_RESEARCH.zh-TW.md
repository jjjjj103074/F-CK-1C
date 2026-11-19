# 原生反制物 API 研究紀錄

研究日期：2026-11-15。範圍為 CockpitBase 原生派送 API、命令編碼、開源呼叫證據、接收端及 EFM 執行環境。本輪整理原始碼與實測證據，操作方式沿用目前設定。「已證實」只指固定原始碼、原作者貼文或本機原始碼查核；不是已驗證目前 DCS binary 的所有內部行為。

**本專案目前缺少處理 357／358 並真正施放反制物的接收端。這是已證實的專案缺口，也是 API 正常返回但庫存不減的最強因果解釋。** 公開範例明確把「派送命令」與「建立 flare／chaff」分成兩步。仍需有接收端的執行期對照，才能確認目前 binary 是否另有問題；本研究沒有宣稱已修復。

## 宣告與真實呼叫

| 原始碼固定快照 | 實際讀到的內容 | 證據強度 |
| --- | --- | --- |
| A-4E，`34c1ef439581234ee24d4bde2e13a54999ebaa5a` | 標頭宣告 digital、analog、to_device。 | 只有宣告；不能當成 digital／analog 成功實例。[cockpit_base_api.h 第 5–10 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/cockpit_base_api.h#L5-L10) |
| A-4E，同一 SHA | Avionics 的 cold／hot／airborne 初始化真正呼叫 **to_device**，設氧氣開關為 0／1。 | 真實 C++ 呼叫式，但入口不同於 digital／analog。[Avionics.cpp 第 61–82 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/Avionics.cpp#L61-L82) |
| A-4E，同一 SHA | 雷達初始化、輸入與旋鈕 helper 真正呼叫 **to_device**。 | 可追到執行路徑的 C++ 使用範例。[Radar.cpp 第 79–83 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/Radar.cpp#L79-L83)、[第 134–178 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/Radar.cpp#L134-L178)、[Radar.h 第 252–262 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/Radar.h#L252-L262) |
| DiJester/yf-23a，`9d875752b1d0494d3cce0bd348d345efe5d545fb` | 同樣的三個 API 宣告。 | 本次只找到標頭；不是第二個 digital／analog 成功實例。[固定標頭第 5–10 行](https://github.com/DiJester/yf-23a/blob/9d875752b1d0494d3cce0bd348d345efe5d545fb/EFM/FM/cockpit_base_api.h#L5-L10) |
| CrudeCoder1/Helicopter-EFM-Demo，`38578cbf5b1f701b99b0d18b4333c417f96dd732` | `CockpitAPI::dispatchAction` 本體呼叫 to_device pointer，pointer 由 GetProcAddress 取得。 | 已有 wrapper 本體；本次 scoped search 未找到實際呼叫該 wrapper 的 EFM 程式端。[CockpitAPI.h 第 47–50 行](https://github.com/CrudeCoder1/Helicopter-EFM-Demo/blob/38578cbf5b1f701b99b0d18b4333c417f96dd732/EFM%20Source%20Code/EFM/include/Cockpit/CockpitAPI.h#L47-L50)、[ccParametersAPI.h 第 35–49 行](https://github.com/CrudeCoder1/Helicopter-EFM-Demo/blob/38578cbf5b1f701b99b0d18b4333c417f96dd732/EFM%20Source%20Code/EFM/include/Cockpit/ccParametersAPI.h#L35-L49) |
| Helicopter-EFM-Demo，同一 SHA | digital／analog 名稱位於多行註解的 DLL 匯出清單。 | 不是呼叫或成功測試。[ccParametersAPI.h 第 55–59 行](https://github.com/CrudeCoder1/Helicopter-EFM-Demo/blob/38578cbf5b1f701b99b0d18b4333c417f96dd732/EFM%20Source%20Code/EFM/include/Cockpit/ccParametersAPI.h#L55-L59) |

以完整名稱 `ed_cockpit_dispatch_action_digital`、`ed_cockpit_dispatch_action_analog` 及去除常見標頭的變體進行公開 GitHub 搜尋後，本次可見結果包含 A-4E、其 forks、yf-23a、NH90 SDK 標頭及 Helicopter-EFM-Demo 註解，**沒有找到可驗證的 digital／analog 實際呼叫端**。這是有範圍的搜尋負面結果，不是「從未有人成功使用」的證明。A-4E forks 也不能算獨立驗證。

demo 的 to_device typedef 使用 int、int、double；A-4E 及 NH90 保存的宣告使用 unsigned char、int、float。不能直接以 demo typedef 決定現行 ABI。[demo 第 11 行](https://github.com/CrudeCoder1/Helicopter-EFM-Demo/blob/38578cbf5b1f701b99b0d18b4333c417f96dd732/EFM%20Source%20Code/EFM/include/Cockpit/ccParametersAPI.h#L11)、[A-4E 第 9 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/cockpit_base_api.h#L9)、[SDK 第 65 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/include/Cockpit/Base/Header/API/ccBaseData.h#L65)

## 命令數字與轉換

Mysteryfx/NH90 的 `SDK_Perso` 是第三方倉庫保存的歷史 SDK 原始碼副本。本文沒有獨立確認發行來源、年份及它與目前 DCS binary 的一致性；以下不是現行官方支援契約。

其 C++ 分支把 REGISTER_COMMAND 展開成遞增 enum，REGISTER_COMMAND_EX 指定明確值。Pickle 固定為 350 後，flare／chaff 依序為 **357／358**。目前沒有證據顯示這兩個數字因為從 Lua 取用，到了 C++ 就應加減固定 offset。[iCommand.h 第 21–28 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/include/inter/Include/iCommand.h#L21-L28)、[第 399–409 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/include/inter/Include/iCommand.h#L399-L409)

另一個 C++ EFM 專案 Zaretto/acEFM 的固定快照也明列 flare=357、chaff=358，名稱查表保持相同值。這是數字的交叉佐證，不是 dispatch 成功呼叫證據。[iCommands.h 第 378–379 行](https://github.com/Zaretto/acEFM/blob/00ea144c5c38b3692156eb4a01dbe3658ad2d4a8/flyt-EFM-dcsJSBSim/iCommands.h#L378-L379)、[查表第 2172–2173 行](https://github.com/Zaretto/acEFM/blob/00ea144c5c38b3692156eb4a01dbe3658ad2d4a8/flyt-EFM-dcsJSBSim/iCommands.h#L2172-L2173)

| 類型 | 原始碼編碼 | 判讀 |
| --- | --- | --- |
| 數位 iCommand | flare=357，chaff=358。 | digital(357) 的歷史符號數字吻合；仍未證實現行 DLL 內入口如何處理。 |
| 類比軸 iCommandKey | null=2000，pitch=2001、roll=2002 等。 | analog(357, 1/0) 不是此表的標準軸用法；沒有找到它可替代數位 flare 命令的契約。[Actions.h 第 18–31 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/include/inter/Include/Actions.h#L18-L31) |
| cockpit device command | 保留 3000–3999。 | to_device 範例使用特定 device command，不能把全域 357 隨意搭配 device ID 當成等價使用。[Actions.h 第 202–207 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/include/inter/Include/Actions.h#L202-L207) |

A-4E 的 to_device 直接使用 brilliance=3058、oxygen=3138，device 為 radar=10、avionics=2；呼叫點沒有額外轉碼。旋鈕 helper 預設將值夾在 0–1，但這只是該控制的語意，不能推成所有 API 的通用範圍。[Commands.h 第 362 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/Common/Common/Commands.h#L362)、[第 500 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/Common/Common/Commands.h#L500)、[Devices.h 第 17 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/Common/Common/Devices.h#L17)、[第 44 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/Common/Common/Devices.h#L44)、[Radar.h 第 252–262 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/Radar.h#L252-L262)

## 接收端與建立反制物

ED Team 的 Alex Okean 在原作者討論中說明 listen_command 是訂閱通知，由 SetCommand 接收；dispatch_action 向玩家單位送命令，並把副本寫入 track。這支持分開查核「派送」和「接收者執行」。該說明是 2012-06-15 的 Lua 行為解釋，沒有承諾現行 C++ MT 呼叫條件。[官方原作者討論第 3 頁，Alex Okean 當日貼文](https://forum.dcs.world/topic/74145-dcs-mods-structure-how-to-create-your-plugin-from-scratch/page/3/)

歷史 SDK 也把三個 dispatch API 註為會派送並記錄 track，set_action 則不記錄；它們回傳 void，沒有「接收者已執行」或「已建立反制物」的 acknowledgement。因此正常返回本身不足以確認施放。[ccBaseData.h 第 59–69 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/include/Cockpit/Base/Header/API/ccBaseData.h#L59-L69)

**完整正面範例：A-29B。** 在 `1f632f004e28ef13a5f3404528de4c3fc982a0e2`，weapon_system.lua 明確定義 357／358 並訂閱；SetCommand 收到後呼叫 dev:drop_flare()／dev:drop_chaff()，其中有自身的庫存條件。該快照的裝置登錄是 **avSimplestWeaponSystem**，與本機使用的 avSimpleWeaponSystem 類別不同；此例用來佐證完整的指令消費鏈。[A-29B 裝置登錄第 38 行](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/device_init.lua#L38) 這證明開源模組明確補上了派送後的接收與施放端，而不是依賴命令數字自行建立反制物。[常數及訂閱第 943–950 行](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L943-L950)、[處理及施放第 1203–1209 行](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L1203-L1209)

較早的 RocketEagle/A-4-Skyhawk 固定快照亦先訂閱 flare／chaff，在 SetCommand 分支呼叫 WeaponSystem:drop_flare(1)／drop_chaff(1)。該快照的裝置登錄使用 **avSimpleWeaponSystem**，與本機同類。[早期 Skyhawk 裝置登錄第 37 行](https://github.com/RocketEagle/A-4-Skyhawk/blob/90a550a86e153e0a302fbaf18936509b3890af43/A-4E/Mods/aircraft/A-4E/Cockpit/Scripts/device_init.lua#L37) 這是第二個可定位的完整接收端範例。[訂閱第 50–51 行](https://github.com/RocketEagle/A-4-Skyhawk/blob/90a550a86e153e0a302fbaf18936509b3890af43/A-4E/Mods/aircraft/A-4E/Cockpit/Scripts/Systems/weapon_system.lua#L50-L51)、[處理第 139–142 行](https://github.com/RocketEagle/A-4-Skyhawk/blob/90a550a86e153e0a302fbaf18936509b3890af43/A-4E/Mods/aircraft/A-4E/Cockpit/Scripts/Systems/weapon_system.lua#L139-L142)

歷史 avSimpleWeaponSystem 範例本身只訂閱 fire、pickle、換武器、master arm、jettison 等；SetCommand 沒有 flare／chaff 分支，未知命令轉交 avLuaDevice。**建立 avSimpleWeaponSystem 不等於自動具備 357／358 接收器。**[初始化第 84–101 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/samplers/WeaponAndSensorsSystems/avSimpleWeaponSystem.cpp#L84-L101)、[SetCommand 第 117–164 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/samplers/WeaponAndSensorsSystems/avSimpleWeaponSystem.cpp#L117-L164)

該類別的 Lua drop_* binding 則取得裝置的 cockpit context 及 human，真正呼叫 Create_Chaff_Human／Create_Flare_Human，帶入 count／group；此處才有建立反制物的 sink。[實作第 326–350 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/samplers/WeaponAndSensorsSystems/avSimpleWeaponSystem.cpp#L326-L350)、[註冊第 364–367 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/samplers/WeaponAndSensorsSystems/avSimpleWeaponSystem.cpp#L364-L367)

目前 A-4E 把 COUNTERMEASURES 建為 avSimpleWeaponSystem，release 函式真正呼叫 CMS:drop_chaff／drop_flare。須精確區分：該檔雖有訂閱舊 357／358 符號，但兩符號僅出現在訂閱處，SetCommand 沒有對應分支；因此本研究沒有證實 A-4E 的 digital(357) 會觸發 release。[device_init.lua 第 83 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/device_init.lua#L83)、[訂閱第 89–90 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/countermeasures.lua#L89-L90)、[施放第 139–157 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/countermeasures.lua#L139-L157)

## EFM、thread 與 context

A-4E 的 ed_fm_simulate 呼叫 radar.update；該 update 在旋鈕移動時經 moveKnob 呼叫 to_device。這是從 EFM callback 可追到的實際 C++ 路徑，因此不能斷言所有 CockpitBase 派送 API 都禁止在 EFM callback 中使用。但此例不能證明 digital／analog 的契約，也不是目前 MT binary 的成功測試。[Scooter.cpp 第 249 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/Scooter.cpp#L249)、[第 312–318 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/Scooter.cpp#L312-L318)、[Radar.cpp 第 358–377 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/Radar.cpp#L358-L377)、[Radar.h 第 257–262 行](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/FM/Radar.h#L257-L262)

drop_* binding 依賴特定裝置的 context／human，這是資料依賴；不能據此推導任意 EFM thread 均可直接使用這些物件。[avSimpleWeaponSystem.cpp 第 339–349 行](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/samplers/WeaponAndSensorsSystems/avSimpleWeaponSystem.cpp#L339-L349)

本次未找到 ED 或 API 作者公開規定 digital／analog 的 EFM 允許執行緒、TLS/current-cockpit 要求、queue flush 時機、可重入性或鎖序。**thread/context 限制目前未知，不能列為已證實根因。** 解除本專案 Bridge 鎖只排除該鎖仍被持有，不能代表已滿足未公開的引擎契約。

## 本專案缺口與實測

研究讀取當前本機兩個 Lua 檔案，並對 Cockpit 與 EFM 搜尋所有相關指令及施放方法：

- device_init.lua 將 CMS device 3 建成 avLuaDevice，weapon device 4 建成 avSimpleWeaponSystem。[本機 device_init.lua 第 31 行](/mnt/d/Programs/F-CK-1C/Cockpit/Scripts/device_init.lua:31)
- CMS 單發路徑只 dispatch 全域 flare／chaff iCommand；武器 adapter 建立 WeaponSystem 裝置供 station 功能使用。[cms_system.lua 第 48 行](/mnt/d/Programs/F-CK-1C/Cockpit/Scripts/Systems/cms_system.lua:48)、[weapon_system.lua 第 2 行](/mnt/d/Programs/F-CK-1C/Cockpit/Scripts/Systems/weapon_system.lua:2)
- 兩個檔案均沒有 listen_command、function SetCommand、:drop_flare 或 :drop_chaff；完整搜尋也未找到其他 357／358 接收與施放端。此項是當前 authored code 的缺口，不靠推測未公開 DLL 實作成立。
- 與 A-29B／早期 Skyhawk 對照，欠缺的是「357／358 → 訂閱 → SetCommand → drop_*」的後半段；API 本身不會替專案補上 Lua handler。

本次重放使用者提供的既有實測 trace：DCS 2.9.31.29434 MT；probe 在 ed_fm_simulate 解除 Bridge 鎖後派送；api_mask=3；digital／analog 共 40 次 flare requests；exception=0；60.737 秒後 LoGetSnares／get_flare_count 仍為 90，無限彈藥關閉。analog 的既有策略是 1 後約 42 ms 送 0。研究工作沒有自行新增 DCS run；這是現有實測紀錄的重放結果。

這個觀察可證明「函式可解析、可返回，但沒有觀察到庫存扣減」。缺少接收端已能合理解釋此現象；它仍不能證明補上接收端後現在的 DLL 入口一定可用。

| 狀態 | 結論 |
| --- | --- |
| 已證實 | 本專案 authored code 沒有 357／358 接收及 drop_* 執行端；可見源碼的 digital／analog 主要是宣告，to_device 則有真實 C++ 呼叫。 |
| 最強因果推論 | 沒有消費者抵達施放 sink，可以同時解釋 digital／analog 都正常返回但不扣彈。 |
| 尚待一個對照實測 | 讓已知接收端記錄收到的 command／value 與呼叫 drop_*，再以同一庫存觀察驗證是否走通；這是驗證建議，不是本研究已執行的修改。 |
| 未知 | 現行 binary 是否另有過濾、context 或 thread 問題；digital／analog 的內部轉換。 |
| 無支持 | 任意改 357／358 offset、只調整 analog 脈衝即認定修復，或把 export／無 SEH 視為施放成功。 |

## 來源與限制

程式碼皆使用固定 SHA 永久連結及精確行號。論壇使用原作者原討論第 3 頁與 2012-06-15 日期；個別 comment permalink 抓取回覆 403，沒有捏造 comment ID。歷史 SDK 副本的實作細節不能當成 DCS 2.9.31 MT 目前 binary 的實作證明。

只採用模組原始碼、保存的 SDK 原始碼及 ED Team 原作者討論；沒有以一般使用者猜測或二手教學支持結論。GitHub 程式碼讀取與公開原作者頁面用於交叉核對；派送成功與實際施放分別判定。

## 擴充公開模組盤點：C++ 施放與 Lua 路徑

### 盤點範圍與判讀方法

2026-11-15 擴充搜尋 OH-6／AH-6、CH-53、UH-60、Hercules／C-130、F-16／F-22／F-35／F-4／F-104／Mirage、JSBSim、Eurofighter、F-117、A-6、T-50、MQ-9、Me-163、SK60、Pucará，以及 EFM template／proxy／UI 工具。下表是本批實際查核的 **40 個 repository 快照**，含同源比較、工具、空倉庫，不能讀成 40 個獨立模組或成功案例。另列主線專項查核的 F-23／JAS 39；A-4E、A-29B、NH90 SDK 的證據見本文前節。

對有 HEAD 的倉庫先讀取完整遞迴 tree，皆未遭截斷，再讀固定 SHA 的全部 `.cpp/.cxx/.cc/.h/.hpp`，以及 cockpit 路徑的全部 Lua。此 GitHub 批次總共成功讀取 **966 個 C++ 檔、1,479 個 Lua 檔及 88 個 README，共 2,533 檔**，最後沒有未處理的讀取錯誤；這是含相同內容／衍生快照的檔案計數。先用 `flare|chaff|counter.?measure|snar|snares?` 獨立比對所有檔，再讀取命中本體、Lua 註解邊界、裝置登錄與動態方法呼叫，沒有使用要求 flare、chaff 同時存在的 AND 搜尋。

**本批未找到可驗證的「社群 EFM C++ 直接建立 flare／chaff」caller。** 這是以上公開快照、下列作者封裝與搜尋方法內的負面結果，不是對未公開 binary、未索引來源、私有模組或整個 DCS 引擎的不存在證明。沒有源碼的 DLL 不能推論「內部沒有 C++」；README 中的 native 字樣也不能取代實際 caller。C++ 派送、C++→Lua 轉接、Lua `drop_*`、只有宣告／enum／註解、缺來源分別判斷。

### 固定快照與逐倉庫結果

表中的「C++ 檔」包含標頭、SDK／測試／產生檔及重複目錄，只表示讀取覆蓋範圍。相同 family、fork 或相同 commit 不構成獨立成功證據。固定 tree 連結可查核各次盤點的實際檔案集合。

| Repository 固定快照 | C++ 檔 | 查核結果 |
| --- | ---: | --- |
| [willhuy/DCS-OH-6A @ 2971d54](https://github.com/willhuy/DCS-OH-6A/tree/2971d5494377b6ea16a5ee3bcb062fd1cee88c6b) | 0 | README 與圖片；非 EFM 原始碼。原 README 指向的 tobi-be 倉庫本輪回覆 404。 |
| [zbenesch/DCS-OH6A-Mod @ 0834035](https://github.com/zbenesch/DCS-OH6A-Mod/tree/0834035225c6e264c6e3a83c55029ebb35d8a86a) | 0 | 保存副本；**Lua 施放**，`Weapons.lua` 的 `update_flares()` 呼叫兩種 `drop_*`；EFM 只有 DLL。 |
| [andyn/dcs-ah6 @ daea3e8](https://github.com/andyn/dcs-ah6/tree/daea3e8bfe49b52eef058ccc01e22233b849f998) | 40 | 有 EFM C++；`drop_*` 只在 Lua `available functions` 多行註解，沒有施放 caller。 |
| [GoldJohnKing/AH-6J @ bcd655a](https://github.com/GoldJohnKing/AH-6J/tree/bcd655ab5a0e47d121691157ab6264e35a169090) | 20 | AH-6 同源比較；同樣只有 Lua 方法清單註解，沒有施放 caller，不算獨立成功案例。 |
| [qman787/CH-53E @ 56cf0a0](https://github.com/qman787/CH-53E/tree/56cf0a02bff745b2d26f82a18a91a1ba777211d1) | 26 | 有 EFM C++；Lua 的 `drop_*` 位於多行註解；3571／3581 只是該檔的命令常數。 |
| [Kinkkujuustovoileipa/uh-60l @ 2c31a8b](https://github.com/Kinkkujuustovoileipa/uh-60l/tree/2c31a8ba2870797051d475293d0e9a4dbb7463ce) | 0 | **Lua 施放**：`M130/device/M130.lua`；公開快照沒有 EFM C++。 |
| [RagnarDa/F-16Demo @ f36c66e](https://github.com/RagnarDa/F-16Demo/tree/f36c66e052ce756a58de6147133749f205052824) | 18 | 有 EFM C++；本輪來源搜尋未找到 CM caller。 |
| [gyrovague/F-16A-Demo-CDMW @ 69ab176](https://github.com/gyrovague/F-16A-Demo-CDMW/tree/69ab176cc5237fc17281cfd91184e2ecbc30de39) | 81 | F-16 Demo 衍生快照；有 EFM／cockpit C++，未找到 CM caller。 |
| [Running-Fish210/DCS-F22 @ 8484b44](https://github.com/Running-Fish210/DCS-F22/tree/8484b44cf8bc46540376435f0ba584f9f3e8bf44) | 9 | 與 grinnellidesigns 相同 HEAD；同一證據，沒有公開 C++ CM caller。 |
| [Ethanw05/DCS-F22 @ fec598b](https://github.com/Ethanw05/DCS-F22/tree/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6) | 154 | 有大量 native C++；**CM 實際仍在 Lua** `EW_System`／`EW_Dispenser` 呼叫 `drop_*`。 |
| [trgswe/F4F-Community @ 9efb0d3](https://github.com/trgswe/F4F-Community/tree/9efb0d3abdde2910b45c3941f74f334d0a8632ae) | 75 | **F4F Wildcat，並非 F-4 Phantom**；C++ 命中只有 `DEVICES_COUNTERMEASURES` enum。 |
| [PeeJott/F-4BC-EFM-Release](https://github.com/PeeJott/F-4BC-EFM-Release) | 0 | 空 Git repository，HEAD／tree 回覆 409；releases 清單亦空。另循作者分享取得 F4 封裝 39 個 C++，見後表。 |
| [PeeJott/VSN_F104C @ dbc32a5](https://github.com/PeeJott/VSN_F104C/tree/dbc32a5d48a88b8d744350ca51a1c1ed34b60aee) | 0 | GitHub 只有 README／Test.lua；已循 README 的 Filen 連結取得作者 F104 封裝並查核 38 個 C++，見後表。 |
| [Zaretto/acEFM @ 00ea144](https://github.com/Zaretto/acEFM/tree/00ea144c5c38b3692156eb4a01dbe3658ad2d4a8) | 43 | JSBSim EFM；C++ 命中只有 `iCommands.h` 的 enum／名稱查表，沒有施放實作。 |
| [Eurofighter-Mod-for-DCS/Eurofighter_EFM @ 6f9ef04](https://github.com/Eurofighter-Mod-for-DCS/Eurofighter_EFM/tree/6f9ef04f4066703668401e479fc769f32b8857e0) | 41 | 有 EFM C++；本輪來源搜尋未找到 CM caller。 |
| [Dillen198/PC9A_EFM @ d24e154](https://github.com/Dillen198/PC9A_EFM/tree/d24e1541c689344d5bc97d691ca2fe84203fd67e) | 0 | 公開 tree 只有 README 與 Visual Studio cache／資料庫，沒有 C++ source。 |
| [WatchBambi/DCS-F-117-EFM @ 92f3bc9](https://github.com/WatchBambi/DCS-F-117-EFM/tree/92f3bc9b6087c519150ae7745c5eea3e7ea5f57a) | 28 | 有 EFM C++；本輪來源搜尋未找到 CM caller。 |
| [halfmanbear/HMB-DCS-F-117A-EFM @ d742224](https://github.com/halfmanbear/HMB-DCS-F-117A-EFM/tree/d742224cc59bf92ad364c238ea46782379fe9c82) | 75 | 有 EFM／native interop C++，未找到 CM caller；Lua CM 命中只切換顯示／安全狀態參數。 |
| [IGServal/DCS-Basic-EFM-Template @ eba9d76](https://github.com/IGServal/DCS-Basic-EFM-Template/tree/eba9d764d278839f92cbdb0306c2a1f5fc2d1a73) | 16 | EFM 模板；沒有 CM caller，不是模組成功案例。 |
| [porkposh/dcs-c130 @ e22286d](https://github.com/porkposh/dcs-c130/tree/e22286ddaeb3b6e4c505c43bf5c0bbfdeda927ea) | 0 | README 與 Blender 模型；沒有可讀 C++／cockpit 來源，不代表 Hercules 的完整實作。 |
| [Summit60/DCS-Hercules-TheWay-patch-v1.01 @ 4da8467](https://github.com/Summit60/DCS-Hercules-TheWay-patch-v1.01/tree/4da846705225576b1bab8931a2eda8ea2080948b) | 0 | 僅 TheWay 整合修補的四個 cockpit Lua；沒有完整 Hercules EFM source／CM sink。 |
| [grinnellidesigns/f-22a @ 8484b44](https://github.com/grinnellidesigns/f-22a/tree/8484b44cf8bc46540376435f0ba584f9f3e8bf44) | 9 | F-22 原始倉庫快照；公開 C++ 無 CM caller；entry 使用 FC4 `MAC_flyable`，引擎接收端未公開。 |
| [Na1veNoob/DCS_A-6E_Intruder_FlightModel @ a9f5a4b](https://github.com/Na1veNoob/DCS_A-6E_Intruder_FlightModel/tree/a9f5a4b12d1b3ceea106871153983a075293bf4e) | 21 | 有 EFM C++；本輪來源搜尋未找到 CM caller。 |
| [S4ndman82/F16DCS @ dfaf07c](https://github.com/S4ndman82/F16DCS/tree/dfaf07ca6d9c4d71eeafc07d9dd114408e0abdc1) | 95 | F-16 Demo 衍生快照；有 C++，未找到 CM caller，不當成新的獨立 API 成功例。 |
| [paweljarosz82/opsdcs @ f684cd0](https://github.com/paweljarosz82/opsdcs/tree/f684cd065f9d2c75689403615b133a34331aecee) | 12 | 混合工具／EFM 範例；已查公開 C++，沒有 CM caller。 |
| [SNOSims/ME-163 @ b506842](https://github.com/SNOSims/ME-163/tree/b506842d4448d6afe2134f8daf619bb92af4abec) | 8 | 有 EFM C++；Lua 只見 `FlareGun` 訊號槍音效，不是反制物施放證據。 |
| [Apkallu-Industries/MQ-9-Reaper @ 2689c1b](https://github.com/Apkallu-Industries/MQ-9-Reaper/tree/2689c1b15224de978347ce87ddad2393a31a6ef2) | 3 | 有 EFM／sample C++；本輪來源搜尋未找到 CM caller。 |
| [Kozyma/T-50B @ 37d950e](https://github.com/Kozyma/T-50B/tree/37d950e68eab79900f4094dfe631ca9290cb0aea) | 10 | 有 EFM C++；本輪來源搜尋未找到 CM caller。 |
| [ArkYk/DCS_API @ 1ce52d1](https://github.com/ArkYk/DCS_API/tree/1ce52d1aa171e205c372493895ad9802b3e26b3a) | 11 | API／EFM template 保存；沒有 CM caller，不是模組成功案例。 |
| [Lokiv2/DCS-OpenVSP-Bridge @ bdc117d](https://github.com/Lokiv2/DCS-OpenVSP-Bridge/tree/bdc117dff7afed1e63a39e5c9f2d24bb7058f898) | 2 | EFM／建模橋接工具；沒有 CM caller。 |
| [madwax/DCS_EFM_PROXY @ 4b09834](https://github.com/madwax/DCS_EFM_PROXY/tree/4b09834c56a63915b2b7732e7b0544d31e130f0f) | 13 | EFM proxy；沒有 CM caller，不是反制物 API 範例。 |
| [bhoffpauir/DCS-EFM-ImGui @ a789fb0](https://github.com/bhoffpauir/DCS-EFM-ImGui/tree/a789fb06214f08cfa1cc7bc121a5db714ffc3a21) | 4 | EFM UI 工具；沒有 CM caller。 |
| [CrudeCoder1/DCS-FPV-Drone @ 77cef74](https://github.com/CrudeCoder1/DCS-FPV-Drone/tree/77cef74874ad942fa897bb9fc8c55210fc367762) | 0 | 公開 cockpit Lua、沒有 C++ source；`drop_*` 只在方法清單註解。 |
| [tspindler-cms/Helicopter-EFM-Demo-CH46 @ 35a4e50](https://github.com/tspindler-cms/Helicopter-EFM-Demo-CH46/tree/35a4e50cf80359db9b9a536c77381a93aac52427) | 28 | Helicopter-EFM-Demo 的 fork；有 C++，CM 方法只在 Lua 註解，不是獨立成功例。 |
| [BAAS-Dynamic/DCS-SK60-Mod @ 1a22bac](https://github.com/BAAS-Dynamic/DCS-SK60-Mod/tree/1a22bac68c7cfef4b5098e3f6a7ea39f259756ae) | 0 | 公開 Lua 與資產、沒有 C++ source；`weapon_system.lua` 兩個 CM 常數皆為註解。 |
| [PucaraPastrana/PucaraDCS @ da3891e](https://github.com/PucaraPastrana/PucaraDCS/tree/da3891e7aff018fcf906545adde84cd30ec5315d) | 0 | README-only；README 自述 SFM 且 EFM 為未來工作，沒有 C++ 可查。 |
| [IGServal/-DEPRECATED--Basic-custom-flight-model-for-DCS-World @ 91a5939](https://github.com/IGServal/-DEPRECATED--Basic-custom-flight-model-for-DCS-World/tree/91a5939f4b0d91d865a5c6994387683b384abccc) | 29 | 舊 EFM 模板；沒有 CM caller，不當成模組成功例。 |
| [ipr/F-16Demo @ 090f249](https://github.com/ipr/F-16Demo/tree/090f249d25c0fa63ca7f8e790e03f533bc1964e6) | 95 | F-16 Demo 同源比較；有 C++，未找到 CM caller。 |
| [SpinosSimulations/DCS-F-15EX-Eagle-II-Standalone-Mod- @ 5ca40e6](https://github.com/SpinosSimulations/DCS-F-15EX-Eagle-II-Standalone-Mod-/tree/5ca40e6cb8f3f490d29d9fbd6c8745712fd978d5) | 0 | **Lua 施放**：`WEAPONS.lua` 動態取得 `dev[drop_*]` 後 `pcall`；沒有公開 C++ source。 |
| [65thhaack/Modern-F15E-Mod @ bda92c2](https://github.com/65thhaack/Modern-F15E-Mod/tree/bda92c21285e092414ed5e5f7ca593014d719bdb) | 0 | README-only 的既有 F-15E 擴充／installer，沒有可讀 C++ CM source。 |

### 已找到的真實施放鏈：皆由 Lua 呼叫原生 weapon device

- **OH-6A 保存副本**：`avSimpleWeaponSystem` 登錄於 [device_init.lua 第 8 行](https://github.com/zbenesch/DCS-OH6A-Mod/blob/0834035225c6e264c6e3a83c55029ebb35d8a86a/Cockpit/Scripts/device_init.lua#L8)；`update_flares()` 檢查裝備、arm、輸入、電源與庫存後，分別呼叫 `dev:drop_flare(1,0)` 及 `dev:drop_chaff(1,0)`。[Weapons.lua 第 267–299 行](https://github.com/zbenesch/DCS-OH6A-Mod/blob/0834035225c6e264c6e3a83c55029ebb35d8a86a/Cockpit/Scripts/Systems/Weapons.lua#L267-L299) 該保存副本的公開 tree 只有 EFM DLL，不能檢查其 C++；原始作者倉庫本輪回覆 404，保存副本不代表現行官方版本。
- **UH-60L**：M130 登錄為 `avSimpleWeaponSystem`。[device_init.lua 第 26 行](https://github.com/Kinkkujuustovoileipa/uh-60l/blob/2c31a8ba2870797051d475293d0e9a4dbb7463ce/Cockpit/Scripts/device_init.lua#L26) `SetCommand` 的 chaff 分支呼叫 `drop_chaff(1,0)`，flare 分支呼叫 `drop_flare(1,1)`、`drop_flare(1,2)`，並有 power／arm／庫存條件。[M130.lua 第 39–68 行](https://github.com/Kinkkujuustovoileipa/uh-60l/blob/2c31a8ba2870797051d475293d0e9a4dbb7463ce/Cockpit/Scripts/M130/device/M130.lua#L39-L68) 這個 sink 在 `M130/device`，只搜尋 `Systems` 目錄會漏掉。
- **Ethanw05 的 F-22 分支**：`WEAPON_NATIVE` 為 `avSimpleWeaponSystem`，`EW_SYSTEM` 為 Lua device。[device_init.lua 第 54–59 行](https://github.com/Ethanw05/DCS-F22/blob/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6/Cockpit/Scripts/device_init.lua#L54-L59) 單發路徑由 `SetCommand` 呼叫 `single()`，選擇 `drop_flare`／`drop_chaff` 字串，再 `pcall(device[method],device,1)`；程式化路徑亦由 Lua helper 呼叫相同方法。[EW_System.lua 第 124–155 行](https://github.com/Ethanw05/DCS-F22/blob/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6/Cockpit/Scripts/Systems/EW_System.lua#L124-L155)、[EW_Dispenser.lua 第 72–86 行](https://github.com/Ethanw05/DCS-F22/blob/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6/Cockpit/Scripts/Systems/EW_Dispenser.lua#L72-L86) 因此其 README 宣稱 native dispensing，並不構成直接 C++ 施放例證。
- **F-15EX standalone**：weapon device 登錄為 `avSimpleWeaponSystem`。[device_init.lua 第 19 行](https://github.com/SpinosSimulations/DCS-F-15EX-Eagle-II-Standalone-Mod-/blob/5ca40e6cb8f3f490d29d9fbd6c8745712fd978d5/Mods/aircraft/F-15EX%20V3/Cockpit/Scripts/device_init.lua#L19) `S7.CM` 保存兩種 `drop_*` 方法字串及 dispenser 編號，`S7.drop()` 取出 `dev[c.fn]`，以 `pcall(f,dev,1,d)` 呼叫，之後重讀庫存。[WEAPONS.lua 第 2043–2066 行](https://github.com/SpinosSimulations/DCS-F-15EX-Eagle-II-Standalone-Mod-/blob/5ca40e6cb8f3f490d29d9fbd6c8745712fd978d5/Mods/aircraft/F-15EX%20V3/Cockpit/Scripts/Systems/WEAPONS.lua#L2043-L2066) 這是動態 Lua 呼叫，不是 C++ caller。

以上只證明固定原始碼中的執行路徑；本研究沒有自行啟動這些模組做 DCS 庫存／視覺施放測試。

### 容易誤判成成功例證的命中

AH-6 的 `drop_flare/drop_chaff` 位於 `--[[ available functions ... --]]`，不是函式呼叫。[andyn 的 Weapons.lua 第 162–185 行](https://github.com/andyn/dcs-ah6/blob/daea3e8bfe49b52eef058ccc01e22233b849f998/AH-6J/Cockpit/Scripts/Systems/Weapons.lua#L162-L185) CH-53E 亦相同。[weapon_system.lua 第 220–243 行](https://github.com/qman787/CH-53E/blob/56cf0a02bff745b2d26f82a18a91a1ba777211d1/Cockpit/Scripts/Systems/weapon_system.lua#L220-L243) CH-46 fork／FPV Drone 亦只有這種方法清單。這些命中不支持 C++→Lua 轉接，更不支持直接 C++ 施放。

SK60 的 357／358 兩行都以 `--` 註解，且本輪完整 cockpit Lua 沒找到 `drop_*` caller。[weapon_system.lua 第 17–27 行](https://github.com/BAAS-Dynamic/DCS-SK60-Mod/blob/1a22bac68c7cfef4b5098e3f6a7ea39f259756ae/Cockpit/Scripts/Systems/weapon_system.lua#L17-L27) F4F C++ 的 `DEVICES_COUNTERMEASURES` 只是 enum；acEFM 的 flare／chaff 只是 enum 與名稱查表。[F4F Devices.h 第 18 行](https://github.com/trgswe/F4F-Community/blob/9efb0d3abdde2910b45c3941f74f334d0a8632ae/F4F-4-C/ExternalFM/FM/Devices.h#L18)、[acEFM iCommands.h 第 378–379 行](https://github.com/Zaretto/acEFM/blob/00ea144c5c38b3692156eb4a01dbe3658ad2d4a8/flyt-EFM-dcsJSBSim/iCommands.h#L378-L379) F-117 HMB 的 CM 命中只切換 `FLAR_TOGGLE/CHAF_TOGGLE` 顯示／安全狀態，沒有建立反制物。[Displays.lua 第 657–669 行](https://github.com/halfmanbear/HMB-DCS-F-117A-EFM/blob/d742224cc59bf92ad364c238ea46782379fe9c82/Cockpit/Scripts/Systems/Displays.lua#L657-L669)

grinnellidesigns 的 F-22 與 Running-Fish210 快照 HEAD 完全相同，必須合併看待；其 entry 呼叫 FC4 `MAC_flyable`。有內建 avionics 後端的模組可以把命令交給未公開引擎接收端；在公開 C++／Lua 沒找到 `drop_*`，不能推成「沒有接收者」或「357／358 一律無效」。[固定 entry.lua](https://github.com/grinnellidesigns/f-22a/blob/8484b44cf8bc46540376435f0ba584f9f3e8bf44/entry.lua)

### 主線專項補查與來源缺口

| 專項固定來源 | C++ 檔 | 本輪可支持的結論 |
| --- | ---: | --- |
| [Mile-High-Digital-Works/f23b-public @ 950a542](https://github.com/Mile-High-Digital-Works/f23b-public/tree/950a542e12b1dcb9587d23fd3dd4849fbf78a47d) | 54 | 原生 C++ 的 flare 命中處理 seeker 排除 decoy，沒有建立／施放 flare。[SeekerDecoyHook.h 第 7–23 行](https://github.com/Mile-High-Digital-Works/f23b-public/blob/950a542e12b1dcb9587d23fd3dd4849fbf78a47d/native/radar-helper/SeekerDecoyHook.h#L7-L23) |
| [whisky-actual/Community-JAS-39-C @ 662166e](https://github.com/whisky-actual/Community-JAS-39-C/tree/662166e106e82a0434445c41b7835ddbe092ca3d) | 0 | `Countermeasures.lua` 訂閱自訂輸入後派送 357／358，沒有 `drop_*`；這是 **Lua 派送**，不是直接 C++ caller。[第 14–35 行](https://github.com/whisky-actual/Community-JAS-39-C/blob/662166e106e82a0434445c41b7835ddbe092ca3d/Mods/aircraft/JAS39/Cockpit/Scripts/Systems/Countermeasures.lua#L14-L35) 手動減顯示參數也不是引擎庫存扣減證據。 |
| [歷史 NH90 SDK sample @ a97da9d](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/samplers/WeaponAndSensorsSystems/avSimpleWeaponSystem.cpp#L326-L350) | 非上述模組計數 | 確有 C++ `Create_Chaff_Human/Create_Flare_Human` 呼叫，但位於 Lua binding／SDK 範例，**不是社群 EFM 可直接使用且已成功的公開實例**；context、human 與現行 ABI 仍需分別確認。 |

F-104 的 README 明寫 C++ EFM source 另存 Filen；沿著固定 [README 第 69–71 行](https://github.com/PeeJott/VSN_F104C/blob/dbc32a5d48a88b8d744350ca51a1c1ed34b60aee/README.md#L69-L71) 的作者分享連結，已另外取得並核驗下列七份 EFM 原始碼封裝。GitHub tree 的 C++ 數量仍為 0；外站封裝另計，兩者不混用。

另以 F-35／F35／F35A、Mirage／Mirage2000、T-45／T45／Goshawk、MB339／MB-339 等 repository query 擴充查找，這些查詢本身未定位可讀的對應模組 C++ source；F-35 最後由上述作者分享取得 EFM source，沒有停在 GitHub 搜尋負面結果。Mirage、T-45、MB-339 等仍未判定，搜尋結果多為硬體 profile、音效、教學或下載管理工具。Hercules 本輪只讀到 TheWay patch 與另一 C-130 模型倉庫，沒有把局部修補當成 Hercules 完整原始碼。此處保留缺口，沒有聲稱已覆蓋所有商用／私有模組、所有歷史 fork 或所有網站下載包。


### 跨倉庫符號查找的涵蓋與限制

除逐倉庫查核外，主線以 `ed_fm_set_command` 的 56 個檔案／25 個 repository、`ed_fm_simulate` 的 52 個檔案／26 個 repository 作為 EFM 專案發現入口；命中數不是獨立模組或成功案例數。精確 `Create_Flare_Human/Create_Chaff_Human` 搜尋只讀到 NH90 保存的歷史 SDK 標頭與 sample。限定 C++ 的 `drop_flare` 有 17 個命中、`drop_chaff` 有 5 個命中，逐一辨識後，DCS 相關者仍是 NH90；其餘屬其他遊戲，未拿來支持 DCS API 結論。[NH90 的實際 C++ sink](https://github.com/Mysteryfx/NH90/blob/a97da9d49b4d520ad169845603fd47c5ed870797/SDK_Perso/samplers/WeaponAndSensorsSystems/avSimpleWeaponSystem.cpp#L326-L350)

較寬的 `Create_Flare` C++ 查找共 662 個命中，本輪只讀前 100，且充斥非 DCS 專案，因此**不把它當完整負面搜尋證據**。上述數字僅為本輪 GitHub 搜尋回覆，不保證涵蓋未索引來源與非預設分支。

### 作者外站來源：補齊 VSN F-35、F-104、F-4 等七份 EFM

從作者 README 指向的[公開 Filen 分享](https://drive.filen.io/f/8ba4c168-a6e3-4863-939f-5fa0bba91d7c#lWYJm8M7n0L3ye3yTkWc0VxPuyiT0dab)，透過服務的公開分享 API 正常列出／下載 `VSN_EFM_GPL3` 內七份檔案，未登入或繞過權限。主線獨立核對下載長度與 SHA-256，研究端再核對 SHA-256、完整列出 archive entry，僅解出原始碼／README／license 後逐檔搜尋。**七份共 246 個 C++ 檔、0 個 Lua；相同 CM regex 全部 0 命中，`Create_.*Human/drop_flare/drop_chaff/ed_cockpit_dispatch_action` 亦全為 0。** 因此外站結果仍未提供直接 C++ 施放 caller。

| 作者封裝 | C++ 檔 | SHA-256 固定識別 |
| --- | ---: | --- |
| `VSN_F35.7z` | 37 | `9d13f0315cade1592337ec3249ac837b95d4d91a10842217eef0463ce0bc54e6` |
| `VSN_F104.7z` | 38 | `c011c9710e78aaf5d8228c3c75fa76bf1bfd33e85f99d524997d5952a0d1e395` |
| `VSN_UFO.7z` | 33 | `188ed18d9c38f9e5c68a8f67dad23878537a0c25028abe0726a5899eed8d5de2` |
| `VSN_F9F.7z` | 33 | `dfae9cf45d18f92b2a9776e4c5294047fc2f56a07dc155bbeb000db9b2010ced` |
| `VSN_F100.7z` | 33 | `2d1fd613f823a89cfe6408bcc3f67d6321b4241714ef29f8b3006e5fe3ca3d67` |
| `VSN_F4.7z` | 39 | `f28140ea91101f9cb0ef76c649db038d73879f5fc48d446895d23ed626c4b458` |
| `G91Y_EFM.zip` | 33 | `d2d979f17d651e9bdb8698ac4db7070b50326344ef617d651ab6bfce9917e4b2` |

可用 archive hash 加以下實際 EFM 入口路徑重現查核；每包亦包含 airframe／flight-model／input／Cockpit parameter API 等 C++ source：

- `VSN_F35.7z`：`VSN_F35/ExtFM/ED_FM_Template/ED_FM_Template.cpp`。
- `VSN_F104.7z`：`VSN_F104/ExternalFM/ED_FM_Template/ED_FM_Template.cpp`。
- `VSN_UFO.7z`：`VSN_UFO/EFM/ED_FM_Template/ED_FM_Template.cpp`。
- `VSN_F9F.7z`：`VSN_F9F/ExtFM/ED_FM_Template/ED_FM_Template.cpp`。
- `VSN_F100.7z`：`VSN_F100/ExtFM/ED_FM_Template/ED_FM_Template.cpp`。
- `VSN_F4.7z`：`VSN_F4/ExtFM/ED_FM_Template/ED_FM_Template.cpp`。
- `G91Y_EFM.zip`：`ExternalFM/ED_FM_Template/ED_FM_Template.cpp`。

這些是作者分享的 **EFM 封裝**，尤其 F-35 並非整個模組的完整 avionics 原始碼。零命中只支持「這份 EFM source 中沒有找到 CM caller」，不能推出該模組沒有反制物、隱藏引擎後端沒有 C++，或目前發布 binary 與封裝完全一致。封裝含歷史建置檔；本研究不把它視為 DCS 2.9.31 MT 的現行支援契約，也沒有執行其中的 DLL。

本輪唯一查核到會真正建立反制物的 C++ source，仍是前述歷史 SDK sample 中的 `get_context()->human->Create_*_Human(count,group)`。它證明存在這種 C++ sink 及 context／human 依賴；沒有提供任意 EFM callback 取得合法裝置 context 的現行契約。**本研究沒有證明純 C++ 施放不可能；只是尚未找到可直接照搬、可驗證的社群 EFM 成功案例。**
