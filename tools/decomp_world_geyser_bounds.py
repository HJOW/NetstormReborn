#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체/분할 간헐천 범위와 실제 Player 예산을 보호 PE의 명령으로 관측한다."""
import argparse
import csv
import json
import random
import socket
import struct
from pathlib import Path
import decomp_priestmenu as base
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

ROOT = base.ROOT  # 현재 PC의 원문/정밀 범위만 읽는다.
KEYS = ('op', 'mission', 'size', 'cell', 'grid', 'l0', 't0', 'r0', 'b0', 'l1', 't1', 'r1', 'b1', 'alter', 'present', 'hidden', 'battle')  # 계산 전 지정하는 독립 입력이다.
OUT = base.TEXT + 0x100  # 반환 사각형은 원본 이미지와 겹치지 않는다.


def case(**changes):
    """게임에서 쓰는 청크/옵션의 기본 입력만 구성한다."""
    result = dict(zip(KEYS, ('W', 0, 2, 1, 3, 2, 3, 13, 12, 5, 1, 15, 14, 0, 15, 0, 1)))
    result.update(changes)
    return result


def cases():
    """전체/분할·역전/감김·현재 모드 재조회와 Player8 조합을 독립 입력한다."""
    result = []
    for mission in (0, 1):  # missionPost는 일반 mission과 별개의 현재 모드다.
        for size in range(4):  # 실제 옵션 표의 네 가지 간격이다.
            result.append(case(mission=mission, size=size))
            for grid in (1, 2, 3, 4, 7):  # 분할의 끝점·유효 밖 fallback도 포함한다.
                for cell in (0, 1, grid, grid*grid, grid*grid+1):  # 맨 앞/뒤 칸 및0/범위 밖 번호의 원문 동작이다.
                    result.append(case(op='P', mission=mission, size=size, cell=cell, grid=grid))
    for rectangle in ((16, 16, 1, 1), (-3, 5, 7, -2), (0x7fffffff, -0x80000000, 0x8000001, -0x8000001)):
        for size in range(4):  # DWORD<<4 뒤 signed 변환과 역전 사각형을 구별한다.
            changes = dict(zip(('l0', 't0', 'r0', 'b0'), rectangle))
            result.append(case(size=size, **changes))
            for cell in (1, 5, 9):  # x폭의 float 저장과 y높이의 double 유지를 검사한다.
                result.append(case(op='P', size=size, cell=cell, **changes))
    for alter in range(1, 6):  # 첫/두 번째 청크 조회가 바꾼 현재 모드/옵션을 다시 읽는다.
        for mission in (0, 1):
            result.extend((case(mission=mission, alter=alter), case(op='P', mission=mission, cell=10, alter=alter)))
    for present in range(256):  # Player0/flags를 세지 않고 정확히+2c의 여덟 DWORD를 검사한다.
        result.append(case(op='B', present=present, hidden=0))
        result.append(case(op='B', present=present, hidden=0x55))
    randomizer = random.Random(0x608)  # 임의 입력 순서는 재현 가능하며 출력은 계산하지 않는다.
    for _ in range(48):  # 비영 숨김 BYTE와 전투 밖 예산0도 포함한다.
        result.append(case(op='B', present=randomizer.randrange(256), hidden=randomizer.randrange(256), battle=randomizer.randrange(2)))
    return list({tuple(item[key] for key in KEYS): item for item in result}.values())


