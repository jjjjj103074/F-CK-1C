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
            package.KEYBOARD: b'local res = { keyCommands = {} }\n-- existing bindings\nreturn res\n',
            package.WEAPONS: b'function update() end\n',
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
                if name not in {package.DEVICE, package.KEYBOARD, package.WEAPONS}:
                    self.assertEqual(archive.read(name), content)
            self.assertTrue(archive.read(package.DEVICE).startswith(self.entries[package.DEVICE]))
            build = json.loads(archive.read('BUILD.json'))
            self.assertEqual(set(build['runtime_changed']), {package.DEVICE, package.KEYBOARD, package.WEAPONS})
            addition = b'dofile(cscripts .. "MCFlightTest/NativeCountermeasureKeys.lua")(res.keyCommands, device_commands)\r\n\r\n'
            self.assertEqual(archive.read(package.KEYBOARD).replace(addition, b''), self.entries[package.KEYBOARD])
            self.assertTrue(archive.read(package.WEAPONS).startswith(self.entries[package.WEAPONS]))
            self.assertIn(package.PROBE_FLAG, archive.namelist())
            self.assertTrue(build['native_countermeasure_probe'])
            self.assertEqual(build['human_cases'], 86)
            self.assertIn(self.source, archive.read(package.TEST_ROOT + 'Build.lua').decode())
        self.assertIn(package.digest(bundle.read_bytes()), Path(str(bundle)+'.sha256').read_text())
    def test_radar_focus_keeps_graphical_runtime_without_legacy_panel(self):
        for filename in ('Forward_init.lua', 'Forward_page.lua', 'Scope_init.lua', 'Scope_page.lua', 'Drawing.lua'):
            self.entries[package.MODULE+'Cockpit/Scripts/RadarDisplay/'+filename] = b'graphical-runtime'
        self.write_zip()
        bundle = package.assemble(self.archive, self.manifest, self.catalog, self.root/'radar', self.source, focus='radar')
        with zipfile.ZipFile(bundle) as archive:
            build = json.loads(archive.read('BUILD.json'))
            self.assertEqual(build['focus'], 'radar')
            self.assertFalse(build['native_countermeasure_probe'])
            self.assertNotIn(package.PROBE_FLAG, archive.namelist())
            self.assertNotIn(package.TEST_ROOT+'Indicator_page.lua', archive.namelist())
            self.assertEqual(archive.read(package.WEAPONS), self.entries[package.WEAPONS])
            self.assertEqual(archive.read(package.DLL), self.entries[package.DLL])
            self.assertNotIn(b'MCFlightTest/Indicator_init.lua', archive.read(package.DEVICE))
            self.assertEqual(build['human_cases'], 12)
            self.assertIn('右 Alt＋I', archive.read('TESTING.zh-TW.md').decode())
            self.assertIn('四個角', archive.read('TESTING.zh-TW.md').decode())

    def test_status_focus_records_confirmed_selection_and_display_text(self):
        for folder, filenames in {
            'RadarDisplay': ('Forward_init.lua', 'Forward_page.lua', 'Scope_init.lua', 'Scope_page.lua', 'Drawing.lua'),
            'FlightStatus': ('Indicator_init.lua', 'Indicator_page.lua', 'Status.lua', 'StoreNames.lua'),
        }.items():
            for filename in filenames:
                self.entries[package.MODULE+'Cockpit/Scripts/'+folder+'/'+filename] = b'graphical-runtime'
        data = json.loads(self.catalog.read_text())
        data['cockpit_params'].extend([
            dict(name='McSelectedStoreIdentifier', value='CPP_MC_SELECTED_STORE_IDENTIFIER'),
            dict(name='FlightStatusMissile', value='FLIGHT_STATUS_MISSILE')])
        self.catalog.write_text(json.dumps(data))
        self.write_zip()
        bundle = package.assemble(self.archive, self.manifest, self.catalog, self.root/'status', self.source, focus='status')
        with zipfile.ZipFile(bundle) as archive:
            build = json.loads(archive.read('BUILD.json'))
            self.assertEqual(build['focus'], 'status')
            self.assertEqual(build['human_cases'], 12)
            self.assertFalse(build['native_countermeasure_probe'])
            self.assertNotIn(package.PROBE_FLAG, archive.namelist())
            self.assertNotIn(package.TEST_ROOT+'Indicator_page.lua', archive.namelist())
            self.assertEqual(archive.read(package.WEAPONS), self.entries[package.WEAPONS])
            self.assertEqual(archive.read(package.DLL), self.entries[package.DLL])
            schema = archive.read(package.TEST_ROOT+'Parameters.lua').decode()
            self.assertIn('CPP_MC_SELECTED_STORE_IDENTIFIER', schema)
            self.assertIn('FLIGHT_STATUS_MISSILE', schema)
            guide = archive.read('TESTING.zh-TW.md').decode()
            self.assertIn('短按 Q', guide)
            self.assertIn('右 Ctrl＋D', guide)
            self.assertIn('所有冒號對齊', guide)

    def test_raw_return_patterns_expand_to_real_parameter_names(self):
        values = package.parameter_names({'cockpit_params': [], 'raw_dcs_cockpit_params': [
            {'value': 'RADAR_CONTACT_%02u_RANGE'}]})
        self.assertIn('RADAR_CONTACT_01_RANGE', values)
        self.assertIn('RADAR_CONTACT_32_RANGE', values)
        self.assertNotIn('RADAR_CONTACT_%02u_RANGE', values)

    def test_records_cm_progress_and_separate_test_tools_revision(self):
        data=json.loads(self.catalog.read_text())
        data['cockpit_params'].extend([
            dict(name='CmAcceptedSequence', value='CPP_CM_ACCEPTED_SEQUENCE'),
            dict(name='CmFlareCount', value='CPP_CM_FLARE_COUNT'),
            dict(name='CmChaffCount', value='CPP_CM_CHAFF_COUNT')])
        self.catalog.write_text(json.dumps(data))
        self.write_zip()
        bundle=package.assemble(self.archive, self.manifest, self.catalog, self.root/'out', self.source, 'c'*40)
        with zipfile.ZipFile(bundle) as archive:
            schema=archive.read(package.TEST_ROOT+'Parameters.lua').decode()
            for name in ['CPP_CM_ACCEPTED_SEQUENCE', 'CPP_CM_FLARE_COUNT', 'CPP_CM_CHAFF_COUNT']:
                self.assertIn(name, schema)
            build=json.loads(archive.read('BUILD.json'))
            self.assertEqual(build['source_commit'], self.source)
            self.assertEqual(build['test_tools_source_commit'], 'c'*40)
            self.assertEqual(archive.read(package.DLL), self.entries[package.DLL])
    def test_runtime_source_is_distinct_from_validated_dll_source(self):
        self.write_zip()
        runtime = 'd' * 40
        bundle = package.assemble(self.archive, self.manifest, self.catalog, self.root/'lua', self.source, runtime_source=runtime)
        self.assertIn(runtime[:8], bundle.name)
        with zipfile.ZipFile(bundle) as archive:
            build = json.loads(archive.read('BUILD.json'))
            self.assertEqual(build['source_commit'], self.source)
            self.assertEqual(build['runtime_source_commit'], runtime)
            self.assertEqual(build['test_tools_source_commit'], runtime)
            self.assertEqual(archive.read(package.DLL), self.entries[package.DLL])
            self.assertIn(runtime, archive.read(package.TEST_ROOT+'Build.lua').decode())

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
            source=(package.ROOT/name).read_text(encoding='utf-8')
            self.assertNotIn('dispatch_action', source)
            self.assertNotIn('performClickableAction', source)
            self.assertNotIn(':set(', source)
    def test_observer_has_no_actuation(self):
        source=(package.ROOT/'NativeCountermeasureObservation.lua').read_text(encoding='utf-8')
        for forbidden in ('dispatch_action', 'drop_flare', 'drop_chaff', 'performClickableAction'):
            self.assertNotIn(forbidden, source)
    def test_rejects_changed_keyboard_shape(self):
        self.entries[package.KEYBOARD] = b'return another_profile'
        self.write_zip()
        with self.assertRaisesRegex(ValueError, 'keyboard profile'): self.assemble()
    def test_deduplicates_existing_parameters(self):
        data=json.loads(self.catalog.read_text())
        data['raw_dcs_cockpit_params'].append(dict(name='duplicate',value='CPP_MC_GENERATION'))
        self.assertEqual(package.parameter_names(data),['CPP_MC_GENERATION','OBS_WEAPON_STATION_01','WS_IR_MISSILE_LOCK'] + package.OBSERVATIONS)

if __name__ == '__main__': unittest.main()
