"""Human checks for the screen-fixed flight and weapon status panel."""
import csv
import html
import io

CASES = [
    dict(id='S01', title='進場與畫面', action='空中開始，按 F1 回座艙，維持 NAV 飛行 30 秒，再使用原本視角控制轉頭。',
         look='左上角、頭盔航向文字。', normal='左上角為小型綠字、透明背景、所有冒號對齊；頭盔文字大小正常，飛行期間無閃退。'),
    dict(id='S02', title='AIM-9 群組與數量', action='掛載 AIM-9P3、AIM-9P5、AIM-9M、AIM-7P、AIM-120C。按 2 進 A-A；短按 Q 輪站，按住 Q 超過 0.5 秒再放開換群組。',
         look='MISSILE、STATION。', normal='AIM-9 顯示全機該群組總數；短按 Q 可在不同 AIM-9 型號的掛點間輪換，總數保持。STATION 顯示實際掛點與該枚型號。長按 Q 才離開 AIM-9 群組。'),
    dict(id='S03', title='原生解籠控制', action='長按 Q 超過 0.5 秒後放開，循環回 AIM-9 群組，再短按 Q 選 AIM-9P5（也可重做 AIM-9P3／M）。按右 Ctrl＋M 設 ON。面向前方敵機，觀察 UNCAGE；按 C，等待一秒，再按 C。',
         look='前視空心圓、菱形、左上角 IR 與 UNCAGE。', normal='空心圓代表原生要求方向；原生捕獲到 IR 目標才有菱形及 IR LOCK。UNCAGE 依原生控制讀回 OFF→ON→OFF；WAIT 表示原生控制尚未取得有效讀回，請回傳紀錄。C 操作保持原雷達模式與鎖定。'),
    dict(id='S04', title='雷達指向與自主追蹤', action='長按 Q 超過 0.5 秒後放開直到選中 AIM-9 群組，再短按 Q 選 AIM-9M。按右 Ctrl＋D 進 DGFT，按下再放開 I 啟動 BORE 捕獲，將機鼻對準前方敵機。UNCAGE ON 時按 C 回 OFF；取得 IR LOCK 後再按 C 解籠，緩慢移動機鼻。',
         look='雷達方框、IR 空心圓與菱形。', normal='STT 方框跟隨雷達目標。AIM-9M／X 在 caged 時接收雷達指向；解籠後使用原生 IR 自主追蹤，菱形是否保持跟隨以原生尋標器實際捕獲結果為準。'),
    dict(id='S05', title='投放與下一枚', action='在 ON、A-A 或 DGFT 下，取得可靠 IR 訊號；若 UNCAGE OFF，按 C 解籠，ON 則保持，觀察追蹤；按住右 Alt＋Space，離架後放開。按 F2 確認離架，再按 F1。',
         look='MISSILE、STATION、UNCAGE、雷達方框。', normal='離架後群組總數減一；SM 選到下一枚時，掛點更新、UNCAGE 回 OFF。放開投放鍵保留原生雷達 STT。'),
    dict(id='S06', title='解籠與投放資格分開', action='在 ON 的 A-A 模式下選 AIM-9；若 UNCAGE ON，按 C 回 OFF。按住右 Alt＋Space 一次，離架或等待一秒後放開。',
         look='外部掛載、MISSILE、回傳紀錄。', normal='MC 送出正常投放要求，C 的開關狀態不再作為所有飛彈共同的發射保險；最終是否離架由原生條件決定。本項不以命中判定。'),
    dict(id='S07', title='BORE 窄區捕獲', action='按右 Ctrl＋D 進 DGFT。按下再放開 I，讓前方敵機進入窄框。',
         look='前視捕獲框、RADAR、右下角雷達圖。', normal='RADAR 顯示 ACM/BORE；捕獲框閃爍，捕獲成功後變成跟隨敵機的完整方框，雷達回報 STT。'),
    dict(id='S08', title='30×20 寬區', action='直接按 L 選寬區，讓敵機位於機鼻左右約 10 度範圍內。',
         look='前視捕獲框、RADAR、右下角掃描邊界。', normal='RADAR 顯示 ACM/30x20；捕獲區域比 BORE 寬，啟動自動捕獲，成功後進入 STT。'),
    dict(id='S09', title='10×60 垂直區', action='直接按 K 選垂直區。讓敵機位於機鼻上方、水平接近中央。',
         look='前視捕獲框、RADAR。', normal='RADAR 顯示 ACM/10x60；狹長窗口朝機鼻參考線上方延伸，上緣可能超出畫面。目標進入區域後自動捕獲並進入 STT。'),
    dict(id='S10', title='返回 RWS', action='按右 Shift＋D 退出超控。若 MODE 不是 A-A，按 2。若 RADAR 顯示 STT，先按 K 回搜尋。按住右 Alt＋I／K／J／L 移動雷達游標，放開停止；按下再放開 I 申請捕獲，STT 後按 K 脫鎖。',
         look='RADAR、右下角游標及目標方框。', normal='回到 CRM/RWS，已有 STT 保留；先按 K 返回搜尋後，可移動游標並重新捕獲。'),
    dict(id='S11', title='主保險與機砲', action='按右 Ctrl＋D 進 DGFT；按右 Shift＋M 設 SAFE，按 Space 及右 Alt＋Space 各一次。按右 Alt＋M 設 SIM，重複兩鍵。按右 Ctrl＋M 設 ON，再按 Space 點射。',
         look='ARM、GUN、外部掛載。', normal='SAFE、SIM 阻止實際機砲與投放；ON＋DGFT 機砲可射擊，彈量減少，與 C 狀態獨立。'),
    dict(id='S12', title='反制程序', action='按右 Ctrl＋J 啟動程序 6，等待完成。',
         look='CHAFF、FLARE。', normal='程序 6 施放 7 發干擾絲，FLARE 不變。'),
    dict(id='S13', title='空戰模式保留雷達目標', action='按右 Shift＋D 回 CENTER；若 MODE 不是 A-A，按 2。使用右 Alt＋I／K／J／L 將游標移近目標，按下再放開 I，等待 RADAR 顯示 STT。依序按右 Ctrl＋D、右 Alt＋D、右 Shift＋D。',
         look='RADAR、STT 方框。', normal='A-A→DGFT→MSL OVRD→A-A 保留相同鎖定；在 DGFT 中按 I／K／L 才拒絕舊鎖並分別重新捕獲 BORE／10x60／30x20。'),
    dict(id='S14', title='AIM-7 正常發射流程', action='按 2 進 A-A（若已是 A-A 保持）。按右 Ctrl＋M 設 ON，長按 Q 超過 0.5 秒後放開，直到選中 AIM-7P。若有 STT，按 K 脫鎖。按右 Alt＋Space 再放開；再按下放開 I 捕獲目標，取得 STT 後按右 Alt＋Space，離架後放開投放鍵；保持 STT 至命中或飛彈飛行結束，再做下一項。',
         look='STATION、RELEASE、外部掛載。', normal='無 STT 時 RELEASE＝NEED STT，MC 不提出發射；有 STT 時＝RADAR TARGET，正常送出投放。未實作 FLOOD。本項觀察離架及持續追蹤，不以命中代替程序判定。'),
    dict(id='S15', title='AIM-120 目視發射同意', action='留在 ON、A-A。長按 Q 超過 0.5 秒後放開直到選中 AIM-120C。若有 STT，按 K 脫鎖。先按右 Alt＋Space 後放開。再按住 C，同時按右 Alt＋Space，離架後放開兩鍵。',
         look='RELEASE、UNCAGE、外部掛載。', normal='無指定目標且未按 C：HOLD C FOR VISUAL，不提出發射。按住 C：VISUAL／HELD，可送出正常投放；放開 C 後同意結束。此路徑不保證命中。'),
    dict(id='S16', title='AIM-120 有指定目標', action='保留至少一枚 AIM-120C，留在 ON、A-A。使用右 Alt＋I／K／J／L 移動游標，按下再放開 I 捕獲，等待 STT；放開 C，按右 Alt＋Space 發射，離架後放開。關閉 DCS 後執行 COLLECT-RESULTS.bat。',
         look='RELEASE、STT 方框、MISSILE、回收 ZIP。', normal='有 STT 時 RELEASE＝RADAR TARGET，不需要按住 C；放開投放後仍保留 STT，群組數量隨原生庫存減少。回傳完整 ZIP。'),

]



