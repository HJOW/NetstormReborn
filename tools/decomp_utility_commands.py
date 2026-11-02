#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""비멀티일반명령5개와실제Run/URLAlias/스크린샷래퍼의현재입력순서를격리관측한다."""
import argparse
import csv
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import decomp_game_menu_actions as base
from unicorn.x86_const import UC_X86_REG_ESP

SOURCE = base.ARENA+0x700  # 명시문자열은기존조회/Pool입력과겹치지않는다.
COMMANDS = (0x0200001c, 0x0200005a, 0x020000da, 0x020000aa)  #실제542768/2860/2a5c/299c다. MP 항해542714는 분류 후 신규 관측에서 제외한다.
SCHEMA = ('event', 'first', 'second', 'paused', 'console', 'guide', 'gump', 'source', 'found', 'target', 'capture', 'trace', 'completed', 'stackdelta', 'captureout')  #각 입력과 원문 반환/국소 효과를 독립 열에 보존한다.


class Oracle(base.Oracle):
    """현재일반입력/래퍼는실행하고저장/Config/링크/Run본문/강조/카메라만명시한다."""

    def __init__(self):
        """현호스트전체함수표에서실제세래퍼의불연속범위와불변본문을추가한다."""
        super().__init__()
        facts = {int(row['entry'], 16): row for row in csv.DictReader((base.ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        self.extra = []  #세 래퍼의 실제 명령 범위만 보관한다.
        # 이름에 대응하는 세 실제 래퍼를 각각 등록한다.
        for entry in (0x4d50e0, 0x4a02b0, 0x4cf970):
            # 함수의 불연속 몸체 범위도 정밀표대로 등록한다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.extra.append((low, high+1))
                self.original[low] = bytes(self.mu.mem_read(low, high+1-low))

    def instruction(self, mu, address, size, data):
        """실제호출인자를기록하고원문래퍼의문자열복사·설정실패·후속호출은대체하지않는다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if any(low <= address < high for low, high in self.extra):
            self.instructions += 1
            self.visited.add(address)
            return
        if address == 0x4db140:
            self.body_event = True
            self.event('save')
            self.returned()
            return
        if address == 0x441300:
            key, dest = self.cstring(self.number(sp+4)), self.number(sp+8)
            self.body_event = True
            self.event('read', (key.encode('ascii').hex(), self.case['found']))
            if self.case['found']:
                self.mu.mem_write(dest, bytes.fromhex(self.case['target'])+b'\0')
            self.returned(self.case['found'])
            return
        if address == 0x4a00c0:
            self.body_event = True
            self.event('url', (self.cstring(self.number(sp+4)).encode('ascii').hex(),))
            self.returned()
            return
        if address == 0x4cf730:
            self.body_event = True
            self.event('run', (self.cstring(self.number(sp+4)).encode('ascii').hex(), self.number(sp+8)))
            self.returned()
            return
        if address == 0x4d5bb0:
            self.body_event = True
            self.event('highlight', (self.number(sp+4),))
            self.returned()
            return
        return super().instruction(mu, address, size, data)

    def run(self, case, control):
        """연속캡처active를독립공급하여5명령이이를멋대로끄지않는지별도로관측한다."""
        self.number(0x5df828, case['capture'])
        result = super().run(case, control)
        result['captureout'] = self.number(0x5df828)
        return result


def cases():
    """Guide/Gump/뗌/Console/정지와두인자폭·설정성공/실패·캡처를독립입력으로교차한다."""
    seed, rows = base.cases()[0], []

    def add(command, flags=0, paused=0, console=0, guide=0, gump=0, source=b'Guide.Topic(3)', found=1, target=b'cmd:Config,sound=1', capture=0, second=None):
        """출력기대값없이현재사건과명시하위응답만원문실행장면으로보관한다."""
        values = list(map(int, seed['globals'].split(':')))
        values[0], values[2], values[3], values[4], values[8], values[9] = 0, 0, 1, 1, paused, console
        devices = bytearray.fromhex(seed['devices'])
        devices[0x700:0x700+len(source)+1] = source+b'\0'
        second = SOURCE if second is None else second
        case = dict(seed, name='utility-'+str(len(rows)), event=base.packed((command|flags, 0x12345678, second)),
                    guideResult=guide, gumpResult=gump, globals=base.packed(values), devices=devices.hex(),
                    read=0, filled=0, mutation='-', stop='-', source=source.hex(), found=found, target=target.hex(), capture=capture)
        rows.append(case)

    # 비멀티 네 명령에 선행 소비/뗌/정지/Console 입력을 독립 교차한다.
    for command, flags, paused, console, guide, gump in itertools.product(COMMANDS, (0, 0x80000000, 0x3c000000, 0xbc000000), (0, 3), (0, 1), (0, 7), (0, 11)):
        add(command, flags, paused, console, guide, gump, second=5 if command==0x02000006 else 50 if command==0x020000aa else None)
    # 별칭의 빈/NUL/현재 설정 성공·실패는 별도로 공급한다.
    for source, found, target in itertools.product((b'', b'Help', b'Mixed Alias', b'Before\0After'), (0, 1), (b'', b'help:temple')):
        add(0x0200005a, source=source, found=found, target=target)
    # SID의 전체DWORD 경계를 관측한다. MP 항해 명령은 실행하지 않는다.
    for command, second in itertools.product((0x020000aa,), (0, 1, 2, 3, 4, 5, 6, 0xffffffff)):
        add(command, second=second)
    # 번호 화면저장과 별개인 연속캡처 active를 보존하는지 관측한다.
    for active in (0, 1, 0xffffffff):
        add(0x0200001c, paused=3, console=1, capture=active)
    return rows


def main():
    """비멀티283입력을두정밀도와재관측으로확인하고새fixture/원시관측만저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists()
    oracle, inputs, lines, observations = Oracle(), cases(), ['\t'.join(SCHEMA)], []
    # 각 독립 입력에서 두 정밀도와 반복의 반환/국소 효과가 일치해야 한다.
    for case in inputs:
        results = [oracle.run(case, control) for control in (0x27f, 0x37f, 0x27f)]
        assert results[0] == results[1] == results[2], case['name']
        result, event = results[0], tuple(map(int, case['event'].split(':')))
        assert result['completed'] == 1 and result['stackdelta'] == 0
        values = list(map(int, case['globals'].split(':')))
        row = (*event, values[8], values[9], case['guideResult'], case['gumpResult'], case['source'], case['found'], case['target'], case['capture'], result['bodytrace'], result['completed'], result['stackdelta'], result['captureout'])
        lines.append('\t'.join(map(str, row)))
        observations.append(dict(input=case, observations=results))
    args.fixture.write_text('\n'.join(lines)+'\n', encoding='utf-8', newline='\n')
    record = dict(unit=760, host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'), inputs=len(inputs), observations=len(inputs)*3,
                  differences=0, nativeBodiesUnchanged=True, gameExecutions=0, multiplayerTests=0,
                  fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(), toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  extraBodies=['004d50e0 screenshot wrapper', '004a02b0 alias copy/read/URL', '004cf970 Run→Tell mode1'],
                  boundaries=['save004db140', 'Config ReadString00441300', 'URL004a00c0', 'Tell004cf730', 'Highlight004d5bb0', 'CenterObject00433ba0'],
                  limits=['실제게임/물리입력/링크프로그램/통신/GUI0', '실제파일I/O실패/전체Tell·강조·SID몸체는새전체원문실행이아니다'], raw=observations)
    args.report.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(inputs=len(inputs), observations=len(inputs)*3, fixtureSha256=record['fixtureSha256'])))


if __name__ == '__main__':
    main()
