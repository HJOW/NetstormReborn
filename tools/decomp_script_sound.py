#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 UserInput 관문/PlaySound 분기와 원본 네 이름표를 제품 변경 전에 동결한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_game_menu_actions import Oracle, cases, ARENA, ARENA_SIZE

ROOT = Path(__file__).resolve().parents[1]  # 도구가 속한 저장소만 원본 출처로 쓴다.
NAMES = {'PlaySound': 0x54295c, 'ViewPriest': 0x5429d4, 'SetAllBridge': 0x542a4c, 'DigUp': 0x5427ac}  # 기존 PE에서 확인할 네 원문 명령 전역이다.
TEXT = ARENA + 0x700  # 원문 효과음 경계가 읽는 명시 NUL 문자열 장치다.
SCHEMA = ('name', 'event', 'first', 'texthex', 'editor', 'mission', 'tracehex', 'completed', 'stackdelta')  # 앞6열 입력과 뒤3열 실제 관측이다.


def sha(path):
    """원본·도구·새 fixture의 실제 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """기존 정밀도별 원문 관측기를 재사용하며 C++ 계산을 호출하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    image = Oracle()
    pe = image.pe
    base = pe.OPTIONAL_HEADER.ImageBase
    lookup = []
    # 실제 이름 문자열/포인터 표/명령 DWORD를 같은 보호 이미지에서 읽는다.
    for name, address in NAMES.items():
        raw = name.encode('ascii') + b'\0'
        offset = pe.__data__.find(raw)
        assert offset >= 0, name
        text = base + pe.get_rva_from_offset(offset)
        entry = pe.__data__.find(struct.pack('<II', text, address))
        assert entry >= 0, name
        code = struct.unpack('<I', pe.get_data(address-base, 4))[0]
        lookup.append({'name': name, 'string': f'{text:08x}', 'entry': f'{base+pe.get_rva_from_offset(entry):08x}', 'global': f'{address:08x}', 'command': f'{code:08x}'})
    seed = cases()[0]
    originals = []
    raw_rows = []
    names = (b'', b'somethingCreated.wav', b'priestMove.wav', b'SomethingCREATED.WAV', b'no-such-sound.wav', b'button.wav\0ignored.wav')
    # 문자열/입력 플래그/모드를교차하고 실제 여섯 재생 인자와 release 거부를 관측한다.
    for index, (text, flags, mode) in enumerate(itertools.product(names, (0, 0x80000000, 0x38000000, 0xb8000000), (0, 1))):
        case = seed.copy()
        globals_ = list(map(int, seed['globals'].split(':')))
        globals_[0] = mode
        globals_[3] = mode
        globals_[8] = 0
        globals_[9] = 0
        devices = bytearray(ARENA_SIZE)
        devices[0x700:0x700+len(text)+1] = text + b'\0'
        event, first = 0x0200009a | flags, 0x76543210 + index
        case.update(name=f'play-{index}', event=f'{event}:{first}:{TEXT}', globals=':'.join(map(str, globals_)), devices=devices.hex(), guideResult=0, gumpResult=0, read=0, filled=0, mutation='-', stop='-')
        observations = [image.run(case, control) for control in (0x27f, 0x37f, 0x27f)]
        assert observations[0] == observations[1] == observations[2], case['name']
        result = observations[0]
        originals.append({'input': case, 'observations': observations})
        raw_rows.append((case['name'], event, first, text.hex() or '-', mode, mode, result['bodytrace'].encode('ascii').hex(), result['completed'], result['stackdelta']))
    fixture = out/'script-sound-x86.tsv'
    fixture.write_text('\t'.join(SCHEMA) + '\n' + ''.join('\t'.join(map(str, row))+'\n' for row in raw_rows), encoding='utf-8', newline='\n')
    (out/'observations.json').write_text(json.dumps(originals, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'tools/decomp_script_sound.py', 'tools/decomp_game_menu_actions.py', 'tools/decomp_production_shortcuts.py')
    report = {'unit': 682, 'host': socket.gethostname(), 'date': '2026-11-01', 'lastPreciseDecompile': {'host': 'HJOW-Athlon', 'date': '2026-10-23'},
              'cases': len(raw_rows), 'observations': len(raw_rows)*3, 'precisionOrRepeatDifferences': 0, 'lookup': lookup,
              'fixtureSha256': sha(fixture), 'sha256': {p: sha(ROOT/p) for p in paths},
              'boundaries': '실제004d6e61..004da477의 Guide/Gump/일반 입력과 PlaySound 분기를 실행했다. 기존 명시 Guide/Gump 및004a9cb0 음향 장치를 경계로 쓰며 장치/PCM/실제GUI·CD본문을 실행하지 않는다. ViewPriest/SetAllBridge/DigUp은 이름표 정적 읽기만 하며 새 행동 검사가 아니다.'}
    (out/'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(f'PlaySound {len(raw_rows)}입력/{len(raw_rows)*3}관측 차이0, 원문 이름표4개')


if __name__ == '__main__':
    main()
