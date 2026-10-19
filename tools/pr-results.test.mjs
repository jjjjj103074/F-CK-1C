import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {metadataOnly, receiptName, reusableResult, planValidation, finishValidation} from './pr-results.mjs';

function fixture(t) {
    const pr = {number: 32, state: 'open', head: {sha: 'a'.repeat(40), repo: {full_name: 'owner/repo'}},
        base: {sha: 'b'.repeat(40), ref: 'dev'}};
    const context = {repo: {owner: 'owner', repo: 'repo'}, runId: 100,
        payload: {action: 'edited', changes: {body: {from: 'old'}}, pull_request: pr}};
    const run = {id: 90, run_number: 51, run_attempt: 1, workflow_id: 12, path: '.github/workflows/pr-native.yml',
        event: 'pull_request', head_sha: pr.head.sha, head_repository: pr.head.repo, pull_requests: [{number: 32}],
        status: 'completed', conclusion: 'success', run_started_at: new Date(Date.now() - 60000).toISOString(),
        updated_at: new Date(Date.now() - 1000).toISOString(), html_url: 'https://github.com/owner/repo/actions/runs/90'};
    const runs = [run, {...run, id: 100, run_number: 55, status: 'in_progress', conclusion: null}];
    const jobs = [
        {name: 'PR / 規劃驗證', steps: [{name: '完整驗證', conclusion: 'success'}]},
        ...['提交檢查', '合併檢查', '檢查結果'].map(name => ({name: `PR / ${name}`, status: 'completed', conclusion: 'success'})),
    ];
    const artifacts = [{name: receiptName(pr, 1), expired: false, created_at: run.updated_at,
        expires_at: new Date(Date.now() + 86400000).toISOString()}];
    const calls = [];
    const github = {
        async request(route, params) {
            calls.push({route, ...params});
            assert.equal(params.owner, 'owner');
            assert.equal(params.repo, 'repo');
            return {data: route.includes('/workflows/') ? {workflow_runs: runs, total_count: runs.length}
                : runs.find(item => item.id === Number(params.run_id))};
        },
        async paginate(route, params) {
            calls.push({route, ...params});
            if (route.endsWith('/jobs')) {
                assert.equal(params.attempt_number, runs.find(item => item.id === params.run_id).run_attempt);
                return jobs;
            }
            assert.ok(route.endsWith('/artifacts'));
            return artifacts;
        },
        rest: {pulls: {get: async () => ({data: pr})}, git: {getRef: async () => ({data: {object: {sha: pr.base.sha}}})}},
    };
    const outputs = {}, failures = [], warnings = [], summaries = [];
    const summary = Object.fromEntries(['addHeading', 'addRaw', 'addTable', 'addDetails'].map(method =>
        [method, (...args) => { summaries.push({method, args}); return summary; }]));
    summary.write = async () => {};
    const core = {summary, setOutput: (key, value) => { outputs[key] = value; },
        setFailed: value => failures.push(value), warning: value => warnings.push(value)};
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'fck1c-pr-results-'));
    t.after(() => {
        assert.equal(path.dirname(root), path.resolve(os.tmpdir()));
        assert.ok(path.basename(root).startsWith('fck1c-pr-results-'));
        fs.rmSync(root, {recursive: true, force: true});
    });
    const env = {SUBMISSION_RESULT: 'success', MERGE_RESULT: 'success', VALIDATION_MODE: 'full',
        GITHUB_RUN_ATTEMPT: '1', RUNNER_TEMP: root};
    return {github, context, core, run, runs, jobs, artifacts, calls, outputs, failures, warnings, summaries, pr, env, root};
}

test('only title/body edits may reuse; code events, retargets and unknown edits validate normally', async t => {
    for (const payload of [{action: 'opened'}, {action: 'synchronize'}, {action: 'reopened'},
        {action: 'edited'}, {action: 'edited', changes: {}}, {action: 'edited', changes: {base: {}}},
        {action: 'edited', changes: {body: {}, base: {}}}, {action: 'edited', changes: {unknown: {}}}]) {
        const f = fixture(t);
        assert.equal(metadataOnly(payload), false);
        Object.assign(f.context.payload, {...payload, changes: payload.changes});
        assert.equal((await planValidation(f)).mode, 'full');
        assert.equal(f.calls.length, 0);
    }
    for (const changes of [{title: {}}, {body: {}}, {title: {}, body: {}}]) {
        assert.equal(metadataOnly({action: 'edited', changes}), true);
    }
});

