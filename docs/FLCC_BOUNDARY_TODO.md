# FLCC 邊界整理代辦

## 已確認

- [x] 刪除參數化的 `make_flight_control_computer_system_entry`。正式 Entry 只回傳系統 ID 與延後建立實例的工廠；預設設定由 FLCC 建構子取得，沒有註冊時的設定複製。
- [x] 測試側以欄位修改函式提供選擇性差異；只有指定差異時，才複製一次完整預設設定、依序套用並驗證，工廠以值保存完整設定直到建立實例。沒有差異時沿用正式 Entry；執行週期不做差異合併。
- [x] 移除沒有參與正式排程的 `SystemGroup`，更新所有 Entry、測試與系統目錄文件。
- [x] `Entry.cpp` 維持回傳 `SystemEntry`；由 Entry 明確設定系統 ID，目錄產生器只收集各 Entry。
- [x] `Entry.cpp`、`FlightControlComputer.cpp`、`FlightControlExecutive.cpp` 及對應的 Computer／Executive 標頭採用 C++17 合併命名空間寫法，避免多層巢狀宣告。
- [x] 移除 `FlightControlComputer::step(FlightControlComputerStepInput)` 測試用入口；直接運算的測試改用 `FlightControlExecutive::update`，Computer 只保留 Pipeline 步進介面。
- [x] 移除 `FlightControlComputer` 的四個結果 getter；設定覆蓋測試改讀 Pipeline 發布的初始 FLCC 診斷快照。
- [x] 移除未被 Pipeline 呼叫的 `FlightControlComputer::handle_command()` 公開入口。Computer 在 `setup()` 向 Pipeline 直接登記指令綁定。
- [x] Executive 在每架飛機建構時彙整自身與 CommandSystem／AutomaticFlightControl 提供的 `CommandId` 和處理函式；Computer 將其轉接給 Pipeline，不再透過 Executive 二次分派。內部模組不依賴 `SystemSetup`。
- [x] 實驗性自動油門停用時不提供其指令綁定；CAT／開發指令仍保持原本的按鍵與可用性語意，AP／自動油門保留在週期中套用指令的佇列。原本直接呼叫 Executive 的測試改用同一份綁定。
- [x] 確認 Entry 經由 Computer 標頭引入內部標頭，是 Computer 以值持有 Executive 與遙測物件所需的完整型別依賴。保留直接擁有關係；目前沒有為縮短 include 鏈而加入 Pimpl 的需求。

## 下一步討論

- 檢視 `FlightControlExecutive::update()` 的更新順序、跨週期狀態與各內部模組依賴，決定哪些細節需要進一步整理。
