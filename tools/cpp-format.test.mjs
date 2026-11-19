import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import test from 'node:test';
import { formatterPath, run } from './cpp-format.mjs';

const repository = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const manifest = JSON.parse(fs.readFileSync(path.join(repository, 'package.json'), 'utf8'));
const runtime = manifest.cppFormat.roots[0];
const tests = manifest.cppFormat.roots[1];
const sample = 'namespace Example {\nint compute(int value){if(value>0){return value;}return 0;}\n}\n';

function git(root, args) {
    const result = spawnSync('git', args, { cwd: root, encoding: 'utf8', windowsHide: true });
    assert.equal(result.status, 0, result.stderr || result.error?.message);
    return result.stdout;
}

function write(root, file, content) {
    const target = path.join(root, file);
    fs.mkdirSync(path.dirname(target), { recursive: true });
    fs.writeFileSync(target, content);
    return target;
}

function fixture(context) {
    const prefix = path.join(os.tmpdir(), 'fck1c-cpp-format-');
    const root = fs.mkdtempSync(prefix);
    context.after(() => {
        // Delete only this test's own verified temporary directory.
        assert.equal(path.dirname(path.resolve(root)), path.resolve(os.tmpdir()));
        assert.ok(path.resolve(root).startsWith(prefix));
        fs.rmSync(root, { recursive: true, force: true });
    });
    git(root, ['init', '--quiet']);
    git(root, ['config', 'core.hooksPath', '.no-hooks']);
    git(root, ['config', 'core.autocrlf', 'false']);
    write(root, 'package.json', JSON.stringify(manifest));
    write(root, '.gitattributes', '*.cpp text eol=crlf\n');
    for (const file of ['.clang-format', '.clang-format-ignore']) {
        write(root, file, fs.readFileSync(path.join(repository, file)));
    }
    git(root, ['add', '.']);
    git(root, ['-c', 'user.name=FormattingTests', '-c', 'user.email=formatting-tests@example.invalid',
        'commit', '--quiet', '-m', 'fixture settings']);
    return root;
}

