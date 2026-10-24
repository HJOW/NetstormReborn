#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""초기 진행값을 보존한 실제 Early1 수확·SP600·G·다음 Tutorial2/Guide4를 검증한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_bridge_ritual_runtime import execute, save
from cpp_menu_smoke import source
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from taff import TaffArchive

PLAN = ROOT / 'cpppj/tests/fixtures/tutorial-one-harvest-ui-input.tsv'  # 현재 화면에 맞춘 실제 입력 계획이다.
REPORT = ROOT / 'cpppj/tutorial-one-harvest-20261023.json'  # 최신 실제 재검증 출처다. 2026-10-19의 기존 보고서는 변경하지 않는다.
INPUT_OPERATIONS = frozenset(('click', 'click-if', 'up-if', 'up', 'right', 'key', 'keyup'))  # UI 입력만 허용하고 검사 전용 상태 쓰기는 거부한다.


def sha(path):
    """전체 원문 바이트의 SHA256을 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def original(name):
    """제품과 같은 디스크 우선 VFS로 보호 원문을 독립적으로 읽는다."""
    archive = TaffArchive(ROOT / 'originals/netstorm.tarc')
    return source(ROOT / 'originals', archive, 'd/' + name).decode('cp1252')


def inspect(directory, code, unchanged, *, expected_stock=None, progress_profile=None):
    """Guide4·실제 수확/완주를 판정하고 초기 완료값 보존과 명시 초기 프로필을 구별한다."""
    assert code == 0 and unchanged
    observed = states(directory / 'states.tsv')
    initial, goal, following = (observed[name] for name in ('initial', 'goal', 'nextBriefing'))
    before = int(initial['mission_progress'][0])  # 원래 완료값이1이어도 실제 플레이를 막거나0으로 바꾸지 않는다.
    if progress_profile is True:
        assert before == 0, '명시 초기 프로필의 완료값0이 실제 시작 관측에 적용되지 않았습니다'
    assert float(initial['raw_sp'][1]) == 0
    assert goal['mission'][0].lower() == 'tutorial1' and goal['page'] == ['G.']
    assert goal['mission_gate'][:6] == ['1', '71', '0', '1', '0', '1'] and goal['paused'] == ['1']
    assert goal['mission_progress'] == ['1'] and {'Leave Tutorials', 'Next Tutorial'} <= set(goal['controls'])
    assert goal['mission_start'][:3] == ['1', '4', '0'] and goal['raw_session'] == ['1', '1']
    history = [current for current in observed.values() if current.get('mission', [''])[0].lower() == 'tutorial1']
    stages = [int(current['mission_gate'][1]) for current in history]
    assert list(dict.fromkeys(stages)) == list(range(ord('A'), ord('G') + 1))
    assert all('test_bridge_patterns' not in current for current in history)
    stock = list(dict.fromkeys(int(current['raw_production_maintenance'][0]) for current in history if current.get('raw_production_maintenance')))  # 실제 공유 재고 설정도 별도로 관측한다.
    if expected_stock is not None:
        assert stock == [expected_stock], ('실제 재고 설정 관측 불일치', stock, expected_stock)
    cost = int(re.search(r'\bcost\s*=\s*(\d+)', original('nugget.type'))[1])
    money = list(dict.fromkeys(float(current['raw_sp'][1]) for current in history))
    assert cost == 200 and money == [0, cost, cost * 2, cost * 3]
    priests = [sid for sid, row in initial['objects'].items() if row[0] == 'priest' and row[1] == '1']
    assert len(priests) == 1
    priest = priests[0]
    positions = {tuple(current['objects'][priest][5:7]) for current in history if priest in current['objects']}
    assert len(positions) > 3 and goal['raw_packets'].count('64') == 1
    assert any(current['page'] == ['E.'] for current in history)
    expected = int(re.search(r'\bmyStartMoney\s*=\s*(\d+)', original('tutorial2.english'))[1])
    assert following['mission'][0].lower() == 'tutorial2' and following['page'] == ['A.'] and following['controls'] and following['paused'] == ['1']
    assert following['mission_start'][:3] == ['1', '4', '0'] and following['raw_session'] == ['1', '1']
    assert float(following['raw_sp'][1]) == expected == 10000 and following['mission_gate'][0] == '2'
    assert 'raw_mission_loading' not in goal and 'raw_mission_loading' not in following
    return dict(returnCode=code, originalsUnchanged=unchanged, stages=[chr(stage) for stage in dict.fromkeys(stages)],
                money=money, doneBefore=before, doneAfter=1, progressObservedTransition=before == 0, persistedProgressVerified=False,
                initialProgressInjected=progress_profile, progressProfile='explicit-reset' if progress_profile else 'preserved' if progress_profile is False else 'unrecorded',
                stockTimersObserved=stock, defaultStockTimersVerified=stock == [1], priest=priest, distinctPositions=len(positions), movePackets=1,
                lastGameTime=float(goal['raw_world'][0]), nextMission='tutorial2', nextSp=expected,
                naturalTutorialOneComplete=True, goalComplete=False,
                scope='실제 A~G/합법 다리·회전/간헐천 연결/한 번의 사제 명령/세 번 채취·귀환/SP600/다음 Tutorial2 Guide4다. 진행0→1은 실제 초기값이0일 때의 메모리 관측이며 디스크 저장·Early2 진행·전체 동일성은 별도다.')


def prepare(directory, reset_progress_profile=False, *, input_plan=None):
    """보존된 입력의 캡처 경로만 옮기고 현재 검사 계약의 실행 명령을 검토 가능하게 만든다."""
    plan = PLAN if input_plan is None else input_plan.resolve()  # 명시 입력만 선택하며 기존 기본 계획은 보존한다.
    rows = []  # 입력 계획의 그림 경로만 현재 관측 폴더로 바꾼다.
    for line in plan.read_text('utf-8').splitlines():  # 조건은 읽기 전용이며 입력 외 동작을 허용하지 않는다.
        if not line or line.startswith('#'): continue
        frame, operation, argument = line.split('\t', 2)
        if operation == 'snapshot': argument = str(directory / (argument + '.bmp'))
        if operation == 'when':
            condition, action, parameter = argument.split(';;', 2)
            assert condition and action in INPUT_OPERATIONS
            if action in ('key', 'keyup'):
                assert len(parameter) == 1 and parameter.lower() in 'qwaszx'  # 조건부 입력은 실제 여섯 다리 슬롯 문자만 허용한다.
            else:
                assert parameter.startswith(('production:', 'placement-', 'world:', 'cell:')) or parameter in ('OK', 'MORE')
        else:
            assert operation in INPUT_OPERATIONS or operation in ('report', 'snapshot')
        rows.append((frame, operation, argument))
    script = directory / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in rows), 'utf-8', newline='\n')
    settings = 'SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0;useProductTimers=1'  # 기존SP/단계/HP/완료값을 주입하지 않는 기본 실행이다.
    if reset_progress_profile:
        settings += ';DoneTutorial1=0'  # 사용자가 명시한 초기 프로필 검사만 기존 완료값0 시작을 선택한다.
    frame_limit = max(3924, max(int(row[0]) for row in rows) + 4)  # 기본3924는 유지하고 긴 실제 입력 계획의 마지막 관측까지 실행한다.
    command = [str(EXE), '--run', str(ROOT / 'originals'), '--mission', 'tutorial1', '--window', '--no-audio', '--frames', str(frame_limit),
               '--set', settings, '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')]
    profile = dict(initialProgressInjected=reset_progress_profile, productTimers=1, testStateInjection=False,
                   noStateInjection=not reset_progress_profile, planSha256=sha(plan), replaySha256=sha(script), command=command)
    save(directory / 'prepared.json', profile)
    return profile


def run(directory, *, reset_progress_profile=False, timeout=600, prepare_only=False, input_plan=None):
    """기본은 기존 초기값을 보존하고 실제 종료·원시 로그·전체 원본 SHA·설정 복구를 보관한다."""
    assert not directory.exists(), directory
    directory.mkdir(parents=True)
    profile = prepare(directory, reset_progress_profile, input_plan=input_plan)  # GUI 전에 실행 명령과 입력 전용 계획을 먼저 고정한다.
    if prepare_only:
        return dict(preparedOnly=True, **profile)
    before = snapshots(ROOT / 'originals')  # 허용 설정을 포함한 모든 파일의 실행 전 SHA다.
    save(directory / 'originals-before.json', before)
    evidence = dict(profile=profile, completed=False, passed=False)  # 예외/상한/설정 복구 실패도 성공으로 덮지 않는다.
    save(directory / 'execution.json', evidence)
    try:
        with preserve_game_settings(ROOT / 'originals', 'd', directory / 'settings-backup'):
            evidence.update(execute(profile['command'], directory, EXE, timeout))
    except Exception as error:
        evidence['error'] = repr(error)
        raise
    finally:
        after = snapshots(ROOT / 'originals')  # 복구 뒤 최종 실제 바이트를 기록해 이후에도 독립 대조할 수 있게 한다.
        save(directory / 'originals-after.json', after)
        evidence['originalsUnchanged'] = before == after
        save(directory / 'execution.json', evidence)
    try:
        assert not evidence['timedOut'], '실제 완주 관측의 실행 상한 도달'
        value = inspect(directory, evidence['returnCode'], evidence['originalsUnchanged'], expected_stock=1,
                        progress_profile=reset_progress_profile)
    except Exception as error:
        evidence['inspectionError'] = repr(error)
        save(directory / 'execution.json', evidence)
        raise
    value.update(binarySha256=evidence['binarySha256'], preparedProfile=profile, screenshots={path.name: sha(path) for path in directory.glob('*.bmp')})
    evidence.update(completed=True, passed=True)
    save(directory / 'execution.json', evidence);save(directory / 'results.json', value)
    return value


def main():
    """새 창/저장 관측 검사/현재 파일 출처 검증을 구별한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--plan', type=Path, help='입력 전용 계획 파일; 기본은 유지하며 QWASZX 여섯 슬롯 계획도 선택할 수 있다')
    parser.add_argument('--inspect', type=Path)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--fresh-progress-profile', action='store_true', help='명시 초기 프로필 검사로 DoneTutorial1=0을 설정한다; 기본은 기존 초기값 보존이다')
    parser.add_argument('--prepare-only', action='store_true', help='새 경로에 입력 계획과 명령만 만들고 게임을 실행하지 않는다')
    parser.add_argument('--timeout', type=float, default=600, help='실제 창 관측 상한 초; 전체 Report 조건 비용도 포함한다')
    args = parser.parse_args()
    if sum((bool(args.output), bool(args.inspect), args.verify)) != 1:
        parser.error('--output/--inspect/--verify 중 하나만 지정해야 합니다')
    if not args.output and (args.fresh_progress_profile or args.prepare_only or args.plan):
        parser.error('초기 프로필/입력 계획/계획 생성 옵션은 --output에만 사용할 수 있습니다')
    if args.timeout <= 0:
        parser.error('--timeout은 양수여야 합니다')
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for group in ('sources', 'evidence'):  # 모든 현재 파일을 확인하며 과거 관측은 덮어쓰지 않는다.
            for path, digest in report[group].items(): assert sha(ROOT / path) == digest, path
        assert report['observation']['naturalTutorialOneComplete'] and not report['goalComplete']
        print('Early1 실제 자연 완주/다음 튜토리얼 진입 출처 통과')
    elif args.inspect:
        evidence = json.loads((args.inspect / 'execution.json').read_text('utf-8'))
        assert not evidence.get('timedOut', False), '저장 관측이 상한 종료했습니다'
        profile = evidence.get('profile', {})  # 옛 결과에 없는 초기 프로필을 보존 실행으로 추정하지 않는다.
        print(json.dumps(inspect(args.inspect, evidence['returnCode'], evidence['originalsUnchanged'],
                                 expected_stock=profile.get('productTimers'), progress_profile=profile.get('initialProgressInjected')), ensure_ascii=False))
    else:
        if not args.output: parser.error('새 관측 폴더 --output이 필요합니다')
        print(json.dumps(run(args.output.resolve(), reset_progress_profile=args.fresh_progress_profile,
                             timeout=args.timeout, prepare_only=args.prepare_only, input_plan=args.plan), ensure_ascii=False))


if __name__ == '__main__':
    main()
