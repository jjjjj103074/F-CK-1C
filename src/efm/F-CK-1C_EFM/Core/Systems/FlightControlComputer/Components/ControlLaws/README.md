# ControlLaws

目前提供三軸直接映射：俯仰輸入至對稱平尾、滾轉輸入至差動襟副翼、偏航輸入至方向舵。
尚未實作回授控制、配平、模式切換、濾波、積分與依系統狀態的控制限制。

## 輸入與輸出

ControlLaws 建構時固定綁定 InputOutputData、ControlLawData 與唯讀的 SystemStatusTables。
每次 step 讀取 InputOutputData.input.pilot_control 的三個操縱軸，依序進行輸入限幅、
曲線線性插值、增益與輸出限幅，再將完整需求寫入 InputOutputData.output.actuator_command。
非有限操縱輸入視為中立。

需求為 [-1, 1] 的無因次舵面位置：0 是中立，正負 1 是該方向完整名義行程。
拉桿、右滾與左偏航為正。
FlightControlActuationSystem 持有舵面行程與運動限制，將需求轉為弧度；
實際舵面位置、角速度及氣動模型輸入仍為 rad 與 rad/s。

## 設定與狀態

ControlLawData.configuration 在建構後唯讀，包含各軸的 gain 與 curve。
ControlLawData.state 保存本次增益後、限幅前的需求，以及是否受到輸出限幅。
限幅前需求可以超出 [-1, 1]；最終需求限制於該範圍。
SystemStatusTables 於建構時綁定，狀態相關運算尚未實作。

設定檔為模組根目錄下的 FM/FLCC/ControlLaws.jsonc，使用 UTF-8，可寫 // 與 /* */ 註解。
ControlLawData 建構時自行呼叫 `Configuration::load<T>()` 完整讀取、轉換並驗證，
取得包含完整設定或失敗診斷的 LoadResult，透過 succeeded() 判斷結果。
設定結構與欄位規則位於 Database/ControlLawData/Configuration，執行狀態位於 State。
Configuration.h 的 load<T> 提供型別轉換回呼；
Configuration.cpp 的 load 主流程依序使用路徑、讀檔與 JSONC 解析工具，
再呼叫設定型別的欄位轉換函式。
共用環境保存 FM 根目錄與診斷回報函式；JsonDocument 保存本次載入的暫存資料。
完整設定存入 ControlLawData，供 ControlLaws 在 step 時讀取。
每次僅讀取指定文件，開啟後從串流確認大小不超過 16 MiB，再一次配置原文緩衝。
每次開始飛行時重新建立 ControlLawData，載入當時的設定內容。

曲線節點為 [輸入, 輸出]；至少三個節點，輸入嚴格遞增，端點為 -1 與 1，
包含 [0, 0]，座標皆為 [-1, 1] 有限數值，且輸出與輸入維持相同操縱方向。
gain 必須為有限非負數值。隨附文件的 gain 為 1，曲線為 [-1, -1]、[0, 0]、[1, 1]。
例如 gain 為 0.5 時，全量操縱對應一半名義行程；修改中間節點可以改變輸入靈敏度。

每個設定欄位都必填；多餘、缺漏、無效 UTF-8、語法、型別與數值規則錯誤均拋出 Error，
工具及欄位驗證提供階段、欄位位置與原因，load 統一補上來源檔案；
load 捕捉後回報並回傳同一份完整診斷。
ControlLawData 需要完整設定，
因此由它在載入失敗時中止自身初始化。
診斷包含實際檔案、處理階段、行列或完整欄位路徑，以及具體原因。
Error 的完整診斷由 load 送入 EventLog，也透過 LoadResult.error() 回傳。
ABI 邊界另行記錄 ControlLawData 中止初始化的狀態。
相對路徑以 FM 為根，限於目前目錄與子目錄；實際文件位置亦須位於 FM 內。

## 驗證

[ControlLawsTests.cpp](../../../../../../F-CK-1C_EFM_Tests/ControlLawsTests.cpp)
包含三軸方向、曲線插值、增益、輸入與輸出限幅、限幅狀態更新及無效設定欄位診斷。
[FlightControlActuationSystemTests.cpp](../../../../../../F-CK-1C_EFM_Tests/FlightControlActuationSystemTests.cpp)
驗證三軸的正負正規化需求依各自名義行程換算為弧度。
