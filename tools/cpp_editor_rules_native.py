#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Edit 시나리오의 기대 설정·방향별 동맹·Off 수명을 원본 기계어에서 먼저 관측한다."""
import argparse
import csv
import hashlib
import json
import socket
import struct
from pathlib import Path
from decomp_mission_start import Oracle as MissionOracle, ROOT
from decomp_ai_brain import Oracle as BrainOracle, PLAYERS, BASE, POOL
from decomp_ai_runtime import RuntimeOracle, BODY, SIZE
from decomp_priestmenu import Oracle as MemoryOracle, TEXT, TYPES, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP
from fort import TypeCatalog, split_sections, SECTION_NAMES, read_chunks

FIELDS = {'startingMoney': 0x22c, 'allyMask': 0x23c, 'abilityMask': 0x248, 'color': 0x250, 'off': 0x254}  # 살아 있는 원문600BYTE에서 대조할DWORD다.


def sha(path):
    """관측과 기대값의 최초 바이트를 해시한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    """새 결과만 UTF-8/LF로 저장하며 기존 fixture를 수정하지 않는다."""
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def scenarios():
    """실제 사용자 언어파일에 넣을 세 독립 입력을 실행 전에 고정한다."""
    common = {'missionType': 'Tutorial', 'missionNumber': '1', 'moreGeysers': '0', 'myStartMoney': '12345', 'myTech': 'all',
              'myAllyList': '2', 'aiName': 'CommonAI', 'aiTech': 'all', 'aiStartMoney': '22222', 'aiColor': 'purple',
              'aiAbility': '!BUILD_BRIDGES;!CAPTURE_ENEMY_PRIEST', 'aiAllyList': '', 'ai2Name': 'ScenarioMyOnly',
              'ai2StartMoney': '23456', 'ai2Color': 'none', 'ai2AllyList': '', 'ai2Off': '0',
              'ai2Ability': 'BUILD_BRIDGES;!SACRIFICE_ENEMY_PRIEST', 'ai3Name': 'ScenarioThird', 'ai3StartMoney': '0', 'ai3AllyList': '',
              'ai4Name': 'NoPriestMustNotCreate', 'ai4AllyList': '1'}
    variants = [('my-only', {}), ('directed', {'aiAllyList': '1', 'ai2AllyList': '3', 'ai2Color': 'green', 'ai2Name': 'ScenarioDirected'}),
                ('off', {'aiAllyList': '1', 'ai2AllyList': '3', 'ai2Color': 'green', 'ai2Name': 'ScenarioOff', 'ai2Off': '1'})]
    for name, changes in variants:  # 모든 변경은 입력에만 적용하고 원본 반환으로 입력을 바꾸지 않는다.
        config = dict(common); config.update(changes); config['title'] = 'TEST02'
        yield {'case': name, 'config': config}


def ranges(entries):
    """현재 PC의 마지막 전체 정밀 표에서 선택된 함수의 정확한 범위만 읽는다."""
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    result = []
    for entry in entries:  # 오래된 부분 export 폴더를 합성하지 않고 현재 전체 표의 각 범위를 요구한다.
        for part in facts[entry]['ranges'].split(';'):  # 서로 떨어진 함수 구간도 원문대로 구별한다.
            low, high = (int(value, 16) for value in part.split('-')); result.append((low, high + 1))
    return result


def fort_input():
    """클론 보고와 별도로 보호 Fort의 레코드·원문 타입 플래그에서 사제 소유자를 읽는다."""
    path = ROOT / 'originals/d/TEST02.fort'; catalog = TypeCatalog(str(ROOT / 'originals')); sections, end = split_sections(path.read_bytes())
    named = dict(zip(SECTION_NAMES, sections)); conversion = catalog.conversion(named.get('TypeNames', b'')); priests = []
    for name, section in named.items():  # 원래 각 지형 섹션의 오브젝트만 독립적으로 파싱한다.
        if (name == 'Chaff' or name.startswith('Terr') and name[4:].isdigit()) and len(section) > 1:
            version, chunks = read_chunks(section, conversion, catalog)
            for chunk in chunks:  # 원래 순서의 모든 청크를 방문한다.
                for obj in chunk:  # 사제 플래그가 실제로 있는 타입의 저장 owner만 읽는다.
                    if catalog.flags[obj.type_name][1] & 0x200000: priests.append({'section': name, 'type': obj.type_name, 'owner': obj.fields['owner']})
    owners = sorted({item['owner'] for item in priests}); assert owners == [1, 2, 3], owners
    return {'file': path.relative_to(ROOT).as_posix(), 'sha256': sha(path), 'priests': priests, 'owners': owners, 'mask': sum(1 << owner for owner in owners)}


class CurrentMissionOracle(MissionOracle):
    """기존 명시 경계는 재사용하고 현재 전체 정밀 범위로만 원본 코드를 허용한다."""
    def __init__(self):
        """현재 PE/분리 storage와 기존 실제 CRT float 반환 경계를 구성한다."""
        MemoryOracle.__init__(self)
        selected = json.loads((ROOT / 'tools/ghidra/mission-start-functions.json').read_text(encoding='utf-8'))['editions']['originals']
        self.allowed = ranges([int(value, 16) for value in selected] + [0x4e49c0, 0x4911a0, 0x48f550])  # 현재 표에 없는 함수는 시작 전에 실패한다.
        self.mu.mem_map(0, 0x1000); self.mu.mem_write(STOP + 0x100, b'\xdd\x05' + struct.pack('<I', TEXT + 0xff00) + b'\xc3')
        self.allowed.append((STOP + 0x100, STOP + 0x107))


class DiagonalBrainOracle(BrainOracle):
    """기존 생성자 관측의 caller 입력을 실제 새 Session의 자기 동맹9행으로 제공한다."""
    def __init__(self):
        """현재 전체 정밀 표의 생성자/소멸자와 방향별 동맹 몸체만 선택한다."""
        MemoryOracle.__init__(self); self.mu.mem_map(BASE, 0x100000); self.mu.mem_map(POOL + 0x100000, 0x100000); self.mu.mem_map(0, 0x1000)
        self.allowed = ranges([0x416860, 0x410100, 0x490ea0, 0x48f6a0, 0x48f550])  # 기존 부분 export 부재와 무관하게 현재 표를 사용한다.

    def put(self, address, value, *args):
        """원본 본문 실행 전의 명시 fixture 쓰기만 조정하며 CPU 명령은 바꾸지 않는다."""
        if 0x595200 <= address < 0x595200 + 324 and value == 7: value = 0  # 기존 도구의 비정상 sentinel 입력 대신 비동맹0을 사용한다.
        super().put(address, value, *args)
        if address == 0x595344:
            for owner in range(9):  # 생성자 앞 Player 마스크의 실제 자기 비트만 공급한다.
                super().put(PLAYERS + owner * 180 + 0x54, 1 << owner)

    def run_pair(self, profiles):
        """실제 새 세계의 같은 Player/동맹표에서 owner2 다음3 생성자를 연속 실행한다."""
        self.next = BASE; self.blocks = []; self.frees = []; self.events = []; self.instructions = 0; self.seed = 19  # 기존 allocator의 독립 caller 입력이다.
        self.priest = 0; self.region = 127  # 이미 첫 지역이 있는 분기를 선택해 동맹 생성자 효과를 분리한다.
        self.mu.mem_write(PLAYERS, bytes(1620)); self.mu.mem_write(TYPES, bytes(48 * 500))
        for address, value in ((0x59534c, PLAYERS), (0x5c8464, POOL), (0x5c847c, 32768), (0x540c70, 1), (0x595344, 0), (0x594fc8, 0), (0x594fbc, 0), (0x541348, 48), (0x50c038, 127), (0x540410, 254)):
            self.put(address, value)  # 세계/TYPE/Chat 경계는 명시 입력이며 원본 생성자 명령은 그대로 실행한다.
        for owner in range(9):  # 처음에는 자기 비트만 있는 실제 새 Player/81DWORD를 공급한다.
            self.put(PLAYERS + owner * 180 + 0x78, 5)
            for target in range(9): self.put(0x595200 + (owner * 9 + target) * 4, int(owner == target))  # 방향마다 별도 DWORD다.
        bodies = []
        for owner, profile in profiles:  # 같은 공유 표를 초기화하지 않고 원본 순회 순서를 따른다.
            self.mu.mem_write(TYPES + 0x1f000, profile); token = self.allocate(0xb08)
            for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP): self.mu.reg_write(register, 0)  # 호출마다 일반 레지스터 입력을 고정한다.
            self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.mem_write(STACK, struct.pack('<III', STOP, owner, TYPES + 0x1f000)); self.mu.reg_write(UC_X86_REG_ECX, token)
            self.mu.emu_start(0x416860, STOP); bodies.append({'owner': owner, 'body': bytes(self.mu.mem_read(token, 0xb08)).hex()})
        return {'players': bytes(self.mu.mem_read(PLAYERS, 1620)).hex(), 'allies': bytes(self.mu.mem_read(0x595200, 324)).hex(), 'bodies': bodies, 'events': self.events}


def observe(cases, world):
    """설정6·Start1·연속 생성자2·Off 프레임2의 고유11입력을 실제 기계어로 실행한다."""
    mission = CurrentMissionOracle(); brain = DiagonalBrainOracle(); runtime = RuntimeOracle(); rows = []
    for case in cases:  # 실제 존재하는 두 AI owner 설정을 각각 독립 관측한다.
        for owner in (2, 3):  # 공통 설정이 owner3에도 적용되는 원문 범위를 포함한다.
            value = mission.run('prepare', [73, owner, 3, 2500, 1], case['config'])
            rows.append({'kind': 'settings', 'case': case['case'], 'owner': owner, 'input': [73, owner, 3, 2500, 1], 'observation': value})
    start_input = [1, 1, 0, 0, world['mask'], 0, 0]  # 보호 Fort의 독립 파싱으로 확정한 실제 사제 소유자 입력이다.
    start = mission.run('start', start_input, cases[0]['config'])
    assert [item for item in start[4].split(';') if item.startswith('I,')] == ['I,2', 'I,3']
    assert 'G,myAllyList' not in start[4].split(';')
    rows.append({'kind': 'start', 'case': 'priests123-name4', 'input': start_input, 'observation': start})
    for case in cases[:2]:  # 서로 다른 두 방향별 마스크를 현재 로컬1/AI2에 적용한다.
        profiles = [(row['owner'], bytes.fromhex(row['observation'][2])) for row in rows if row['kind'] == 'settings' and row['case'] == case['case']]  # 선행 원문 설정을 원문 생성자에만 전달한다.
        value = brain.run_pair(profiles)
        rows.append({'kind': 'alliance', 'case': case['case'], 'input': [{'owner': owner, 'profile': profile.hex()} for owner, profile in profiles], 'observation': value})
    for off in (0, 1):  # 같은 프레임 caller 입력에서 Off DWORD만 달리하여 수명 효과를 관측한다.
        bodies = bytearray(SIZE * 2); players = bytearray(180 * 9)
        struct.pack_into('<I', bodies, 0x254, off); struct.pack_into('<I', bodies, 0x258, 1); struct.pack_into('<I', bodies, 0x260, 2)
        struct.pack_into('<I', players, 2 * 180 + 0xb0, BODY)
        struct.pack_into('<I', bodies, SIZE + 0x258, 1); struct.pack_into('<I', bodies, SIZE + 0x260, 3); struct.pack_into('<I', players, 3 * 180 + 0xb0, BODY + SIZE)  # owner3의 독립 살아 있는 본체도 같은 순회에서 유지한다.
        cfg = [1, 0, 1, 0, 0, 0, 0, 0, 0, 2]
        value = runtime.sample(2, bodies, players, cfg, 0x27f)
        rows.append({'kind': 'lifetime', 'case': 'off' + str(off), 'input': {'off': off, 'cfg': cfg, 'bodies': bodies.hex(), 'players': players.hex()}, 'observation': value})
    return rows


def main():
    """클론 실행 전에 최초11/독립 재관측11을 확인하고 실제 왕복 도구의 기대 JSON을 쓴다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output', type=Path, required=True); args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=False); cases = list(scenarios()); save(out / 'inputs.json', cases)
    world = fort_input(); save(out / 'fort-input.json', world)
    first = observe(cases, world); save(out / 'observations-first.json', first)
    replay = observe(cases, world); save(out / 'observations-replay.json', replay); assert first == replay
    for case in cases:  # 기대 설정과 방향별81DWORD는 최초 원문 관측에서만 읽는다.
        settings = next(row['observation'] for row in first if row['kind'] == 'settings' and row['case'] == case['case'] and row['owner'] == 2)
        profile = bytes.fromhex(settings[2]); fields = {name: struct.unpack_from('<I', profile, offset)[0] for name, offset in FIELDS.items()}
        alliance_case = 'directed' if case['case'] == 'off' else case['case']
        alliance = next(row['observation'] for row in first if row['kind'] == 'alliance' and row['case'] == alliance_case)
        table = list(struct.unpack('<81I', bytes.fromhex(alliance['allies'])))
        lifetime = next(row['observation'] for row in first if row['kind'] == 'lifetime' and row['case'] == 'off' + str(fields['off']))
        present = struct.unpack_from('<I', bytes.fromhex(lifetime[1]), 2 * 180 + 0xb0)[0] != 0
        third = next(row['observation'] for row in first if row['kind'] == 'settings' and row['case'] == case['case'] and row['owner'] == 3)
        third_profile = bytes.fromhex(third[2]); third_fields = {name: struct.unpack_from('<I', third_profile, offset)[0] for name, offset in FIELDS.items()}  # owner3는 별도 원문 설정의 결과다.
        expected = {'ai2Brain': present, 'brains': [False, False, present, True, False, False, False, False, False],
                    'alliances': table, 'ai2Settings': fields if present else {}, 'otherAiSettings': {'3': third_fields}, 'playerNames': {'3': case['config']['ai3Name']}, 'absentPlayerNames': {'4': case['config']['ai4Name']},
                    'nativeSettings': fields, 'nativeSettingsName': profile[:30].split(b'\0', 1)[0].decode('ascii'),
                    'nativeLifetimeFreedMask': lifetime[2], 'nativeLifetimeTrace': lifetime[4]}
        assert expected['nativeSettingsName'] == case['config']['ai2Name']
        save(out / (case['case'] + '-expect.json'), expected)
    sources = ['originals/Netstorm.exe', 'originals/d/TEST02.fort', 'tools/fort.py', 'tools/shp.py', 'tools/typefile.py', 'tools/taff.py', 'extracted/refined/originals/functions.tsv', 'tools/ghidra/mission-start-functions.json',
               'tools/ghidra/ai-brain-functions.json', 'tools/decomp_mission_start.py', 'tools/decomp_ai_brain.py',
               'tools/decomp_ai_runtime.py', 'tools/decomp_priestmenu.py', 'tools/decomp_ai_bridge_initialize.py',
               'tools/decomp_ai_bridge_start.py', 'tools/cpp_editor_rules_native.py']
    report = {'host': socket.gethostname(), 'date': '2026-10-30', 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23',
              'uniqueCases': len(first), 'observations': len(first) + len(replay), 'differences': [], 'gameStarted': False, 'fortInput': world,
              'entries': ['004c2d40', '004c2b20', '00416860', '00417de0', '00417d30'],
              'boundaries': ['설정/CRT getter는 기존 도구의 명시 경계, 원본 설정/Start 분기 실행', '생성자 caller의 Player/81동맹을 자기 비트 기본값으로 제공, SetAlliance/AddAlly 실제 기계어',
                             'Off 프레임의 행동·소멸/반납 하위는 명시 경계이며 본문 프레임/삭제/Player0쓰기는 실제 기계어', '살아 있는600BYTE 전체 관측은 보존하되 미초기화 영역의 클론 바이트 동일성을 주장하지 않음'],
              'sourceSha256': {name: sha(ROOT / name) for name in sources},
              'evidenceSha256': {path.name: sha(path) for path in sorted(out.iterdir()) if path.is_file()}}
    save(ROOT / 'cpppj/editor-rules-native-20261030.json', report)
    print(json.dumps({'cases': len(first), 'observations': len(first) + len(replay), 'differences': 0}))


if __name__ == '__main__':
    main()
