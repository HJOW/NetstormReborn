#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CreateHelpList 실제 일반 부모·이름/FIFO·해당 파일 전체열기 관측을 work에만 추가한다."""
from pathlib import Path
import argparse
import hashlib
import itertools
import json
import socket
import sys

ROOT = Path(__file__).resolve().parents[1]  # 같은 보호 PE와 정밀 디컴파일을 읽기만 한다.
WORK = None  # 새 --output 디렉터리를 지정하기 전에는 파일을 작성하지 않는다.
sys.path.insert(0, str(ROOT / 'tools'))
import decomp_game_menu_actions as general
from decomp_mission_abort import Oracle as NamesOracle
from decomp_basefile_open_policy import OpenOracle


class GeneralOracle(general.Oracle):
    """Guide/Gump/공통release/일반분기만 실행하고 새 부모 호출을 명시 관측한다."""
    def instruction(self, mu, address, size, data):
        """원문 부모를 새로 합성하지 않고 실제004d7cba의 목적지 진입을 기록한다."""
        if address == 0x4c63e0:
            self.body_event = True
            self.event('create')
            self.returned()
            return
        super().instruction(mu, address, size, data)


def main():
    """서로 다른 세 관측의 몸체/경계를 분리하며 기존 fixture를 덮어쓰지 않는다."""
    global WORK
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True,help="기존 파일이 없는 새 원시 디렉터리")
    WORK=parser.parse_args().output.resolve()
    if WORK.exists(): raise FileExistsError(WORK)
    (WORK/"modified/cpppj/tests/fixtures").mkdir(parents=True)
    target = WORK / 'modified/cpppj/tests/fixtures'
    report = WORK / 'native-command-and-file.json'
    assert not report.exists()
    command = target / 'quick-help-list-input-x86.tsv'
    names = target / 'quick-help-list-names-x86.tsv'
    opened = WORK / 'whole-open-x86.tsv'
    assert not any(p.exists() for p in (command, names, opened))
    oracle = GeneralOracle()
    seed = general.cases()[0]
    lines = ['# event first second paused console guide gump editor mission trace completed stackdelta']
    count = 0
    for flags, paused, console, guide, gump, mode in itertools.product((0, 0x80000000, 0x3c000000, 0xbc000000), (0, 3), (0, 1), (0, 7), (0, 11), (0, 1)):  # 일반 parent는 정지/Console/Editor/미션 상태를 새 관문으로 사용하지 않는다.
        values = list(map(int, seed['globals'].split(':')))
        values[0], values[1], values[2], values[3], values[8], values[9] = mode, 0, 0, mode, paused, console
        assert values[2] == 0  # 새 멀티플레이 challenge 양성 입력은 관측하지 않는다.
        event = (0x0200009c | flags, 0x12345678, 0xfedcba98)
        case = dict(seed, name='help-list-' + str(count), event=general.packed(event), globals=general.packed(values), guideResult=guide, gumpResult=gump, mutation='-', stop='-')
        observations = [oracle.run(case, precision) for precision in (0x27f, 0x37f)]
        assert observations[0] == observations[1]
        result = observations[0]
        assert result['completed'] == 1 and result['stackdelta'] == 0
        lines.append('\t'.join(map(str, (*event, paused, console, guide, gump, mode, mode, result['bodytrace'], result['completed'], result['stackdelta']))))
        count += 1
    command.write_text('\n'.join(lines) + '\n', 'utf-8', newline='\n')
    names_oracle = NamesOracle()
    names_oracle.operation = 'S'
    lines = ['# operation source delimiter existed value next queue ring events']
    name_count = 0
    for name, argument in itertools.product(('CreateHelpList', 'createhelplist', 'CREATEHELPLIST'), ('0', '3', '-1', '4294967295', 'section', '"two words"', '')):  # 실제 등록명/토큰/숫자·문자열 인자 둘을 같은 원본 큐로 보낸다.
        lines.append('\t'.join(names_oracle.run('Command', (name + ',' + argument + '>').encode('ascii'))))
        name_count += 1
    names.write_text('\n'.join(lines) + '\n', 'utf-8', newline='\n')
    opened_oracle = OpenOracle()
    lines = ['# exact original whole Open body; nameHex mode create mask archiveOpen encrypted initial createAlt allocateFail outcome events finalFields index slot key']
    open_count = 0
    for mask, create_alt in ((2, 0), (0, 1), (0, 0), (8, 0), (4, 0)):  # 현재 r+b 성공/생성 retry/실패/보조/TAFF를 실제 부모에서 구별한다.
        case = (b'NetStormPieceList.html', 1, 1, mask, 1, 0, 0, create_alt, 0)
        first = opened_oracle.sample(case, 0x27f)
        assert first == opened_oracle.sample(case, 0x37f)
        lines.append(first)
        open_count += 1
    opened.write_text('\n'.join(lines) + '\n', 'utf-8', newline='\n')
    record = dict(unit=766, host=socket.gethostname(), lastDecompile='HJOW-Athlon / 2026-10-23', generalInputs=count, generalObservations=count * 2, nameInputs=name_count, nameObservations=name_count, wholeOpenInputs=open_count, wholeOpenObservations=open_count * 2, gameExecutions=0, multiplayerTests=0, builds=0,
                  boundary='일반 parent의 Guide/Gump·하위 CreateHelpList는 명시 경계. 이름/FIFO는 실제 원본 조회/토큰/큐이며 TLS/할당 경계. 전체41b4c0/하위파일/경로는 실제 원문이며 fopen/TAFF metadata/SEH throw 경계. 실제 FILE ABI/SEH unwind/게임 관측은 아니다.',
                  sha256={str(p.relative_to(WORK)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (command, names, opened)})
    report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps({k: record[k] for k in ('generalInputs', 'generalObservations', 'nameInputs', 'wholeOpenInputs')}))


if __name__ == '__main__':
    main()
