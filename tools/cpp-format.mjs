import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const projectRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const extensions = new Set(['.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.inc', '.inl', '.ipp', '.tpp']);

function execute(command, args, options = {}) {
    const result = spawnSync(command, args, {
        encoding: 'utf8',
        maxBuffer: 16 * 1024 * 1024,
        windowsHide: true,
        ...options,
    });
    if (result.error || result.status !== 0) {
        throw new Error(result.error?.message || result.stderr.trim() || `${command} exited with ${result.status}`);
    }
    return result.stdout;
}

function git(root, args) {
    return execute('git', args, { cwd: root });
}

function pathsFromGit(root, args) {
    return git(root, args).split('\0').filter(Boolean);
}

export function isOwnedCpp(file, roots) {
    return roots.some((root) => file.startsWith(`${root}/`))
        && extensions.has(path.posix.extname(file).toLowerCase())
        && !file.startsWith('src/efm/F-CK-1C_EFM/include/')
        && !file.startsWith('src/efm/F-CK-1C_EFM/ThirdParty/')
        && !/\.g\.[^.]+$/i.test(file);
}

function assertStagedSettings(root, candidates) {
    const cpp = candidates.filter((file) => file.startsWith('src/efm/')
        && extensions.has(path.posix.extname(file).toLowerCase()));
    if (!cpp.length) return;
    const settings = new Set(['package.json', 'package-lock.json']);
    for (const file of cpp) {
        let directory = path.posix.dirname(file);
        while (true) {
            for (const name of ['.clang-format', '_clang-format', '.clang-format-ignore']) {
                settings.add(directory === '.' ? name : `${directory}/${name}`);
            }
            if (directory === '.') break;
            directory = path.posix.dirname(directory);
        }
    }
    const dirty = [...new Set([
        ...pathsFromGit(root, ['diff', '--name-only', '-z']),
        // Include ignored overrides: clang-format can still read those files.
        ...pathsFromGit(root, ['ls-files', '--others', '-z', '--',
            ...[...settings].filter((file) => fs.existsSync(path.join(root, file)))]),
    ])].filter((file) => settings.has(file));
    if (dirty.length) {
        throw new Error(`Formatting settings have unstaged changes: ${dirty.join(', ')}.\n`
            + 'Stage the intended settings or restore them before checking staged C++.');
    }
}

export function formatterPath() {
    const manifest = JSON.parse(fs.readFileSync(path.join(projectRoot, 'package.json'), 'utf8'));
    const expected = manifest.devDependencies['@polycam/clang-tools'];
    let executable;
    try {
        const installed = require('@polycam/clang-tools/package.json');
        if (installed.version !== expected) {
            throw new Error(`Installed ${installed.version}; expected ${expected}.`);
        }
        executable = require('@polycam/clang-tools').clangFormatPath();
    } catch (error) {
        throw new Error(`C++ formatter is unavailable: ${error.message}\nRun npm ci with optional dependencies enabled.`);
    }
    const version = execute(executable, ['--version']).trim();
    if (!version.startsWith(`clang-format version ${expected} `) && version !== `clang-format version ${expected}`) {
        throw new Error(`Expected clang-format ${expected}, received: ${version}\nRun npm ci and remove any CLANG_FORMAT_PATH override.`);
    }
    return executable;
}

