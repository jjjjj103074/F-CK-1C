"""Generate DCS registration data from the shared JSON weapon catalog."""
import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
SOURCE = Path('FM/Weapons/WeaponCatalog.json')
OUTPUT = Path('F-CK-1C.lua')
BEGIN = '-- BEGIN GENERATED WEAPON GROUPS'
END = '-- END GENERATED WEAPON GROUPS'
BINDINGS = {
    'aam_light': 'WPN_AAM_Light', 'aam_medium': 'WPN_AAM_Med',
    'ground_light': 'WPN_AG_LIGHT', 'ground_heavy': 'WPN_AG_HEAVY',
    'targeting_pod': 'WPN_POD_Targeting', 'misc_pod': 'WPN_POD_Misc', 'tank': 'WPN_TANK_Standard',
}


def lua_string(value):
    escaped = []
    for character in value:
        if character in ('\\', '"'):
            escaped.append('\\' + character)
        elif ord(character) < 32:
            escaped.append('\\%03d' % ord(character))
        else:
            escaped.append(character)
    return '"' + ''.join(escaped) + '"'


def lua_value(value, depth=0):
    if isinstance(value, str):
        return lua_string(value)
    if isinstance(value, bool):
        return 'true' if value else 'false'
    if value is None:
        raise ValueError('Weapon catalog fields must have explicit values')
    if isinstance(value, (int, float)):
        return json.dumps(value, allow_nan=False)
    indent = '    ' * (depth + 1)
    if isinstance(value, list):
        entries = [lua_value(item, depth + 1) for item in value]
    else:
        entries = ['[' + lua_string(key) + '] = ' + lua_value(item, depth + 1)
                   for key, item in value.items()]
    if not entries:
        return '{}'
    return '{\n' + ''.join(indent + entry + ',\n' for entry in entries) + '    ' * depth + '}'


def build_groups(data):
    if data.get('schema_version') != 1:
        raise ValueError('Unsupported weapon catalog schema')
    groups = {}
    for group in data['groups']:
        name = group['id']
        if name not in BINDINGS or name in groups or not isinstance(group['air_to_air'], bool):
            raise ValueError('Invalid weapon registration group: ' + name)
        groups[name] = []
    identities = set()
    models = set()
    for store in data['stores']:
        if not store['id'] or store['id'] in models:
            raise ValueError('Duplicate weapon model')
        models.add(store['id'])
        for identifier in store['aliases']:
            if not identifier or identifier in identities:
                raise ValueError('Duplicate weapon alias')
            identities.add(identifier)
        for loadout in store['loadouts']:
            identifier = loadout['clsid']
            if identifier in identities or 'CLSID' in loadout['parameters']:
                raise ValueError('Duplicate loadout or reserved CLSID parameter')
            identities.add(identifier)
            group = groups[loadout['group']]
            group.append(dict(CLSID=identifier, **loadout['parameters']))
    if set(groups) != set(BINDINGS):
        raise ValueError('Weapon registration groups are incomplete')
    return groups


def generate(root):
    data = json.loads((root / SOURCE).read_text(encoding='utf-8-sig'))
    groups = build_groups(data)
    lines = [BEGIN, '-- 由共用 JSON 產生機型註冊的武器群組。']
    for group in data['groups']:
        name = group['id']
        value = groups[name]
        entries = ['    _isAAM = true,'] if group['air_to_air'] else []
        for loadout in value:
            entries.append('    ' + lua_value(loadout, 1) + ',')
        lines += [BINDINGS[name] + ' = wpnGroup({'] + entries + ['})', '']
    lines.append(END)
    source = (root / OUTPUT).read_text(encoding='utf-8-sig')
    if source.count(BEGIN) != 1 or source.count(END) != 1:
        raise ValueError('Aircraft weapon registration markers must be unique')
    start = source.index(BEGIN)
    end = source.index(END, start) + len(END)
    return (source[:start] + '\n'.join(lines) + source[end:]).replace('\n', '\r\n').encode('utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--root', type=Path, default=ROOT)
    args = parser.parse_args()
    expected = generate(args.root)
    output = args.root / OUTPUT
    if args.check:
        if not output.is_file() or output.read_bytes().replace(b'\r\n', b'\n') != expected.replace(b'\r\n', b'\n'):
            print('Weapon catalog Lua data is stale; run python tools/generate_weapon_catalog.py', file=sys.stderr)
            return 1
        print('Aircraft weapon registration matches shared JSON.')
    else:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(expected)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
