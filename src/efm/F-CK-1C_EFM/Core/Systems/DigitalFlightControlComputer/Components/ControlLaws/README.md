# ControlLaws

三軸直接映射：俯仰至對稱平尾、滾轉至差動襟副翼、偏航至方向舵。
回授控制、配平、模式切換、濾波、積分與系統狀態限制尚未實作。

## 計算

建構時綁定 InputOutputData、ControlLawData 與唯讀 SystemStatusTables。
每次 step 讀取 pilot_control，依序執行：

1. 非有限輸入視為 0，其餘限於 [-1, 1]。
2. 依曲線線性插值。
3. 乘上該軸 gain，再將需求限制於 [-1, 1]。
4. 保存限幅前需求、限幅狀態及完整 actuator_command。

正方向為拉桿、右滾、左偏航。輸出代表名義舵面行程比例；
FlightControlActuationSystem 轉成 rad 並處理物理運動限制。
SystemStatusTables 目前只固定綁定，未參與模式選擇。

## 設定

模組根目錄下的 `FM/DFLCC/ControlLaws.jsonc` 定義各軸的 gain 與 curve。
ControlLawData 每次開始飛行重新載入，成功後設定保持唯讀。

- gain 為有限非負數。
- 曲線至少三點，格式為 [輸入, 輸出]。
- 輸入嚴格遞增，端點為 -1、1，包含 [0, 0]。
- 座標為 [-1, 1] 的有限值，輸出與輸入維持相同操縱方向。
- 所有宣告欄位必填，多餘或無效欄位使初始化失敗。

隨附設定的 gain 為 1，曲線為 [-1, -1]、[0, 0]、[1, 1]。
gain 0.5 對應一半名義行程；中間節點控制靈敏度。

路徑、JSONC 與診斷規則見[設定檔載入器](../../../../../Common/Configuration/README.md)。
ControlLawData 在載入失敗時中止初始化，ABI 邊界記錄錯誤。

## 驗證

[ControlLawsTests](../../../../../../F-CK-1C_EFM_Tests/ControlLawsTests.cpp)
涵蓋方向、插值、增益、限幅、狀態與無效設定。
[Actuation 測試](../../../../../../F-CK-1C_EFM_Tests/FlightControlActuationSystemTests.cpp)
涵蓋正規化需求至弧度的換算。
