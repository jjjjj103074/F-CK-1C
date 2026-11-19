import {createHash} from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';

const workflowPath = '.github/workflows/pr-native.yml';
const requiredJobs = ['PR / 提交檢查', 'PR / 合併檢查', 'PR / 檢查結果'];
const lifetime = 7 * 24 * 60 * 60 * 1000;

export function metadataOnly(payload) {
    const fields = Object.keys(payload.changes ?? {});
    return payload.action === 'edited' && fields.length > 0
        && fields.every(field => ['title', 'body'].includes(field));
}

// The artifact name binds the receipt to immutable inputs and one run attempt.
// The API must also confirm the producer's workflow and all three real jobs.
export function receiptName(pr, attempt) {
    const target = createHash('sha256').update(pr.base.ref).digest('hex');
    return `native-v1-pr${pr.number}-${pr.head.sha}-${pr.base.sha}-${target}-a${attempt}`;
}

function api(github, context) {
    const params = context.repo;
    return {
        run: async id => (await github.request('GET /repos/{owner}/{repo}/actions/runs/{run_id}',
            {...params, run_id: id})).data,
        async runs(current) {
            const runs = [];
            // GitHub limits filtered workflow searches to 1,000 runs. If the
            // history is incomplete, an unseen failed re-run could invalidate it.
            for (let page = 1; page <= 10; page++) {
                const {data} = await github.request('GET /repos/{owner}/{repo}/actions/workflows/{workflow_id}/runs',
                    {...params, workflow_id: current.workflow_id, event: 'pull_request',
                        head_sha: context.payload.pull_request.head.sha, per_page: 100, page});
                if (data.total_count > 1000) throw new Error('驗證歷史超出可查證範圍。');
                runs.push(...data.workflow_runs);
                if (runs.length >= data.total_count) return runs;
            }
            throw new Error('驗證歷史不完整。');
        },
        jobs: run => github.paginate('GET /repos/{owner}/{repo}/actions/runs/{run_id}/attempts/{attempt_number}/jobs',
            {...params, run_id: run.id, attempt_number: run.run_attempt, per_page: 100}),
        artifacts: run => github.paginate('GET /repos/{owner}/{repo}/actions/runs/{run_id}/artifacts',
            {...params, run_id: run.id, per_page: 100}),
    };
}

function samePullRequest(run, current, pr) {
    return run.id !== current.id
        && run.workflow_id === current.workflow_id && run.path === workflowPath
        && run.event === 'pull_request' && run.head_sha === pr.head.sha
        && run.head_repository?.full_name === pr.head.repo.full_name
        && run.pull_requests?.some(item => item.number === pr.number);
}

function passedStep(jobs, name) {
    return jobs.some(job => job.name === 'PR / 規劃驗證'
        && job.steps?.some(step => step.name === name && step.conclusion === 'success'));
}

export async function reusableResult({github, context, now = Date.now, sleep = ms => new Promise(resolve => setTimeout(resolve, ms)),
    maxWaitMs = 40 * 60 * 1000}) {
    const client = api(github, context);
    const current = await client.run(context.runId);
    const pr = context.payload.pull_request;
    if (current.path !== workflowPath) throw new Error('非預期的驗證 workflow。');
    // A re-run must not hide a failed full-validation attempt of its own run.
    if (current.run_attempt > 1) return null;
    const deadline = now() + maxWaitMs;
    for (;;) {
        const runs = (await client.runs(current)).filter(run => samePullRequest(run, current, pr))
            .sort((a, b) => Date.parse(b.updated_at) - Date.parse(a.updated_at) || b.run_number - a.run_number);
        let pending = false;
        for (const listed of runs) {
            // Always fetch the latest attempt, including manual re-runs.
            const run = await client.run(listed.id);
            if (!samePullRequest(run, current, pr)) continue;
            if (run.status !== 'completed') {
                // Newer metadata may itself be waiting for this run. Completed
                // newer runs are still inspected so a real failure cannot hide.
                if (run.run_number > current.run_number) continue;
                pending = true;
                break;
            }
            const jobs = await client.jobs(run);
            // Reuse runs never mint receipts or extend the original expiry.
            if (passedStep(jobs, '沿用有效結果') && !passedStep(jobs, '完整驗證')) continue;
            if (run.conclusion !== 'success' || !passedStep(jobs, '完整驗證')) return null;
            if (!requiredJobs.every(name => {
                const matches = jobs.filter(job => job.name === name);
                return matches.length === 1 && matches[0].status === 'completed' && matches[0].conclusion === 'success';
            })) return null;
            const receipts = (await client.artifacts(run)).filter(artifact => artifact.name === receiptName(pr, run.run_attempt)
                && !artifact.expired && Date.parse(artifact.expires_at) > now()
                && Date.parse(artifact.created_at) <= now() && now() - Date.parse(artifact.created_at) < lifetime);
            if (receipts.length !== 1) return null;
            // Reject evidence invalidated while its jobs/artifacts were being read.
            const latest = await client.run(run.id);
            if (latest.run_attempt !== run.run_attempt || latest.status !== 'completed' || latest.conclusion !== 'success') return null;
            return {runId: String(run.id), url: run.html_url, label: `#${run.run_number}（第 ${run.run_attempt} 次）`};
        }
        if (!pending || now() >= deadline) return null;
        await sleep(Math.min(30000, deadline - now()));
    }
}

