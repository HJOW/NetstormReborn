#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 메뉴의 Early6·캠페인16·Priest Training9와 Demo3의 실제 브리핑/플레이 진입을 검사한다."""
import argparse
import re
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_demo_start_smoke import DEMOS
from cpp_menu_smoke import source
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from taff import TaffArchive

BUTTONS = ('MORE', 'More', 'OK', 'Ok', 'ok', 'Play Mission', 'Lets do this!', ' Sun', 'Sun')  # 실제 안내 종료 라벨과 훈련 Fury의 명시 Sun 선택이다.
GROUPS = {'early': (1,), 'training': (2,), 'campaign': (3, 4, 5), 'all': (1, 2, 3, 4, 5)}  # 보호 officalN의 실제 분류다.


def catalog(archive, scope):
    """원본 officalN의 MissionBegin 메타에서 이름을 읽으며 미션 목록을 제품 출력으로 만들지 않는다."""
    result = []  # 순서와 이름은 보호 메뉴 원문을 따른다.
    for number in GROUPS[scope]:  # 선택한 원본 메뉴만 순서대로 읽는다.
        text = source(ROOT / 'originals', archive, f'd/offical{number}.english').decode('cp1252')
        result.extend(re.findall(r'(?im)^\$Checked=[^\r\n,]*,MissionBegin,([^,\r\n]+)', text))
    return result


def run(directory, mission, demo_label=None):
    """현재 브리핑의 관측 조건을 만족하는 버튼 입력과 자연 프레임만 사용한다."""
    directory.mkdir(parents=True, exist_ok=False)
    frames = 602 if demo_label else 98  # 데모는 최소18초 벽시계를 확보해 원본15초 timeout 뒤 진입까지 관측한다.
    steps = []  # 검사 전용 상태 쓰기 없이 같은 입력 큐를 통과시키는 계획이다.
    if demo_label:
        steps.extend(((3, 'click', 'Demo'), (5, 'click', demo_label)))
    for frame in range(12, frames-2, 8):  # Guide의 일곱 프레임 재개 관문을 지난 다음 상태를 관측한다.
        if not demo_label:
            for label in BUTTONS:  # 현재 표시된 정확한 라벨만 선택하고 없는 버튼 입력은 만들지 않는다.
                condition = r'phase\t3\n&&control\t'+re.escape(label)+r'\t'
                steps.append((frame, 'when', condition+';;click;;'+label))
        steps.append((frame, 'report', str(frame)))
    steps.extend(((frames-1, 'report', 'final'), (frames-1, 'snapshot', str(directory / 'final.bmp'))))
    steps.sort(key=lambda row: row[0])  # 같은 프레임에서 입력 뒤 보고하는 순서를 보존한다.
    plan = directory / 'steps.tsv'  # 실패 후에도 실제 입력 계획을 다시 재생할 수 있다.
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps),
                    encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(ROOT / 'originals'), '--window', '--no-audio',
               '--frames', str(frames), '--set',
               'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop='+('30' if demo_label else '1'),
               '--ui-script', str(plan), '--ui-report', str(directory / 'states.tsv')]
    if not demo_label:
        command.extend(('--mission', mission))
    result = execute(command, directory, GAME, 180 if demo_label else 65)  # 실제15초 안내 뒤 렌더링을 포함한 데모 관측의 상한이다.
    record = dict(mission=mission, demo=bool(demo_label), result=result, passed=False)  # 원시 실행 결과는 판정 전에 보존한다.
    save(directory / 'result.json', record)
    observed = states(directory / 'states.tsv') if (directory / 'states.tsv').exists() else {}
    final = observed.get('final', {})  # 실제 종료 직전 장면이며 중간 Mission 단계만으로 성공 처리하지 않는다.
    record['final'] = {key: final.get(key) for key in ('phase', 'page', 'paused', 'mission', 'mission_start', 'mission_gate', 'raw_session', 'raw_sp', 'raw_technology', 'raw_brains')}  # 같은 최종 관측의 판정 필드만 복사한다.
    record['checks'] = dict(normalExit=result['returnCode'] == 0 and not result['timedOut'],
                            playing=final.get('phase') == ['4'] and final.get('paused') == ['0'],
                            correctMission=bool(final.get('mission')) and final['mission'][0].casefold() == mission.casefold(),
                            world=bool(final.get('raw_objects')) and final.get('raw_session') == ['1', '1'],
                            guide=final.get('mission_start', [])[:2] == ['1', '4'],
                            loadingReleased='raw_mission_loading' not in final)
    if demo_label:
        record['checks']['demo'] = final.get('mission_start', [0, 0, 0])[2] == '1'
        record['checks']['localAI'] = final.get('raw_brains', ['0', '0'])[1] != '0'
    record['passed'] = all(record['checks'].values())
    save(directory / 'result.json', record)
    return record


def main():
    """선택한 범위를 순차 실행하고 실패한 스테이지도 빠짐없이 인수인계 자료에 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--scope', choices=tuple(GROUPS), default='all')
    parser.add_argument('--mission', action='append', help='원본 목록 안에서 실행할 미션을 제한한다')
    parser.add_argument('--demos', action='store_true', help='일반 미션 대신 실제 Demo 메뉴 세 항목을 실행한다')
    args = parser.parse_args()
    output = args.output.resolve()  # 기존 결과를 덮어쓰지 않는 이번 검사 경로다.
    output.mkdir(parents=True, exist_ok=False)
    archive = TaffArchive(ROOT / 'originals/netstorm.tarc')  # 읽기 전용 원본 메뉴 입력이다.
    names = list(DEMOS) if args.demos else [(name, None) for name in catalog(archive, args.scope)]  # 일반 미션 이름마다 데모 라벨 없음 표시를 붙인다.
    if args.mission:
        wanted = {name.casefold() for name in args.mission}  # 제한 이름도 원본 목록에 있는지 확인한다.
        names = [(name, label) for name, label in names if name.casefold() in wanted]  # 요청한 이름만 원본 순서로 남긴다.
        if {name.casefold() for name, _ in names} != wanted:
            raise RuntimeError('원본 목록에 없는 미션 이름')
    selected = [name for name, _ in names]  # 중단 시에도 남길 전체 선택 목록이며 게임 상태를 변경하지 않는다.
    before = snapshots(ROOT / 'originals')  # 허용 설정까지 포함한 전체 파일의 실행 전 해시다.
    records = []  # 실패가 뒤 스테이지의 관측을 생략하지 않게 한다.
    try:
        with preserve_game_settings(ROOT / 'originals', 'd'):
            for mission, label in names:  # 같은 옵션 파일을 사용하는 실제 창은 한 번에 하나만 실행한다.
                try:
                    record = run(output / mission, mission, label)
                except Exception as error:
                    record = dict(mission=mission, passed=False, error=repr(error))
                records.append(record)
                save(output / 'results.json', dict(runs=records, complete=False, selectedMissions=selected))
                print(mission, 'PASS' if record['passed'] else 'FAIL', record.get('checks', record.get('error')), flush=True)
    finally:
        unchanged = before == snapshots(ROOT / 'originals')  # 예외가 나도 원본 파일의 최종 바이트를 검사한다.
        summary = dict(runs=records, complete=len(records) == len(names), originalsUnchanged=unchanged,
                       selectedMissions=selected,
                       scope='실제 안내 버튼/자연 timeout 뒤 플레이 장면 진입이며 사용자 완주·승패·전체 동일성 검사가 아니다.')
        save(output / 'results.json', summary)
    if not names or not unchanged or any(not record['passed'] for record in records):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
