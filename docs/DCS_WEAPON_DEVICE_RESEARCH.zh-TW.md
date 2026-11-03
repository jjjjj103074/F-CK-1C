# DCS 武器裝置與機砲射擊介面研究

查核日期：2026-11-03。僅研究，未修改執行程式碼。A-4E 固定版本 `34c1ef439581234ee24d4bde2e13a54999ebaa5a`；F-22A 固定版本 `8484b44cf8bc46540376435f0ba584f9f3e8bf44`。本文件直接核對上述版本的公開原始碼；不聲稱已在 DCS 執行驗證。

## 本專案目前的分工

- `Cockpit/Scripts/device_init.lua:31-38` 分別建立 CMS 的 `avLuaDevice` 與武器介面的 `avSimpleWeaponSystem`。
- `Core/Systems/MissionComputer/Components/FireControl/Internal/WeaponOperation.cpp:38,63` 產生 `GunOn`／`GunOff` 語意指令；`Cockpit/Scripts/Systems/cms_system.lua:220-245` 將它們映射為 DCS 的 `iCommandPlaneFire`／`iCommandPlaneFireOff`，再交給 `dispatch_action(nil, command)`。
- `F-CK-1C.lua:468-489` 以 `Guns` 與 `gun_mount("M_61", ...)` 配置機砲，供彈初始設定為 523 發。

因此，本專案有使用 `avSimpleWeaponSystem` 作為座艙武器裝置；開火控制的程式入口則是 DCS 命令。命令入口與裝置配置是兩個不同層次，單憑 Lua 呼叫形式無法判定 ED 原生程式內的命令分派。

## A-4E：avSimpleWeaponSystem 搭配 DCS 射擊命令

固定版本：`34c1ef439581234ee24d4bde2e13a54999ebaa5a`。

