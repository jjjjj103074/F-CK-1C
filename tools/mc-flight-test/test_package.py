import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

import guide
import package_test_bundle as package

class TestFlightBundle(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.source = 'a' * 40
        self.entries = {
            package.DLL: b'MZvalidated-test-dll',
            package.DEVICE: b'creators = {}\nindicators = {}\nneed_to_be_closed = true\n',
            package.MODULE + 'Cockpit/Scripts/devices.lua': b'devices = { CMS = 3, AUTOPILOT = 9 }',
            package.MODULE + 'Input/F-CK-1C/keyboard/default.lua': b'unchanged default bindings',
            package.MODULE + 'entry.lua': b'unchanged runtime registration',
        }
        self.archive = self.root / 'module.zip'
        self.manifest = self.root / 'manifest.json'
        self.catalog = self.root / 'catalog.json'
        self.manifest.write_text(json.dumps(dict(source_commit=self.source, source_tree='b'*40,
            dll_sha256=package.digest(self.entries[package.DLL]), validation='passed', dcs_runtime='not_run')))
        self.catalog.write_text(json.dumps({'cockpit_params': [
            dict(name='McGeneration', value='CPP_MC_GENERATION'),
            dict(name='WeaponStation01', value='OBS_WEAPON_STATION_01'),
            dict(name='HmcsHeadingSlotTickPrefix', value='not_a_numeric_parameter')],
            'raw_dcs_cockpit_params': [dict(name='IrLock', value='WS_IR_MISSILE_LOCK')]}))
    def tearDown(self):
        self.temp.cleanup()
    def write_zip(self, entries=None, windows_paths=False):
        with zipfile.ZipFile(self.archive, 'w') as archive:
            for path, content in (entries or self.entries).items():
                archive.writestr(path.replace('/', '\\') if windows_paths else path, content)
    def assemble(self):
        return package.assemble(self.archive, self.manifest, self.catalog, self.root / 'out', self.source)
    def test_preserves_dll_inputs_and_existing_runtime(self):
        self.write_zip(windows_paths=True)
        bundle = self.assemble()
        with zipfile.ZipFile(bundle) as archive:
            for name, content in self.entries.items():
                if name != package.DEVICE:
                    self.assertEqual(archive.read(name), content)
            self.assertTrue(archive.read(package.DEVICE).startswith(self.entries[package.DEVICE]))
            build = json.loads(archive.read('BUILD.json'))
            self.assertEqual(build['runtime_changed'], [package.DEVICE])
            self.assertEqual(build['human_cases'], 69)
            self.assertIn(self.source, archive.read(package.TEST_ROOT + 'Build.lua').decode())
        self.assertIn(package.digest(bundle.read_bytes()), Path(str(bundle)+'.sha256').read_text())
    def test_rejects_wrong_dll(self):
        self.entries[package.DLL] = b'MZwrong-build'
        self.write_zip()
        with self.assertRaisesRegex(ValueError, 'DLL hash'): self.assemble()
    def test_rejects_wrong_source(self):
        self.write_zip()
        data=json.loads(self.manifest.read_text()); data['source_commit']='c'*40
        self.manifest.write_text(json.dumps(data))
        with self.assertRaisesRegex(ValueError, 'source revision'): self.assemble()
    def test_rejects_failed_validation(self):
        self.write_zip()
        data=json.loads(self.manifest.read_text()); data['validation']='failed'
        self.manifest.write_text(json.dumps(data))
        with self.assertRaisesRegex(ValueError, 'validation'): self.assemble()
    def test_rejects_traversal(self):
        self.entries[package.MODULE + '../outside'] = b'bad'
        self.write_zip()
        with self.assertRaisesRegex(ValueError, 'ZIP path'): self.assemble()
    def test_rejects_device_collision(self):
        self.entries[package.MODULE + 'Cockpit/Scripts/devices.lua']=b'devices = { Other = 10, }'
        self.write_zip()
        with self.assertRaisesRegex(ValueError, 'slot 10'): self.assemble()
    def test_guide_has_each_case_and_default_key(self):
        data=guide.cases()
        self.assertEqual(len({case['id'] for case in data}),len(data))
        document=guide.html_document()
        for case in data: self.assertIn('id="'+case['id']+'"',document)
        for _, key in guide.KEYS: self.assertIn(key, guide.markdown())
        self.assertNotIn('<script',document)
    def test_recorder_and_indicator_are_passive(self):
        for name in ['Recorder.lua','Indicator_page.lua']:
            source=(package.ROOT/name).read_text()
            self.assertNotIn('dispatch_action', source)
            self.assertNotIn('performClickableAction', source)
            self.assertNotIn(':set(', source)
    def test_deduplicates_existing_parameters(self):
        data=json.loads(self.catalog.read_text())
        data['raw_dcs_cockpit_params'].append(dict(name='duplicate',value='CPP_MC_GENERATION'))
        self.assertEqual(package.parameter_names(data),['CPP_MC_GENERATION','OBS_WEAPON_STATION_01','WS_IR_MISSILE_LOCK'])

if __name__ == '__main__': unittest.main()
