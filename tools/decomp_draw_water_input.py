#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DrawWater 실제 일반 입력의 장기DWORD store와 원문 이름/FIFO를 work에만 고정한다."""
from pathlib import Path
import argparse
import itertools
import json
import hashlib
import socket
import sys

ROOT = Path(__file__).resolve().parents[1]  # 같은 보호 PE와 정밀 디컴파일을 읽기만 한다.
WORK = None  # 새 --output 디렉터리를 지정하기 전에는 파일을 작성하지 않는다.
sys.path.insert(0, str(ROOT / 'tools'))
import decomp_game_menu_actions as general
from decomp_mission_abort import Oracle as NamesOracle


class Oracle(general.Oracle):
    """원문 일반 입력의 두 직접 store만 기존 쓰기 허용 범위에 더한다."""
    def guard(self, mu, access, address, size, value, data):
        """다른 게임 상태에 대한 새 쓰기는 허용하지 않는다."""
        if address == 0x54d930 and size == 4:
            self.writes['drawWater'] += 1
            return
        super().guard(mu, access, address, size, value, data)


def main():
    """정지/모드와 무관한 원문 입력/현재DWORD를 실제 PE 반환으로 관측한다."""
    global WORK
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True,help="기존 파일이 없는 새 원시 디렉터리")
    WORK=parser.parse_args().output.resolve()
    if WORK.exists(): raise FileExistsError(WORK)
    (WORK/"modified/cpppj/tests/fixtures").mkdir(parents=True)
    fixture = WORK / 'modified/cpppj/tests/fixtures/draw-water-input-x86.tsv'
    names = WORK / 'modified/cpppj/tests/fixtures/draw-water-names-x86.tsv'
    report = WORK / 'native-input.json'
    assert not fixture.exists() and not names.exists() and not report.exists()
    oracle = Oracle()
    seed = general.cases()[0]
    lines = ['# event first second paused console guide gump mode flagIn redrawIn flagOut redrawOut completed stackdelta']
    count = 0
    for flags, paused, console, guide, gump, mode, active in itertools.product((0, 0x80000000, 0x3c000000, 0xbc000000), (0, 3), (0, 1), (0, 7), (0, 11), (0, 1), (0, 7, 0xffffffff)):  # 현재flag는bool이 아니라 전체DWORD 입력이며 일반 명령은 mode gate를 추가하지 않는다.
        values = list(map(int, seed['globals'].split(':')))
        values[0], values[3], values[7], values[8], values[9] = mode, mode, 92, paused, console
        assert values[1] == 0 and values[2] == 0  # 새 battle/challenge 양성 입력은 관측하지 않는다.
        event = (0x0200001a | flags, 0x12345678, 0xfedcba98)
        case = dict(seed, name='draw-water-' + str(count), event=general.packed(event), globals=general.packed(values), guideResult=guide, gumpResult=gump, mutation='-', stop='-')
        samples = []
        for precision in (0x27f, 0x37f):  # 두 정밀도에서 같은 현재 flag store/정상 반환이 나와야 한다.
            oracle.number(0x54d930, active)
            result = oracle.run(case, precision)
            samples.append((oracle.number(0x54d930), oracle.number(0x59a8b0), result))
        assert samples[0] == samples[1]
        final, redraw, result = samples[0]
        assert result['completed'] == 1 and result['stackdelta'] == 0
        lines.append('\t'.join(map(str, (*event, paused, console, guide, gump, mode, active, 92, final, redraw, result['completed'], result['stackdelta']))))
        count += 1
    fixture.parent.mkdir(parents=True, exist_ok=True)
    fixture.write_text('\n'.join(lines) + '\n', 'utf-8', newline='\n')
    name_oracle = NamesOracle()
    name_oracle.operation = 'S'
    lines = ['# operation source delimiter existed value next queue ring events']
    name_count = 0
    for name, argument in itertools.product(('DrawWater', 'drawwater', 'DRAWWATER'), ('0', '3', 'section')):  # 실제 lookup/토큰/문자열 고리/큐만 실행한다.
        lines.append('\t'.join(name_oracle.run('Command', (name + ',' + argument + '>').encode('ascii'))))
        name_count += 1
    names.write_text('\n'.join(lines) + '\n', 'utf-8', newline='\n')
    record = dict(unit=768, host=socket.gethostname(), lastDecompile='HJOW-Athlon / 2026-10-23', inputs=count, observations=count * 2, names=name_count, nameObservations=name_count, actual='Guide/Gump/공통 release/일반 입력의0059a8b0=1 후0054d930=old==0 store; 실제 이름/토큰/FIFO', boundary='Guide/Gump와 하위 비대상 효과·TLS/할당/로그는 명시 장치이며 실제 UI/게임/OS는 실행하지 않는다', gameExecutions=0, multiplayerTests=0, builds=0, sha256={str(p.relative_to(WORK)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (fixture, names)})
    report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps(dict(inputs=count, observations=count * 2, names=name_count)))


if __name__ == '__main__':
    main()
