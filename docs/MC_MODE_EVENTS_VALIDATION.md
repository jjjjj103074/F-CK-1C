# MC 模式事件重構驗證紀錄

- 比較基準：`9eb8ff319e822f11aa0f747a4c6aeb282b7149ef`。
- 最終執行期來源：`5ca3675a5b31aee58340d77edea7cde19f4085eb`。
- 來源 tree：`1703f331fb3c80b26e69a8a71876539aac475487`。
- [Windows 完整驗證](https://github.com/jjjjj103074/F-CK-1C/actions/runs/41201960741)：passed。
- 架構、資料流與指令流見 [MC 模式事件重構](MC_MODE_EVENTS.md)。

## 來源提交

| Commit | 內容 |
|---|---|
| `5bab676` | chore(efm): vendor sigslot for shared synchronous events |
| `906ddd5` | refactor(efm): decouple MC mode handling with ordered events |
| `5ca3675` | docs(efm): clarify MC component behavior comments |

三筆來源提交完成後，以最終來源執行完整 Windows 驗證。

## 本機驗證

| 項目 | 結果 |
|---|---|
| MC／模式 C++17 回歸 | 219 checks，0 failures |
| Exporter 暫存相容 fixture | 122 checks，0 failures |
| Lua 5.1.5 介面／顯示回歸 | 6,169 checks，0 failures |
| Node.js 22 工具回歸 | 84 passed、5 個 Windows 限定 skipped、0 failed |
| clang-format 22.1.8 | 完整範圍 276 檔通過；各筆提交另核對 index |
| EFM 架構及檢查器 fixtures | passed |
| git diff --check | passed |

Linux Exporter fixture 的 Windows API stub 與產生標頭適配細節見架構文件。
Windows CI 使用正式來源、SDK 與產生檔進行建置與驗證。

## Standards

來源變更：0 hard violations、0 heuristic findings。
凍結審查 patch SHA256：`e92291850f0b20fe438b0116a42832fd440e1262fd2a7a9ee441fd704c49ce66`。

## Spec

來源變更：0 findings。已核對既有模式／主保險連動、CMS、FIFO、事件順序、
訂閱生命週期、參數映射，以及這次要求的精簡功能註解。
已提交來源差異 SHA256：`3408e35b02f1fb44a91f7833e2f3ee88e65c0348e9d85821a2f0c026ac2880cd`。

## Windows 完整驗證

全專案原生測試：**3,746 checks，0 failures**。
DLL 與測試均完成指定工具鏈的 MSVC 靜態分析。

| 檢查 | 結果 |
|---|---|
| 提交版本 | passed |
| 編譯工具 | passed |
| DLL 編譯／分析 | passed |
| DLL 雜湊 | passed |
| DLL 接口 | passed |
| DLL 載入／卸載 | passed |
| 架構測試 | passed |
| 系統目錄產生器 | passed |
| DCS ID 產生器 | passed |
| 座艙架構 | passed |
| 座艙架構測試 | passed |
| 驗證工具 | passed |
| 分析規則測試 | passed |
| 測試編譯／分析 | passed |
| 原生測試 | passed |
| 來源完整性 | passed |

工具鏈為專案指定 v142：MSVC 19.29.30159.0、link 14.29.30159.0、
SDK 10.0.26100.0、MSBuild 18.10.1.42706。
首次建置 DLL 與完整驗證中的乾淨重建 DLL 雜湊相同，接口及載入／卸載檢查通過。

## DLL 與來源憑證

- DLL：`bin/F-CK-1C_EFM.dll`，931,840 bytes。
- DLL SHA256：`fd684a367916dc5be37f36593bc339c75d41f6a4acf2337a82f1b1bb867f231e`。
- [來源憑證](../bin/F-CK-1C_EFM.validation.json)：來源 commit、tree 與 CI run 均符合上述版本。
- 產物 ZIP SHA256：`f44f3a2934038db20ae8e12601f18a9c38c8b40dd1bde89b5264c219ed4fa6d0`。
- 驗證證據 ZIP SHA256：`9df3ccfef493bca2fabdb0a966f4831ea852c8134b6739f68594efe5ba894866`。

已核對 GitHub 產物摘要、DLL 雜湊、憑證與完整驗證報告中的上傳／重建雜湊。
產物提交包含 DLL、憑證及驗證文件，執行期來源維持上述已驗證版本。

## DCS

DCS 遊戲內驗證尚未執行；人工驗證依 [DCS 測試](DCS_TESTING.md) 記錄。
