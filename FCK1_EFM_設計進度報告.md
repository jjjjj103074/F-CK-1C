# FCK1_EFM 設計進度報告

更新日期：2026-03-06  
掃描範圍：目前 repository 全部檔案與主要程式碼（Lua、C++、資源結構）

## 1. 專案概述

- 本專案目的為在 DCS 中建立 F-CK-1（經國號）模組，並以 EFM（External Flight Model）取代預設 SFM，逐步建立可維護、可調校、可擴充的真實化飛行行為。
- F-CK-1 EFM 目標為：
  - 提供可控的空氣動力與發動機推力模型。
  - 建立 FBW（Fly-By-Wire）控制律與限制器。
  - 與 DCS API 正確整合（姿態、力矩、起落架、引擎參數、輸入命令）。
  - 最終達成地面/空中碰撞、起降、武器與座艙系統一致運作。
- 目前開發主要方向：
  - EFM 穩定性與接地/碰撞行為修正。
  - 油門軸、怠速區間、加力段（AB）動力曲線校準。
  - FBW 控制律與配平/抬頭行為調校。
  - Lua 層與 C++ DLL 的整合與啟動流程穩定化。

## 2. 概念設計（Conceptual Design）

- 整體系統架構概念：
  - `entry.lua` 載入模組、掛接 `BasicEFM_template.dll`、切換 EFM 模式。
  - `F-CK-1C.lua` 定義機體資料（外形、任務、感測器、武器配置、SFM 兼容資料）。
  - `FM/config.lua` 提供起落架/懸吊/輪胎/碰撞殼設定，供 DCS FM 介面使用。
  - `Basic_EFM_Template.cpp` 負責每幀力、力矩、控制律、推力、燃油、損傷與 DCS API 介面。
  - `Cockpit/Scripts/*` 提供座艙裝置、命令映射、CMS 操作邏輯。
- 專案設計理念：
  - 先「可飛且可測」，再逐步提升真實度。
  - 以參數化表格與可插拔控制律，支援快速迭代。
  - 將 DCS 介接、飛行物理、座艙腳本分層，降低耦合。
- 系統主要模組：
  - 飛行模型（EFM）：C++ 核心 API 實作。
  - 空氣動力模型：Mach 對應係數插值、升阻力/側向力。
  - 飛控系統：FBW 狀態機（RATE/HOLD/DEGRADE）與 CAT 模式切換。
  - 武器系統：機砲、主武裝開關、扳機門檻、基礎放焰彈流程。
  - 感測器系統：`Sensors` 目前僅基礎佔位（RWR 抽象定義）。
  - 航電系統：目前多為框架，尚未進入完整作戰航電實作。

## 3. 初步設計（Preliminary Design）

- 子系統拆分與責任：
  - `Aerodynamics` 模組：依速度/攻角/側滑與表格係數計算氣動力，輸出到局部力與力矩。
  - `Flight Control` 模組：接收操縱輸入、執行 FBW 控制律、限幅、反積分飽和、舵面作動器模型。
  - `Engine` 模組：油門命令整形、雙發啟停、軍推/加力分段、燃油消耗、引擎讀值輸出。
  - `Landing Gear & Suspension` 模組：輪煞、前輪轉向、懸吊回饋（WoW）與接地狀態判定。
  - `Weapon Interface` 模組：機砲射擊、主武裝模式、CMS 指令分派。
  - `DCS API` 介面：實作 `ed_fm_*` 系列函式與 DCS 引擎交換資料。
- 資料流程與模組關係：
  - 玩家輸入（Input Lua）→ `ed_fm_set_command` → 控制狀態（pitch/roll/yaw/throttle）。
  - DCS 狀態回呼（atmosphere/current_state/surface/suspension_feedback）→ EFM 內部狀態更新。
  - `ed_fm_simulate(dt)` 每幀計算空氣力、推力、控制律、燃油、地面判定 → 輸出 `common_force/common_moment`。
  - `ed_fm_get_param` 提供引擎、起落架、煞車、操縱面等參數給 DCS 子系統。
  - Cockpit Lua（CMS/Actuator）透過 `dispatch_action` 與命令 ID 連接 DCS 武器與反制邏輯。

## 4. 細部設計（Detailed Design）

