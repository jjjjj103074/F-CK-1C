# avSimpleRadar 空對空介面調查

查閱日：2026-11-20。範圍限定 F-CK-1C 的空對空雷達；參考機型為早期 F-16C Block 25／APG-68。本文只調查 avSimpleRadar 的資料／控制能力及軟體模擬可行性，不把原生通用雷達直接視為 APG-68。早期 Block 25 的特定 OFP 模式清單尚未取得專屬原廠來源；不把較晚 Block 50／MLU 文件中的 TMS、SMT 等能力倒推成 Block 25 已確認配備。

## 判定方式與結論

- **已確認**：文件作者原始說明、開源模組實際程式，或使用者提供 DLL 的靜態證據支持。這不等於已在目前安裝的 DCS 實飛驗證。
- **需實機探測**：已有入口或可行設計，但單位、時序、有效範圍、目前版本效果尚未完整驗證。
- **未見公開入口**：本次已查來源沒有可直接使用的介面；不是宣稱引擎內部不存在這項能力。

**可採用的基礎是原生 RWS／STT、執行期掃描幾何及 TDC 控制，再由 RadarSystem 管理 F-16 的模式與軟體航跡。原生設備沒有已證明的 APG-68 模式選擇、指定回波 ID 鎖定或多目標 TWS。** 航跡記憶／外推可以在軟體層做；原生 STT 期間沒有持續搜尋其他目標的保證，因此只能稱為近似 SAM／TWS，不能宣稱已取得原生同時搜尋與多目標武器支援。[D1]

## 第一手來源與版本

