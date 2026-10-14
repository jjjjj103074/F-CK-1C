import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const marker = '<!-- efm-pr-rule-notice -->';
const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

export function classifyRuleFile(filename) {
    const name = filename.replaceAll('\\', '/');
    if (/^\.github\/(workflows|actions)\//.test(name)) return 'CI 觸發、權限與執行流程';
    if (/^src\/efm\/.*\.(vcxproj|props|targets|sln)$/.test(name) || /(^|\/)Directory\.Build\.(props|targets)$/.test(name)) return 'DLL 編譯／連結與工具鏈';
    if (name === '.husky/pre-commit' || name === '.gitattributes' || name === '.gitignore') return '提交、建置輸入與來源正規化';
    if (name === 'package.json' || name === 'package-lock.json') return '工具版本與指令入口';
    if (/(^|\/)\.?_?clang-format(?:-ignore)?$/.test(name) || name === '.editorconfig') return 'C++ 排版驗證規則';
    if (/^src\/efm\/F-CK-1C_EFM_Tests\//.test(name) || name.endsWith('/EfmExports.baseline.txt')) return '測試內容與接口基準';
    if (/^tools\/(build_|efm-build|generate_)/.test(name)) return 'DLL 建置與來源產生流程';
    if (/^tools\/.*\.(ps1|mjs|js)$/.test(name)) return '檢查、測試與結果判定程式';
    return null;
}

export function changedRules(files) {
    return files.flatMap(file => {
        const categories = [...new Set([classifyRuleFile(file.filename), file.previous_filename && classifyRuleFile(file.previous_filename)].filter(Boolean))];
        if (!categories.length) return [];
        const match = /^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@/m.exec(file.patch ?? '');
        return [{ ...file, categories, line: Math.max(1, Number(match?.[1] ?? 1)) }];
    });
}

export function sameRevision(expected, current) {
    return current.state === 'open' && expected.head.sha === current.head.sha && expected.base.sha === current.base.sha;
}

function inlineCode(value) {
    const longest = Math.max(0, ...[...String(value).matchAll(/`+/g)].map(m => m[0].length));
    return '`'.repeat(longest + 1) + ' ' + String(value).replaceAll('\n', '\\n').replaceAll('\r', '\\r') + ' ' + '`'.repeat(longest + 1);
}

function patchBlock(value) {
    const shortened = value.split('\n').slice(0, 45).map(line => line.slice(0, 600)).join('\n');
    const longest = Math.max(2, ...[...shortened.matchAll(/`+/g)].map(m => m[0].length));
    const fence = '`'.repeat(longest + 1);
    return `${fence}diff\n${shortened}\n${fence}`;
}

export function renderNotice({ repository, number, head, base, rules, incomplete = false, preview = false }) {
    if (!/^[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+$/.test(repository) || !Number.isSafeInteger(number) || number < 1 || !/^[a-f0-9]{40}$/i.test(head) || !/^[a-f0-9]{40}$/i.test(base)) throw new Error('Invalid PR identity.');
    const prUrl = `https://github.com/${repository}/pull/${number}`;
    const lines = [marker, rules.length || incomplete ? '## ⚠️ 建置／驗證規則變更：請人工檢視' : '## 建置／驗證規則：未偵測到變更', '',
        preview ? `本機工作目錄預覽；比較基準：${inlineCode(base)}，尚未對應 GitHub PR。` : `PR 提交：${inlineCode(head)}　目標提交：${inlineCode(base)}`, '',
        '這是提醒，不是新的人工核准門檻。自動檢查通過不代表變更後的規則已經人工確認。', ''];
    if (incomplete) lines.push('⚠️ GitHub 未提供完整變更清單，不能判定所有規則；請檢視 PR 的完整檔案差異。', '');
    lines.push('| 規則類型 | 具體檔案 | 狀態 | 行數變更 | 差異 |', '|---|---|---|---|---|');
    for (const rule of rules) {
        const anchor = crypto.createHash('sha256').update(rule.filename).digest('hex');
        const fileLabel = inlineCode(rule.filename).replaceAll('|', '\\|');
        const link = preview ? '本機差異見下方' : `[檢視差異](${prUrl}/files#diff-${anchor})`;
        lines.push(`| ${rule.categories.join('、')} | ${fileLabel} | ${rule.status} | +${rule.additions ?? 0} / −${rule.deletions ?? 0} | ${link} |`);
        if (lines.join('\n').length > 22000) { lines.push('', '其餘檔案請查看 PR 完整差異；摘要已截短。'); break; }
    }
    for (const rule of rules) {
        const details = [`\n**${inlineCode(rule.filename)}** — ${rule.categories.join('、')}，變更區段約在第 ${rule.line} 行。`];
        if (rule.previous_filename) details.push(`原檔案：${inlineCode(rule.previous_filename)}`);
        details.push(rule.patch ? patchBlock(rule.patch) : 'GitHub 未提供文字差異；請由上方連結檢視。');
        if (lines.join('\n').length + details.join('\n').length > 52000) { lines.push('', '差異片段已截短；上方檔案連結提供完整內容。'); break; }
        lines.push(...details);
    }
    return lines.join('\n');
}

function annotationValue(value) { return String(value).replaceAll('%', '%25').replaceAll('\r', '%0D').replaceAll('\n', '%0A').replaceAll(',', '%2C').replaceAll(':', '%3A'); }

export async function publishNotice(event, request, writeSummary) {
    const expected = event.pull_request;
    if (!expected || !event.repository?.full_name) throw new Error('A pull_request_target event is required.');
    const repository = event.repository.full_name;
    if (!/^[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+$/.test(repository) || !Number.isSafeInteger(expected.number)) throw new Error('Invalid repository or PR number.');
    const route = `/repos/${repository}`;
    const current = await request(`${route}/pulls/${expected.number}`);
    if (!sameRevision(expected, current)) throw new Error('PR identities changed; rerun the notice for the latest revision.');
    const files = [];
    for (let page = 1; page <= 30; page++) {
        const batch = await request(`${route}/pulls/${expected.number}/files?per_page=100&page=${page}`);
        if (!Array.isArray(batch)) throw new Error('Invalid changed-file response.');
        files.push(...batch);
        if (batch.length < 100) break;
    }
    const rules = changedRules(files);
    const incomplete = files.length < current.changed_files;
    const body = renderNotice({ repository, number: expected.number, head: expected.head.sha, base: expected.base.sha, rules, incomplete });
    await writeSummary(body);
    for (const rule of rules) console.log(`::warning file=${annotationValue(rule.filename)},line=${rule.line}::${annotationValue(rule.categories.join('、') + '變更；請人工檢視具體差異。')}`);
    let existing;
    for (let page = 1; page <= 30; page++) {
        const comments = await request(`${route}/issues/${expected.number}/comments?per_page=100&page=${page}`);
        existing = comments.find(c => c.user?.login === 'github-actions[bot]' && c.body?.startsWith(marker));
        if (existing || comments.length < 100) break;
    }
    if (!sameRevision(expected, await request(`${route}/pulls/${expected.number}`))) throw new Error('PR changed during notice generation; no stale notice was published.');
    if (existing) {
        if (existing.body !== body) await request(`${route}/issues/comments/${existing.id}`, { method: 'PATCH', body: { body } });
    } else if (rules.length || incomplete) {
        await request(`${route}/issues/${expected.number}/comments`, { method: 'POST', body: { body } });
    }
    return { rules, incomplete };
}

async function preview(base) {
    const git = args => execFileSync('git', args, { cwd: repositoryRoot, encoding: 'utf8', maxBuffer: 8 * 1024 * 1024 });
    const commit = git(['rev-parse', '--verify', '--end-of-options', `${base}^{commit}`]).trim();
    const changes = git(['diff', '--name-status', '--no-renames', '-z', commit]).split('\0').filter(Boolean);
    const statusByFile = new Map();
    for (let i = 0; i < changes.length; i += 2) statusByFile.set(changes[i + 1], changes[i]);
    const tracked = [...statusByFile.keys()];
    const untracked = git(['ls-files', '--others', '--exclude-standard', '-z']).split('\0').filter(Boolean);
    const files = [...new Set([...tracked, ...untracked])].filter(classifyRuleFile).map(filename => {
        const isNew = untracked.includes(filename);
        const content = isNew ? fs.readFileSync(path.join(repositoryRoot, filename), 'utf8').replaceAll('\r\n', '\n') : '';
        const addedLines = isNew ? content.replace(/\n$/, '').split('\n') : [];
        const patch = isNew ? `@@ -0,0 +1,${addedLines.length} @@\n` + addedLines.map(line => '+' + line).join('\n') : git(['diff', '--no-ext-diff', commit, '--', filename]);
        const status = isNew || statusByFile.get(filename) === 'A' ? 'added' : statusByFile.get(filename) === 'D' ? 'removed' : 'modified';
        return { filename, status, patch, additions: patch.split('\n').filter(l=>l.startsWith('+')&&!l.startsWith('+++')).length, deletions: patch.split('\n').filter(l=>l.startsWith('-')&&!l.startsWith('---')).length };
    });
    return renderNotice({ repository: 'jjjjj103074/F-CK-1C', number: 1, head: git(['rev-parse','HEAD']).trim(), base: commit, rules: changedRules(files), preview: true });
}

async function main() {
    if (process.argv[2] === '--preview') { console.log(await preview(process.argv[3] ?? 'HEAD')); return; }
    const event = JSON.parse(fs.readFileSync(process.env.GITHUB_EVENT_PATH, 'utf8'));
    const token = process.env.GITHUB_TOKEN;
    if (!token) throw new Error('GITHUB_TOKEN is required for the metadata-only notice job.');
    const api = process.env.GITHUB_API_URL ?? 'https://api.github.com';
    const request = async (route, options = {}) => {
        const response = await fetch(api + route, { method: options.method ?? 'GET', signal: AbortSignal.timeout(20000),
            headers: { Authorization: `Bearer ${token}`, Accept: 'application/vnd.github+json', 'Content-Type': 'application/json' },
            body: options.body ? JSON.stringify(options.body) : undefined });
        if (!response.ok) throw new Error(`GitHub metadata request failed: ${response.status} ${route}`);
        return response.json();
    };
    await publishNotice(event, request, body => fs.appendFileSync(process.env.GITHUB_STEP_SUMMARY, body + '\n'));
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    main().catch(error => { console.error(error.message); process.exitCode = 1; });
}
