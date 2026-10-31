#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 메뉴의 Early6·캠페인16·Priest Training9와 Demo3의 실제 브리핑/플레이 진입을 검사한다."""
import argparse
import math
import re
import subprocess
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_demo_start_smoke import DEMOS
from cpp_menu_smoke import source
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from taff import TaffArchive

BUTTONS = ('MORE', 'More', 'OK', 'Ok', 'ok', 'Play Mission', 'Lets do this!', ' Sun', 'Sun', 'Fight On')  # 실제 안내 종료·훈련 Fury의 Sun·ThunderingPower 원문 @20의 계속 버튼이다.
GROUPS = {'early': (1,), 'training': (2,), 'campaign': (3, 4, 5), 'all': (1, 2, 3, 4, 5)}  # 보호 officalN의 실제 분류다.


def prepare_first_run(directory, game_root, binary, timeout):
    """새 배포본의 인트로를 실제 ESC로 닫고 안내 종료·메인 복귀·완료 저장을 확인한다."""
    directory.mkdir(parents=True, exist_ok=False)
    steps = [(3, 'report', 'startup')]  # 최초 영상/메뉴 상태를 그대로 남긴다.
    for frame in range(12, 180, 8):  # 실제 영상 singleton이 있을 때만 ESC를 보내 일반 메뉴를 열지 않는다.
        steps.append((frame, 'when', r'movie_runtime\t[1-9][0-9]*\t;;esc;;'))
        for label in ('OK', 'Ok', 'ok', 'Main Menu'):  # 새 사용자 안내의 실제 표시 버튼만 누른다.
            steps.append((frame, 'click-if', label))
    steps.append((199, 'report', 'final'))
    plan = directory / 'steps.tsv'  # 검사 입력과 실제 관측 파일을 새 경로에 보존한다.
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    command = [str(binary), '--run', str(game_root), '--no-audio', '--window', '--frames', '200',
               '--set', 'SCREENW=1024;SCREENH=768;maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=25',
               '--ui-script', str(plan), '--ui-report', str(directory / 'states.tsv')]  # intro 키나 미션 상태를 직접 덮어쓰지 않는다.
    result = execute(command, directory, binary, timeout or 65)  # 기존 실제 입력 큐와 자연 상태 전환을 사용한다.
    observed = states(directory / 'states.tsv') if (directory / 'states.tsv').exists() else {}  # 실패한 관측도 보존한다.
    final = observed.get('final', {})  # 영상 종료 뒤 실제 메인 상태를 확인한다.
    record = dict(result=result, final={key: final.get(key) for key in ('phase', 'page', 'paused', 'movie_runtime')})  # 완료 판정 전 원시를 남긴다.
    record['introSaved'] = subprocess.check_output([str(binary), '--config-get', str(game_root), 'intro'], cwd=ROOT, text=True, encoding='utf-8').strip() == '1'  # 실제 종료 때 저장한 완료 키를 읽기만 한다.
    record['passed'] = result['returnCode'] == 0 and not result['timedOut'] and final.get('phase') == ['0'] and final.get('page') == ['main'] and final.get('paused') == ['0'] and final.get('movie_runtime', ['0'])[0] == '0' and record['introSaved']
    save(directory / 'result.json', record)
    if not record['passed']:
        raise RuntimeError('새 설치 인트로/안내의 실제 종료 실패')
    return record


def catalog(archive, scope):
    """원본 officalN의 MissionBegin 메타에서 이름을 읽으며 미션 목록을 제품 출력으로 만들지 않는다."""
    result = []  # 순서와 이름은 보호 메뉴 원문을 따른다.
    for number in GROUPS[scope]:  # 선택한 원본 메뉴만 순서대로 읽는다.
        text = source(ROOT / 'originals', archive, f'd/offical{number}.english').decode('cp1252')
        result.extend(re.findall(r'(?im)^\$Checked=[^\r\n,]*,MissionBegin,([^,\r\n]+)', text))
    return result


