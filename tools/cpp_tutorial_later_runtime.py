#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Early3~6 실제 연결의 저장 관측과 현재 출처를 게임 실행 없이 검증한다."""
import argparse
import hashlib
import json
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT
from cpp_goal_stage_entry_smoke import MISSIONS, ARCHIVE
from cpp_fort_smoke import script_value
from cpp_menu_smoke import source
from cpp_world_smoke import states

REPORT = ROOT / 'cpppj/tutorial-later-runtime.json'  # 현재 연결부와 단위206 관측의 출처다.


def sha(path):
    """파일 전체 원문 바이트의 SHA256을 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect_entry(directory):
    """저장된 마지막 실제 프레임을 보호 영어 Header와 비교한다. 게임 상태를 만들거나 실행하지 않는다."""
    records = []  # 각 미션의 최종 실제 플레이 진입과 사용한 입력 접두다.
    # 모든 목표 미션을 요구해 일부 진입을 전체 성공으로 세지 않는다.
    for mission in MISSIONS:
        runs = sorted((path for path in (directory / mission).iterdir() if path.is_dir() and path.name.isdecimal()), key=lambda path: int(path.name))
        assert runs, mission
        last = runs[-1]
        current = states(last / 'states.tsv')['current']
        script = source(ROOT / 'originals', ARCHIVE, f'd/{mission}.english').decode('cp1252')
        money = int(script_value(script, 'myStartMoney') or '0')  # C++ 출력과 독립인 원문 초기 SP다.
        technology = sorted(filter(None, (script_value(script, 'myTech') or '').split(';')))
        assert current['mission'][0].lower() == mission
        assert current['phase'] == ['4'] and current['paused'] == ['0']
        assert '1' in [row[1] for row in current['objects'].values() if row[0] == 'priest']
        assert current['raw_world'][6:9] == ['0', '0', '0']
        assert current['mission_start'][:3] == ['1', '1', '0'] and current['raw_session'] == ['1', '1']
        assert float(current['raw_sp'][1]) == money and sorted(current['raw_technology']) == technology
        if mission.startswith('tutorial'):
            assert current['mission_gate'][:6] == [mission.removeprefix('tutorial'), '65', '0', '0', '0', '1']
        clicks = []  # 실제 저장된 클릭만 복원하며 안내나 진행값을 합성하지 않는다.
        # 같은 입력 파일에서 사용자 버튼을 누른 순서를 읽는다.
        for line in (last / 'steps.tsv').read_text('utf-8').splitlines():
            frame, operation, argument = line.split('\t', 2)
            if operation == 'click':
                clicks.append(argument)
        records.append(dict(mission=mission, passed=True, clicks=clicks, directory=last.relative_to(ROOT).as_posix(),
            phase=current['phase'], paused=current['paused'], gate=current['mission_gate'], money=money, technology=technology))
    return dict(passed=len(records), runs=records, scope='저장된 실제 프레임/입력과 보호 Header의 읽기 전용 대조다. 자연 완주와 전체 동일성은 별도다.')


def verify():
    """현재 소스와 보존한 성공/실패 관측을 모두 확인하며 과거 실행을 새 실행으로 세지 않는다."""
    report = json.loads(REPORT.read_text('utf-8'))
    # 현재 핀과 역사 관측의 바이트를 각각 확인한다.
    for group in ('sources', 'evidence'):
        # 어느 파일의 차이인지 실제 저장소 경로를 유지한다.
        for name, expected in report[group].items():
            assert sha(ROOT / name) == expected, name
    tests = json.loads((ROOT / report['validation']['fullTests']).read_text('utf-8'))
    assert tests['tests'] == 1726 and tests['failed'] == tests['returnCode'] == 0
    observed = inspect_entry(ROOT / report['validation']['serialEntryDirectory'])
    assert observed['passed'] == 11
    restore = json.loads((ROOT / report['validation']['settingsRestore']).read_text('utf-8'))
    assert restore['restored'] and report['settings']['baselineSha256'] == restore['baselineSha256']
    assert not report['goalComplete'] and not report['fullFidelityVerified']
    print('Early3~6 연결/전체1726검사/저장11진입/수동 설정 복구 출처 통과')


def main():
    """관측 검사와 현재 출처 검사를 구분하며 이 도구에서는 게임을 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inspect', type=Path)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        verify()
    elif args.inspect:
        print(json.dumps(inspect_entry(args.inspect.resolve()), ensure_ascii=False))
    else:
        parser.error('--inspect 또는 --verify가 필요합니다')


if __name__ == '__main__':
    main()
