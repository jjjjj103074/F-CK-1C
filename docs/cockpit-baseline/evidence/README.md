# Runtime Evidence

每個子目錄代表一次真正的 DCS 執行，不是預期結果。

必要檔案：

- `RUN.md`：版本、hash、scenario、結論與可追溯性。
- `observations.csv`：逐項 expected／observed／result。
- `dcs-relevant.txt`：從該次完整 `dcs.log` 摘出的原始行。

完整 log 不直接放入 repository，但 `RUN.md` 必須保存其 SHA256、bytes、修改時間與原始位置。若要長期保存完整 log，應另存到不會被下一次 DCS 啟動覆寫的地方，再把保存位置寫入 `RUN.md`。

禁止：

- 事後手寫一份「看起來像 DCS」的 log。
- 沒執行就填 `PASS`。
- 只保存成功行、刪除同一 run 的錯誤行。
- 無法證明部署 binary 對應哪個 commit 時，假設它一定對應目前 HEAD。
