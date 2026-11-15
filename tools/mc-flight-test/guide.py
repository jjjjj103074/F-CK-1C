"""Render the minimal C++ countermeasure test guide and result sheet."""
import csv
import html
import io
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
KEYS = [
    ('主保險 ON', '右 Ctrl＋M'),
    ('C++ 直接施放一發熱焰彈', '左 Alt＋左 Shift＋Insert'),
    ('C++ 直接施放一發干擾絲', '左 Alt＋左 Shift＋Delete'),
    ('外部視角／座艙', 'F2／F1'),
]

def cases():
    return json.loads((ROOT / 'cases.json').read_text(encoding='utf-8'))

def markdown():
    lines = ['# C++ 熱焰彈／干擾絲最小測試', '',
        '只做 N01–N03。一個空中任務，約一分鐘即可完成；測試兩種庫存讀取與各一發直接施放。', '',
        '## 安裝與任務', '',
        '1. 關閉 DCS。解壓縮本包，進入 F-CK-1C 資料夾，雙擊 install.bat；預設安裝到 Saved Games\\DCS。沿用原有操作按鍵，本包另附下表兩個測試鍵。',
        '2. 使用你原本的 F-CK-1C 空中任務，關閉「無限彈藥」，熱焰彈與干擾絲都要有庫存。外掛武器不限。特殊選項選用 JHMCS＋2D Overlay。',
        '3. 進任務後，右上應出現「C++ COUNTERMEASURE DIRECT TEST」面板。C++ 與 Lua 讀數會自動保存成 CSV。', '',
        '## 測試按鍵', '', '| 功能 | 本包預設鍵 |', '|---|---|']
    lines += [f'| {name} | {key} |' for name, key in KEYS]
    lines += ['', '## 測試順序', '',
        'N01 的 READ、MATCH、HOST 或 LUA OK 不符合正常值，或任何時候 ERROR 不為 0，停止施放並直接回收紀錄；回報停在哪一項及面板數值。', '']
    for c in cases():
        lines += [f"## {c['id']}　{c['title']}", '', c['action'], '', '看哪裡：'+c['look'], '', '正常：'+c['normal'], '']
    lines += ['## 回傳紀錄', '',
        '1. 測完按 Esc，點「結束任務」，再關閉 DCS。',
        '2. 回到本包最外層，雙擊 COLLECT-RESULTS.bat。它會產生 FCK1C-CM-DIRECT-RESULTS-日期.zip。',
        '3. 把這個 ZIP 回傳，並告訴我 N02 是否看到熱焰彈離機，以及有無異常。RESULTS.csv 可選填。',
        'Saved Games 使用其他位置時，可將該資料夾拖放到 COLLECT-RESULTS.bat。', '',
        '面板 STATUS=3 表示原生呼叫已返回，成功以庫存正確減少及熱焰彈離機為準。C++ 與 Lua 取樣時間不同，操作後等 3 秒再看穩定數字。', '',
        '紀錄包含：C++ 與 Lua 庫存、來源有效性、payload 比對、兩次呼叫前後庫存、執行緒、例外碼、DCS 日誌、安裝 DLL 雜湊與建置版本。', '']
    return '\n'.join(lines)

def html_document():
    parts = []
    table = False
    for line in markdown().splitlines():
        if line.startswith('|'):
            if set(line.replace('|', '').replace('-', '').strip()) == set():
                continue
            if not table:
                parts.append('<table>'); table = True
            parts.append('<tr>'+''.join('<td>'+html.escape(c.strip())+'</td>' for c in line.strip('|').split('|'))+'</tr>')
            continue
        if table:
            parts.append('</table>'); table = False
        if line.startswith('# '): parts.append('<h1>'+html.escape(line[2:])+'</h1>')
        elif line.startswith('## '):
            title = line[3:]
            ident = title.split('　', 1)[0]
            parts.append('<h2 id="'+html.escape(ident, quote=True)+'">'+html.escape(title)+'</h2>')
        elif line: parts.append('<p>'+html.escape(line)+'</p>')
    if table: parts.append('</table>')
    return '''<!doctype html><html lang="zh-Hant"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>C++ 反制物最小測試</title><style>
body{font-family:system-ui,"Microsoft JhengHei",sans-serif;max-width:880px;margin:auto;padding:24px;line-height:1.85;color:#152331;background:#f4f6f8}h1{font-size:28px}h2{margin-top:32px;border-bottom:2px solid #267268}table{width:100%;border-collapse:collapse;background:white}td{border:1px solid #ccd5dc;padding:8px 12px}tr:first-child{font-weight:700;background:#e6eef2}@media print{body{background:white;padding:0}h2{break-after:avoid}}
</style>'''+'\n'.join(parts)+'</html>\n'

def results_csv():
    stream = io.StringIO(newline='')
    writer = csv.writer(stream)
    writer.writerow(['代號', '測試項目', '結果', '實際情況'])
    for c in cases(): writer.writerow([c['id'], c['title'], '', ''])
    return stream.getvalue()
