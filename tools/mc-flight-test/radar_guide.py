"""Render the graphical radar flight checks and their expected visual results."""
import csv
import html
import io

CASES = [
    dict(id='R01', title='RWS 雷達圖', action='按 2 進入 A-A。', look='右下角。', normal='出現綠色方位—距離圖、距離格線及虛線掃描線；偵測到飛機後出現短橫線回波。'),
    dict(id='R02', title='移動游標', action='分別按住右 Alt＋I／K／J／L，再放開。', look='雷達圖內的白色括號。', normal='依序往上／下／左／右移動；放開後停止，機身操縱不受這組游標按鍵影響。'),
    dict(id='R03', title='RWS 捕獲與 STT', action='用右 Alt＋I／K／J／L 將白色括號移到回波；按下再放開 I。', look='雷達圖與前方目標。', normal='捕獲成功後，雷達圖出現實線鎖定框；前視出現完整實線方框，中心對準鎖定飛機。'),
    dict(id='R04', title='追蹤框與轉頭', action='保持 STT，使用原有視角控制左右轉頭，再看回目標；再按下並放開 I。', look='前視實線方框。', normal='方框跟隨飛機方向，不固定黏在畫面中央；重按 I 保留既有鎖定。'),
    dict(id='R05', title='取消鎖定', action='按 K。', look='前視與右下角雷達圖。', normal='實線追蹤框消失；RWS 恢復搜尋與回波顯示。'),
    dict(id='R06', title='BORE 待操作', action='按右 Ctrl＋D 進入 DGFT。', look='機鼻前方與雷達圖中央。', normal='前視出現只有四個角的矩形，穩定顯示；雷達圖顯示窄捕獲區，沒有來回的虛線掃描動畫。'),
    dict(id='R07', title='BORE 閃爍與自動捕獲', action='先讓目標離開機鼻四角框；按下再放開 I，再把機鼻對準目標。', look='前視四角框。', normal='等待捕獲時四角框閃爍；目標進入捕獲範圍並鎖定後，變成完整實線方框並追蹤目標。I 放開後仍持續捕獲。'),
    dict(id='R08', title='STT 離開原捕獲框', action='保持鎖定，正常操縱飛機，讓目標稍微離開原本機鼻捕獲位置。', look='前視實線方框與雷達圖鎖定框。', normal='只要原生 STT 仍有效，方框繼續跟隨目標，不因離開原 BORE 角窗就自行消失。'),
    dict(id='R09', title='停止 BORE 捕獲', action='按 K。', look='前視框。', normal='恢復穩定的四角矩形；停止自動捕獲，直到再次按下並放開 I。'),
    dict(id='R10', title='NAV 與重新進入', action='按 1 回 NAV，再按 2 回 A-A。', look='前視與右下角。', normal='NAV 時新雷達圖形隱藏；回 A-A 後恢復 RWS 雷達圖，不殘留舊的前視追蹤框。'),
]

def cases():
    return CASES

def markdown():
    lines = ['# 雷達圖形實機測試', '',
        '## 安裝與任務', '',
        '1. 關閉 DCS，解壓縮測試包，先將舊版 F-CK-1C 資料夾移出 Saved Games\\DCS\\Mods\\aircraft，再放入測試包的 F-CK-1C 資料夾，避免沿用舊測試檔案。',
        '2. 使用空中起飛任務，在正前方約 3–6 km、相近高度放一架不反擊的敵機。這次只測雷達，主保險任一位置皆可，無須發射武器。',
        '3. 進入任務後按 F1 回座艙。新雷達圖形會隨空戰模式自動出現，維持預設按鍵即可。', '',
        '## 按鍵', '', '| 功能 | 預設鍵 |', '|---|---|',
        '| A-A／NAV | 2／1 |', '| DGFT／退出超控 | 右 Ctrl＋D／右 Shift＋D |',
        '| 捕獲／取消 | I（按下再放開）／K |',
        '| 雷達游標上／下／左／右 | 按住右 Alt＋I／K／J／L；放開停止 |',
        '| 座艙視角 | F1 |', '| 詳細 debug（需要時） | 右 Ctrl＋右 Shift＋F12 |', '',
        '## 看圖', '',
        '- 機鼻四角矩形：BORE 捕獲區；穩定表示待操作，閃爍表示捕獲中。',
        '- 前視完整實線方框：原生 STT 鎖定目標。轉頭時它仍應對準目標方向。',
        '- 雷達圖短橫線：原生搜尋回波，舊回波逐漸淡出；白色括號是游標，完整方框是 STT 目標。',
        '- 雷達圖實線邊界：掃描範圍。來回的虛線是估算動畫，用來辨認 RWS 搜尋；BORE 為固定窄區。',
        '- 雷達圖顯示紅色叉號：來源目前不可用；請回報該畫面。', '',
        '## 測試項目', '']
    for case in CASES:
        lines += [f"### {case['id']}　{case['title']}", '', f"操作：{case['action']}", '',
                  f"看哪裡：{case['look']}", '', f"正常：{case['normal']}", '']
    lines += ['## 回傳', '',
        '測完關閉 DCS，在解壓縮的測試包執行 COLLECT-RESULTS.bat，回傳產生的 FCK1C-MC-RESULTS-日期.zip。',
        '有異常時附上項目代號、實際看到的情況與畫面截圖；任務 .miz 也可一起回傳，方便核對目標位置。',
        '來源版本與 DLL 雜湊在 BUILD.json；紀錄會保存原生雷達、繪圖參數、回波來源編號及原有 debug.csv。', '']
    return '\n'.join(lines)

def html_document():
    parts = []
    table = False
    for line in markdown().splitlines():
        if line.startswith('|'):
            if not table:
                parts.append('<table>')
                table = True
            if set(line.replace('|', '').strip()) <= {'-', ' '}:
                continue
            parts.append('<tr>' + ''.join('<td>' + html.escape(value.strip()) + '</td>' for value in line.strip('|').split('|')) + '</tr>')
            continue
        if table:
            parts.append('</table>')
            table = False
        if not line:
            continue
        tag = 'h1' if line.startswith('# ') else 'h2' if line.startswith('## ') else 'h3' if line.startswith('### ') else 'p'
        text = line.lstrip('# ') if line.startswith('#') else line
        parts.append(f'<{tag}>' + html.escape(text) + f'</{tag}>')
    return '<!doctype html><html lang="zh-Hant"><meta charset="utf-8"><title>雷達圖形測試</title><style>body{max-width:880px;margin:32px auto;padding:0 20px;font:17px/1.7 system-ui;background:#101914;color:#e6f0e8}h1,h2,h3{color:#8affad}h3{border-top:1px solid #34513e;padding-top:16px}p{margin:8px 0}table{border-collapse:collapse;width:100%}td{padding:8px 12px;border:1px solid #34513e}tr:first-child{font-weight:bold}</style>' + ''.join(parts) + '</html>'

def results_csv():
    output = io.StringIO(newline='')
    writer = csv.writer(output)
    writer.writerow(['id', 'title', 'result', 'actual'])
    for case in CASES:
        writer.writerow([case['id'], case['title'], '', ''])
    return output.getvalue()
