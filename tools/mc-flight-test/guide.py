"""Render the flight-test instructions and result sheet from one case list."""
import csv
import html
import io
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SCENARIOS = [
    ('A', '空中空載', '全部掛點維持空；機砲 100%，反制充足。用來測模式、保險、機砲、反制與畫面。'),
    ('B', '空中八枚 AAM＋兩架目標', 'RT／RO／LO／LT：AIM-9X 各一；RI／LI：AIM-7P 各一；MF／MB：AIM-120C 各一；MM：CLEAN。前方兩架不反擊、直飛的敵機，與玩家同高度，距離約 3 km、6 km；靠近至 1–3 km 測 IR。'),
    ('C', '空中混合掛載', 'RT：AIM-9X；LT：AIM-9L；RO／LO：Mk-82；RI／LI：330 加侖副油箱；MM：BRU-33＋兩枚 Mk-20；MF／MB：CLEAN。'),
    ('E', '空中全部 CLEAN', '全部九站明確選 <CLEAN>。'),
    ('F', '空中全部維持空', '全部九站維持未選掛載；與 E 分開存成另一任務。'),
    ('C-AG', '混合掛載的空對地飛彈', '任務編輯器有可用空對地飛彈時，將 C 的 MM 改成該飛彈並記下型號；其餘相同。'),
    ('D', '友軍機場熱機補給', '熱機停機位出生、機砲與反制充足，起始外掛維持空；選可提供地勤的友軍機場。'),
]
KEYS = [
    ('C++ Digital 熱焰彈／干擾絲（本包預設）', '左 Alt＋左 Shift＋Insert／左 Alt＋左 Shift＋Delete'),
    ('C++ Analog 熱焰彈／干擾絲（本包預設）', '左 Ctrl＋左 Shift＋Insert／左 Ctrl＋左 Shift＋Delete'),
    ('主模式 NAV／DGFT／MSL', '1／右 Ctrl＋D／4'),
    ('保險 ON／OFF／SIM', '右 Ctrl＋M／右 Shift＋M／右 Alt＋M'),
    ('第一段扳機／機砲', '左 Alt＋Space／Space'),
    ('Uncage（按住）／投放', '右 Alt＋右 Shift＋M／右 Alt＋Space'),
    ('TMS 上／下／左／右', 'Enter／Backspace／-／='),
    ('CMS Forward／Aft／Left', '右 Alt＋Delete／右 Alt＋Home／右 Alt＋Insert'),
    ('CMS Right／Press', '右 Alt＋End／右 Alt＋PgDn'),
    ('手動音效測試', '右 Shift＋T'),
    ('座艙／外部視角', 'F1／F2'),
    ('地勤通訊／離開任務選單', '反斜線鍵 \\／Esc'),
    ('搖桿預設機砲／投放', 'JOY_BTN1／JOY_BTN2'),
]

def cases():
    result = json.loads((ROOT / 'cases.json').read_text(encoding='utf-8'))
    assert len({case['id'] for case in result}) == len(result)
    return result

