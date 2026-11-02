#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CreateAt의 실제 일반 입력/이름/문자열 FIFO만 독립 원문 CPU로 관측한다."""
from pathlib import Path
import hashlib
import itertools
import json
import socket
import sys
from collections import Counter

ROOT = Path(r'H:\T5Workspace\NetstormReborn')  # 보호 PE와 완료된 전체 정밀 디컴파일을 읽기만 한다.
WORK = Path(__file__).resolve().parent  # 최초 기대값/보고서의 유일한 새 작성 경로다.
sys.path.insert(0, str(ROOT / 'tools'))
import decomp_game_menu_actions as general
from decomp_mission_abort import Oracle as NamesOracle
from unicorn.x86_const import UC_X86_REG_ESP


class Oracle(general.Oracle):
    """실제 Guide/Gump/일반 입력을 실행하며 선택된 생성 함수만 토큰 전달 경계로 둔다."""
    def instruction(self, mu, address, size, data):
        """현재 param2 전체 DWORD를 기록하고 생성/게임/멀티플레이는 호출하지 않는다."""
        if address == 0x44c660:
            self.body_event = True
            self.event('createAt', (self.number(mu.reg_read(UC_X86_REG_ESP) + 4),))
            self.returned()
            return
        super().instruction(mu, address, size, data)


def main():
    """현재 사건 입력과 원문 이름 조회/토큰/큐를 최초 기대값으로 동결한다."""
    global WORK  # 독립 출력 경로에서만 최초 fixture를 작성한다.
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)  # 제품과 기존 fixture는 덮어쓰지 않는다.
    parser.add_argument('--output',required=True,type=Path)
    WORK=parser.parse_args().output.resolve()
    WORK.mkdir(parents=True,exist_ok=True)
    fixture = WORK / 'modified/cpppj/tests/fixtures/create-at-input-x86.tsv'
    names = WORK / 'modified/cpppj/tests/fixtures/create-at-names-x86.tsv'
    report = WORK / 'native-input.json'
    assert not fixture.exists() and not names.exists() and not report.exists()
    oracle = Oracle()
    seed = general.cases()[0]
    lines = ['# event first second paused console guide gump editor mission bodytrace envelopetrace completed stackdelta']
    inputs = list(itertools.product((0, 0x80000000, 0x3c000000, 0xbc000000), (0, 1, 3), (0, 1), (0, 7), (0, 11), range(4)))
    cases = [(*values, 0xfedcba98) for values in inputs]
    cases += [(flags, 0, 0, 0, 0, mode, token) for flags, mode, token in itertools.product((0, 0x80000000), range(4), (0, 0x80000000, 0xffffffff))]
    bodies, envelopes = Counter(), Counter()
    for index, (flags, paused, console, guide, gump, mode, token) in enumerate(cases):  # 관문/권한/정지/Console/플래그와 전체 DWORD는 독립 입력이다.
        values = list(map(int, seed['globals'].split(':')))
        values[0], values[3], values[8], values[9] = mode & 1, (mode >> 1) & 1, paused, console
        event = (0x020000e0 | flags, 0x12345678, token)
        case = dict(seed, name='create-at-' + str(index), event=general.packed(event), globals=general.packed(values), guideResult=guide, gumpResult=gump, mutation='-', stop='-')
        first, second = (oracle.run(case, precision) for precision in (0x27f, 0x37f))
        assert first == second and first['completed'] == 1 and first['stackdelta'] == 0
        bodies[first['bodytrace']] += 1
        envelopes[first['envelopetrace']] += 1
        lines.append('\t'.join(map(str, (*event, paused, console, guide, gump, values[0], values[3], first['bodytrace'], first['envelopetrace'], first['completed'], first['stackdelta']))))
    fixture.parent.mkdir(parents=True, exist_ok=True)
    fixture.write_text('\n'.join(lines) + '\n', 'utf-8', newline='\n')
    name_oracle = NamesOracle()
    name_oracle.operation = 'S'
    sources = [(name + ',' + argument + '>').encode('ascii') for name, argument in itertools.product(('CreateAt', 'createat', 'CREATEAT'), ('0', '4294967295', 'section', '(20x21)provider/0#1'))]
    sources += [b'CreateAt\0Ignored,section>', b'CreateAt,(20x21)provider/0#1\0,ignored>', b'CreateAts,section>']
    name_lines = ['# operation source delimiter existed value next queue ring events']
    for source in sources:  # 부모 생성은 호출하지 않고 실제 이름/토큰/숫자·문자열 고리/큐만 실행한다.
        name_lines.append('\t'.join(name_oracle.run('Command', source)))
    names.write_text('\n'.join(name_lines) + '\n', 'utf-8', newline='\n')
    record = dict(unit=770, host=socket.gethostname(), lastDecompile='HJOW-Athlon / 2026-10-23', inputs=len(cases), observations=len(cases)*2, names=len(sources), nameObservations=len(sources), actual='004d6e 입력의 실제 Guide/Gump/release→004d84f8..851b editor OR mission→0044c660(param2);00542a74 이름의 실제 ScriptCommand/토큰/고리/FIFO', bodyCounts=dict(bodies), envelopeCounts=dict(envelopes), nativeTotals=oracle.totals, nameInstructions=name_oracle.instructions, nameQueueCalls=name_oracle.queueCalls, boundary='Guide/Gump와 생성0044c660·비대상 하위 효과, CRT TLS/할당/로그는 명시 장치. 생성 성공/실제 Runtime/게임/OS/멀티플레이는 이 관측에 포함하지 않는다. Editor/mission 이외 MP전용 globals 양성 입력은 추가하지 않았다.', gameExecutions=0, multiplayerTests=0, builds=0, sha256={str(p.relative_to(WORK)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (fixture,names)})
    report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps(dict(inputs=len(cases), observations=len(cases)*2, names=len(sources), totals=oracle.totals)))


if __name__ == '__main__':
    main()
