#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본83VT 메뉴 슬롯과9명령의 현재 소스 연결을 읽기 전용으로 대조한다."""
import collections
import hashlib
import json
from pathlib import Path
import re
import socket
import struct
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 이 도구가 속한 저장소만 읽는다.
REPORT = ROOT / 'cpppj/object-menu-wiring-20261101.json'  # 실행/플레이 검증과 구별한 정적 보고서다.
POPULATE = {  # 보호 PE의 실제+a8 주소와 현재 소비자 이름이다.
    0x454c00: ('CampaignObjectMenuClient.cpp', 'OpenFactory'),
    0x494870: ('CampaignObjectMenuClient.cpp', 'OpenPriest'),
    0x4288f0: ('CampaignObjectMenuClient.cpp', 'OpenCarrier'),
    0x44cbd0: ('CampaignObjectMenuClient.cpp', 'OpenDamageable'),
    0x4b1270: ('CampaignObjectMenuClient.cpp', 'OpenObject'),
    0x41c210: ('RawObjectMenuVariants.cpp', 'Meltdown'),
    0x422000: ('RawObjectMenuVariants.cpp', 'Bridge'),
    0x462390: ('RawObjectMenuVariants.cpp', 'Buried'),
    0x462550: ('RawObjectMenuVariants.cpp', 'Help'),
    0x485600: ('RawObjectMenuVariants.cpp', 'Geyser'),
    0x48eca0: ('RawObjectMenuVariants.cpp', 'IceTowerGrowth'),
    0x4dcca0: ('RawObjectMenuVariants.cpp', 'Temple'),
    0x420b30: ('RawObjectMenuVariants.cpp', 'Spell'),
}
OPEN = {  # 보호 PE의 실제+ac 주소와 현재 열기 경로다.
    0x4754b0: 'OpenFactory', 0x4adb30: 'Open',
    0x4268c0: 'OpenSelectionMenu(false)', 0x492080: 'OpenSelectionMenu(false)',
    0x41e660: 'OpenSelectionMenu(true)',
    0x4789b0: 'MP Metadisplay Battle/Challenge', 0x479390: 'MP Challenge Area',
}
COMMANDS = {  # 원본 명령 전역 주소별 현재 입력/세션의 필수 연결 지점이다.
    0x542ad4: ('Meltdown', [('CampaignObjectMenuClient.cpp', 'command==0x030000f8U'), ('RawCampaignSession.cpp', 'current.meltdown->Command')]),
    0x542b5c: ('Set as Cracked', [('UberGump.cpp', 'transitionCode>=0x02000125U'), ('RawCampaignSession.cpp', 'RawCampaignSession::SetBridgeQuality')]),
    0x542b60: ('Set as Normal', [('UberGump.cpp', 'transitionCode<=0x02000127U'), ('RawCampaignSession.cpp', 'RawCampaignSession::SetBridgeQuality')]),
    0x542b64: ('Set as Hard', [('UberGump.cpp', 'transitionCode-0x02000125U'), ('RawCampaignSession.cpp', 'RawCampaignSession::SetBridgeQuality')]),
    0x542a2c: ('Turn To Spell/Obelisk', [('CampaignObjectMenuClient.cpp', 'command==0x030000ceU'), ('RawCampaignSession.cpp', 'RawObjectConversion(current.core.pool')]),
    0x5427ac: ('CHEAT Dig Up', [('UberGump.cpp', 'transitionCode==0x0200002dU'), ('UberGump.cpp', 'GemReward().Reveal')]),
    0x5428c8: ('Detonate', [('UberGump.cpp', 'DispatchDetonateInput(event'), ('DetonateInput.cpp', 'objectInput(target,event.code)'), ('RawCampaignSession.cpp', 'RawObjectConversion(current.core.pool')]),
    0x542754: ('About/Help', [('UberGump.cpp', 'transitionCode==0x02000017U'), ('UberGump.cpp', 'Menus()->OpenHelp')]),
    0x542974: ('Knowledge', [('UberGump.cpp', 'transitionCode==kShowTechnologyCommand'), ('UberGump.cpp', 'OpenKnowledge(world_->Session()')]),
}