def markdown():
    data = cases()
    lines = ['# MC 完整實機測試流程', '',
        '使用本包 F-CK-1C 模組；依序做 A、B、C、E、F、C-AG、D。每項只需回報異常代號、實際情況；未測或被阻擋的項目請另列。', '',
        '## 安裝與開始', '',
        '1. 完全關閉 DCS，解壓縮測試包，在 F-CK-1C 資料夾執行 install.bat；預設安裝到 Saved Games\\DCS。使用 DCS.openbeta 或其他資料夾時，將該 Saved Games 路徑作為 install.bat 的第一個參數。',
        '2. 在任務編輯器依下表存好各任務；使用空中或熱機出生、關閉無限彈藥，選 JHMCS＋2D Overlay。全部保持專案預設按鍵。',
        '3. 進任務後看 HMCS 與右上 MC TEST 面板。記錄器會自動將每次任務另存到 Saved Games\\DCS\\Logs\\FCK1C-MC-TEST；視窗／按鍵設定採用現有配置。', '',
        '## 任務配置', '', '| 任務 | 配置 |', '|---|---|']
    lines += [f'| {code}：{title} | {detail} |' for code, title, detail in SCENARIOS]
    lines += ['', '可將同一份任務另存並改掛載。A 的機砲耗盡後，機砲相關項目需重新載入有彈任務；B 中標示「重新載入」的項目要補回原八枚配置。', '',
        '## 面板怎麼看', '',
        '- MC=1 表示 MC 快照可用；GEN 持續增加。ARM：0=SAFE、1=SIM、2=ON；MODE：0=NAV、1=DGF、2=MSL。',
        '- SUB：0=---、1=HMD、2=VS、3=HUD、4=BVR。UNCAGE、TRIG 分別為解籠與第二段扳機的 MC 按住狀態。',
        '- GUN OK／AAM OK=1 時，旁邊的 GUN／AAM 是有效存量；GUN 是原生實際存量。DGF 的 WPN 顯示 AAM 並不阻擋機砲。',
        '- SEL 是 Lua 最近完成選站呼叫的紀錄：0 RT、1 RO、2 RI、3 MM、4 MF、5 MB、6 LI、7 LO、8 LT；-1 表示邏輯選擇為空。實際選站以離架掛點再核對。',
        '- RDR 是 DCS 原始雷達模式；RDR=3 且 STT 為正距離表示來源回報單目標追蹤。這版 HMCS 中央圈是固定瞄準圖形，用右上數值確認追蹤。',
        '- IR OK=1、LOCK=1 表示 MC 已接受有效 IR 鎖定回報；鎖定音跟這個值。RDR=3 與 IR LOCK 分開看。',
        '- TX 是目前 DCS 動作視窗數量，ACK 是已執行前綴。停止操作後通常 1–2 秒回 TX=0；停超過 5 秒仍卡住請回報畫面與前一個操作。', '',
        '## C++ 反制驗證', '',
        '先做 A 的 N01–N09，再繼續原本 69 項 MC 測試。四個原生測試鍵已在本包綁定；原有鍵位保持。使用同一個記錄器與結果收集器。', '',
        'STAT：1=就緒、2=排隊、3=原生呼叫返回、4=Analog 放開返回、5=保險阻擋、6=具名入口缺少、7=呼叫失敗、8=離開任務、9=佇列已滿。數字 3／4 代表呼叫返回；實際成功還要看 FLR／CHF 減少及離機效果。', '',
        '若 CMOBS 某一位不可讀，該彈量顯示 -1，直接回報 CMOBS 數值與可見離機效果，原始記錄仍會保存。STAT=7 時停止原生測試，繼續原有 MC 測試；收集包會保留例外碼。', '',
        '## 預設快捷鍵', '', '| 功能 | 預設鍵 |', '|---|---|']
    lines += [f'| {name} | {key} |' for name, key in KEYS]
    lines += ['', 'K01 先確認預設 Uncage 與投放組合能同時送到 MC。若多個修飾鍵使投放沒有送達，保留原按鍵並回報 K01；後續依賴同時投放的項目填「按鍵阻擋」。已有獨立搖桿按鈕可照原配置測；純鍵盤的結果不代替獨立停止輸入測試。', '']
    for code, title, _ in SCENARIOS:
        lines += [f'## {code}：{title}', '']
        for c in [c for c in data if c['scenario'] == code]:
            lines += [f"### {c['id']}　{c['title']}", '', f"操作：{c['action']}", '', f"看哪裡：{c['look']}", '', f"正常：{c['normal']}", '']
    lines += ['## 測完回傳', '',
        '1. 完成後關閉 DCS，在解壓縮的測試包執行 COLLECT-RESULTS.bat。若 Saved Games 路徑不同，將該資料夾路徑作為第一個參數。',
        '2. 回傳產生的 FCK1C-MC-RESULTS-日期.zip，並寫「異常代號＋實際情況」及未測／阻擋代號；可選填 RESULTS.csv。將自己建立的 .miz 任務一併附上，供核對掛載、目標與彈量設定。',
        '3. 記錄器會保存各次任務的座艙參數、原始掛點字串、MC 快照、DCS 視窗／ACK、雷達與 IR 來源。收集包另含安裝 DLL 雜湊、DCS 日誌及既有輸入設定。', '',
        '## 驗證範圍', '',
        f'人工流程共 {len(data)} 項，涵蓋目前實作的模式、反制、掛載、選站、火控資格、捕獲、原生彈量、IR 狀態、座艙呈現與生命週期。', '',
        '逐站分類／時效、缺失觀測、錯誤／部分 ACK、80 ms 保持及跨 tick 順序，另由 CI 的原生／Lua 回歸測試注入並核對；人工只核對可操作的結果。原生測試的 C++ 派送使用獨立測試佇列，Lua 僅取樣實際反制存量。後續目標為透過 SystemPipeline 的 C++ 反制系統；本包保留現行 CMS 程序供回歸。座艙記錄是取樣，MC 與掛點的 coherent 欄位各自表達同一份完整來源版本；原始雷達與呈現值各自取樣。', '',
        '目前 SystemMonitor／FailureManager 是預留入口；冷啟動、供電、故障備援、A-G 發射規劃、HUD 掃描實作及自動 Seek／Acquire 音效留在後續功能範圍。', '',
        '本輪刻意保留的行為：保險與主模式獨立；機砲僅在 ON＋DGF＋未按 Uncage 時接受新開始；AAM 投放在 ON＋空戰模式＋Uncage 時接受新開始；已開始操作的主模式切換採原規則，OFF／SIM 才主動中止；鎖定音由有效 IR 來源回報決定。', '',
        '來源 commit、DLL SHA256 與 Windows CI 位於測試包根目錄 BUILD.json；實機項目在收到你的回報前維持未驗證。', '']
    return '\n'.join(lines)

