"""Assemble MC regression and native countermeasure tests around a CI-validated module."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import zipfile

import guide
import radar_guide
import status_guide

ROOT = Path(__file__).resolve().parent
MODULE = 'F-CK-1C/'
DLL = MODULE + 'bin/F-CK-1C_EFM.dll'
DEVICE = MODULE + 'Cockpit/Scripts/device_init.lua'
TEST_ROOT = MODULE + 'Cockpit/Scripts/MCFlightTest/'
KEYBOARD = MODULE + 'Input/F-CK-1C/keyboard/default.lua'
WEAPONS = MODULE + 'Cockpit/Scripts/Systems/weapon_system.lua'
PROBE_FLAG = MODULE + 'FM/native-countermeasure-probe.enabled'
OBSERVATIONS = ['FCK1C_TEST_CM_OBSERVATION_AVAILABLE', 'FCK1C_TEST_CM_FLARE_COUNT', 'FCK1C_TEST_CM_CHAFF_COUNT']

def digest(data):
    return hashlib.sha256(data).hexdigest()

def read_module(path):
    entries = {}
    with zipfile.ZipFile(path) as archive:
        for entry in archive.infolist():
            name = entry.filename.replace('\\', '/')
            if entry.is_dir() or name.endswith('/'):
                continue
            parts = PurePosixPath(name).parts
            if name.startswith('/') or '..' in parts or not name.startswith(MODULE):
                raise ValueError('Unexpected module ZIP path: ' + name)
            if name in entries:
                raise ValueError('Duplicate module ZIP path: ' + name)
            entries[name] = archive.read(entry)
    for required in (DLL, DEVICE, MODULE + 'Cockpit/Scripts/devices.lua', KEYBOARD, WEAPONS):
        if required not in entries:
            raise ValueError('Missing runtime file: ' + required)
    return entries

def parameter_names(catalog):
    names = []
    for item in catalog['cockpit_params']:
        key = item['name']
        if (key.startswith(('Mc', 'Weapon', 'Radar', 'NativeCm', 'Cm', 'FlightStatus', 'Infrared'))
                or key in {'HmcsMasterMode', 'HmcsFireControlMode', 'HmcsAamSubmode', 'HmcsGunFiring',
                           'HmcsWeaponClass', 'HmcsWeaponQuantity', 'HmcsWeaponQuantityAvailable'}):
            names.append(item['value'])
    for item in catalog['raw_dcs_cockpit_params']:
        value = item['value']
        if re.search(r'%02[du]', value):
            names.extend(value % index for index in range(1, 33))
        else:
            names.append(value)
    return list(dict.fromkeys(names + OBSERVATIONS))

def assemble(module_zip, manifest_path, catalog_path, output, expected_source, test_tools_source=None, focus="mc", runtime_source=None):
    if focus not in {"mc", "radar", "status"}:
        raise ValueError("Unknown flight-test focus: " + focus)
    selected_guide = {"mc": guide, "radar": radar_guide, "status": status_guide}[focus]
    manifest = json.loads(Path(manifest_path).read_text(encoding='utf-8-sig'))
    if manifest.get('validation') != 'passed' or manifest.get('source_commit') != expected_source:
        raise ValueError('Module validation/source revision does not match the requested source.')
    if not re.fullmatch('[0-9a-f]{40}', expected_source):
        raise ValueError('Expected source must be a full commit SHA.')
    runtime_source = runtime_source or expected_source
    if not re.fullmatch("[0-9a-f]{40}", runtime_source):
        raise ValueError("Runtime source must be a full commit SHA.")
    test_tools_source = test_tools_source or runtime_source
    if not re.fullmatch('[0-9a-f]{40}', test_tools_source):
        raise ValueError('Test-tools source must be a full commit SHA.')
    original = read_module(module_zip)
    if digest(original[DLL]) != manifest.get('dll_sha256'):
        raise ValueError('Module DLL hash differs from the validated DLL manifest.')
    if any(name.startswith(TEST_ROOT) for name in original):
        raise ValueError('Module already contains a flight-test overlay.')
    devices = original[MODULE + 'Cockpit/Scripts/devices.lua'].decode('utf-8-sig')
    if re.search(r'=\s*10\s*[,}]', devices):
        raise ValueError('Flight-test device slot 10 is already declared.')
    source = original[DEVICE].decode('utf-8-sig')
    if re.search(r'creators\s*\[\s*10\s*\]', source):
        raise ValueError('Flight-test device slot 10 is already registered.')
    additions = '''
-- 註冊本測試包的參數紀錄與唯讀面板。
creators[10] = { "avLuaDevice", LockOn_Options.script_path .. "MCFlightTest/Recorder.lua" }
indicators[#indicators + 1] = { "ccControlsIndicatorBase", LockOn_Options.script_path .. "MCFlightTest/Indicator_init.lua" }
'''
    if focus in {'radar', 'status'}:
        required = ('Forward_init.lua', 'Forward_page.lua', 'Scope_init.lua', 'Scope_page.lua', 'Drawing.lua')
        for filename in required:
            if MODULE + 'Cockpit/Scripts/RadarDisplay/' + filename not in original:
                raise ValueError('Missing graphical radar runtime: ' + filename)
        additions = '\ncreators[10] = { "avLuaDevice", LockOn_Options.script_path .. "MCFlightTest/Recorder.lua" }\n'
    if focus == 'status':
        for filename in ('Indicator_init.lua', 'Indicator_page.lua', 'Status.lua'):
            if MODULE + 'Cockpit/Scripts/FlightStatus/' + filename not in original:
                raise ValueError('Missing flight status runtime: ' + filename)
    for required in ('FM/Weapons/WeaponCatalog.json', 'Cockpit/Scripts/Stores/WeaponCatalog.lua',
                     'Cockpit/Scripts/ThirdParty/json/json.lua'):
        if MODULE + required not in original:
            raise ValueError('Missing shared weapon catalog runtime: ' + required)
    entries = dict(original)
    entries[DEVICE] = original[DEVICE] + additions.replace('\n', '\r\n').encode('utf-8')
    keyboard = original[KEYBOARD]
    marker = b'return res'
    if keyboard.count(marker) != 1 or not keyboard.rstrip().endswith(marker):
        raise ValueError('Unexpected keyboard profile return statement.')
    binding = b'dofile(cscripts .. "MCFlightTest/NativeCountermeasureKeys.lua")(res.keyCommands, device_commands)\r\n\r\n'
    entries[KEYBOARD] = keyboard.replace(marker, binding + marker)
    observation = '''
-- 在原有掛點更新後取樣測試包的反制存量。
local native_cm_observer = dofile(LockOn_Options.script_path .. "MCFlightTest/NativeCountermeasureObservation.lua").new(GetSelf())
local native_cm_production_update = update
function update()
    native_cm_production_update()
    native_cm_observer.update()
end
'''
    if focus == 'mc':
        entries[WEAPONS] = original[WEAPONS] + observation.replace('\n', '\r\n').encode('utf-8')
        entries[PROBE_FLAG] = b'Native C++ countermeasure flight probe enabled by this test bundle.\r\n'
    files = ['Recorder.lua', 'NativeCountermeasureKeys.lua']
    if focus == 'mc':
        files += ['Indicator_init.lua', 'Indicator_page.lua', 'NativeCountermeasureObservation.lua']
    for filename in files:
        entries[TEST_ROOT + filename] = (ROOT / filename).read_bytes()
    names = parameter_names(json.loads(Path(catalog_path).read_text(encoding='utf-8-sig')))
    entries[TEST_ROOT + 'Parameters.lua'] = ('return {\n' + ''.join('    ' + json.dumps(name) + ',\n' for name in names) + '}\n').encode()
    entries[TEST_ROOT + 'Build.lua'] = ('return { source_commit = ' + json.dumps(expected_source)
        + ', runtime_source_commit = ' + json.dumps(runtime_source)
        + ', dll_sha256 = ' + json.dumps(manifest['dll_sha256']) + ' }\n').encode()
    changed = [name for name in original if original[name] != entries[name]]
    assert set(changed) == ({DEVICE, KEYBOARD, WEAPONS} if focus == 'mc' else {DEVICE, KEYBOARD})
    assert entries[DLL] == original[DLL]
    for filename in ('COLLECT-RESULTS.bat', 'COLLECT-RESULTS.ps1'):
        entries[filename] = (ROOT / filename).read_bytes()
    entries['START-HERE.zh-TW.html'] = selected_guide.html_document().encode('utf-8')
    entries['TESTING.zh-TW.md'] = selected_guide.markdown().encode('utf-8')
    entries['RESULTS.csv'] = b'\xef\xbb\xbf' + selected_guide.results_csv().encode('utf-8')
    entries['TEST-CASES.json'] = (json.dumps(selected_guide.cases(), ensure_ascii=False, indent=2) + '\n').encode('utf-8')
    build = dict(manifest, package_kind=focus + '-flight-test', focus=focus, dcs_runtime='not_run',
        test_tools_source_commit=test_tools_source, runtime_source_commit=runtime_source,
        validation_scope="native DLL at source_commit; runtime Lua checked separately",
        base_module_sha256=digest(Path(module_zip).read_bytes()),
        human_cases=len(selected_guide.cases()), test_device_id=10, recorded_parameters=len(names),
        native_countermeasure_probe=focus == "mc", native_dispatch='C++ ed_fm_simulate',
        native_test_keys='prebound; existing bindings preserved',
        migration_goal={'status': 'screen-fixed flight and confirmed SM weapon status', 'radar': 'RWS/BORE/STT graphical radar displays', 'mc': 'MC SOI, mode-driven display assignments and real-aircraft HOTAS interfaces'}[focus],
        runtime_changed=changed,
        runtime_added=sorted(name for name in set(entries) - set(original) if name.startswith(MODULE)),
        bundle_added=sorted({name for name in set(entries) - set(original) if not name.startswith(MODULE)} | {'BUILD.json'}),
        runtime_preserved_files=len(original) - len(changed))
    entries['BUILD.json'] = (json.dumps(build, ensure_ascii=False, indent=2) + '\n').encode('utf-8')
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    name = 'F-CK-1C-' + focus + '-flight-test-' + runtime_source[:8] + '.zip'
    archive_path = output / name
    with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path, content in entries.items():
            archive.writestr(path, content)
    hash_value = digest(archive_path.read_bytes())
    (output / (name + '.sha256')).write_text(hash_value + ' *' + name + '\n', encoding='ascii')
    (output / 'BUILD.json').write_text(json.dumps(build, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    (output / 'START-HERE.zh-TW.html').write_bytes(entries['START-HERE.zh-TW.html'])
    (output / 'TESTING.zh-TW.md').write_bytes(entries['TESTING.zh-TW.md'])
    with zipfile.ZipFile(archive_path) as archive:
        assert archive.testzip() is None
        assert digest(archive.read(DLL)) == manifest['dll_sha256']
        assert all(archive.read(path) == value for path, value in entries.items())
    print(f'MC flight test: {len(selected_guide.cases())} cases; {len(names)} recorded parameters; {archive_path}')
    return archive_path

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module-zip', required=True, type=Path)
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--catalog', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--expected-source', required=True)
    parser.add_argument('--test-tools-source', help='Source revision of the test overlay; defaults to the validated module revision.')
    parser.add_argument('--runtime-source', help='Source revision of the packaged runtime; defaults to the validated DLL revision.')
    parser.add_argument('--focus', choices=['mc', 'radar', 'status'], default='mc')
    args = parser.parse_args()
    assemble(args.module_zip, args.manifest, args.catalog, args.output, args.expected_source, args.test_tools_source, args.focus, args.runtime_source)
