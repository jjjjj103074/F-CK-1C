import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {execFileSync, spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {planChecks, selectChecks} from './pr-checks.mjs';

const repository = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const runtime = 'src/efm/F-CK-1C_EFM/';
const tests = 'src/efm/F-CK-1C_EFM_Tests/';

for (const file of [runtime + 'Core/A.cpp', runtime + 'Core/A.hpp', runtime + 'SDK/wHumanCustomPhysicsAPI.h',
    runtime + 'ThirdParty/data.inc', runtime + 'DcsIds/CommandIds.json', runtime + 'DcsBridge/EfmExports.baseline.txt',
    runtime + 'F-CK-1C_EFM.vcxproj', 'src/efm/EfmBuild.props', 'src/efm/EfmCore.targets',
    'tools/generate_system_catalog.ps1', 'tools/efm-build-common.ps1', '.gitattributes', '.vsconfig', 'bin/F-CK-1C_EFM.dll']) {
    test(`DLL verification covers ${file} without requiring a DLL change`, () => {
        assert.equal(selectChecks([file]).dll, true);
        assert.equal(selectChecks([file], 'Merge').dll, true);
    });
}

test('test and FM-only changes run native tests without rebuilding/loading the DLL', () => {
    for (const file of [tests + 'CoreTests.cpp', tests + 'F-CK-1C_EFM_Tests.vcxproj', 'FM/config.jsonc']) {
        const checks = selectChecks([file]);
        assert.equal(checks.native, true);
        assert.equal(checks.dll, false);
    }
});

test('checks own their scripts, fixtures, configuration and dependency gates', () => {
    for (const [file, key] of [['.clang-format', 'format'], ['package-lock.json', 'format'],
        ['tools/cpp-format.test.mjs', 'tools'], ['tools/pr-checks.test.mjs', 'tools'],
        ['.github/workflows/pr-title.yml', 'tools'], ['tools/test_efm_architecture_checker.ps1', 'architecture'],
        ['tools/test_system_catalog_generator.ps1', 'catalog'], ['Input/F-CK-1C/joystick/default.lua', 'dcs'],
        ['tools/test_dcs_id_generator.ps1', 'dcs'], ['tools/check_efm_exports.ps1', 'validation'],
        ['tools/check_efm_toolchain.ps1', 'validation'], ['tools/efm-build-common.ps1', 'validation'],
        ['src/efm/EfmBuild.props', 'validation'], ['src/.clang-format', 'format']]) {
        assert.equal(selectChecks([file])[key], true, file);
    }
    assert.equal(selectChecks(['tools/check_efm_dll.ps1']).dll, true, 'Loading validators require a rebuilt/hash-verified DLL.');
    for (const file of ['tools/pr-checks.mjs', 'tools/validate_efm.ps1', '.github/workflows/pr-native.yml']) {
        assert.ok(Object.values(selectChecks([file])).every(Boolean), file);
    }
    assert.ok(Object.values(selectChecks(['docs/PR_VALIDATION.md', runtime + 'README.md'])).every(value => !value));
    assert.equal(selectChecks([runtime.toUpperCase() + 'A.CPP']).dll, true, 'Windows build paths are case-insensitive.');
});

function fixture(context) {
    const prefix = path.join(os.tmpdir(), 'fck1c-pr-gates-');
    const root = fs.mkdtempSync(prefix);
    context.after(() => {
        assert.equal(path.dirname(path.resolve(root)), path.resolve(os.tmpdir()));
        assert.ok(path.resolve(root).startsWith(prefix));
        fs.rmSync(root, {recursive: true, force: true});
    });
    const git = args => execFileSync('git', args, {cwd: root, encoding: 'utf8', windowsHide: true}).trim();
    const write = (file, contents) => {
        fs.mkdirSync(path.dirname(path.join(root, file)), {recursive: true});
        fs.writeFileSync(path.join(root, file), contents);
    };
    const commit = () => {
        git(['add', '-A']);
        git(['-c', 'user.name=GatesTest', '-c', 'user.email=gates@example.invalid', 'commit', '--quiet', '-m', 'fixture']);
        return git(['rev-parse', 'HEAD']);
    };
    git(['init', '--quiet']);
    git(['config', 'core.hooksPath', '.no-hooks']);
    return {root, git, write, commit};
}

test('whole-PR diff covers earlier commits, deletion and rename out of runtime scope', context => {
    const {root, git, write, commit} = fixture(context);
    write(runtime + 'A.cpp', 'int value = 1;');
    const base = commit();
    git(['mv', runtime + 'A.cpp', 'renamed.txt']);
    commit();
    write('docs/new.md', 'later docs-only commit');
    const head = commit();
    assert.equal(planChecks(root, head, base, 'Submission').dll, true);
    git(['checkout', '--quiet', '--detach', base]);
    git(['rm', runtime + 'A.cpp']);
    assert.equal(planChecks(root, commit(), base, 'Submission').dll, true);
});

test('merge gates include target-only source changes and reject missing/wrong revisions', context => {
    const {root, git, write, commit} = fixture(context);
    write(runtime + 'A.cpp', 'int value = 1;');
    const ancestor = commit();
    write(runtime + 'A.cpp', 'int value = 2;');
    const base = commit();
    git(['checkout', '--quiet', '--detach', ancestor]);
    write('docs/new.md', 'PR docs only');
    const head = commit();
    assert.equal(planChecks(root, head, base, 'Submission').dll, false);
    assert.equal(planChecks(root, head, base, 'Merge').dll, true);
    assert.throws(() => planChecks(root, ancestor, base, 'Merge'), /Checkout does not match/);
    assert.throws(() => planChecks(root, head, '--all', 'Submission'), /full head\/base/);
    assert.throws(() => planChecks(root, head, '0'.repeat(40), 'Submission'));
    assert.throws(() => selectChecks([], 'Typo'), /Unknown validation phase/);
});

test('selected source-only DLL check stops at hash mismatch before any DLL execution',
    {skip: process.platform !== 'win32'}, context => {
        const {root, write} = fixture(context);
        write('tools/validate_efm.ps1', fs.readFileSync(path.join(repository, 'tools/validate_efm.ps1')));
        write('tools/efm-build-common.ps1', `
function Get-EfmSourceFingerprint { 'fixture sources' }
function Get-EfmBuildProfile { @{EfmCompilerVersion='fixture'; EfmLinkerVersion='fixture'; EfmWindowsSdkVersion='fixture'} }
function Get-EfmSha256($file) { [BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash([IO.File]::ReadAllBytes($file))).Replace('-', '') }
function Invoke-EfmNativeBuild { @{Output = (Join-Path $RepoRoot 'rebuilt.dll')} }
`);
        write('tools/check_efm_toolchain.ps1', '# fixture toolchain');
        write('tools/check_efm_exports.ps1', "throw 'Must not reach exports before a matching hash.'");
        write('tools/check_efm_dll.ps1', "throw 'Must not execute the DLL before a matching hash.'");
        write('bin/F-CK-1C_EFM.dll', 'unchanged uploaded artifact');
        write('rebuilt.dll', 'new result from changed source');
        const checks = Object.entries(selectChecks([runtime + 'Core/A.cpp']))
            .filter(([key, run]) => run && !['format', 'tools'].includes(key)).map(([key]) => key).join(',');
        const result = spawnSync('powershell.exe', ['-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
            '-File', path.join(root, 'tools/validate_efm.ps1'), '-Checks', checks,
            '-ReportDirectory', path.join(root, 'report')], {encoding: 'utf8', windowsHide: true, timeout: 30000});
        assert.notEqual(result.status, 0);
        const report = JSON.parse(fs.readFileSync(path.join(root, 'report/report.json'), 'utf8'));
        assert.equal(report.status, 'failed');
        assert.equal(report.steps.at(-1).name, 'DLL 雜湊', report.error);
        assert.match(report.error, /DLL 雜湊不同/);
        assert.equal(fs.readFileSync(path.join(root, 'bin/F-CK-1C_EFM.dll'), 'utf8'), 'unchanged uploaded artifact');
    });