def sha(path):
    """보호 자료와 이번에 읽은 소스를 원래 바이트로 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """코드를 실행하지 않고 원본 DWORD, 생성 자료, 소스 분배 지점만 대조한다."""
    paths = {'originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'AGENTS.md',
             'cpppj/src/o/DerivedConstructors.inc', 'cpppj/src/client/ObjectMenuSlots.inc',
             'tools/cpp_object_menu_wiring_audit.py'}
    image = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    base = image.OPTIONAL_HEADER.ImageBase
    def read(address, count):
        """PE를 매핑/실행하지 않고 지정 VA의 실제 파일 바이트를 읽는다."""
        data = image.get_data(address - base, count)
        assert len(data) == count, hex(address)
        return data
    def source(name):
        """실제 Client 파일을 읽고 보고서의 입력 해시에 포함한다."""
        path = 'cpppj/src/client/' + name
        paths.add(path)
        return (ROOT / path).read_text('utf-8')
    def anchor(name, text):
        """필수 소스 지점과 현재1기준 줄을 기록하며 런타임 효과 증거로 쓰지 않는다."""
        code = source(name)
        offset = code.index(text)
        return {'path': 'cpppj/src/client/' + name, 'line': code.count('\n', 0, offset) + 1, 'text': text}
    recipes = (ROOT / 'cpppj/src/o/DerivedConstructors.inc').read_text('utf-8').split('kCdRecipes')[0]
    vtables = {int(v, 16) for _, v in re.findall(r'\{(0x[0-9a-f]+)U?,(0x[0-9a-f]+)U?,', recipes, re.I)} | {0x501dd0}
    table = (ROOT / 'cpppj/src/client/ObjectMenuSlots.inc').read_text('utf-8')
    registered = [tuple(int(n, 16) for n in row) for row in re.findall(r'\{(0x[0-9a-f]+)U,(0x[0-9a-f]+)U,(0x[0-9a-f]+)U\}', table, re.I)]
    original = [(v, *struct.unpack('<II', read(v + 0xa8, 8))) for v in sorted(vtables)]
    assert len(vtables) == len(registered) == 83 and original == registered
    populate = collections.Counter(row[1] for row in original)
    opened = collections.Counter(row[2] for row in original)
    assert set(populate) == set(POPULATE) and set(opened) == set(OPEN)
    populated = []
    # 생성 VT가 가리키는 실제13작성 입구를 현재case와 한 번씩 대조한다.
    for address, count in sorted(populate.items()):
        name, consumer = POPULATE[address]
        populated.append({'originalBody': f'{address:08x}', 'vtables': count, 'consumer': consumer, 'source': anchor(name, f'case 0x{address:x}:')})
    open_routes = []
    deferred = []
    # 비멀티5입구는 현재Client분배를, MP2입구는 기존 디컴파일의 별도 본체를 기록한다.
    for address, count in sorted(opened.items()):
        if address in (0x4789b0, 0x479390):
            path = 'extracted/refined/originals/Netstorm.c'
            paths.add(path)
            raw = (ROOT / path).read_text('utf-8')
            marker = f'// ==== FUN_{address:08x} @ {address:08x}'
            offset = raw.index(marker)
            deferred.append({'originalBody': f'{address:08x}', 'vtables': count, 'consumer': OPEN[address], 'path': path, 'line': raw.count('\n', 0, offset) + 1, 'status': '3차 MP 무시험 사전 구현으로 이월; 현재 Client에 미연결'})
            continue
        open_routes.append({'originalBody': f'{address:08x}', 'vtables': count, 'consumer': OPEN[address], 'source': anchor('CampaignObjectMenuClient.cpp', f'case 0x{address:x}:')})
    variant_source = source('RawObjectMenuVariants.cpp')
    declared = {int(a, 16): int(c, 16) for a, c in re.findall(r'\{(0x[0-9a-f]+),(0x[0-9a-f]+)\}', variant_source, re.I)}
    assert set(declared) == set(COMMANDS)
    commands = []
    # 원문9명령의 DWORD를 생성표와 대조한 뒤 각각의 소비 지점을 기록한다.
    for address, (label, links) in COMMANDS.items():
        command = struct.unpack('<I', read(address, 4))[0]
        assert declared[address] == command, hex(address)
        commands.append({'global': f'{address:08x}', 'command': f'{command:08x}', 'label': label,
                         'sources': [anchor(name, text) for name, text in links]})
    report = {'unit': 681, 'date': '2026-11-01', 'host': socket.gethostname(),
              'lastPreciseDecompile': {'host': 'HJOW-Athlon', 'date': '2026-10-23'},
              'kind': 'read-only static PE/source audit', 'nativeExecutions': 0, 'gameExecutions': 0,
              'vtables': len(original), 'populateRoutes': populated, 'openRoutes': open_routes,
              'variantCommands': commands, 'deferredOpenRoutes': deferred, 'unclassifiedRegisteredRoutes': [],
              'preliminary': '최초 도구의 열기5개 전체 가정이 실패했다. 실제 PE에는7입구가 있었으며 남은2개는 기존 Metadisplay/Challenge MP 본체였다. 이를 비멀티 누락이나 실행 통과로 세지 않고3차 이월로 분류했다.',
              'limits': '연결 지점/생성표를 대조했다. 모든 소유자/상태/픽셀/수명 효과의 실행 검증, CD 본문 대조, 전체 선택기 및 간접 alias 부재 증거가 아니다. 공중3종도 정적으로만 읽었으며 MP/포획/희생/공격/승리 검사는0이다.',
              'sha256': {p: sha(ROOT / p) for p in sorted(paths)}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('원본83VT / 작성13 / 비멀티 열기5 / MP 이월2 / 명령9 연결 정적 대조 통과; 실행0')


if __name__ == '__main__':
    main()