export function run(args, root = projectRoot) {
    const valid = new Set(['--setup', '--write', '--check', '--staged', '--all', '--help']);
    if (args.includes('--help')) {
        console.log('cpp-format.mjs --setup | --write [--all] | --check [--staged]');
        return 0;
    }
    if (args.some((arg) => !valid.has(arg))
        || args.filter((arg) => ['--setup', '--write', '--check'].includes(arg)).length !== 1
        || (args.includes('--staged') && !args.includes('--check'))
        || (args.includes('--setup') && args.length !== 1)
        || (args.includes('--all') && (!args.includes('--write') || args.includes('--staged')))) {
        throw new Error('Use --setup, --write [--all], or --check [--staged].');
    }

    if (args.includes('--setup')) {
        console.log(`Pinned formatter ready: ${formatterPath()}\nUse this executable path in Visual Studio.`);
        return 0;
    }

    const staged = args.includes('--staged');
    const stagedCandidates = staged
        ? pathsFromGit(root, ['diff', '--no-renames', '--name-only', '-z', '--diff-filter=ACM', '--cached'])
        : [];
    if (staged) assertStagedSettings(root, stagedCandidates);
    const manifest = JSON.parse(fs.readFileSync(path.join(root, 'package.json'), 'utf8'));
    const roots = manifest.cppFormat.roots;
    if (!Array.isArray(roots) || !roots.length || roots.some((entry) =>
        typeof entry !== 'string' || !entry.startsWith('src/efm/')
        || entry.split('/').some((part) => !part || part === '.' || part === '..' || part.includes('\\')))) {
        throw new Error('cppFormat.roots must contain repository-relative src/efm directories without a trailing slash.');
    }

    const write = args.includes('--write');
    let candidates;
    const diff = ['diff', '--no-renames', '--name-only', '-z', '--diff-filter=ACM'];
    if (staged) {
        candidates = stagedCandidates;
    } else if (!write || args.includes('--all')) {
        candidates = pathsFromGit(root, ['ls-files', '-z', '--cached', '--others', '--exclude-standard']);
    } else {
        candidates = [
            ...pathsFromGit(root, diff),
            ...pathsFromGit(root, [...diff, '--cached']),
            ...pathsFromGit(root, ['ls-files', '-z', '--others', '--exclude-standard']),
        ];
    }
    const files = [...new Set(candidates)].filter((file) => isOwnedCpp(file, roots)).sort();
    if (!files.length) {
        console.log('C++ format: no files in the active scope.');
        return 0;
    }
    const executable = formatterPath();
    const mismatches = [];
    let processed = 0;
    for (const file of files) {
        const absolute = path.join(root, file);
        if (!staged && !fs.existsSync(absolute)) continue;
        if (!staged && !fs.lstatSync(absolute).isFile()) {
            throw new Error(`Expected a regular C++ file: ${file}`);
        }
        if (staged && !git(root, ['ls-files', '--stage', '--', file]).startsWith('100')) {
            throw new Error(`Expected a regular staged C++ file: ${file}`);
        }
        const source = staged ? git(root, ['show', `:${file}`]) : fs.readFileSync(absolute, 'utf8');
        const formatted = execute(executable, [
            '--style=file', '--fallback-style=none', '--Werror', '--fail-on-incomplete-format',
            `--assume-filename=${absolute}`,
        ], { cwd: root, input: source });
        if (source.length && !formatted.length) {
            throw new Error(`Formatter skipped or returned no content for ${file}; the source was preserved. Check .clang-format-ignore.`);
        }
        // Git normalizes CRLF to LF in the index; compare that stored representation.
        const normalized = (text) => staged ? text.replace(/\r\n/g, '\n') : text;
        if (normalized(source) !== normalized(formatted)) {
            mismatches.push(file);
            if (write) fs.writeFileSync(absolute, formatted, 'utf8');
        }
        processed += 1;
    }
    for (const file of mismatches) console.log(`${write ? 'Formatted' : 'Needs formatting'}: ${file}`);
    console.log(`C++ format: ${processed} checked, ${mismatches.length} ${write ? 'updated' : 'need formatting'}.`);
    if (!write && mismatches.length) {
        console.error('Run npm run format:cpp (or format:cpp:all for all authored C++), then stage the intended files again.');
        return 1;
    }
    return 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    try {
        process.exitCode = run(process.argv.slice(2));
    } catch (error) {
        console.error(`[C++ format] ${error.message}`);
        process.exitCode = 2;
    }
}
