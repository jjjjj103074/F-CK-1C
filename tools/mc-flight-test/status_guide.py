"""Human checks for the screen-fixed flight and weapon status panel."""
import csv
import html
import io

CASES = [
    dict(id='S01', title='畫面與飛行數據', action='進入空中開始的任務，按 F1 回座艙，使用原本的視角控制轉頭。',
         look='左上角。', normal='白色文字固定在畫面左上角，所有冒號對齊；IAS 以 kt、高度 ALT 以 ft 顯示並隨飛行更新。頭盔原本的 WPN／SYS／MODE／SUB 文字移到此面板。'),
    dict(id='S02', title='模式與主保險', action='按 2 選 A-A，按右 Ctrl＋D 選 DGFT，再按右 Shift＋D 回 CENTER。依序按右 Shift＋M、右 Alt＋M、右 Ctrl＋M。',
         look='MODE、OVRD、ARM。', normal='MODE 維持 A-A；OVRD 依序為 CENTER、DGFT、CENTER。ARM 依序顯示 SAFE、SIM、ON，模式不跟著保險改變。'),
    dict(id='S03', title='武器與選站', action='掛載至少兩種空對空飛彈、且同型掛在兩站。按 2 選 A-A；短按 Q 輪站，再按住 Q 超過 0.5 秒後放開以換型。',
         look='MISSILE、STATION、GUN。', normal='MISSILE 顯示已確認選站的型號與該站數量，例如 AIM-9M x1；STATION 顯示 RT／LT 等掛點。短按 Q 輪換同型掛點，長按 Q 換型；機砲彈量獨立顯示。切換期間可能短暫出現 SELECTING。'),
    dict(id='S04', title='發射後數量', action='按右 Ctrl＋D 選 DGFT、右 Ctrl＋M 設 ON。若 UNCAGE 為 OFF，按 C。將目標放到機鼻前方，按住右 Alt＋Space，飛彈離架後放開。',
         look='MISSILE、STATION；以 F2 外觀確認離架掛點，再按 F1 回座艙。', normal='顯示跟著 SM 更新數量及後續選站，與實際離架掛點一致；不把全部空對空飛彈總數當成單站數量。命中不是本項顯示測試的通過條件。'),
    dict(id='S05', title='機砲與反制數量', action='在 DGFT／ON 下按 Space 點射；按右 Ctrl＋J 施放程序 6。',
         look='GUN、CHAFF、FLARE。', normal='機砲實際射擊後 GUN 減少；程序 6 完成後 CHAFF 減少 7，FLARE 不變。任務須關閉無限彈藥。'),
    dict(id='S06', title='感測器狀態', action='按右 Ctrl＋D 選 DGFT，按下並放開 I 啟動 BORE 捕獲，機鼻朝向目標；按 K 取消雷達鎖定。按 C 切換解籠。',
         look='RADAR、IR、UNCAGE 及前視框。', normal='RADAR 區分 SEARCH／ACQUIRING／STT；IR 獨立顯示 LOCK／NO LOCK。UNCAGE 為 C 所選的 ON／OFF，與雷達鎖定分開。--- 表示來源不可用。'),
]


def cases():
    return CASES


def markdown():
    lines = ['# 左上角狀態顯示測試', '',
             '關閉 DCS，解壓縮測試包。將舊模組移出 Saved Games\\DCS\\Mods\\aircraft，再放入本包 F-CK-1C 資料夾。保留預設按鍵。', '',
             '使用空中開始的任務，關閉無限彈藥；掛載至少兩種空對空飛彈，同型放在兩個掛點。', '',
             'MODE＝保存的主模式；OVRD＝超控開關位置；ARM＝主保險。MISSILE 的 xN 是所選掛點數量。', '',
             'NONE＝未選擇；SELECTING＝正在選站；---＝來源不可用。IR 是原生全域紅外線觀測；UNCAGE 是駕駛所選的解籠命令狀態。', '']
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
    return '<!doctype html><html lang="zh-Hant"><meta charset="utf-8"><title>左上角狀態顯示測試</title><style>body{max-width:860px;margin:32px auto;padding:0 20px;font:18px/1.7 system-ui;background:#101914;color:#e6f0e8}h2{border-top:1px solid #34513e;padding-top:16px}</style>' + ''.join(body) + '</html>'


def results_csv():
    output = io.StringIO(newline='')
    writer = csv.writer(output)
    writer.writerow(['id', 'title', 'result', 'actual'])
    for case in CASES:
        writer.writerow([case['id'], case['title'], '', ''])
    return output.getvalue()