def cases():
    return CASES


def markdown():
    lines = ['# 空對空武器、尋標器與 ACM 測試', '',
             '關閉 DCS，解壓縮測試包。將舊模組移出 Saved Games\\DCS\\Mods\\aircraft，再放入本包 F-CK-1C 資料夾。保留預設按鍵。', '',
             '以下依 S01→S16 順序測試。使用空中開始的任務，關閉無限彈藥；混掛 AIM-9P3／P5／M 各至少一枚、AIM-7P 至少一枚、AIM-120C 至少兩枚，前方約 3–5 NM 配置一架非交戰敵機。', '',
             'MODE＝保存的主模式；OVRD＝超控開關位置；ARM＝主保險。MISSILE 的 xN 是目前選擇群組的全機總數；STATION 顯示掛點與實際型號。', '',
             '右 Ctrl＋右 Shift＋F12 可開啟詳細 debug；再按一次關閉，左上角面板恢復。NONE＝未選擇；SELECTING＝正在選站；---＝來源不可用。IR 是原生全域紅外線觀測；選 AIM-9 時 UNCAGE 是原生自主追蹤讀回；選 AIM-120 時是 C 是否按住。RELEASE 表示 MC 程序判定。WAIT 表示控制來源尚未確認。', '']
    for case in CASES:
        lines += [f"## {case['id']}　{case['title']}", '', f"操作：{case['action']}", '',
                  f"看哪裡：{case['look']}", '', f"正常：{case['normal']}", '']
    lines += ['## 回傳', '', '關閉 DCS，執行本包 COLLECT-RESULTS.bat，回傳產生的 FCK1C-MC-RESULTS-日期.zip。異常請附項目代號、實際情況與截圖。', '']
    return '\n'.join(lines)


def html_document():
    body = []
    for line in markdown().splitlines():
        tag = 'h1' if line.startswith('# ') else 'h2' if line.startswith('## ') else 'p'
        text = line.lstrip('# ') if line.startswith('#') else line
        if text:
            body.append(f'<{tag}>' + html.escape(text) + f'</{tag}>')
    return '<!doctype html><html lang="zh-Hant"><meta charset="utf-8"><title>空對空武器、尋標器與 ACM 測試</title><style>body{max-width:860px;margin:32px auto;padding:0 20px;font:18px/1.7 system-ui;background:#101914;color:#e6f0e8}h2{border-top:1px solid #34513e;padding-top:16px}</style>' + ''.join(body) + '</html>'


def results_csv():
    output = io.StringIO(newline='')
    writer = csv.writer(output)
    writer.writerow(['id', 'title', 'result', 'actual'])
    for case in CASES:
        writer.writerow([case['id'], case['title'], '', ''])
    return output.getvalue()