裝置註冊為 `avSimpleWeaponSystem`。Lua 檢查供電、上膛、起落架及備便狀態，再透過 `set_gun_firing()` 派送 `Fire`／`FireOff`；內置砲與掛點槍莢分別處理，槍莢使用 `launch_station()`。這與本專案的原生命令入口相近。來源：[註冊](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/device_init.lua#L54)、[機砲許可與啟停](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/weapon_system.lua#L686-L729)、[槍莢](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/weapon_system.lua#L907-L915)。

`gun_charges` 記錄上膛操作次數；掛點 `count` 的讀取也不能作為已取得內置砲餘彈的證據。C++ 的 `getGunShells` 綁定仍被註解，符號字串留空，並非可用 getter。來源：[上膛操作](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/weapon_system.lua#L736-L763)、[C++ 綁定](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/ExternalFM/include/Cockpit/ccParametersAPI.h#L172)。

## SK60：同樣使用 avSimpleWeaponSystem，以掛點介面控制槍莢

固定版本：`ce578058da225ae8cf00b1a2a8cdd1dee48edac5`。

`device_init.lua:22` 註冊 `avSimpleWeaponSystem`。`weapon_system.lua:151-169` 讀取各掛點的 `CLSID` 與 `count`，區分火箭、槍莢等掛載；`412-427` 在扳機與許可條件成立時，對槍莢掛點呼叫 `launch_station()`。這是外掛槍莢案例，其射擊入口與本專案內置機砲不同。來源：[裝置](https://github.com/BAAS-Dynamic/DCS-SK60-Mod/blob/ce578058da225ae8cf00b1a2a8cdd1dee48edac5/Cockpit/Scripts/device_init.lua#L22)、[掛點觀測](https://github.com/BAAS-Dynamic/DCS-SK60-Mod/blob/ce578058da225ae8cf00b1a2a8cdd1dee48edac5/Cockpit/Scripts/Systems/weapon_system.lua#L151-L169)、[槍莢射擊](https://github.com/BAAS-Dynamic/DCS-SK60-Mod/blob/ce578058da225ae8cf00b1a2a8cdd1dee48edac5/Cockpit/Scripts/Systems/weapon_system.lua#L412-L427)。

## A-29B：使用另一個裝置 avSimplestWeaponSystem

固定版本：`1f632f004e28ef13a5f3404528de4c3fc982a0e2`。

- `device_init.lua:37` 註冊的是 `avSimplestWeaponSystem`，名稱與 `avSimpleWeaponSystem` 不同。`entry.lua:15-18` 載入 `avSimplest` binary。此處只能證實模組所選的裝置名稱及 DLL 依賴，沒有據此確認兩個 C++ 類別的繼承關係。來源：[裝置](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/device_init.lua#L37)、[載入 binary](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/entry.lua#L15-L18)。
- 機砲扳機同樣派送 DCS `Fire`／`FireOff` 命令；左右餘彈各自從 250 開始，依射擊時間及設定射速扣除。這個公開 Lua 實作採用餘彈估算，換裝置本身不構成已取得原生彈量的證據。來源：[射擊](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L1155-L1161)、[初始彈量](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L53-L54)、[估算扣除](https://github.com/luizrenault/a-29b-community/blob/1f632f004e28ef13a5f3404528de4c3fc982a0e2/Cockpit/Scripts/Systems/weapon_system.lua#L894-L905)。

## F-22A：借用 F-15C 航電的配置

固定版本：`8484b44cf8bc46540376435f0ba584f9f3e8bf44`。

作者說明模組依賴 F-15C 航電；自訂 `WEAPON_SYSTEM` 註冊為 `avLuaDevice`，武器腳本仍派送 `Fire`／`FireOff`。該版載入點使用 `MAC_flyable(...)`，註解寫 FC4，README 則使用 F-15C／FC3 的依賴表述。因此這是借用既有航電的實例，無法單由其 Lua 推定 ED 內部的原生類別或命令路由。來源：[作者說明](https://github.com/grinnellidesigns/f-22a/blob/8484b44cf8bc46540376435f0ba584f9f3e8bf44/README.md#L1-L6)、[裝置](https://github.com/grinnellidesigns/f-22a/blob/8484b44cf8bc46540376435f0ba584f9f3e8bf44/Cockpit/Scripts/device_init.lua#L33-L40)、[載入點](https://github.com/grinnellidesigns/f-22a/blob/8484b44cf8bc46540376435f0ba584f9f3e8bf44/entry.lua#L67-L76)、[停止射擊](https://github.com/grinnellidesigns/f-22a/blob/8484b44cf8bc46540376435f0ba584f9f3e8bf44/Cockpit/Scripts/Systems/Weapon_System.lua#L209-L218)、[開火](https://github.com/grinnellidesigns/f-22a/blob/8484b44cf8bc46540376435f0ba584f9f3e8bf44/Cockpit/Scripts/Systems/Weapon_System.lua#L447-L448)。

## 正式模組：Ka-50 的機型專用武器介面

ED Team 的 Alex Okean 在 2009-03-23 的資料匯出說明中，直接列出 `GetDevice(devices.WEAP_INTERFACE)`、`get_selected_gun_ammo_count()` 與掛點查詢方法。這證實 Ka-50 武器裝置曾公開機型專用的餘彈讀取介面；原文上下文是 Export，不是所有座艙腳本或通用武器裝置的 API 保證。來源：[ED 開發者原文](https://forum.dcs.world/topic/33182-informacija-pro-patch-101/page/8/)。

開源 DCS-ExportScripts 的固定版本 `0df634fb4edb72450e67f2a6502ead38e54f218b` 也以 `GetDevice(12)` 取得 Ka-50 裝置，再呼叫該 getter 並換算顯示單位。這是公開呼叫端，與官方裝置內部的原始碼分開看待。來源：[Ka-50 呼叫端](https://github.com/s-d-a/DCS-ExportScripts/blob/0df634fb4edb72450e67f2a6502ead38e54f218b/Scripts/DCS-ExportScript/ExportsModules/Ka-50.lua#L671-L696)。

正式合作的工具存取範圍亦有差異：BIGNEWY 在 2026-01-05 明確表示 SDK 僅向正式第三方合作夥伴提供。這能說明工具授權差異，無法據此重建每個模組的內部餘彈同步方式。來源：[ED 的 SDK 說明](https://forum.dcs.world/topic/383843-community-modding-in-dcs/)。

對 A-4E 控制流程的解讀：`set_gun_firing()` 在派送 Fire／FireOff 時更新 `gun_firing`，因此這個旗標記錄 Lua 下達的射擊狀態；它不是回讀已射出多少砲彈。機砲裝填配置與 Lua 射擊許可分別存在，讀取原生餘彈是另外一項介面需求。來源：[射擊狀態更新](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/Cockpit/Scripts/Systems/weapon_system.lua#L711-L729)、[原生供彈配置](https://github.com/Community-A-4E/community-a4e-c/blob/34c1ef439581234ee24d4bde2e13a54999ebaa5a/A-4E-C/A-4E-C.lua#L19-L27)。

## 回答邊界

不應只刪掉 F-CK-1C 的 `avSimpleWeaponSystem`，留下 `dispatch_action(84/85)` 就宣稱替代完成。命令派送只顯示輸入方式；新的原生航電／武器裝置仍需有可驗證的裝填、射擊、扣彈、補彈與掛點契約。SDK 自訂原生武器裝置在本子研究沒有找到可公開核對的類別實作或官方 API 契約，故不給出聲稱可直接使用的 C++ 替代函式名稱。

## 座艙內置機砲餘彈：論壇與公開原生橋接的新查核

補充日期：2026-11-03。目標是讓自訂飛機的座艙／MC 從原生存量取得餘彈，不把已驗證的 Export 路徑當成新的座艙直接讀取方案。

### 最值得繼續查核的實作：Ethanw05/DCS-F22

固定版本：`fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6`。這是另一個 F-22 航電分支，與前文引用的 Grinnelli 配置分開核對。

`EFM/source/NativeSidewinder.cpp` 的 `publish_gun_ammunition()` 取得原生 payload，再查詢其中 gun mount 的機砲存量，將結果寫成 `F22_GUN_AMMO_COUNT`／`F22_GUN_AMMO_VALID` 座艙參數。它的資料來源是原生 gun 物件，沒有按扳機時間扣算。呼叫入口經由 `CockpitBase.dll` 的 `cockpit::c_payload()` 匯出符號取得，後續物件配置及虛擬函式位置依賴版本。來源：[原生綁定](https://github.com/Ethanw05/DCS-F22/blob/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6/EFM/source/NativeSidewinder.cpp#L163-L198)、[讀取及發布](https://github.com/Ethanw05/DCS-F22/blob/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6/EFM/source/NativeSidewinder.cpp#L751-L779)。

作者在設計文件中記錄 DCS **2.9.30.28738、Windows x64** 的射擊結果：機砲存量 **480 → 423**。這是作者的實機報告；本輪取得了對應公開程式，但未獨立重現，也未取得原始飛行 log。來源：[版本與測試結果](https://github.com/Ethanw05/DCS-F22/blob/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6/docs/STANDALONE_DESIGN.md#L3-L28)。

2026-10-29 的 `af79e0296d0f72d48658749b630495111bf423cc` 提交恢復 **2.9.31** 的 gun／payload 等綁定，但提交說明將下一次飛行效果列為尚未驗證。因此，舊版實機結果與新版 profile 的驗證狀態須分開記錄。`NativeSidewinderHost.h` 以 PE timestamp、image size 與局部程式指紋篩選版本；它不是 ED 公開穩定的 SDK 契約。來源：[2.9.31 更新說明](https://github.com/Ethanw05/DCS-F22/commit/af79e0296d0f72d48658749b630495111bf423cc)、[版本 profile](https://github.com/Ethanw05/DCS-F22/blob/fec598b7ad75714ed9aae7ee87be81eaf1c5f8a6/EFM/source/NativeSidewinderHost.h#L8-L37)。

這個案例支持「由模組內 C++ 橋接原生機砲存量，再提供給 MC／座艙」的研究方向。它不是 `avSimpleWeaponSystem` 新增的 Lua getter；移植前需對照本機版本的匯出符號與物件呼叫契約。整個來源檔還包含尋標器等功能，研究機砲存量時應只追蹤相關讀取路徑。

### 論壇查核與已排除的誤解

- gyrovague（Heatblur／3rd Party Developers）在 2017-01-31 提出從 `CockpitBase.dll` 字串、匯出符號、其他模組 Lua，以及 `_G`／裝置 metatable 探索介面；作者明確提醒可能出現誤導線索，仍需驗證呼叫參數。來源：[原作者討論](https://forum.dcs.world/topic/153703-finding-new-functionality-in-lua-aircraft-mods/)。
- ED Alex Okean 的 2012 範例中，`WS_GUN_PIPER_AVAILABLE`／`AZIMUTH`／`ELEVATION` 是瞄準點資料。`get_param_handle()` 在名稱不存在時會建立項目，所以猜測彈量參數名後讀到零，不能作為已找到引擎資料的證據。來源：[ED 範例](https://forum.dcs.world/topic/74145-dcs-mods-structure-how-to-create-your-plugin-from-scratch/)。
- 本輪沒有找到可核查的 `avSimpleWeaponSystem` 內置機砲純 Lua getter 成功案例，也未找到把 `get_station_info(-1)` 定義為內置機砲的第一手契約。這是查核結果的界線，不等於原生存量無法讀取。
- Ka-50 的機型專用 getter 已有前節的 ED 原文；它能作介面設計參考，無法直接證明本機通用裝置提供同一方法。A-4E 的空白 `getGunShells` 綁定則仍維持未完成狀態。

### F-CK-1C 已持有的證據與最小補件

使用者提供的 `FCK1C-AMMO-RESULTS-20261102-224732-258.zip` 中，`DCS-Logs\\dcs.log` 記錄：

- `DCS/2.9.31.29434`，Windows x64、MT。
- 執行程式：`D:\SteamLibrary\steamapps\common\DCSWorld\bin\DCS.exe`。
- 當次載入：`D:/SteamLibrary/steamapps/common/DCSWorld/bin/CockpitBase.dll`。

第一輪只需要補充該次安裝版本的 **CockpitBase.dll**，即可先檢查 PE 資訊、SHA-256、匯出符號、`c_payload` 與相關虛擬函式路徑。現有專案 Lua、EFM 原始碼、DCS 版本與測試 log 已可交叉核對。若後續靜態追蹤發現 gun 的實作歸屬另一個 DLL，再依實際呼叫關係補件。

若使用者已有 IDA／Ghidra 文字輸出，最有用的是同版本的 `c_payload`、`avArcadeRadar::get_shells_count`、`avSimpleWeaponSystem` 的 Lua 方法註冊，以及 `get_station_info` 的 wrapper／實作；需要保留函式原型、caller／xref 與版本資訊。只有名字清單不足以核對物件與呼叫契約。

目前結論：**純 Lua 直接 getter 尚未取得成功證據；原生 C++ 讀取已有公開實作與作者回報的實機數值，值得優先比對。F-CK-1C 的相容性仍以自身 DLL 比對及後續實測為準。**


## 對本專案的建議（依上述證據的設計判斷）

保留 `avSimpleWeaponSystem` 作為 DCS 轉接層的一部分，讓 MC 保有模式、許可與射擊決策，座艙層將語意指令轉成原生命令。原生機砲餘彈的來源應另行評估；目前的射擊入口與已驗證的彈量讀取能力分別記錄，避免把更換裝置等同於取得餘彈介面。

本輪新增原生橋接證據後，優先對照使用者當版 `CockpitBase.dll` 的 payload／gun 讀取契約，再決定能否以模組內 C++ 資料來源接入既有 SM。這項研究先獨立於射擊控制與 SM 資料模型進行。
