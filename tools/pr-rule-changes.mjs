import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const marker = '<!-- efm-pr-rule-notice -->';
const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

export function classifyRuleFile(filename) {
    const name = filename.replaceAll('\\', '/');
    if (/^\.github\/(workflows|actions)\//.test(name)) return 'CI 流程';
    if (/^src\/efm\/.*\.(vcxproj|props|targets|sln)$/.test(name) || /(^|\/)Directory\.Build\.(props|targets)$/.test(name)) return '編譯設定';
    if (name === '.husky/pre-commit' || name === '.husky/commit-msg' || name === '.gitattributes' || name === '.gitignore') return '提交設定';
    if (name === 'commitlint.config.js') return '標題規則';
    if (name === 'package.json' || name === 'package-lock.json') return '工具版本';
    if (/(^|\/)\.?_?clang-format(?:-ignore)?$/.test(name) || name === '.editorconfig') return '排版規則';
    if (/^src\/efm\/F-CK-1C_EFM_Tests\//.test(name) || name.endsWith('/EfmExports.baseline.txt')) return '測試與接口';
    if (/^tools\/(build_|efm-build|generate_)/.test(name)) return '建置腳本';
    if (/^tools\/.*\.(ps1|mjs|js)$/.test(name)) return '檢查腳本';
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
    if (!/^[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+$/.test(repository) || !Number.isSafeInteger(number) || number < 1 || !/^[a-f0-9]{40}$/i.test(head) || !/^[a-f0-9]{40}$/i.test(base)) throw new Error('PR 版本資料格式錯誤。');
    const prUrl = `https://github.com/${repository}/pull/${number}`;
    const lines = [marker, noticeHeading(rules.length, incomplete), ''];
    if (preview) lines.push('本機預覽', '');
    if (incomplete) lines.push(preview ? '變更清單不完整，請檢視本機差異。' : `變更清單不完整，請檢視[全部差異](${prUrl}/files)。`, '');
    else if (rules.length) lines.push('請檢視下列規則變更。', '');
    const statuses = {added: '新增', modified: '修改', removed: '刪除', renamed: '重新命名', copied: '複製', changed: '變更', unchanged: '未變更'};
    if (rules.length) lines.push('| 類型 | 檔案 | 變更 |', '|---|---|---|');
    for (const rule of rules) {
        const anchor = crypto.createHash('sha256').update(rule.filename).digest('hex');
        const fileLabel = inlineCode(rule.filename).replaceAll('|', '\\|');
        const link = preview ? fileLabel : `[${fileLabel}](${prUrl}/files#diff-${anchor})`;
        lines.push(`| ${rule.categories.join('、')} | ${link} | ${statuses[rule.status] ?? '變更'} +${rule.additions ?? 0} / −${rule.deletions ?? 0} |`);
        if (lines.join('\n').length > 22000) { lines.push('', '檔案清單已截短，請查看全部差異。'); break; }
    }
    if (rules.length) lines.push('', '<details>', '<summary>差異片段</summary>', '');
    for (const rule of rules) {
        const details = [`\n**${inlineCode(rule.filename)}** · 第 ${rule.line} 行`];
        if (rule.previous_filename) details.push(`原檔案：${inlineCode(rule.previous_filename)}`);
        details.push(rule.patch ? patchBlock(rule.patch) : '無文字片段，請查看檔案連結。');
        if (lines.join('\n').length + details.join('\n').length > 52000) { lines.push('', '片段已截短，完整內容見檔案連結。'); break; }
        lines.push(...details);
    }
    if (rules.length) lines.push('', '</details>');
    lines.push('', '<details>', '<summary>版本</summary>', '',
        ...(preview ? [`基準：${inlineCode(base)}`] : [`PR：${inlineCode(head)}`, '', `目標：${inlineCode(base)}`]), '', '</details>');
    return lines.join('\n');
}

function noticeHeading(count, incomplete) {
    if (incomplete) return '## ⚠️ 規則清單不完整';
    return count ? `## ⚠️ 規則變更：${count} 個檔案` : '## 規則未變更';
}

function annotationValue(value) { return String(value).replaceAll('%', '%25').replaceAll('\r', '%0D').replaceAll('\n', '%0A').replaceAll(',', '%2C').replaceAll(':', '%3A'); }

export async function publishNotice(event, request, writeSummary) {
    const expected = event.pull_request;
    if (!expected || !event.repository?.full_name) throw new Error('缺少 PR 事件資料。');
    const repository = event.repository.full_name;
    if (!/^[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+$/.test(repository) || !Number.isSafeInteger(expected.number)) throw new Error('Repository 或 PR 編號格式錯誤。');
    const route = `/repos/${repository}`;
    const current = await request(`${route}/pulls/${expected.number}`);
    if (!sameRevision(expected, current)) throw new Error('PR 版本已變更，請重跑規則檢查。');
    const files = [];
    for (let page = 1; page <= 30; page++) {
        const batch = await request(`${route}/pulls/${expected.number}/files?per_page=100&page=${page}`);
        if (!Array.isArray(batch)) throw new Error('變更清單格式錯誤。');
        files.push(...batch);
        if (batch.length < 100) break;
    }
    const rules = changedRules(files);
    const incomplete = files.length < current.changed_files;
    const body = renderNotice({ repository, number: expected.number, head: expected.head.sha, base: expected.base.sha, rules, incomplete });
    for (const rule of rules) console.log(`::warning file=${annotationValue(rule.filename)},line=${rule.line}::${annotationValue(rule.categories.join('、') + '變更')}`);
    let existing;
    for (let page = 1; page <= 30; page++) {
        const comments = await request(`${route}/issues/${expected.number}/comments?per_page=100&page=${page}`);
        existing = comments.find(c => c.user?.login === 'github-actions[bot]' && c.body?.startsWith(marker));
        if (existing || comments.length < 100) break;
    }
    if (!sameRevision(expected, await request(`${route}/pulls/${expected.number}`))) throw new Error('PR 版本已變更，請重跑規則檢查。');
    let notice = existing;
    if (existing) {
        if (existing.body !== body) await request(`${route}/issues/comments/${existing.id}`, { method: 'PATCH', body: { body } });
    } else if (rules.length || incomplete) {
        notice = await request(`${route}/issues/${expected.number}/comments`, { method: 'POST', body: { body } });
    }
    const prUrl = `https://github.com/${repository}/pull/${expected.number}`;
    const link = notice?.id ? `${prUrl}#issuecomment-${notice.id}` : prUrl;
    await writeSummary([noticeHeading(rules.length, incomplete), ...(rules.length || incomplete || existing ? ['', `[查看檔案與差異](${link})`] : [])].join('\n'));
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
    if (!token) throw new Error('缺少 GITHUB_TOKEN。');
    const api = process.env.GITHUB_API_URL ?? 'https://api.github.com';
    const request = async (route, options = {}) => {
        const response = await fetch(api + route, { method: options.method ?? 'GET', signal: AbortSignal.timeout(20000),
            headers: { Authorization: `Bearer ${token}`, Accept: 'application/vnd.github+json', 'Content-Type': 'application/json' },
            body: options.body ? JSON.stringify(options.body) : undefined });
        if (!response.ok) throw new Error(`GitHub 資料讀寫失敗：HTTP ${response.status} ${route}`);
        return response.json();
    };
    await publishNotice(event, request, body => fs.appendFileSync(process.env.GITHUB_STEP_SUMMARY, body + '\n'));
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    main().catch(error => { console.error(error.message); process.exitCode = 1; });
}
