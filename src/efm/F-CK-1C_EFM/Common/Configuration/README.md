# 設定檔載入器

`Configuration::load<T>(relative_path)` 每次讀取一份 UTF-8 JSONC，
回傳完整設定或包含完整 `Diagnostic` 的 `LoadResult<T>`。
設定儲存、載入時機及失敗後是否繼續，由呼叫端決定。

## 型別與生命週期

呼叫端提供與 T 同命名空間的 `read_configuration(const Value&, T&)`，
宣告欄位並驗證數值。`load<T>` 建立 T，將轉換函式交給內部載入流程。

DcsBridge 以 `initialize()` 設定 FM 根目錄與診斷出口，
環境銷毀時以 `shutdown()` 清除。載入器使用單執行緒共用環境，
不保存設定或快取文件。

載入順序為解析路徑、讀取原文、解析 JSONC、轉換型別。
原文在解析後釋放；暫存 `JsonDocument` 擁有 JSON 樹，
`Value` 是其唯讀檢視。呼叫端保存轉換後的設定。

## 路徑與欄位

- 路徑以 FM 為起點，禁止絕對路徑；解析後的實際文件必須位於 FM 內。
- `..` 與副檔名不另行限制；單份文件上限為 16 MiB。
- 同一串流取得大小並一次讀入；超出大小限制或讀取長度改變時失敗。
- 宣告欄位均必填；缺漏、多餘欄位及型別／數值錯誤中止載入。
- 解析器驗證資料字串及物件鍵的 UTF-8；註解不另做編碼驗證。
- 原始 NUL 被拒絕，JSON 跳脫序列 `\u0000` 可正常解析。
- 重複鍵由 nlohmann 解析器保留一個值，不額外回報。

## 錯誤

工具與欄位驗證使用 `Configuration::Error(stage, reason, field)`。
內部載入流程補上來源檔名，回報診斷並回傳失敗結果。
成功解析使用實際文件位置；路徑失敗使用請求位置。

語法與資料編碼錯誤保留解析器行列與原因；原始 NUL 回報位元組位置。
欄位錯誤含完整 JSON Pointer 路徑，名稱中的 `/` 與 `~` 會跳脫。
數值解析溢位保留解析器原因。`Diagnostic.line/column` 為 0，
解析器行列位於 `reason`。

日誌使用 `Diagnostic::message()`。
程式缺陷、資源失敗或診斷出口拋出的其他例外直接傳遞；
`load<T>` 不提供 noexcept 保證。
