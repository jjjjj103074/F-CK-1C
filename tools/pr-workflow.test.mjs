import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import vm from 'node:vm';

const repository = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

function readWorkflow(filename) {
    return fs.existsSync(path.join(repository, filename))
        ? fs.readFileSync(path.join(repository, filename), 'utf8')
        : git(repository, ['show', `HEAD:${filename}`]);
}

function git(root, args, input) {
    const result = spawnSync('git', args, { cwd: root, input, encoding: 'utf8', windowsHide: true, timeout: 30000 });
    assert.equal(result.status, 0, result.stderr || result.error?.message);
    return result.stdout;
}

function gitShell() {
    return process.platform === 'win32'
        ? path.resolve(git(repository, ['--exec-path']).trim(), '../../../bin/bash.exe') : 'bash';
}

test('Git shell can invoke DLL hashing in Windows PowerShell', { skip: process.platform !== 'win32' }, () => {
    const result = spawnSync(gitShell(), ['--noprofile', '--norc', '-c',
        `powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command '. "./tools/efm-build-common.ps1"; Get-EfmSha256 "./bin/F-CK-1C_EFM.dll"'`],
    { cwd: repository, env: process.env, encoding: 'utf8', windowsHide: true, timeout: 30000 });
    assert.equal(result.status, 0, result.stderr || result.error?.message);
    const expected = createHash('sha256').update(fs.readFileSync(path.join(repository, 'bin/F-CK-1C_EFM.dll'))).digest('hex');
    assert.equal(result.stdout.trim().toLowerCase(), expected);
});

test('the real sparse checkouts retain native-test configuration and DCS generator fixtures', { timeout: 90000 }, context => {
    const workflowPath = '.github/workflows/pr-native.yml';
    const workflow = readWorkflow(workflowPath);
    const sections = workflow.split('\n  merge-compatibility:');
    const checkoutPatterns = section => {
        const block = section.match(/sparse-checkout: \|\r?\n((?: {12}\/[^\r\n]+\r?\n)+)/);
        assert.ok(block, 'Sparse checkout patterns were not found.');
        return block[1].split(/\r?\n/).map(line => line.trim()).filter(Boolean);
    };
    const patterns = checkoutPatterns(sections[0]);
    const mergePatterns = checkoutPatterns(sections[1]);
    const prefix = path.join(os.tmpdir(), 'fck1c-pr-checkout-');
    const root = fs.mkdtempSync(prefix);
    context.after(() => {
        assert.equal(path.dirname(path.resolve(root)), path.resolve(os.tmpdir()));
        assert.ok(path.resolve(root).startsWith(prefix));
        fs.rmSync(root, { recursive: true, force: true });
    });
    // CI uses a shallow partial clone. Copy only materialized generator inputs
    // into an independent fixture instead of cloning incomplete object storage.
    git(root, ['init', '--quiet']);
    git(root, ['config', 'core.hooksPath', '.no-hooks']);
    const inputs = git(repository, ['ls-files', '-z', '--', '.gitattributes', 'commitlint.config.js',
        'Cockpit', 'Input', 'FM', 'src/efm/F-CK-1C_EFM/DcsIds',
        'tools/generate_dcs_ids.ps1', 'tools/test_dcs_id_generator.ps1']).split('\0').filter(Boolean);
    for (const relative of inputs) {
        const source = path.join(repository, relative);
        assert.ok(fs.existsSync(source), `Missing checked-out generator input: ${relative}`);
        const target = path.join(root, relative);
        fs.mkdirSync(path.dirname(target), { recursive: true });
        fs.copyFileSync(source, target);
    }
    git(root, ['add', '.']);
    git(root, ['-c', 'user.name=PRCheckoutTests', '-c', 'user.email=pr-checkout-tests@example.invalid',
        'commit', '--quiet', '-m', 'fixture generator inputs']);
    git(root, ['sparse-checkout', 'init', '--no-cone']);
    git(root, ['sparse-checkout', 'set', '--no-cone', '--stdin'], patterns.join('\n') + '\n');
    git(root, ['checkout', '--quiet', '--detach', git(root, ['rev-parse', 'HEAD']).trim()]);
    assert.ok(fs.existsSync(path.join(root, 'commitlint.config.js')), 'PR title rules must be present.');
    for (const file of ['Cockpit/Scripts/command_defs.lua', 'Cockpit/Scripts/generated/CockpitParams.g.lua']) {
        assert.ok(fs.existsSync(path.join(root, file)), `Missing generator output: ${file}`);
    }
    assert.ok(fs.existsSync(path.join(root, 'Input')), 'Input command-reference coverage must be present.');
    const configurationFiles = inputs.filter(file => file.startsWith('FM/') && file.endsWith('.jsonc'));
    assert.ok(configurationFiles.length, 'Native-test configuration inputs were not found.');
    const assertConfiguration = () => {
        for (const file of configurationFiles) {
            assert.ok(fs.existsSync(path.join(root, file)), `Missing native-test configuration: ${file}`);
        }
    };
    assertConfiguration();
    if (process.platform === 'win32') {
        const result = spawnSync('powershell.exe', ['-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
            '-File', path.join(root, 'tools/test_dcs_id_generator.ps1'), '-RepoRoot', root],
        { cwd: root, encoding: 'utf8', windowsHide: true, timeout: 30000 });
        assert.equal(result.status, 0, result.stderr || result.error?.message);
        assert.match(result.stdout, /DCS ID generator fixtures passed/);
    }
    git(root, ['sparse-checkout', 'set', '--no-cone', '--stdin'], mergePatterns.join('\n') + '\n');
    assertConfiguration();
});