1. **[D1] 文件原稿**：[DCS-OpenSource/dcs-opensource-modding-guide 的 avSimpleRadar.md](https://github.com/DCS-OpenSource/dcs-opensource-modding-guide/blob/31cba3ef2600084662750e0bd4c139cf0465069f/docs/Aircraft/Lua/Devices/avSimpleRadar.md)。[網站入口](https://modding.caffeinesimulations.com/Aircraft/Lua/Devices/avSimpleRadar/) 與此原稿一致；[CNAME](https://github.com/DCS-OpenSource/dcs-opensource-modding-guide/blob/31cba3ef2600084662750e0bd4c139cf0465069f/docs/CNAME) 指向同一網域。初稿作者為 CrudeCoder1，[2026-02-05 初稿 commit](https://github.com/DCS-OpenSource/dcs-opensource-modding-guide/commit/d7519bb1ae70cf58cc9f39227da0423815f5c3a9)；Hayds_93 於 [2026-02-06 整理格式](https://github.com/DCS-OpenSource/dcs-opensource-modding-guide/commit/2b041ac829dffa380e0574cc4c5f37ad298a7d87)。它是社群對介面的原始調查文件，不能稱為 ED 正式 SDK 保證。
2. **[V1] 實際開源呼叫**：[VSN_F4B_C_Standalone／Radar_init.lua](https://github.com/D4n2021/VSN_F4B_C_Standalone/blob/95696a499b12e128b584f48072e23dd064f2a997/Cockpit/Scripts/avRadar/Device/Radar_init.lua)。固定 commit `95696a499b12e128b584f48072e23dd064f2a997`；程式直接寫天線仰角、TDC 距離及方位。[V2] [indication_page.lua](https://github.com/D4n2021/VSN_F4B_C_Standalone/blob/95696a499b12e128b584f48072e23dd064f2a997/Cockpit/Scripts/avRadar/Indicator/indication_page.lua) 直接讀回波與 STT handles，篩掉過時回波。[V3] [原模組 readme.lua](https://github.com/D4n2021/VSN_F4B_C_Standalone/blob/95696a499b12e128b584f48072e23dd064f2a997/Cockpit/Scripts/avRadar/readme.lua) 將 509／510 綁成按下／放開。
3. **[N1] 原始範例作者 Nero.ger 的說明與參數實測**：[Basic Radar example code](https://forum.dcs.world/topic/188347-basic-radar-example-code/)。作者於 2020-06-02 補上實際 `RADAR_CONTACT_01_*` 值及編號規則；這是範例作者的第一手觀察。文中也說尚未探完全部能力。
4. **[E1] ED 的模組載入說明**：[Alex Okean 的 DCS Mods structure 教學](https://forum.dcs.world/topic/74145-dcs-mods-structure-how-to-create-your-plugin-from-scratch/)。可核對 `creators[devices.RADAR] = {"avSimpleRadar", ...}` 及原生 device 的載入方式；年代久遠，不用來保證現行行為。
5. **[B1] 使用者提供的引擎 DLL**：[CockpitBase.dll](/home/jjjjj103074/.codex/attachments/fe331f9c-4299-4693-8aa1-a9f4b940922b/CockpitBase.dll)。PE64，PE timestamp `2026-10-24 02:52:57 UTC`；SHA-256 `fb0a09be70627c7790dea1527cedfee43082bb8b112484d5c85be1d99d33de95`。timestamp 不等於已驗證 DCS 發行版本。僅做匯出表、字串與靜態反組譯；沒有載入 DLL、呼叫原生方法或改動檔案。下述 RVA 都只對此 SHA 有效，並非建議硬編碼。
6. **[L1] 本機專案的 cockpit C API header**：`src/efm/F-CK-1C_EFM/include/Cockpit/ccParametersAPI.h`。可核對 C++ 參數存取簽章；本次不把專案內副本的來源或完整性冒充成現行官方完整 SDK。

## 載入設定和執行期介面不同

載入方式是 `creators[devices.RADAR_SYSTEM] = {"avSimpleRadar", script_path}`；腳本以 `local dev = GetSelf()` 取得 Lua device。依文件，需要同時有已供電的 `avSimpleElectricSystem`。`perfomance` 拼字就是如此，不應自行改成 `performance`。[D1][E1][B1]

| 載入時全域設定 | 作用／單位 | 狀態 |
|---|---|---|
| `perfomance.scan_volume_azimuth` | 初始水平掃描總寬，rad；120° 代表 ±60° | 已確認：D1、V1、B1 initialize |
| `perfomance.scan_volume_elevation` | 初始垂直掃描總高，rad；D1 範例的「±5°」註解與其後「total elevation」說明不一致，應以實測判定邊界 | 已確認入口；總高／半高邊界需實機探測 |
| `perfomance.scan_beam` | 掃描條帶的垂直高度，rad；總高大於它時分多次水平掃描 | 已確認入口；實際條帶／更新率需實機探測 |
| `perfomance.max_available_distance` | 公尺；依文件是大 RCS 物件的理想最大探測距離，不能當所有戰機的探測距離 | 已確認 |
| `tracking_azimuth`、`tracking_elevation` | 宣告 STT 追蹤角度上下限，rad | D1、V1 有設定；本 DLL 可明確見 `tracking_azimuth` 讀取，`tracking_elevation` 的載入效果需實機探測 |
| `roll_compensation_limits`、`pitch_compensation_limits` | 姿態穩定補償限制，rad | 已確認 |
| `scan_speed`、`dead_zone` | 文件作者自己將語意列為未知 | 需實機探測；本 DLL 沒有同名字串，不能因設定表存在就保證會載入或動態生效 |

**改 Lua 的 `perfomance` 表不等於通知引擎重載。** 動態調整應走下表的參數 handles。二進位 `initialize()` 載入表後，`update_radar()` 會每次讀四個 `SCAN_ZONE_ORIGIN_*`／`SCAN_ZONE_VOLUME_*` 參數，更新當前搜尋 pattern；所以動態改寬與高已具有超出文件敘述的靜態證據。[B1]

## 可直接使用的 Lua 簽章

| 簽章 | 輸入／回傳 | 狀態與依據 |
|---|---|---|
| `local dev = GetSelf()` | 回傳本 device 的 Lua 物件 | 已確認 D1、V1 |
| `dev:set_power(value)` | `value` 是 Lua boolean；無回傳值 | 已確認 D1、B1 `l_set_power` |
| `dev:get_power()` | 回傳一個 Lua boolean | 已確認 B1 註冊與 wrapper；D1 漏列，需實機確認與實際輻射／供電觀測的關係 |
| `local h = get_param_handle(name)` | `name` 字串，回傳 handle | 已確認 D1、V1 |
| `h:set(value)` | 數字或字串；雷達控制欄位用數字 | 已確認 [共通 Param Handles 說明](https://github.com/DCS-OpenSource/dcs-opensource-modding-guide/blob/31cba3ef2600084662750e0bd4c139cf0465069f/docs/Aircraft/Lua/BasicPrinciples.md) 及 V1 |
| `h:get()` | 讀出數字或字串 | 已確認；回傳型別依欄位，例如 `NCTR` 是字串 |
| `dispatch_action(nil, command[, value])` | 將原生通用指令送入 DCS；509 的二參數形式由 D1 直接示範 | 已確認有通用指令路徑；不會回傳鎖定成功或指定目標 ID |

`SetCommand(command, value)`、`update()`、`post_initialize()` 是 Lua 回呼；`dev:listen_command(command)` 訂閱命令。它們不是可直接選擇 F-16 雷達模式的專用 API。[D1][V1]

## 執行期控制參數

讀寫方式一律是 `get_param_handle("名稱"):set(value)`／`:get()`，不另有 `set_scan_volume()` 已確認的 Lua 方法。

| 名稱 | 方向與語意 | 單位／限制 | 狀態 |
|---|---|---|---|
| `SCAN_ZONE_ORIGIN_AZIMUTH` | 寫；搜尋中心水平指向 | rad 受 V1／初始化及掃描運算支持；正負方向、姿態參考需實機確認 | 已確認動態讀取 B1；細部座標需實機探測 |
| `SCAN_ZONE_ORIGIN_ELEVATION` | 寫；搜尋中心垂直指向 | rad；V1 每次以 `math.rad(2)` 調整 | 已確認動態讀取 D1、V1、B1 |
| `SCAN_ZONE_VOLUME_AZIMUTH` | 寫；水平總掃描寬度 | rad；全寬；不是「半角」 | 已確認 D1、B1 |
| `SCAN_ZONE_VOLUME_ELEVATION` | 寫；垂直總掃描高度 | rad；與 `scan_beam` 配合 | 已確認 D1、B1；實際條帶時間需實機探測 |
| `RADAR_TDC_AZIMUTH` | 寫；TDC 水平位置 | 單位文件未明說；從角度使用／原生搜尋運算可推 rad，但需校準正負號與原點 | 已確認入口；座標需實機探測 |
| `RADAR_TDC_RANGE` | 寫；TDC 距離 | V1 以公尺數值控制，限制在 300 至約 76 km；這是該模組自己的限制，非 SDK 上限 | 已確認入口與公尺用法；需實機校準 |
| `RADAR_TDC_RANGE_CARRET_SIZE` | 寫；捕獲距離容差／游標捕獲窗 | D1 預設 6000；是否為全寬或半寬、邊界值需測 | 已確認入口；精確語意需實機探測 |
| `ACQUSITION_ZONE_VOLUME_AZIMUTH` | 寫；捕獲水平窗候選 | 拼字缺少第二個 i；存在且在更新時讀取。角度範圍／全寬半寬尚未校準 | 已確認 B1 入口；需實機探測 |
| `SCAN_VOLUME_CUT_OFF_DISTANCE_MAX` | 寫；搜尋回波最大距離裁切 | D1 漏列；B1 初始化保存 handle，`is_rejected_by_dist(double)` 每次讀取並比較最大距離 | 已確認動態裁切入口；公尺及交互作用需實機探測 |
| `SCAN_VOLUME_CUT_OFF_DISTANCE_MIN` | 寫；搜尋回波最小距離裁切 | B1 同上；不是已確認能根除 300 m 追蹤限制的旋鈕 | 已確認入口；需實機探測 |
| `IFF_INTERROGATOR_STATUS` | 寫；1 啟用 IFF | 數值旗標 | 已確認 D1 |
| `RADAR_PITCH_STABILIZATION` | 寫；1 啟用俯仰穩定 | 數值旗標 | 已確認 D1、V1、B1 |
| `RADAR_BANK_STABILIZATION` | 寫；滾轉穩定 | V1 及 B1 都用 BANK；D1 使用 `RADAR_ROLL_STABILIZATION`，應視為待核對的文件名稱差異 | 已確認 BANK 入口；ROLL 未見本 DLL 對應字串／初始化 |
| `RADAR_PITCH_BANK_STABILIZATION` | 寫；1 啟用綜合穩定 | 數值旗標 | 已確認 D1、V1、B1 |
| `RADAR_BIT`、`CLOSEST_RANGE_RESPONSE` | 存在，但完整控制／回報契約未明 | 不因參數存在就當作任意模式設定入口 | 需實機探測 |

距離顯示刻度可以由 RadarSystem 自訂，例如 10／20／40／80 NM。那是顯示尺度；原生可回波距離還受到 `max_available_distance`、距離裁切、RCS 與原生偵測邏輯影響。V1 自訂的 `RADAR_RANGE_SCALE`／`RADAR_RANGE_MULTI` 不是證明原生雷達具有同名公開控制。[V1][B1]

## 捕獲、指定目標與解除鎖定

| 操作 | 入口 | 判定 |
|---|---|---|
| 開始 TDC 捕獲 | `dispatch_action(nil, 509)` | 已確認 D1、V3；B1 的 `SetCommand` 將模式 1 搜尋改成 2 捕獲 |
| 放開捕獲鍵 | `dispatch_action(nil, 510)` | 已確認 B1：只在模式 2 時改回模式 1；**不是此版本已確認的 STT 解鎖** |
| 解除已建立 STT | 模式為 3 時送 509 | 已確認 B1 靜態分支會改成模式 1；實際釋放武器／追蹤狀態仍需實機探測 |
| 指定 `RADAR_CONTACT_XX` 鎖定 | 無已確認的 Lua `lock(contact_id)`／`set_target(id)` | 未見公開入口；可先把 TDC、掃描中心與捕獲窗對準該回波，再要求原生捕獲，但無成功／必為該物件的保證 |
| 直接指定 DCS 物件 ID 鎖定 | C++ 有 protected `mail_lock(MovingObject*)` 匯出 | 方法存在已確認；這需要 `MovingObject*`，不是回波編號；公開可取得物件／可用 ABI 路徑未驗證，不得當成可直接實作的 API |

上述 509／510 分支在 B1 `avSimpleRadar::SetCommand(int,float)`，RVA `0x42e970`。命令分支不以 value 作為目標 ID。此結論只套用該 DLL；應避免用「509＋510 固定延時後必定鎖到選中目標」作為模型契約。[B1]

## 搜尋回波、單位、更新與消失

原生回報由一組 `RADAR_CONTACT_%02d_` 參數構成，不是已確認可列舉的 Lua `get_contacts()` 表。Nero.ger 明確說編號會由 01、02 持續超過 99 至 100、101；V2 畫 1..900 個 slot 只是那個顯示腳本的選擇，不是引擎容量保證。[N1][V2][B1]

| 欄位 | 已確認內容 | 未確定部分／判定 |
|---|---|---|
| `AZIMUTH`、`ELEVATION` | 回波角度位置；N1 列出原始角值；D1 說明用途 | rad 很合理，但 D1 未逐欄明訂搜尋回波角度單位及座標系；**需實機探測**，不要直接假設與 STT 滾轉補償相同 |
| `RANGE` | 回波距離；V2 以數萬尺度與公尺用法顯示 | 公尺使用有原碼依據；量測偏差、斜距定義及值失效方式需實測 |
| `RANGE_SCALED` | 本次 D1、V1、V2、N1 與 B1 字串未見此欄位 | **未見公開入口**；若專案已有它，先視為專案衍生值，不能代替確認原生 RANGE 單位 |
| `TIME` | D1 定義為回波出現後經過秒數；V2 用 0..5 秒顯示存活區間 | **已確認文件語意**；新回波歸零、舊 slot 重用、無效值、實際壽命與何時移除需實機探測。它不是穩定更新時戳的保證 |
| `VX`、`VY`、`VZ` | D1／N1／B1 證明有三個速度分量 | **需實機探測**：來源沒有完整標明 m/s、世界／機體座標、絕對／相對速度。不能直接宣告 X=北、Y=上、Z=東 |
| `FRIENDLY` | IFF 開啟且友軍為 1；N1 實測曾為 -1 | 已確認；搜尋模式其他數值語意應與實際 IFF 狀態比對 |
| `RCS`、`RCS_COEFF` | 確認有欄位；D1 將 COEFF 描述成 RCS | **需實機探測**：RCS 與係數不能混用，m²／無單位及校準未完整說明 |
| `NCTR` | 飛機型別字串；N1 範例為 `Su-25` | 已確認字串用途；不是物件 ID，也不是已驗證的真實 APG-68 NCTR 限制模型 |
| `RND`、`NOISE` | N1 與 B1 有欄位 | 需實機探測完整語意 |
| `XX`／物件 ID | D1 明說同一架飛機會產生多個 contact，contact 代表一次回波 | **已確認 XX 不能當穩定目標 ID**。搜尋回波物件 ID、跨掃描關聯鍵：未見公開入口 |

在 `RADAR_MODE == 1` 消費搜尋回波；進入 STT 後沒有新鮮的其他搜尋回波保證。`TIME` 和距離等資料還可能殘留，必須加上模式與新鮮度判斷。軟體航跡應自行保有 TrackId、來源量測時刻、最後命中、外推／失聯狀態；不要把 slot、NCTR、速度相同或回波持續存在誤當成同一目標已確認。[D1][N1][V2]

## STT 回報

| handle | 定義 | 狀態 |
|---|---|---|
| `RADAR_MODE` | 1 搜尋、2 嘗試捕獲、3 STT。B1 另可見 0 的關機狀態 | 已確認；它是觀測，不是 F-16 模式要求 |
| `RADAR_STT_AZIMUTH` | 鎖定目標水平角，文件說已做 roll normalization | 已確認入口；rad 由同組角度使用支持，D1 此欄未明寫單位；座標正負號需實機探測 |
| `RADAR_STT_ELEVATION` | 鎖定目標垂直角，rad，已做 roll normalization | 已確認 D1 |
| `RADAR_STT_RANGE` | 鎖定目標距離，公尺 | 已確認 D1 |
| `RADAR_STT_FRIENDLY` | -1=IFF 關；0=敵或未知；1=友軍 | 已確認 D1 |
| STT 速度、航向、物件 ID、鎖定品質 | Lua param 清單中未見完整公開欄位 | 未見公開入口。可由本機位置／LOS 時序推估的量另標為衍生量；不可宣稱原生真值 |

## C++：可用參數橋和原生匯出要分開看

L1 的 C linkage 簽章如下，B1 也匯出同名入口；它們處理的值仍受同一組 param 的語意限制，不會額外提供完整世界目標清單。

```cpp
void* ed_cockpit_get_parameter_handle(const char* name);
void ed_cockpit_update_parameter_with_number(void* handle, double number_value);
void ed_cockpit_update_parameter_with_string(void* handle, const char* string_value);
bool ed_cockpit_parameter_value_to_number(const void* handle, double& res, bool interpolated = false);
bool ed_cockpit_parameter_value_to_string(const void* handle, char* buffer, unsigned buffer_size);
int ed_cockpit_compare_parameters(void* handle_1, void* handle_2);
```

以下是 B1 匯出名稱可解出的簽章；**已確認「匯出存在」，沒有驗證本專案可取得正確的 `this`／型別佈局／呼叫時機。** 虛擬／protected 方法的存在不等於公開可直接呼叫的 EFM API。

| 原生方法 | RVA | 靜態可確認內容 |
|---|---|---|
| `public virtual void avSimpleRadar::SetCommand(int, float)` | `0x42e970` | 通用命令接收，不含指定 contact ID 參數 |
| `public virtual bool avSimpleRadar::getElecPower() const` | `0x42f4f0` | 供電查詢方法存在 |
| `public virtual void avSimpleRadar::setElecPower(bool)` | `0x432e40` | 供電設定方法存在 |
| `public virtual double avSimpleRadar::get_target_range() const` | `0x42ff80` | 原生追蹤距離方法存在；單位不能只靠 mangled name 推定 |
| `public virtual unsigned avSimpleRadar::get_target_to_engage() const` | `0x42ffa0` | unsigned 回傳值，不是 lock-by-ID setter；ID 命名空間及有效期需實機探測 |
| `public virtual bool avSimpleRadar::is_tracking(unsigned, unsigned) const` | `0x430fa0` | 兩個 unsigned 參數的具體意義不在匯出名稱內 |
| `public double avSimpleRadar::get_closest_range_response() const` | `0x42f640` | 方法存在，完整契約未確認 |
| `protected virtual void avSimpleRadar::mail_lock(MovingObject*)` | `0x4311b0` | 接收物件指標的內部鎖定方法存在；不是可用回波 slot 直接鎖定的公開 API |
| `protected virtual double avSimpleRadar::get_contact_timeout_time() const` | `0x42f670` | 逾時方法存在；不等於 Lua 可設定 timeout |
| `protected virtual void avSimpleRadar::l_register_functions(lua_State*, int)` | `0x4310a0` | 先呼叫基底註冊，再把 `get_power`／`set_power` 綁至 Lua wrapper；此類別自身未另外註冊 lock-by-ID／set-mode 方法 |

本 DLL 匯出及字串中未見 `avBaseRadar` 類別名稱；不能據此否定其他模組 DLL 或非匯出類別，也不能憑類別名稱假設 `avSimpleRadar` 繼承某個可公開使用的 `avBaseRadar` SDK 介面。未找到公開官方 `avSimpleRadar.h` 的完整可用契約；本次也沒有採用來歷不明的 SDK header 當現行 API 依據。

## 對 F-16 模式實作的含意

| 目標功能 | 能力標註 | RadarSystem 可合理負責的部分／限制 |
|---|---|---|
| RWS | 已確認 | 讀原生回波，控制範圍／掃描中心／寬高；軟體負責顯示尺度與回波壽命 |
| STT | 已確認原生基礎；需實機探測本專案整合 | TDC 捕獲、確認 mode=3，再使用 STT handles；不同於保存「已要求鎖定」 |
| ACM：HUD／boresight／vertical 等 | 已確認有幾何與捕獲入口；模式行為需實機探測 | 用模式狀態機設定不同幾何、指向與自動捕獲策略。不能只送名稱相近的通用 FC3 命令就視為 APG-68 ACM 已成立 |
| SAM | 未見原生專用入口；軟體近似需實機探測 | 搜尋時維持軟體指定航跡並調整掃描；若切原生 STT，其他航跡只能暫時記憶／外推 |
| TWS | D1 明確不提供原生 TWS；軟體近似需實機探測 | 可在 RWS 回波上做關聯、優先序、記憶與顯示；不能保證多目標原生持續追蹤或多枚主動彈中段支援 |
| 直接選 `F16_RWS`／`F16_SAM`／`F16_TWS`／`APG68_*` | 未見公開入口 | 本次文件、開源用法、此 DLL 都沒有已確認的 F-16 雷達模式 API。`iCommandPlaneRadarChangeMode` 285 是通用指令名稱，V1 的相應 mode 寫入程式實際在註解區，不是支援 TWS 的證據 |

這份調查支持「APG-68 操作狀態機＋原生通用感測器」的架構方向；它沒有驗證 Block 25 各掃描圖樣、探測性能或火控數據的真實度。那些數值應由另份 Block 25／APG-68 來源研究與 DCS 情境測試決定。

## 建議的最小實機探測

1. 一個已知位置／速度的空中目標，測量不同姿態下搜尋和 STT 的方位／仰角正負號、rad，以及 RANGE 是否為斜距公尺；不要混用兩者座標。
2. 固定 `perfomance`，只改四個 `SCAN_ZONE_*` handles，測掃描邊界與高度條帶；加入 MAX／MIN 距離裁切，確認真實偵測與畫面尺度分開。
3. 記錄 509 按下／510 放開及已有 STT 後再次 509 的 `RADAR_MODE`、STT handles、武器目標狀態，驗證取消與解鎖。
4. 單機、兩架同型機、交叉航跡情境，記錄每個 slot 的 TIME、新回波及重用；檢查 VX／VY／VZ 是世界絕對速度、機體速度或相對速度。
5. 指定軟體航跡後，把 TDC 對準並觸發原生捕獲，核對最終 STT 是否符合被指定的方位／距離門限；同門限多目標時不得自動宣告正確 ID。
6. 確認 STT 期間其他搜尋回波是否停止更新，讓 SAM／TWS 顯示清楚區分量測與外推。這是功能宣稱的必要邊界。