- 主要程式檔案功能：
  - `entry.lua`：
    - 宣告外掛、掛載資源路徑。
    - 提供 `baseline / efm_min / efm_full` 診斷模式切換。
    - 在 EFM 模式下掛接 `BasicEFM_template.dll` 與 `FM/config.lua`。
  - `F-CK-1C.lua`：
    - 定義機體外型、尺寸重量、任務類型、SFM 參數、機砲、反制器、感測器。
    - `Pylons` 目前為空集合（掛載點尚未完成）。
  - `FM/config.lua`：
    - 設定重心、慣性矩、三點起落架懸吊參數與輪胎摩擦、煞車矩、`collision_shell_name`。
  - `DCS-Basic-EFM-Template-main/Basic_EFM_Template/Basic_EFM_Template.cpp`：
    - EFM 主回圈 `ed_fm_simulate`。
    - 控制輸入 `ed_fm_set_command`。
    - 引擎/油門/加力與燃油消耗。
    - FBW 控制律與狀態機。
    - 懸吊回饋 `ed_fm_suspension_feedback` 與接地判定。
  - `Cockpit/Scripts/Systems/cms_system.lua`：
    - 主武裝 ON/OFF/SIM、扳機保護邏輯、單發/程式放焰彈。
  - `Input/F-CK-1C/*/default.lua`：
    - 軸向與按鍵映射（含 FBW/CMS/武器相關命令）。

- 各模組程式架構：
  - EFM C++ 以全域狀態 + DCS 回呼函式為核心。
  - FBW 內含 CAT 參數結構、增益排程、姿態保持狀態機、作動器限制器。
  - Lua 端採裝置化設計（`devices.lua` + `device_init.lua` + 系統腳本）。

- 重要演算法與資料結構：
  - 氣動係數插值：以 Mach 表格做線性插值（`lerp`）計算 `Cx/Cy/CyMax/AlphaMax`。
  - FBW 狀態機：`RATE -> HOLD -> DEGRADE`，依死區、AoA、動壓、飽和狀態切換。
  - 作動器模型：命令限幅 + 速率限制 + 一階遲滯 + anti-windup。
  - 發動機分段：油門至軍推（detent 前）與加力附加推力（detent 後）。
  - 接地判定：以懸吊回饋（壓縮量/受力）建立 WoW，不再只靠 AGL。

- 模組互動方式：
  - `entry.lua` 決定是否走 EFM 路徑。
  - EFM 每幀輸出力/矩，DCS 負責剛體積分。
  - 座艙 CMS 命令同時可走 `dispatch_action` 直連 DCS 武器命令。
  - `FM/config.lua` 與 EDM 輪殼命名必須一致，否則地面接觸/碰撞行為會異常。

## 5. 系統架構

```text
F-CK-1C
├─ entry.lua
├─ F-CK-1C.lua
├─ FM
│  └─ config.lua
├─ bin
│  └─ BasicEFM_template.dll
├─ DCS-Basic-EFM-Template-main
│  ├─ Basic_EFM_Template
│  │  ├─ Basic_EFM_Template.cpp
│  │  ├─ Basic_EFM_Template.h
│  │  ├─ FM_data.h
│  │  ├─ FM_data.cpp
│  │  ├─ Inputs.h
│  │  ├─ FCS.h / FCS.cpp
│  │  └─ include/FM, include/Cockpit
│  └─ x64/Release
├─ Cockpit
│  └─ Scripts
│     ├─ device_init.lua
│     ├─ devices.lua
│     ├─ command_defs.lua
│     └─ Systems
│        ├─ cms_system.lua
│        ├─ gear_system.lua
│        └─ actuators_system.lua
├─ Input
│  └─ F-CK-1C
│     ├─ joystick/default.lua
│     └─ keyboard/default.lua
├─ Shapes
│  └─ F-CK-1C.edm
├─ Textures
├─ Liveries
└─ baseline_original
   ├─ F-CK-1C (原始可飛行版本備份)
   └─ DCS-Basic-EFM-Template-main (原始 EFM 參考)
```

## 6. 設計清單（Design Checklist）

| 系統               | 目前狀態 | 備註                                                     |
|:-------------------|:--------:|:---------------------------------------------------------|
| 飛行模型（EFM 主介面） | 開發中   | 已接入 DCS API，仍需穩定化調校                          |
| 空氣動力           | 開發中   | 已有插值與基礎力模型，需實機化校準                      |
| 發動機模型         | 開發中   | 已有雙發、怠速、軍推/加力段，仍有油門/推力調校議題      |
| 飛控系統（FBW）     | 開發中   | 已有 CAT 與姿態保持狀態機，仍有抬頭/操穩問題            |
| 武器系統           | 開發中   | 機砲與主武裝/CMS 基礎可用，掛載點與火控未完成           |
| 雷達               | 尚未開始 | `Sensors` 尚無實際雷達實作                              |
| RWR                | 開發中   | 目前僅 `Abstract RWR` 佔位定義                          |
| 航電系統           | 尚未開始 | 缺少完整導航/火控/多功能航電                             |
| 座艙整合           | 開發中   | 有裝置框架與少量系統腳本，尚未完整化                    |
| 碰撞/接地整合      | 開發中   | 地面與空中碰撞仍需 EDM + EFM 聯合驗證                   |