def html_document():
    # 將同一份流程轉為可離線閱讀的測試卡片。
    lines = markdown().splitlines()
    parts = []
    table = False
    for line in lines:
        if line.startswith('|'):
            if not table: parts.append('<table>'); table = True
            if set(line.replace('|', '').strip()) <= {'-', ' '}:
                continue
            parts.append('<tr>' + ''.join('<td>' + html.escape(v.strip()) + '</td>' for v in line.strip('|').split('|')) + '</tr>')
            continue
        if table: parts.append('</table>'); table = False
        if line.startswith('### '):
            text = line[4:]; id = text.split('\u3000')[0]
            parts.append(f'<h3 id="{html.escape(id)}">{html.escape(text)}</h3>')
        elif line.startswith('## '): parts.append('<h2>' + html.escape(line[3:]) + '</h2>')
        elif line.startswith('# '): parts.append('<h1>' + html.escape(line[2:]) + '</h1>')
        elif line: parts.append('<p>' + html.escape(line) + '</p>')
    if table: parts.append('</table>')
    toc = ' '.join(f'<a href="#{c["id"]}">{c["id"]}</a>' for c in cases())
    return '''<!doctype html><html lang="zh-Hant"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>MC 完整實機測試流程</title><style>
body{font-family:system-ui,"Microsoft JhengHei",sans-serif;max-width:960px;margin:auto;padding:24px;background:#f4f6f8;color:#152331;line-height:1.85}h1{font-size:28px}h2{margin-top:48px;border-bottom:2px solid #267268}h3{scroll-margin-top:16px;margin:40px 0 12px;padding:12px 16px;background:#123c47;color:white;border-radius:7px}p{margin:9px 0;padding:0 12px}table{width:100%;border-collapse:collapse;background:white;font-size:15px}td{border:1px solid #ccd5dc;padding:9px 12px;vertical-align:top}tr:first-child{font-weight:700;background:#e6eef2}nav{background:#e8eeee;border-radius:8px;padding:12px}nav a{display:inline-block;padding:2px 6px;color:#14565b}footer{margin-top:32px;color:#52636b}@media print{body{padding:0;background:white;font-size:11pt}nav{display:none}h3{break-after:avoid}p{orphans:3;widows:3}}
</style><nav>項目索引：''' + toc + '</nav>' + '\n'.join(parts) + '<footer>用預設按鍵；回報異常代號與實際結果即可。</footer></html>\n'

def results_csv():
    stream = io.StringIO(newline='')
    writer = csv.writer(stream)
    writer.writerow(['代號', '任務', '測試項目', '結果（通過／異常／未測／阻擋）', '實際情況'])
    for c in cases(): writer.writerow([c['id'], c['scenario'], c['title'], '', ''])
    return stream.getvalue()
