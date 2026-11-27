# DCS 原生介面相容性核對

2026-11-27 核對使用者提供的兩份 x64 `CockpitBase.dll`。最新飛測紀錄顯示 DCS/2.9.32.30490；
實際選擇相容資料以 DLL 的 PE timestamp、image size 及各功能的函式指紋為準。
DLL 本身由 DCS 載入；本專案將核對結果保存在
`DcsBridge/Internal/Aircraft/AircraftBuildProfiles.h`。

## 失效原因與修正

`FCK1C-MC-RESULTS-20261127-211830-383.zip` 中，新版 DLL 被原生介面判為 `UnsupportedBuild`。
C 的要求已到達 MC，但尋標控制讀回持續不可用；機砲原生彈量也不可用。
反制使用同一建置識別條件，因此一併更新。

本次加入新版相容資料並保留原版資料，各功能依自己的指紋組核對後才取得物件、進行呼叫。
版本或指紋不符時仍回傳來源狀態。更新只涉及原生存取層，MC、SM、雷達與反制程序沿用現有職責。

## DLL 識別

| 項目 | 原版樣本 | 新版樣本 |
|---|---|---|
| PE timestamp | `0x6adbad09` | `0x6afec04f` |
| SizeOfImage | `0x68e000` | `0x68e000` |
| 檔案位元組 | 6,831,104 | 6,834,176 |
| SHA256 | `fb0a09be70627c7790dea1527cedfee43082bb8b112484d5c85be1d99d33de95` | `78768f59241deff86177b74a1c99658b99aa43592101d9ee657a94046e13f570` |

SHA256 識別本次分析的輸入檔案。執行時沿用 PE 識別及局部程式指紋核對，
個別功能的指紋錯誤只影響該功能。

## 程式指紋

下表使用匯出函式的簡稱；完整修飾名稱、長度與兩版預期值集中存放在相容設定。
FNV-1a 64-bit 依指定入口的連續位元組計算。

| 介面／入口 | 位元組 | 原版 RVA | 新版 RVA | 新版 FNV-1a |
|---|---:|---|---|---|
| IR `get_wide_device` | 32 | `0x2c9180` | `0x2c9180` | `41d2d25f18107729` |
| IR `update_ir_seeker` | 32 | `0x363210` | `0x362fc0` | `98f33ac9cf396e65` |
| IR `avSidewinderSeeker` 建構式 | 64 | `0x442510` | `0x442c20` | `1a2079ded3a1ce88` |
| IR `setConsentToTrack` | 26 | `0x363130` | `0x362ee0` | `0df74a7ec2826fc8` |
| IR `getStatusID` | 7 | `0x3618f0` | `0x3616a0` | `ec4c08ae667a8ea4` |
| IR Sidewinder `cage` | 5 | `0x440260` | `0x440970` | `7503bb3a0ff66589` |
| IR `uncage` | 34 | `0x41fea0` | `0x4205b0` | `c117d6212af01575` |
| IR `slaveToDirection` | 48 | `0x4413b0` | `0x441ac0` | `181082bbc1acedb8` |
| IR `toSlave` | 32 | `0x441490` | `0x441ba0` | `6b730569969a099e` |
| 反制 `c_payload` | 29 | `0x4f3c00` | `0x4f4490` | `46c1a7daa0c3383f` |
| 反制 `l_drop_chaff` | 202 | `0x361eb0` | `0x361c60` | `5166f4e210af3452` |
| 反制 `l_drop_flare` | 202 | `0x361f80` | `0x361d30` | `83981d0b957ef8ab` |
| 反制 `l_get_chaff_count` | 199 | `0x362290` | `0x362040` | `2e3707e3ac6b86c4` |
| 反制 `l_get_flare_count` | 199 | `0x362360` | `0x362110` | `45d000871e765ccd` |
| 機砲建置識別錨點 | 49 | `0x2fe650` | `0x2fe400` | `51418c51962d3af8` |

機砲錨點用於建置核對。實際彈量透過 `c_payload()`、掛架及機砲物件讀取；
另核對 `avArcadeRadar::get_shells_count`（新版 `0x41baa0`）與
`GunWeaponCommon::get_gun_mount`（新版 `0x4447f0`）的原生路徑。

## 已核對的布局與呼叫

以 PE 匯出表、x64 反組譯及可達控制流程比對 14 個指紋入口，
再檢查武器裝置／IR 建構式、IR cage／update_angles、機砲計數／掛架取得共 6 個入口。
對這 20 個入口，解析重定位、匯入與匯出參照後，相關指令、分支及物件欄位存取一致。
這是靜態入口與布局核對；DCS 中的物件生命週期、捕獲與實際投放仍以飛測確認。

| 存取 | 核對結果 |
|---|---|
| context → human | `+0x10` |
| human → payload | vtable `+0x120` |
| payload 熱焰彈／干擾絲庫存 | vtable `+0x1a8`／`+0x1b0` |
| human 熱焰彈／干擾絲施放 | vtable `+0x180`／`+0x178` |
| weapon → IR sensor | `+0x158` |
| sensor 模式／目標 ID／自主追蹤許可 | `+0x284`／`+0x2c8`／`+0x42d` |
| payload → gun mount | vtable `+0x230` |
| mount → gun | `+0x8` |
| gun 彈量 | vtable `+0x58` |

## 驗證與飛測

新版 CM 存取以實際 `DcsAircraftInterface` 配合可控制的 backend 重現：
更新前 `UnsupportedBuild`，更新後讀取及單發呼叫成功。
回歸測試涵蓋兩版存量、兩種反制扣量、IR 許可與方向讀回、模組識別變動時重新解析，
並確認指紋不符會在原生呼叫前拒絕，以及各能力獨立判定。
Windows CI 使用專案指定的工具鏈重建 DLL、執行完整驗證，再將該 DLL 裝入測試包。
建置與驗證結果以包內 `BUILD.json`、DLL manifest 及對應 CI 報告為準。

飛測包的 S03／S04 檢查原生解籠、IR LOCK 與雷達指向；S11 檢查機砲實際彈量；
S12 檢查熱焰彈與干擾絲的庫存、實際投放及扣量。每一步的預設按鍵與正常結果列於包內說明。

所有執行時紀錄在 `F-CK-1C/log`，回收工具在 `F-CK-1C/FlightTest`。
安裝與測試工具只在模組目錄寫入；提供的 DCS DLL 僅作為唯讀核對來源。
