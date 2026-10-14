import test from 'node:test';
import assert from 'node:assert/strict';
import { classifyRuleFile, changedRules, renderNotice, publishNotice } from './pr-rule-changes.mjs';

const head = 'a'.repeat(40), base = 'b'.repeat(40);
const expected = { number: 7, head: {sha: head}, base: {sha: base} };
const event = { repository: {full_name: 'owner/repo'}, pull_request: expected };
const current = { ...expected, state: 'open', changed_files: 1 };
const file = { filename: 'src/efm/EfmBuild.props', status: 'modified', additions: 1, deletions: 1,
    patch: '@@ -8,1 +8,1 @@\n-<EfmVCToolsVersion>old</EfmVCToolsVersion>\n+<EfmVCToolsVersion>new</EfmVCToolsVersion>' };

test('build, CI, validation and baseline changes are distinguished from ordinary runtime code', () => {
    for (const name of ['src/efm/EfmBuild.props', 'tools/build_dll.ps1', '.github/workflows/pr-native.yml', '.husky/pre-commit', 'tools/check_efm_dll.ps1', 'src/efm/F-CK-1C_EFM/DcsBridge/EfmExports.baseline.txt', '.clang-format']) assert.ok(classifyRuleFile(name), name);
    assert.equal(classifyRuleFile('src/efm/F-CK-1C_EFM/Core/Example.cpp'), null);
    assert.equal(classifyRuleFile('docs/BUILD_DLL.md'), null);
});

test('renaming or removing a rule file cannot hide its former category', () => {
    const rules = changedRules([{filename:'other.txt', previous_filename:'tools/build_dll.ps1', status:'renamed'}]);
    assert.equal(rules.length, 1);
    assert.match(rules[0].categories.join(), /建置/);
    assert.equal(changedRules([{filename:file.filename,status:'removed'}]).length,1);
});

test('notice includes concrete file, changed line, old/new setting and both commit identities', () => {
    const text = renderNotice({repository:'owner/repo',number:7,head,base,rules:changedRules([file])});
    for (const value of [file.filename, '第 8 行', '>old<', '>new<', head, base, '/pull/7/files#diff-']) assert.ok(text.includes(value), value);
});

test('untrusted backticks and very large patches remain fenced and bounded', () => {
    const hostile = {...file, patch:'@@ -1 +1 @@\n+```\n+@someone\n+' + 'x'.repeat(100000)};
    const text = renderNotice({repository:'owner/repo',number:7,head,base,rules:changedRules(Array(100).fill(hostile))});
    assert.ok(text.length < 60000);
    assert.ok(text.includes('````diff'));
    assert.throws(()=>renderNotice({repository:'owner/repo;bad',number:7,head,base,rules:[]}));
});

test('local preview identifies the working tree without linking a fabricated PR', () => {
    const text = renderNotice({repository:'owner/repo',number:1,head,base,rules:changedRules([file]),preview:true});
    assert.match(text, /本機工作目錄預覽/);
    assert.ok(!text.includes('/pull/1'));
});

test('metadata publisher paginates files and updates only its own sticky notice', async () => {
    const writes = [], summaries = [];
    let pages = 0;
    const request = async (route, options) => {
        if (options) { writes.push({route,options}); return {}; }
        if (route.includes('/files?')) { pages++; return pages===1 ? Array(100).fill({filename:'docs/a.md'}) : [file]; }
        if (route.includes('/comments?')) return [{id:4,user:{login:'a-human'},body:'<!-- efm-pr-rule-notice -->'}, {id:9,user:{login:'github-actions[bot]'},body:'<!-- efm-pr-rule-notice --> old'}];
        return {...current,changed_files:101};
    };
    await publishNotice(event,request,body=>summaries.push(body));
    assert.equal(pages,2); assert.equal(writes.length,1); assert.equal(writes[0].options.method,'PATCH');
    assert.ok(writes[0].route.endsWith('/comments/9')); assert.ok(summaries[0].includes(file.filename));
});

test('stale source or target identities abort before publishing', async () => {
    let writes=0;
    await assert.rejects(()=>publishNotice(event,async (_,options)=>{if(options) writes++; return {...current,base:{sha:'c'.repeat(40)}};},()=>{}),/identities changed/);
    assert.equal(writes,0);
});

test('missing changed-file data is reported as incomplete rather than no rules', async () => {
    const writes=[];
    const request=async(route,options)=>{
        if(options){writes.push(options);return{};}
        if(route.includes('/files?')||route.includes('/comments?'))return[];
        return current;
    };
    const result=await publishNotice(event,request,()=>{});
    assert.equal(result.incomplete,true); assert.equal(writes.length,1);
    assert.match(writes[0].body.body,/未提供完整/);
});
