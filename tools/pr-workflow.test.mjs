import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const repository = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

function git(root, args, input) {
    const result = spawnSync('git', args, { cwd: root, input, encoding: 'utf8', windowsHide: true, timeout: 30000 });
    assert.equal(result.status, 0, result.stderr || result.error?.message);
    return result.stdout;
}

test('the real submission sparse checkout supports DCS generator fixtures', { timeout: 90000 }, context => {
    const workflowPath = '.github/workflows/pr-native.yml';
    const workflow = fs.existsSync(path.join(repository, workflowPath))
        ? fs.readFileSync(path.join(repository, workflowPath), 'utf8')
        : git(repository, ['show', `HEAD:${workflowPath}`]);
    const submission = workflow.split('\n  merge-compatibility:')[0];
    const block = submission.match(/sparse-checkout: \|\r?\n((?: {12}\/[^\r\n]+\r?\n)+)/);
    assert.ok(block, 'Submission sparse checkout patterns were not found.');
    const patterns = block[1].split(/\r?\n/).map(line => line.trim()).filter(Boolean);
    const prefix = path.join(os.tmpdir(), 'fck1c-pr-checkout-');
    const root = fs.mkdtempSync(prefix);
    context.after(() => {
        assert.equal(path.dirname(path.resolve(root)), path.resolve(os.tmpdir()));
        assert.ok(path.resolve(root).startsWith(prefix));
        fs.rmSync(root, { recursive: true, force: true });
    });
    git(repository, ['clone', '--quiet', '--shared', '--no-checkout', '--local', repository, root]);
    git(root, ['config', 'core.hooksPath', '.no-hooks']);
    git(root, ['sparse-checkout', 'init', '--no-cone']);
    git(root, ['sparse-checkout', 'set', '--no-cone', '--stdin'], patterns.join('\n') + '\n');
    git(root, ['checkout', '--quiet', '--detach', git(repository, ['rev-parse', 'HEAD']).trim()]);
    for (const file of ['Cockpit/Scripts/command_defs.lua', 'Cockpit/Scripts/generated/CockpitParams.g.lua']) {
        assert.ok(fs.existsSync(path.join(root, file)), `Missing generator output: ${file}`);
    }
    assert.ok(fs.existsSync(path.join(root, 'Input')), 'Input command-reference coverage must be present.');
    if (process.platform === 'win32') {
        const result = spawnSync('powershell.exe', ['-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
            '-File', path.join(root, 'tools/test_dcs_id_generator.ps1'), '-RepoRoot', root],
        { cwd: root, encoding: 'utf8', windowsHide: true, timeout: 30000 });
        assert.equal(result.status, 0, result.stderr || result.error?.message);
        assert.match(result.stdout, /DCS ID generator fixtures passed/);
    }
});