test('formats changed source files, including untracked paths with spaces, and is idempotent', (context) => {
    const root = fixture(context);
    const file = `${runtime}/nested path/New File.cpp`;
    const target = write(root, file, sample);
    assert.equal(run(['--write'], root), 0);
    const formatted = fs.readFileSync(target, 'utf8');
    assert.match(formatted, /namespace Example\r\n\{/);
    assert.match(formatted, /\r\n\tif \(value > 0\)/);
    assert.equal(run(['--write'], root), 0);
    assert.equal(fs.readFileSync(target, 'utf8'), formatted);
    assert.equal(run(['--check'], root), 0);
    assert.equal(git(root, ['diff', '--cached', '--name-only']), '');
});

test('staged check rejects an unformatted index even when the worktree is formatted', (context) => {
    const root = fixture(context);
    const file = `${runtime}/Partial.cpp`;
    const target = write(root, file, sample);
    git(root, ['add', '--', file]);
    const indexBefore = git(root, ['show', `:${file}`]);
    run(['--write'], root);
    const worktreeBefore = fs.readFileSync(target, 'utf8');
    assert.equal(run(['--check', '--staged'], root), 1);
    assert.equal(git(root, ['show', `:${file}`]), indexBefore);
    assert.equal(fs.readFileSync(target, 'utf8'), worktreeBefore);
});

test('staged check accepts a formatted index with dirty worktree content and normalized LF', (context) => {
    const root = fixture(context);
    const file = `${runtime}/Partial.cpp`;
    const target = write(root, file, sample);
    run(['--write'], root);
    git(root, ['add', '--', file]);
    const indexBefore = git(root, ['show', `:${file}`]);
    assert.ok(!indexBefore.includes('\r'));
    fs.writeFileSync(target, sample);
    assert.equal(run(['--check', '--staged'], root), 0);
    assert.equal(git(root, ['show', `:${file}`]), indexBefore);
    assert.equal(fs.readFileSync(target, 'utf8'), sample);
});

test('formats all authored runtime and test files while preserving SDK, vendor, and generated files', (context) => {
    const root = fixture(context);
    const excluded = [
        'src/efm/F-CK-1C_EFM/include/SDK.h',
        'src/efm/F-CK-1C_EFM/ThirdParty/nlohmann/json.hpp',
        `${runtime}/Generated.g.cpp`,
        `${tests}/Generated.g.inl`,
    ];
    for (const file of excluded) write(root, file, sample);
    write(root, 'Root.cpp', sample);
    const authored = [`${runtime}/Core/Owned.cpp`, `${tests}/Owned.cpp`];
    for (const file of authored) write(root, file, sample);
    // Include already committed files in --all, beyond the changed-file default.
    git(root, ['add', '.']);
    git(root, ['-c', 'user.name=FormattingTests', '-c', 'user.email=formatting-tests@example.invalid',
        'commit', '--quiet', '-m', 'fixture sources']);
    assert.equal(run(['--write'], root), 0);
    for (const file of authored) assert.equal(fs.readFileSync(path.join(root, file), 'utf8'), sample);
    assert.equal(run(['--write', '--all'], root), 0);
    for (const file of authored) assert.notEqual(fs.readFileSync(path.join(root, file), 'utf8'), sample);
    assert.equal(fs.readFileSync(path.join(root, 'Root.cpp'), 'utf8'), sample);
    for (const file of excluded) assert.equal(fs.readFileSync(path.join(root, file), 'utf8'), sample);
    // Match the editors' stdin invocation as well as the script's scope filter.
    for (const file of excluded) {
        const result = spawnSync(formatterPath(), [
            '--style=file', '--output-replacements-xml', `--assume-filename=${path.join(root, file)}`,
        ], { input: sample, encoding: 'utf8', windowsHide: true });
        assert.equal(result.status, 0, result.stderr);
        assert.ok(!result.stdout.includes('<replacement '), result.stdout);
    }
});

test('preserves source if a newly ignored owned path produces empty formatter output', (context) => {
    const root = fixture(context);
    const file = `${runtime}/Ignored.cpp`;
    const target = write(root, file, sample);
    fs.appendFileSync(path.join(root, '.clang-format-ignore'), `\n${file}\n`);
    assert.throws(() => run(['--write'], root), /source was preserved/);
    assert.equal(fs.readFileSync(target, 'utf8'), sample);
});

test('rejects unstaged root style and scope changes without altering partially staged source', (context) => {
    const root = fixture(context);
    const file = `${runtime}/Partial.cpp`;
    const target = write(root, file, sample);
    git(root, ['add', '--', file]);
    const indexBefore = git(root, ['show', `:${file}`]);
    assert.equal(run(['--check', '--staged'], root), 1);
    for (const setting of ['.clang-format', '.clang-format-ignore', 'package.json', 'package-lock.json']) {
        const original = fs.existsSync(path.join(root, setting)) ? fs.readFileSync(path.join(root, setting), 'utf8') : null;
        const changed = setting === 'package.json'
            ? JSON.stringify({ ...manifest, cppFormat: { roots: ['src/efm/Empty'] } })
            : setting === '.clang-format' ? original.replace('DisableFormat: false', 'DisableFormat: true') : '{}\n';
        write(root, setting, changed);
        assert.throws(() => run(['--check', '--staged'], root), /settings have unstaged changes/);
        assert.equal(git(root, ['show', `:${file}`]), indexBefore);
        assert.equal(fs.readFileSync(target, 'utf8'), sample);
        if (original === null) fs.unlinkSync(path.join(root, setting));
        else write(root, setting, original);
    }
});

test('rejects untracked and ignored nested settings, and accepts settings that match the index', (context) => {
    const root = fixture(context);
    const file = `${runtime}/Nested/Partial.cpp`;
    write(root, file, sample);
    git(root, ['add', '--', file]);
    const override = `${runtime}/Nested/.clang-format`;
    write(root, override, 'BasedOnStyle: InheritParentConfig\nDisableFormat: true\n');
    assert.throws(() => run(['--check', '--staged'], root), /settings have unstaged changes/);
    write(root, '.gitignore', '**/.clang-format\n');
    assert.throws(() => run(['--check', '--staged'], root), /settings have unstaged changes/);
    write(root, override, 'BasedOnStyle: InheritParentConfig\nDisableFormat: false\n');
    git(root, ['add', '--force', '--', override]);
    assert.equal(run(['--check', '--staged'], root), 1);
    run(['--write'], root);
    git(root, ['add', '--', file]);
    assert.equal(run(['--check', '--staged'], root), 0);
    write(root, override, 'BasedOnStyle: InheritParentConfig\nDisableFormat: true\n');
    assert.throws(() => run(['--check', '--staged'], root), /settings have unstaged changes/);
});

test('documents-only staged changes skip formatter resolution', (context) => {
    const root = fixture(context);
    write(root, 'README.md', 'Documentation\n');
    git(root, ['add', 'README.md']);
    const previous = process.env.CLANG_FORMAT_PATH;
    process.env.CLANG_FORMAT_PATH = path.join(root, 'missing-formatter');
    try {
        assert.equal(run(['--check', '--staged'], root), 0);
    } finally {
        if (previous === undefined) delete process.env.CLANG_FORMAT_PATH;
        else process.env.CLANG_FORMAT_PATH = previous;
    }
});

test('refuses a formatter version override and refuses staged writes', () => {
    const previous = process.env.CLANG_FORMAT_PATH;
    process.env.CLANG_FORMAT_PATH = process.execPath;
    try {
        assert.throws(() => formatterPath(), /Expected clang-format/);
    } finally {
        if (previous === undefined) delete process.env.CLANG_FORMAT_PATH;
        else process.env.CLANG_FORMAT_PATH = previous;
    }
    assert.throws(() => run(['--write', '--staged']), /Use --setup/);
});