def run(directory, mission, demo_label=None, game_root=None, fullscreen=False, frame_limit=98, fps=60, timeout=None, binary=None, capture_output=False):
    """현재 브리핑의 관측 조건을 만족하는 버튼 입력과 자연 프레임만 사용한다."""
    directory.mkdir(parents=True, exist_ok=False)
    game_root=game_root or ROOT/'originals' # 새 검사는 원본 사본을 지정할 수 있으며 기존 호출의 기본 경로는 유지한다.
    binary=binary or GAME  # 실제 압축 해제본도 선택할 수 있으며 지정하지 않은 기존 호출은 현재 빌드를 사용한다.
    frames = 602 if demo_label else frame_limit  # 데모는 최소18초 벽시계를 확보해 원본15초 timeout 뒤 진입까지 관측한다.
    steps = []  # 검사 전용 상태 쓰기 없이 같은 입력 큐를 통과시키는 계획이다.
    if demo_label:
        steps.extend(((3,'click-if','OK'),(5,'click-if','OK'),(7,'click-if','Main Menu'),(10,'report','menu-before-demo'),(12,'click','Demo'),(16,'report','demo-choices'),(18,'click',demo_label))) # 새 설정의 실제 Welcome2 안내도 Main Menu 버튼으로 닫은 뒤 같은 Demo를 누른다. 프레임/시계/판정은 유지한다.
    for frame in range(12, frames-2, 8):  # Guide의 일곱 프레임 재개 관문을 지난 다음 상태를 관측한다.
        if not demo_label:
            for label in BUTTONS:  # 현재 표시된 정확한 라벨만 선택하고 없는 버튼 입력은 만들지 않는다.
                condition = r'phase\t[13]\n&&(?:control\t'+re.escape(label)+r'\t|raw_html_button[^\n]*\t'+re.escape(label)+r'\n)'  # 실제 Tell 큐로 연 Dialog와 초기 Briefing의 표시된 버튼을 모두 허용한다.
                steps.append((frame, 'when', condition+';;click;;'+label))
        steps.append((frame, 'report', str(frame)))
    steps.extend(((frames-1, 'report', 'final'), (frames-1, 'snapshot-presented', str(directory / 'final.bmp')),
                  (frames-1, 'snapshot', str(directory / 'final-after-camera.bmp'))))  # 원문 Present 뒤 카메라 Scroll은 다음 버퍼를 바꾸므로 창 비교는 표시 직후 원본에 없는 읽기 전용 관찰점을 쓴다. 기존 뒤 버퍼도 별도 보존한다.
    if capture_output: steps.append((frames-1,'snapshot-output',str(directory/'final-output.bmp')))  # 같은 완성 프레임의 실제 창 DC도 별도로 읽는다.
    steps.sort(key=lambda row: row[0])  # 같은 프레임에서 입력 뒤 보고하는 순서를 보존한다.
    plan = directory / 'steps.tsv'  # 실패 후에도 실제 입력 계획을 다시 재생할 수 있다.
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps),
                    encoding='utf-8', newline='\n')
    command = [str(binary), '--run', str(game_root), '--no-audio',
               '--frames', str(frames), '--set',
               f'SCREENW=1024;SCREENH=768;maxFPS={fps};autoDemo=0;tellTips=0;startInFullScreen=1;sleepPerLoop='+('30' if demo_label else '1'),
               '--ui-script', str(plan), '--ui-report', str(directory / 'states.tsv')]
    if not fullscreen:
        command.append('--window')
    if not demo_label:
        command.extend(('--mission', mission))
    result = execute(command, directory, binary, timeout if timeout is not None else (180 if demo_label else 65))  # 명시 상한은 대기만 바꾸며 프레임/시계/판정은 유지한다. 생략 시 기존 일반65초/데모180초다.
    record = dict(mission=mission, demo=bool(demo_label), result=result, passed=False)  # 원시 실행 결과는 판정 전에 보존한다.
    save(directory / 'result.json', record)
    observed = states(directory / 'states.tsv') if (directory / 'states.tsv').exists() else {}
    final = observed.get('final', {})  # 실제 종료 직전 장면이며 중간 Mission 단계만으로 성공 처리하지 않는다.
    record['final'] = {key: final.get(key) for key in ('phase', 'page', 'paused', 'mission', 'mission_start', 'mission_gate', 'raw_session', 'raw_sp', 'raw_technology', 'raw_brains', 'fort_names', 'raw_uber', 'raw_hud_owners','time','screen_mode','screen_output','screen_window')}  # 같은 최종 관측의 판정 필드만 복사한다.
    playing_times=[float(scene['time'][0]) for scene in observed.values() if scene.get('phase')==['4'] and scene.get('paused')==['0'] and scene.get('raw_session')==['1','1'] and scene.get('time')]  # 비정지 실제 Session의 시각만 관측 순서대로 읽는다.
    record['gameClock']={'samples':len(playing_times),'first':playing_times[0] if playing_times else None,'last':playing_times[-1] if playing_times else None,'increase':playing_times[-1]-playing_times[0] if len(playing_times)>1 else 0}
    record['checks'] = dict(normalExit=result['returnCode'] == 0 and not result['timedOut'],
                            playing=final.get('phase') == ['4'] and final.get('paused') == ['0'],
                            correctMission=bool(final.get('mission')) and final['mission'][0].casefold() == mission.casefold(),
                            world=bool(final.get('raw_objects')) and final.get('raw_session') == ['1', '1'],
                            guide=final.get('mission_start', [])[:2] == ['1', '4'],
                            loadingReleased='raw_mission_loading' not in final,
                            gameClockAdvanced=len(playing_times)>1 and playing_times[-1]>playing_times[0],
                            gameClockMonotonic=all(after>=before for before,after in zip(playing_times,playing_times[1:])))
    if demo_label:
        record['checks']['demo'] = final.get('mission_start', [0, 0, 0])[2] == '1'
        record['checks']['localAI'] = final.get('raw_brains', ['0', '0'])[1] != '0'
    if capture_output:
        from cpp_movie_runtime_smoke import verify_presentation  # 현재 논리 프레임을 정수 최근접/레터박스로 펼쳐 실제 창의 모든 RGB와 대조한다.
        if (directory/'final.bmp').exists() and (directory/'final-output.bmp').exists() and final.get('screen_output'):
            record['presentation']=verify_presentation(directory/'final.bmp',directory/'final-output.bmp',final,directory);record['checks']['physicalOutput']=record['presentation']['same']
        else: record['checks']['physicalOutput']=False
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
    parser.add_argument('--game-dir',type=Path,default=ROOT/'originals',help='실제 게임 자료 경로. 원본을 보호하려면 별도 사본을 지정한다')
    parser.add_argument('--binary',type=Path,default=GAME,help='실제 실행할 빌드 또는 압축 해제본의 클론 실행 파일')
    parser.add_argument('--capture-output',action='store_true',help='같은 프레임의 실제 창 전체 RGB/레터박스도 비교한다')
    parser.add_argument('--prepare-first-run',action='store_true',help='같은 설정 보호 구간에서 인트로를 실제 ESC로 닫고 메인 안내를 끝낸 뒤 미션을 검사한다')
    parser.add_argument('--fullscreen',action='store_true',help='현재 전체화면 호환 장치에서 실행한다')
    parser.add_argument('--frames',type=int,default=98,help='일반 미션의 짧은 관측 프레임 수')
    parser.add_argument('--fps',type=int,default=60,help='실제 프레임 상한. 데모는 기존 최소15초 안내를 유지한다')
    parser.add_argument('--timeout',type=float,help='프로세스 종료 대기 상한(초). 생략 시 일반65초/데모180초이며 게임 시각이나 프레임을 바꾸지 않는다')
    args = parser.parse_args()
    if args.timeout is not None and (not math.isfinite(args.timeout) or args.timeout <= 0):
        parser.error('--timeout은 양수여야 합니다')
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
    game_root=args.game_dir.resolve();before = snapshots(game_root)  # 선택한 사본/원본의 허용 설정까지 포함한 실행 전 해시다.
    original_root=(ROOT/'originals').resolve();original_before=before if game_root==original_root else snapshots(original_root) # 사본 상태와 보호 원본의 증거를 구별한다.
    records = []  # 실패가 뒤 스테이지의 관측을 생략하지 않게 한다.
    startup = None  # 첫 실행 준비의 결과는 미션34개와 별도로 기록한다.
    try:
        with preserve_game_settings(game_root, 'd', output / 'settings-backup'):
            if args.prepare_first_run:
                startup = prepare_first_run(output / 'startup', game_root, args.binary.resolve(), args.timeout)
            for mission, label in names:  # 같은 옵션 파일을 사용하는 실제 창은 한 번에 하나만 실행한다.
                try:
                    record = run(output / mission, mission, label,game_root,args.fullscreen,args.frames,args.fps,args.timeout,args.binary.resolve(),args.capture_output)
                except Exception as error:
                    record = dict(mission=mission, passed=False, error=repr(error))
                records.append(record)
                save(output / 'results.json', dict(runs=records, complete=False, selectedMissions=selected))
                print(mission, 'PASS' if record['passed'] else 'FAIL', record.get('checks', record.get('error')), flush=True)
    finally:
        unchanged = before == snapshots(game_root)  # 예외가 나도 선택한 자료의 최종 바이트를 검사한다.
        original_unchanged=unchanged if game_root==original_root else original_before==snapshots(original_root)
        summary = dict(runs=records, startup=startup, complete=len(records) == len(names), originalsUnchanged=original_unchanged,gameDataUnchanged=unchanged,gameDirectory=str(game_root),
                       selectedMissions=selected,
                       scope='실제 안내 버튼/자연 timeout 뒤 플레이 장면 진입이며 사용자 완주·승패·전체 동일성 검사가 아니다.')
        save(output / 'results.json', summary)
    if not names or not unchanged or not original_unchanged or any(not record['passed'] for record in records):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