狀態分類說明：
- 已完成：功能可穩定使用且已通過主要驗證流程。
- 開發中：已有程式骨架或部分功能，仍需除錯/調校/整合。
- 尚未開始：目前僅規劃或佔位，尚無有效實作。

## 7. 完成進度

- 已完成的功能：
  - 模組基本註冊與資源掛載（`entry.lua`）。
  - EFM DLL 介接路徑與模式切換機制（`baseline / efm_min / efm_full`）。
  - 基礎氣動力、控制面、推力、燃油消耗主流程。
  - 基本輸入映射（軸向、飛控、武器、CMS）。
  - CMS 基礎邏輯（主武裝模式、扳機門檻、放焰彈程式）。
  - 懸吊回饋介接函式與 WoW 判定框架。
  - 原始版本與原始 EFM 程式備份（`baseline_original`）。

- 正在開發的功能：
  - 地面接觸/下沉與空中穿模問題收斂。
  - 油門軸方向、怠速推力、加力段手感與推力曲線細調。
  - FBW 抬頭與操穩異常修正。
  - DCS 參數回傳（`ed_fm_get_param`）與視覺/音效耦合一致性。
  - EDM 碰撞殼命名與 `FM/config.lua` 對位驗證。

- 尚未開發的功能：
  - 真實化雷達/火控邏輯與資料鏈。
  - 完整 RWR/ECM 系統。
  - 掛載點與武器整合（`Pylons`、彈藥管理、投放邏輯）。
  - 完整航電頁面、導航系統、告警系統與座艙深度互動。

## 8. 待完成任務

- 完成 EDM 碰撞幾何檢查，確認空中碰撞可正確阻擋（非僅地面接觸）。
- 重新檢視 `FM/config.lua` 懸吊參數與輪胎摩擦係數，建立可重現測試場景。
- 調整 `throttle_axis_inverted`、`afterburner_detent`、`engine_power_table`，消除怠速與加力分界異常。
- 修正 FBW 配平/抬頭問題，降低不合理自發性俯仰趨勢。
- 建立標準化測試流程（冷啟動、熱啟動、起飛、落地、高速轉向、撞擊測試）。
- 清理編碼與建置警告（目前有 C4819、C4244 警告）。
- 釐清 `FM_data.cpp` 與主流程的實際使用關係，避免未整合檔案造成維護風險。
- 補齊 `.gitignore` 規則（目前僅 `list/`，大小寫與 VS 暫存檔尚未完整排除）。

## 9. 未來開發計畫

- 短期目標（1~3 週）：
  - 先鎖定「可穩定起降且不穿模」版本。
  - 完成油門/加力段可預期控制手感。
  - 建立最小可回歸測試清單並固定測試任務。

- 中期目標（1~2 個月）：
  - 逐步以實機資料校準氣動與發動機參數。
  - 完成 FBW 控制律第一版（含保護律與故障降級邏輯）。
  - 打通武器掛載點與基本火控流程。
  - 提升座艙系統覆蓋率（飛行/武器/告警關鍵回路）。

- 長期目標（3 個月以上）：
  - 建立接近實機等級之 F-CK-1 全域飛行包線行為。
  - 完成雷達、RWR、航電頁面、資料鏈等戰術系統整合。
  - 建立多人連線與 AI 相容驗證流程。
  - 形成可長期維護之模組化架構與版本管理流程。

## 10. 版本控制與版本迭代

- 版本策略：採 `主版號.次版號.修訂號`，目前以功能整合與飛行穩定性修正為主。

| 版本 | 日期 | 主要修改內容 |
|:-----|:-----|:-------------|
| v0.1.0 | 早期基線 | 模組可被 DCS 載入，完成基本機體結構與資源掛載。 |
| v0.1.1 | 2026-03-06 前 | 導入 EFM 路徑與 DLL 掛接，建立 `baseline/efm_min/efm_full` 測試模式。 |
| v0.1.2-dev | 2026-03-06 | 強化版本註記與報告管理；整理設計進度文件與版本迭代紀錄格式。 |

- 版本迭代重點（近期）：
  - EFM 接地與碰撞整合（地面下沉、空中穿模）持續修正。
  - 油門軸方向、怠速段與加力段分界持續校準。
  - FBW 抬頭與整體操穩行為持續調整。