test('valid evidence reports the original run and checks the specific latest attempt', async t => {
    const f = fixture(t);
    const result = await planValidation(f);
    assert.equal(result.mode, 'reuse');
    assert.equal(result.sourceRun, '90');
    assert.equal(result.sourceUrl, f.run.html_url);
    assert.match(result.sourceLabel, /#51/);
    assert.ok(f.calls.some(call => call.route.endsWith('/attempts/{attempt_number}/jobs') && call.attempt_number === 1));
    assert.equal(f.outputs.mode, 'reuse');
});

for (const [name, invalidate] of [
    ['different head', f => { f.run.head_sha = 'c'.repeat(40); }],
    ['different source repository', f => { f.run.head_repository = {full_name: 'other/repo'}; }],
    ['different PR', f => { f.run.pull_requests = [{number: 33}]; }],
    ['missing PR association', f => { f.run.pull_requests = []; }],
    ['different workflow', f => { f.run.workflow_id = 999; }],
    ['different workflow path', f => { f.run.path = '.github/workflows/fake.yml'; }],
    ['different event', f => { f.run.event = 'workflow_dispatch'; }],
    ['different target SHA', f => { f.artifacts[0].name = receiptName({...f.pr, base: {...f.pr.base, sha: 'c'.repeat(40)}}, 1); }],
    ['different target branch', f => { f.artifacts[0].name = receiptName({...f.pr, base: {...f.pr.base, ref: 'main'}}, 1); }],
    ['expired receipt', f => { f.artifacts[0].expired = true; }],
    ['elapsed expiry', f => { f.artifacts[0].expires_at = new Date(Date.now() - 1).toISOString(); }],
    ['seven-day-old receipt', f => { f.artifacts[0].created_at = new Date(Date.now() - 7 * 86400000).toISOString(); }],
    ['future timestamp', f => { f.artifacts[0].created_at = new Date(Date.now() + 86400000).toISOString(); }],
    ['missing receipt', f => { f.artifacts.length = 0; }],
    ['duplicate receipt', f => { f.artifacts.push({...f.artifacts[0]}); }],
    ['failed run', f => { f.run.conclusion = 'failure'; }],
    ['cancelled run', f => { f.run.conclusion = 'cancelled'; }],
    ['skipped required job', f => { f.jobs[1].conclusion = 'skipped'; }],
    ['missing required job', f => { f.jobs.pop(); }],
    ['duplicate required job', f => { f.jobs.push({...f.jobs[1]}); }],
    ['no proof of real validation', f => { f.jobs[0].steps = []; }],
    ['receipt from an earlier attempt', f => { f.run.run_attempt = 2; }],
]) {
    test(`${name} falls back to full validation`, async t => {
        const f = fixture(t);
        invalidate(f);
        assert.equal((await planValidation(f)).mode, 'full');
        assert.equal(f.outputs.sourceRun, '');
    });
}

test('newer real failure blocks an older pass, including a re-run of an older workflow run', async t => {
    const f = fixture(t);
    f.runs.push({...f.run, id: 89, run_number: 50, run_attempt: 2, conclusion: 'failure',
        updated_at: new Date().toISOString()});
    assert.equal(await reusableResult(f), null);
});

test('a completed real failure created after the metadata run still blocks reuse', async t => {
    const f = fixture(t);
    f.runs.push({...f.run, id: 101, run_number: 56, conclusion: 'failure', updated_at: new Date().toISOString()});
    assert.equal(await reusableResult(f), null);
    f.env.VALIDATION_MODE = 'reuse';
    f.env.SOURCE_RUN = '90';
    await finishValidation(f);
    assert.equal(f.failures.length, 1);
});

test('manual re-runs validate normally instead of hiding their own previous failed full attempt', async t => {
    const f = fixture(t);
    f.runs.find(run => run.id === 100).run_attempt = 2;
    assert.equal((await planValidation(f)).mode, 'full');
});

test('history pagination finds a failed re-run beyond the first page', async t => {
    const f = fixture(t);
    const failed = {...f.run, id: 89, run_number: 50, run_attempt: 2, conclusion: 'failure', updated_at: new Date().toISOString()};
    f.runs.push(failed);
    const request = f.github.request;
    f.github.request = async (route, params) => route.includes('/workflows/')
        ? {data: {workflow_runs: params.page === 1 ? Array(100).fill(f.run) : [failed], total_count: 101}}
        : request(route, params);
    assert.equal(await reusableResult(f), null);
});

test('unverifiably large history validates normally instead of choosing a partial search result', async t => {
    const f = fixture(t);
    const request = f.github.request;
    f.github.request = async (route, params) => route.includes('/workflows/')
        ? {data: {workflow_runs: [f.run], total_count: 1001}} : request(route, params);
    assert.equal((await planValidation(f)).mode, 'full');
});

test('reuse runs refer back to real validation and never renew its receipt', async t => {
    const f = fixture(t);
    f.runs.push({...f.run, id: 95, run_number: 54, updated_at: new Date().toISOString()});
    const paginate = f.github.paginate;
    f.github.paginate = (route, params) => params.run_id === 95 && route.endsWith('/jobs')
        ? [{name: 'PR / 規劃驗證', steps: [{name: '沿用有效結果', conclusion: 'success'}]}]
        : paginate(route, params);
    assert.equal((await reusableResult(f)).runId, '90');
    f.artifacts[0].created_at = new Date(Date.now() - 8 * 86400000).toISOString();
    assert.equal(await reusableResult(f), null);
});

test('metadata waits for an active earlier run without a circular wait on newer metadata', async t => {
    const f = fixture(t);
    f.run.status = 'in_progress';
    f.runs.push({...f.run, id: 101, run_number: 56});
    let sleeps = 0;
    const result = await reusableResult({...f, sleep: async () => { sleeps++; f.run.status = 'completed'; }});
    assert.equal(result.runId, '90');
    assert.equal(sleeps, 1);
});

test('an active run exceeding the wait limit falls back to full validation', async t => {
    const f = fixture(t);
    f.run.status = 'queued';
    let time = 0;
    assert.equal((await planValidation({...f, now: () => time, maxWaitMs: 10,
        sleep: async ms => { time += ms; }})).mode, 'full');
});

test('API failures fall back to validation rather than reporting reuse success', async t => {
    const f = fixture(t);
    f.github.request = async () => { throw new Error('API unavailable'); };
    assert.equal((await planValidation(f)).mode, 'full');
    assert.equal(f.warnings.length, 1);
});

test('an original run restarted while evidence is read cannot be reused', async t => {
    const f = fixture(t);
    const request = f.github.request;
    let reads = 0;
    f.github.request = async (route, params) => {
        const result = await request(route, params);
        if (params.run_id === 90 && ++reads === 2) return {data: {...f.run, run_attempt: 2, status: 'queued'}};
        return result;
    };
    assert.equal(await reusableResult(f), null);
});

test('successful real validation records immutable inputs for its current attempt', async t => {
    const f = fixture(t);
    await finishValidation(f);
    assert.equal(f.failures.length, 0);
    assert.equal(f.outputs.receipt, receiptName(f.pr, 1));
    assert.deepEqual(JSON.parse(fs.readFileSync(path.join(f.root, 'native-validation.json'), 'utf8')),
        {schema: 1, repository: f.context.repo, pr: 32, head: f.pr.head.sha, base: f.pr.base.sha,
            baseRef: 'dev', runId: 100, attempt: 1});
});

test('summary refuses failed/skipped jobs, stale versions, a closed PR and missing mode', async t => {
    for (const invalidate of [
        f => { f.env.SUBMISSION_RESULT = 'failure'; }, f => { f.env.MERGE_RESULT = 'skipped'; },
        f => { f.env.VALIDATION_MODE = ''; }, f => { f.pr.state = 'closed'; },
        f => { f.github.rest.pulls.get = async () => ({data: {...f.pr, head: {sha: 'c'.repeat(40)}}}); },
        f => { f.github.rest.pulls.get = async () => ({data: {...f.pr, base: {...f.pr.base, ref: 'main'}}}); },
        f => { f.github.rest.pulls.get = async () => ({data: {...f.pr, base: {...f.pr.base, sha: 'c'.repeat(40)}}}); },
        f => { f.github.rest.git.getRef = async () => ({data: {object: {sha: 'c'.repeat(40)}}}); },
    ]) {
        const f = fixture(t);
        invalidate(f);
        await finishValidation(f);
        assert.equal(f.failures.length, 1);
        assert.equal(f.outputs.receipt, undefined);
        assert.equal(fs.existsSync(path.join(f.root, 'native-validation.json')), false);
    }
});

test('reuse summary rechecks its original evidence and does not publish another receipt', async t => {
    const f = fixture(t);
    f.env.VALIDATION_MODE = 'reuse';
    f.env.SOURCE_RUN = '90';
    await finishValidation(f);
    assert.equal(f.failures.length, 0);
    assert.equal(f.outputs.receipt, undefined);
    assert.equal(fs.existsSync(path.join(f.root, 'native-validation.json')), false);
    assert.ok(f.summaries.some(item => item.method === 'addRaw' && item.args[0].includes(f.run.html_url)));
    f.artifacts.length = 0;
    await finishValidation(f);
    assert.equal(f.failures.length, 1);
});