test('notice checks the actual target before checking out its renderer', async () => {
    const workflow = readWorkflow('.github/workflows/pr-rule-notice.yml');
    const block = workflow.match(/ {10}script: \|\r?\n((?: {12}[^\r\n]*\r?\n)+)/);
    assert.ok(block, 'Target guard not found.');
    assert.ok(block.index < workflow.indexOf('uses: actions/checkout@'));
    const script = block[1].split(/\r?\n/).map(line => line.replace(/^ {12}/, '')).join('\n');
    for (const current of ['a'.repeat(40), 'b'.repeat(40)]) {
        const failures = [];
        await vm.runInNewContext(`(async () => {${script}\n})()`, {
            context: {payload: {pull_request: {base: {ref: 'release/candidate', sha: 'a'.repeat(40)}}}, repo: {owner: 'owner', repo: 'repo'}},
            github: {rest: {git: {getRef: async ({ref}) => {
                assert.equal(ref, 'heads/release/candidate');
                return {data: {object: {sha: current}}};
            }}}},
            core: {setFailed: message => failures.push(message)},
        });
        assert.equal(failures.length === 0, current === 'a'.repeat(40));
    }
});

test('summary requires successful checks and current open PR revisions', async () => {
    const workflow = readWorkflow('.github/workflows/pr-native.yml');
    const block = workflow.match(/ {10}script: \|\r?\n([\s\S]+)$/);
    assert.ok(block, 'Summary script not found.');
    const script = block[1].split(/\r?\n/).map(line => line.replace(/^ {12}/, '')).join('\n');
    const expected = {number: 7, head: {sha: 'a'.repeat(40)}, base: {sha: 'b'.repeat(40), ref: 'main'}};
    const fresh = {...expected, state: 'open'};
    for (const [submission, merge, current, passes, targetSha = current.base.sha] of [
        ['success', 'success', fresh, true],
        ['failure', 'skipped', fresh, false],
        ['cancelled', 'cancelled', fresh, false],
        ['success', 'unknown', fresh, false],
        ['success', 'success', {...fresh, head: {sha: 'c'.repeat(40)}}, false],
        ['success', 'success', {...fresh, base: {sha: 'c'.repeat(40)}}, false],
        ['success', 'success', {...fresh, base: {...fresh.base, ref: 'dev'}}, false],
        ['success', 'success', fresh, false, 'c'.repeat(40)],
        ['success', 'success', {...fresh, state: 'closed'}, false],
    ]) {
        const failures = [], rows = [], details = [];
        const summary = {
            addHeading() { return this; },
            addTable(value) { rows.push(...value); return this; },
            addDetails(...value) { details.push(value); return this; },
            async write() {},
        };
        await vm.runInNewContext(`(async () => {${script}\n})()`, {
            context: {payload: {pull_request: expected}, repo: {owner: 'owner', repo: 'repo'}},
            github: {rest: {pulls: {get: async () => ({data: current})}, git: {getRef: async ({ref}) => {
                assert.equal(ref, `heads/${current.base.ref}`);
                return {data: {object: {sha: targetSha}}};
            }}}},
            core: {summary, setFailed: message => failures.push(message)},
            process: {env: {SUBMISSION_RESULT: submission, MERGE_RESULT: merge}},
        });
        assert.equal(failures.length === 0, passes, `${submission}/${merge}/${current.state}`);
        assert.equal(rows.length, 4);
        assert.ok(details[0][1].includes(expected.head.sha));
        assert.ok(details[0][1].includes(expected.base.sha));
    }
});

test('PR title input preserves shell characters and commitlint still validates format', () => {
    const workflow = readWorkflow('.github/workflows/pr-title.yml');
    const run = workflow.match(/ {8}run: (.*commitlint.*)/)?.[1];
    assert.ok(run, 'Title command not found.');
    assert.match(workflow, /PR_TITLE: \$\{\{ github\.event\.pull_request\.title \}\}/);
    assert.ok(!run.includes('${{'));
    const shell = gitShell();
    for (const title of ['chore(tooling): 保留 "引號"', 'chore(tooling): $(printf EXECUTED)', 'chore(tooling): `printf EXECUTED`; $PATH']) {
        const result = spawnSync(shell, ['--noprofile', '--norc', '-c', run.replace('./node_modules/.bin/commitlint --no-color', 'cat')],
            {cwd: repository, env: {...process.env, PR_TITLE: title}, encoding: 'utf8', windowsHide: true, timeout: 10000});
        assert.equal(result.status, 0, result.stderr || result.error?.message);
        assert.equal(result.stdout.replaceAll('\r\n', '\n'), title + '\n');
    }
    const cli = path.join(repository, 'node_modules/@commitlint/cli/cli.js');
    for (const [title, valid] of [['chore(tooling): 檢查文案', true], ['chore: missing scope', false]]) {
        const result = spawnSync(process.execPath, [cli, '--no-color'],
            {cwd: repository, input: title + '\n', encoding: 'utf8', windowsHide: true, timeout: 10000});
        assert.equal(result.status === 0, valid, result.stdout + result.stderr);
    }
});
