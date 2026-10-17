import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const runtime = 'src/efm/f-ck-1c_efm/';
const tests = 'src/efm/f-ck-1c_efm_tests/';
const dllPath = 'bin/f-ck-1c_efm.dll';
const cpp = /\.(c|cc|cpp|cxx|h|hh|hpp|hxx|inc|inl|ipp|tpp)$/i;

export function selectChecks(files, phase = 'Submission') {
    if (!['Submission', 'Merge'].includes(phase)) throw new Error('Unknown validation phase.');
    files = files.map(file => file.toLowerCase());
    const changed = pattern => files.some(file => new RegExp(pattern.source, 'i').test(file));
    const has = (...paths) => files.some(file => paths.some(candidate => candidate.toLowerCase() === file));
    const control = has('tools/pr-checks.mjs', 'tools/validate_efm.ps1', 'tools/prepare_efm_merge.ps1',
        '.github/workflows/pr-native.yml', '.github/workflows/pr-native-checks.yml');
    const packages = has('package.json', 'package-lock.json');
    const runtimeChanged = files.some(file => file.startsWith(runtime) && !file.endsWith('.md'));
    const build = control || has('.gitattributes', '.vsconfig') || changed(/^src\/efm\/[^/]+\.(props|targets|sln|vcxproj)$/i)
        || has('tools/efm-build-common.ps1', 'tools/check_efm_toolchain.ps1', 'tools/build_dll.ps1',
            'tools/build_efm.ps1', 'tools/generate_system_catalog.ps1', 'tools/check_efm_architecture.ps1',
            'tools/generate_dcs_ids.ps1');
    const validation = control || has('tools/test_efm_validation.ps1', 'tools/test_cpp_analysis.ps1', 'tools/check_efm_dll.ps1', 'tools/check_efm_exports.ps1',
        'tools/check_efm_toolchain.ps1', 'tools/efm-build-common.ps1', 'src/efm/EfmBuild.props', 'src/efm/EfmBuild.targets', '.vsconfig');
    const dll = build || runtimeChanged || has(dllPath) || validation;
    const testChanged = files.some(file => file.startsWith(tests) && !file.endsWith('.md'));
    const analysis = build || runtimeChanged || testChanged || has('tools/test_cpp_analysis.ps1');
    const native = build || runtimeChanged || testChanged
        || changed(/^FM\/.*\.jsonc$/i);
    if (phase === 'Merge') return {dll, native, analysis};
    return {
        format: control || packages || changed(/(^|\/)\.?_?clang-format(?:-ignore)?$/)
            || has('.gitattributes', '.editorconfig', 'tools/cpp-format.mjs', 'tools/cpp-format.test.mjs')
            || files.some(file => (file.startsWith(runtime) || file.startsWith(tests)) && cpp.test(file)),
        tools: control || packages || changed(/^tools\/(cpp-format|pr-rule-changes|pr-workflow|pr-checks)(\.test)?\.mjs$/)
            || changed(/^\.github\/workflows\/(pr-title(?:-checks)?|pr-rule-notice)\.yml$/)
            || has('commitlint.config.js', '.gitattributes'),
        dll, native, analysis,
        architecture: control || runtimeChanged || has('tools/check_efm_architecture.ps1', 'tools/test_efm_architecture_checker.ps1'),
        catalog: control || changed(/^src\/efm\/F-CK-1C_EFM\/Core\/Systems\//)
            || has('tools/generate_system_catalog.ps1', 'tools/test_system_catalog_generator.ps1'),
        dcs: control || changed(/^(Cockpit|Input)\/.*\.lua$/i) || changed(/^src\/efm\/F-CK-1C_EFM\/DcsIds\//)
            || has('tools/generate_dcs_ids.ps1', 'tools/test_dcs_id_generator.ps1'),
        validation,
    };
}

export function planChecks(root, head, base, phase) {
    for (const sha of [head, base]) {
        if (!/^[a-f0-9]{40}$/i.test(sha)) throw new Error('Expected full head/base commit SHA.');
    }
    const git = args => execFileSync('git', args, {cwd: root, encoding: 'utf8', windowsHide: true, maxBuffer: 16 * 1024 * 1024});
    if (git(['rev-parse', 'HEAD']).trim() !== head) throw new Error('Checkout does not match PR head.');
    const ancestor = git(['merge-base', head, base]).trim();
    // Disable rename detection so both deleted and added paths are checked.
    const pathsAt = tip => git(['diff', '--name-only', '--no-renames', '-z', ancestor, tip, '--']).split('\0').filter(Boolean);
    const baseFiles = phase === 'Merge' ? pathsAt(base) : [];
    const files = [...new Set([...pathsAt(head), ...baseFiles])];
    const checks = selectChecks(files, phase);
    // Submission already analyzed the PR; analyze the merge when its base adds C++ inputs.
    if (phase === 'Merge') checks.analysis = selectChecks(baseFiles).analysis;
    return checks;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    const [head, base, phase = 'Submission'] = process.argv.slice(2);
    const checks = planChecks(process.cwd(), head, base, phase);
    const labels = {format: 'C++ 排版', tools: '工具測試', dll: 'DLL 一致性與接口', native: '原生測試',
        analysis: 'C++ 靜態分析', architecture: '架構測試', catalog: '系統目錄產生器', dcs: 'DCS ID 產生器', validation: '驗證工具'};
    const summary = ['### 檢查範圍', '', '| 檢查 | 執行 |', '|---|---|',
        ...Object.entries(checks).map(([key, run]) => `| ${labels[key]} | ${run ? '需要' : '不適用'} |`), ''].join('\n');
    process.stdout.write(summary);
    if (process.env.GITHUB_OUTPUT) {
        fs.appendFileSync(process.env.GITHUB_OUTPUT, Object.entries(checks).map(([key, run]) => `${key}=${run}\n`).join('')
            + `checks=${Object.entries(checks).filter(([key, run]) => run && !['format', 'tools'].includes(key)).map(([key]) => key).join(',') || 'none'}\n`);
    }
}