export async function planValidation({github, context, core, ...options}) {
    let source = null;
    if (metadataOnly(context.payload)) {
        try {
            source = await reusableResult({github, context, ...options});
        } catch (error) {
            core.warning(`無法查證既有結果，重新驗證：${error.message}`);
        }
    }
    const plan = {mode: source ? 'reuse' : 'full', sourceRun: source?.runId ?? '',
        sourceUrl: source?.url ?? '', sourceLabel: source?.label ?? ''};
    for (const [key, value] of Object.entries(plan)) core.setOutput(key, value);
    await core.summary.addHeading('驗證方式').addRaw(source
        ? `沿用已查證的 [${source.label}](${source.url})；來源與目標版本相同。`
        : '執行完整驗證；沒有可沿用的有效結果。').write();
    return plan;
}

export async function finishValidation({github, context, core, env = process.env}) {
    const expected = context.payload.pull_request;
    const {data: current} = await github.rest.pulls.get({...context.repo, pull_number: expected.number});
    const {data: target} = await github.rest.git.getRef({...context.repo, ref: `heads/${current.base.ref}`});
    const fresh = current.state === 'open' && current.head.sha === expected.head.sha
        && current.base.ref === expected.base.ref && current.base.sha === expected.base.sha && target.object.sha === expected.base.sha;
    const passed = env.SUBMISSION_RESULT === 'success' && env.MERGE_RESULT === 'success';
    const labels = {success: '通過', failure: '失敗', cancelled: '已取消', skipped: '已跳過'};
    await core.summary.addHeading('檢查結果').addTable([
        [{data: '項目', header: true}, {data: '結果', header: true}],
        ['提交檢查', labels[env.SUBMISSION_RESULT] ?? '未知'],
        ['合併檢查', labels[env.MERGE_RESULT] ?? '未知'],
        ['檢查版本', fresh ? '最新' : '已過期，請同步目標分支後重跑'],
    ]).addDetails('版本', `PR: ${expected.head.sha}\n\n目標: ${expected.base.ref} / ${expected.base.sha}`).write();
    if (!fresh || !passed || !['full', 'reuse'].includes(env.VALIDATION_MODE)) {
        core.setFailed(fresh ? '檢查未通過，請查看失敗項目。' : '檢查版本已過期，請同步目標分支後重跑。');
        return;
    }
    if (env.VALIDATION_MODE === 'reuse') {
        const source = await reusableResult({github, context, maxWaitMs: 0});
        if (!source || source.runId !== env.SOURCE_RUN) {
            core.setFailed('沿用結果已失效，請選擇 Re-run all jobs 重新驗證。');
            return;
        }
        await core.summary.addRaw(`沿用已查證的 [${source.label}](${source.url})；未重新建置。`).write();
    } else {
        const receipt = {schema: 1, repository: context.repo, pr: expected.number, head: expected.head.sha,
            base: expected.base.sha, baseRef: expected.base.ref, runId: context.runId, attempt: Number(env.GITHUB_RUN_ATTEMPT)};
        fs.writeFileSync(path.join(env.RUNNER_TEMP, 'native-validation.json'), JSON.stringify(receipt, null, 2) + '\n');
        core.setOutput('receipt', receiptName(expected, receipt.attempt));
    }
}