class Oracle(base.Oracle):
    """사각형 계산/CRT/옵션/예산은 실제 명령이며 큰 ChunkMap만 명시 경계다."""
    def __init__(self):
        """현재 정밀 함수 범위와 실제 PE를 분리 emulator에 설치한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        for entry in (0x485bc0, 0x485cc0, 0x40d900, 0x41d0d0, 0x40e800, 0x40e880, 0x40e8f0, 0x41ca10, 0x41c8e0, 0x4e49c0):  # 승인한 계산 몸체 외의 명령은 거부한다.
            for part in facts[entry]['ranges'].split(';'):  # 분리된 실제 함수 조각도 현재 정밀 표에서 읽는다.
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high+1))

    def instruction(self, mu, address, size, data):
        """현재 입력의 청크 경계와 실제 옵션 조회 순서를 기록한다."""
        if address == 0x432210:
            assert mu.reg_read(UC_X86_REG_ECX) == 0x54c8a0
            target, region = self.words(mu.reg_read(UC_X86_REG_ESP)+4, 2)
            assert region in (0, 1)
            self.events.append('C,'+str(region))
            self.chunk_calls += 1
            if self.chunk_calls == 1:  # 앞선 하위가 현재 모드를 바꾸는 명시 외부 입력이다.
                if self.values['alter'] in (1, 5): self.put(0x594fa4, 1)
                elif self.values['alter'] == 2: self.put(0x594fa4, 0)
                elif self.values['alter'] == 3: self.put(0x52f7e6, 3, 1)
                elif self.values['alter'] == 4: self.put(0x52f7e6, 0, 1)
            elif self.chunk_calls == 2 and self.values['alter'] == 5:
                self.put(0x594fa4, 0)
            fields = ('l0', 't0', 'r0', 'b0') if region == 0 else ('l1', 't1', 'r1', 'b1')
            mu.mem_write(target, struct.pack('<4I', *(self.values[key] & 0xffffffff for key in fields)))
            self.ret(target, 8)
            return
        if address == 0x41ca10: self.events.append('S')  # getter 본문도 그대로 실행한다.
        assert any(low <= address and address+size <= high for low, high in self.allowed), f'미등록 명령 {address:08x}'
        self.instructions += 1

    def observe(self, values):
        """한 사례의 원문 반환/출력16BYTE/공유 쓰기/효과 순서를 수집한다."""
        self.values, self.events, self.chunk_calls = values, [], 0
        self.put(0x594fa4, values['mission']);self.put(0x594fbc, values['battle']);self.put(0x52f7e6, values['size'], 1)
        self.put(0x542654, 0x12345678);self.put(0x59534c, base.PLAYERS)
        self.mu.mem_write(base.PLAYERS, bytes(9*0xb4));self.mu.mem_write(OUT, bytes([0xa5])*16)
        for owner in range(9):  # owner0의 비영 값은 원본이 세지 않아야 한다.
            self.put(base.PLAYERS+owner*0xb4+0x2c, 0x80000000 if not owner or values['present'] & (1 << (owner-1)) else 0)
            self.put(base.PLAYERS+owner*0xb4+0x50, 0xff, 1)
            if owner: self.put(0x52f81e+owner, 0x80 if values['hidden'] & (1 << (owner-1)) else 0, 1)
        if values['op'] == 'B': returned, _ = self.call(0x41c8e0)
        elif values['op'] == 'W': returned, _ = self.call(0x485bc0, (OUT,))
        else: returned, _ = self.call(0x485cc0, (OUT, values['cell'], values['grid']))
        if values['op'] != 'B': assert returned == OUT
        return [self.mu.mem_read(OUT, 16).hex(), str(self.words(0x542654, 1)[0]), str(returned if values['op'] == 'B' else 0), ';'.join(self.events) or '-', str(self.words(0x594fa4, 1)[0]), str(self.mu.mem_read(0x52f7e6, 1)[0])]


def main():
    """독립 두 실행의 최초 바이트가 같을 때만 새 fixture/보고서를 작성한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--verify', action='store_true')  # 최초 fixture를 쓰지 않고 전체 원문을 다시 관측해 대조한다.
    args = parser.parse_args()
    output = ROOT / args.output;output.mkdir(parents=True, exist_ok=False)
    inputs = cases();first, replay = Oracle(), Oracle()
    observed = [[str(value[key]) for key in KEYS]+first.observe(value) for value in inputs]
    repeated = [[str(value[key]) for key in KEYS]+replay.observe(value) for value in inputs]
    (output / 'first.json').write_text(json.dumps(observed, indent=2)+'\n', encoding='utf-8', newline='\n')
    (output / 'replay.json').write_text(json.dumps(repeated, indent=2)+'\n', encoding='utf-8', newline='\n')
    assert observed == repeated
    fixture = ROOT / 'cpppj/tests/fixtures/world-geyser-bounds-x86.tsv'
    payload = '# '+'/'.join(KEYS)+'/원문출력6열\n'+''.join('\t'.join(row)+'\n' for row in observed)
    if args.verify: assert fixture.read_bytes() == payload.encode('utf-8')
    else:
        with fixture.open('xb') as stream: stream.write(payload.encode('utf-8'))
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/Netstorm.c', 'tools/decomp_priestmenu.py', 'tools/decomp_world_geyser_bounds.py', 'cpppj/tests/fixtures/world-geyser-bounds-x86.tsv']
    report = {'unit': 608, 'host': socket.gethostname(), 'cases': len(inputs), 'observations': len(inputs)*2, 'differences': [], 'verificationOnly': args.verify, 'instructions': [first.instructions, replay.instructions], 'scope': '00485bc0/00485cc0/0040d900/0041d0d0/0040e800/0040e880/0040e8f0/0041ca10/0041c8e0와 실제 CRT. 큰 ChunkMap.bounds만 명시 경계. 새 게임/GUI/MP 없음.', 'sourceSha256': {name: base.digest(ROOT / name) for name in paths}, 'evidenceSha256': {path.name: base.digest(path) for path in output.iterdir() if path.is_file()}}
    (ROOT / 'cpppj/world-geyser-bounds-native-20261030.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
