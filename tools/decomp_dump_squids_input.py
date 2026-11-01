#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DumpSquids의 실제 일반 입력 제외 분기를 제품 변경 전에 고정한다."""
import hashlib
import itertools
import json
from pathlib import Path
import socket
import sys
from decomp_game_menu_actions import Oracle, cases

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE/같은 PC의 정밀 범위와 기존 명시 장치를 재사용한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/dump-squids-input-x86.tsv'  # 최초 출력은 이후 구현에 맞춰 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/dump-squids-input-native-20261102.json'  # 게임/전체 GUI와 구별한 독립 명령 관측이다.
WORK = ROOT / 'extracted/cpp-dump-squids-715'  # 세 정밀도/반복의 원시 관측을 보관한다.
COMMAND = 0x0200011b  #00543418의 DumpSquids가 가리키는00542b3c의 실제 PE 초기 명령이다.


def digest(path):
    """현재 전체 바이트를 읽어 같은 원문/도구/fixture 수명을 확인한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """48입력의 실제 원문만 먼저 실행하며 이후에는 최초 자료를 읽기 검증한다."""
    if '--verify' in sys.argv:
        report = json.loads(REPORT.read_text('utf-8'))
        for name, sha in report['sha256'].items():  # 최초 원문/도구/fixture의 바이트를 변경하지 않는다.
            assert digest(ROOT / name) == sha, name
        assert len(FIXTURE.read_text('utf-8').splitlines()) == 49;print('DumpSquids 최초48입력 보존 확인');return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 원문 덮어쓰기 금지'
    oracle = Oracle();seed = cases()[0];rows = [];observations = []
    for index, (flags, argument, mode) in enumerate(itertools.product((0, 0x80000000, 0x38000000, 0xb8000000), (0, 6008, 0xffffffff), range(4))):  # Edit/미션은 독립 입력이고 메타맵/정지/Console은 비활성이다.
        values = list(map(int, seed['globals'].split(':')));values[0], values[3] = mode & 1, (mode >> 1) & 1;values[8] = values[9] = 0
        case = seed.copy();case.update(name=f'DumpSquids-{index}', event=f'{COMMAND | flags}:305419896:{argument}', globals=':'.join(map(str, values)), guideResult=0, gumpResult=0, read=0, filled=0, mutation='-', stop='-')
        observed = [oracle.run(case, control) for control in (0x27f, 0x37f, 0x27f)]  # x87 정밀도/첫 반복이 같은 정상 정수 분기를 선택하는지 대조한다.
        assert observed[0] == observed[1] == observed[2]
        result = observed[0];assert result['completed'] == 1 and result['bodytrace'] == '-' and result['stackdelta'] == 0
        rows.append((COMMAND | flags, argument, mode, result['bodytrace'], result['completed'], result['stackdelta']));observations.append(dict(input=case, observations=observed))
    FIXTURE.write_text('event\targument\tmode\ttrace\tcompleted\tstackdelta\n' + ''.join('\t'.join(map(str, row)) + '\n' for row in rows), 'utf-8', newline='\n')
    (WORK / 'native-observations.json').write_text(json.dumps(observations, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    paths = ['AGENTS.md', 'originals/Netstorm.exe', 'extracted/refined/originals/raw.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_dump_squids_input.py', 'tools/decomp_game_menu_actions.py', 'tools/decomp_production_shortcuts.py', FIXTURE.relative_to(ROOT).as_posix()]
    record = dict(unit=715, date='2026-11-02', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'), inputs=len(rows), observations=len(rows) * 3, precisionOrRepeatDifferences=0, gameExecutions=0, multiplayerTests=0, fixtureSha256=digest(FIXTURE), boundaries=['실제004d6e61..004da477의 일반 입력 분기와 기존 실제 큐/Guide 관문 문맥만 격리 실행한다', '큰 Guide/Gump/UI 장치는 기존 명시 입력이며 게임/OS/덤프 파일 생성/통신을 실행하지 않는다', 'DumpSquids는00542b3c의 명시 제외이며 별도 개발자 F7의004d6020 덤프 호출과 다르다', '48개 사건마다 실제 전체 출력/명시 trace/정상 완료/스택·x87를 관측했다. 한정 CPP 입력 소비와 전체 부모의 부수 효과를 혼동하지 않는다'], sha256={name: digest(ROOT / name) for name in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n');print('DumpSquids 원문48입력/144관측, 차이0')


if __name__ == '__main__':
    main()
