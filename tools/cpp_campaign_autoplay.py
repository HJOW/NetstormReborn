# -*- coding: utf-8 -*-
"""
cpppj 테스트 전용 캠페인/스커미시 자동 플레이 실행 도구 (2026-11-14 신규, 같은 날 스커미시 모드 추가)

cpppj Netstorm.exe 의 테스트 전용 옵션(--test-*)으로 캠페인 미션 또는 스커미시를 아군 AI 자동 플레이로 실행하고,
일정 프레임마다 session-dump·report(선택: 화면 캡처)를 남겨 소유자별 객체 수·SP·AI 부착 여부·승패를 요약한다.
게임 창이 뜨므로 AGENTS.md 의 실제 게임 실행 확인 규칙(예외 호스트 포함)을 따른다.

설정·캠페인 진행 기록은 출력 폴더의 user-data 로 격리하므로 실제 %USERPROFILE%\\Saved Games\\Netstorm 은 바뀌지 않는다.

사용법 예:
    python -X utf8 tools/cpp_campaign_autoplay.py --mission thewarbegins
    python -X utf8 tools/cpp_campaign_autoplay.py --mission thewarbegins --start-sp 20000 --enemy-start-sp 1000 --frames 72000 --snapshot-every 12000
    python -X utf8 tools/cpp_campaign_autoplay.py --skirmish --skirmish-ai-count 2 --skirmish-allies 3 --frames 72000

AI 에이전트용 안내: docs/testing/cpp-autoplay-test-guide.md
"""
import argparse
import datetime
import json
import os
import re
import struct
import subprocess
import sys
import time
from pathlib import Path

# 저장소 최상위 경로이며 기본 실행 파일/게임 자료/출력 위치의 기준이다.
ROOT = Path(__file__).resolve().parents[1]
# Player 기록 하나의 크기와 AI 본체 토큰 위치다(cpppj Player+0xb0).
PLAYER_BYTES = 180
BRAIN_OFFSET = 0xB0
# 미션 진행 중 표시되는 안내/선택 창을 닫는 버튼 이름이다(캠페인 16개 미션의 [A.]·[A1.]·[@시각] 구간 조사 결과).
# 승패 창의 버튼(Next Mission/Leave Missions/Continue/OK 등)과 항복 쪽 선택(Surrender 등)은 누르지 않는다. 'OK'는 FinalConfrontation 승리 창의 MissionAbort 버튼이라 뺐다.
SAFE_BUTTONS = ('Play Mission', 'Go!', 'MORE', 'More', 'Fight On', 'Fight On!', 'Continue Fighting', 'Ignore', 'Go to Hell')
# 스커미시 시작까지 메인 메뉴에서 차례로 누르는 버튼이다(Campaign → Skirmish → Start).
SKIRMISH_MENU_BUTTONS = ('Campaign', 'Skirmish', 'Start')
# 스커미시 메뉴 버튼을 누르는 마지막 프레임이다. 이후에는 메뉴로 돌아가도 새 스커미시를 시작하지 않는다.
SKIRMISH_MENU_LAST_FRAME = 1500
# 승리/패배 안내 섹션 이름(소문자)이다. 캠페인은 Done<미션> 설정 값도 승리로 본다.
VICTORY_PAGES = {'badteamdead', 'succeeded', 'success', 'ai2priestsaved', 'victorynote'}
DEFEAT_PAGES = {'goodteamdead', 'failed'}
# 소유자별 사제/신전 사망 패배 섹션 이름(예: Ai2PriestDead, Ai1TempleDead)이다.
DEFEAT_PAGE_PATTERN = re.compile(r'^ai\d+(priest|temple)dead$')
# 난이도 이름을 스커미시 설정 값(1/2/3)으로 바꾸는 표다.
LEVELS = {'easy': 1, 'normal': 2, 'hard': 3, '1': 1, '2': 2, '3': 3}


def build_steps(frames, dump_every, click_every, snapshot_every, skirmish, out_dir, extra_buttons=(), when_clicks=()):
    """UI 스크립트 단계(프레임, 동작, 인자) 목록을 만든다. 안내 버튼 클릭과 덤프/보고(선택: 화면 캡처)를 주기적으로 넣는다.

    extra_buttons: 기본 목록 외에 항상 누를 버튼 이름. when_clicks: (보고 정규식, 버튼) 쌍이며 현재 보고가 정규식과 맞을 때만 그 버튼을 누른다(승패 창과 같은 이름의 버튼을 안내 창에서만 누를 때 쓴다).
    """
    steps = []
    # 스커미시는 처음 구간에서만 메인 메뉴 → Campaign → Skirmish → Start 를 차례로 누른다(없는 버튼은 click-if 가 무시한다).
    for frame in range(30, min(frames, SKIRMISH_MENU_LAST_FRAME), 10) if skirmish else ():
        for button in SKIRMISH_MENU_BUTTONS:
            steps.append((frame, 'click-if', button))
    # 매 click_every 프레임마다 표시된 안내 버튼만 누른다(없으면 click-if 는 아무 것도 하지 않는다).
    for frame in range(5, frames, click_every):
        for button in (*SAFE_BUTTONS, *extra_buttons):
            steps.append((frame, 'click-if', button))
        # 'when' 단계의 인자는 "정규식;;동작;;인자"이며 보고에 정규식이 있을 때만 입력을 넣는다.
        for pattern, button in when_clicks:
            steps.append((frame, 'when', f'{pattern};;click-if;;{button}'))
    # 매 dump_every 프레임마다 그리기 뒤 상태를 기록한다.
    for frame in range(dump_every, frames, dump_every):
        steps.append((frame, 'session-dump', str(out_dir / f'dump-{frame:06d}.tsv')))
        steps.append((frame, 'report', ''))
    # 매 snapshot_every 프레임마다 화면을 BMP로 남긴다(0이면 끈다).
    for frame in range(snapshot_every, frames, snapshot_every) if snapshot_every > 0 else ():
        steps.append((frame, 'snapshot', str(out_dir / f'snap-{frame:06d}.bmp')))
    steps.sort(key=lambda item: (item[0], item[1] != 'click-if'))
    return steps


def float_bits(text):
    """session-dump 의 float 비트 정수를 float 로 바꾼다."""
    return struct.unpack('<f', struct.pack('<I', int(text) & 0xFFFFFFFF))[0]


def parse_dump(path):
    """session-dump 한 파일에서 소유자별 객체 수·SP·AI 토큰을 읽는다."""
    owners, sp, brains, frame = {}, {}, {}, None
    # 탭 구분 행을 첫 열 이름으로 나눠 필요한 행만 해석한다.
    for line in path.read_text(encoding='ascii', errors='replace').splitlines():
        row = line.split('\t')
        if row[0] == 'frame' and len(row) >= 2:
            frame = int(row[1])
        elif row[0] == 'object' and len(row) >= 5:
            owner = int(row[4])
            owners[owner] = owners.get(owner, 0) + 1
        elif row[0] == 'player' and len(row) >= 3:
            raw = bytes.fromhex(row[2])
            if len(raw) >= BRAIN_OFFSET + 4:
                brains[int(row[1])] = struct.unpack_from('<I', raw, BRAIN_OFFSET)[0]
        elif row[0] == 'sp' and len(row) >= 12 and row[1] == '1':
            # 값 칸은 소유자 0~8의 float 비트다.
            for owner, value in enumerate(row[3:12]):
                sp[owner] = round(float_bits(value), 1)
    return {'frame': frame, 'objects_by_owner': owners, 'sp': sp, 'ai_owners': sorted(owner for owner, token in brains.items() if token)}


def parse_reports(path):
    """report.tsv 에서 보고 시점별 page/popup/미션 완료 값을 읽는다."""
    reports, current = [], None
    if not path.exists():
        return reports
    # 'frame\tN\treportN' 머리 뒤의 행들을 그 시점의 관측으로 묶는다.
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        row = line.split('\t')
        if row[0] == 'frame' and len(row) >= 2 and row[1].isdigit():
            current = {'frame': int(row[1])}
            reports.append(current)
        elif current is not None and row[0] in ('page', 'popup', 'mission_progress', 'paused', 'current_dialog') and len(row) >= 2:
            current[row[0]] = row[1]
        elif current is not None and row[0] == 'mission' and len(row) >= 2:
            current['mission'] = row[1]
    return reports


def judge(reports):
    """보고 목록에서 처음 관측된 승리/패배와 그 프레임을 돌려준다. 둘 다 없으면 ('undecided', None)이다."""
    # 시간 순서대로 처음 결정된 결과를 쓴다.
    for item in reports:
        page = item.get('page', '').lower()
        if item.get('mission_progress', '0') not in ('0', '') or page in VICTORY_PAGES:
            return 'victory', item['frame']
        if page in DEFEAT_PAGES or DEFEAT_PAGE_PATTERN.match(page):
            return 'defeat', item['frame']
    return 'undecided', None


def skirmish_settings(args):
    """스커미시 화면 설정 키를 --set 문자열 조각으로 만든다(격리된 user-data 의 options.cfg 에만 쓰인다)."""
    count = max(1, min(7, args.skirmish_ai_count))
    allies = {int(item) for item in args.skirmish_allies.split(',') if item.strip()}
    diffs = {}
    # "3=hard,4=easy" 형식의 AI별 스커미시 난이도를 읽는다.
    for item in (part for part in args.skirmish_ai_diff.split(',') if part.strip()):
        owner, level = item.split('=')
        diffs[int(owner)] = LEVELS[level.strip().lower()]
    pieces = [f'skirmishAiCount={count}', f'skirmishBans={args.skirmish_bans}']
    # 소유자 2..8 의 난이도/동맹 값을 모두 명시해 이전 설정이 섞이지 않게 한다.
    for owner in range(2, 9):
        pieces.append(f'skirmishAi{owner}Diff={diffs.get(owner, 2)}')
        pieces.append(f'skirmishAi{owner}Ally={1 if owner in allies else 0}')
    return ';'.join(pieces)


def main():
    """명령줄을 해석해 게임을 실행하고 요약을 출력한다."""
    parser = argparse.ArgumentParser(description='cpppj 테스트 전용 캠페인/스커미시 자동 플레이 실행')
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument('--mission', help='캠페인/일반 미션 키 (예: thewarbegins)')
    target.add_argument('--skirmish', action='store_true', help='메인 메뉴에서 스커미시를 시작한다')
    parser.add_argument('--skirmish-ai-count', type=int, default=1, help='스커미시 AI 수 1..7 (소유자 2..AI수+1)')
    parser.add_argument('--skirmish-allies', default='', help='플레이어 편 스커미시 AI 소유자 목록 (예: "3,4")')
    parser.add_argument('--skirmish-ai-diff', default='', help='스커미시 화면의 AI별 난이도 (예: "2=hard,3=easy"). --ally-ai/--enemy-ai 가 지정되면 그쪽이 우선한다')
    parser.add_argument('--skirmish-bans', default='', help='금지 유닛 TYPE 이름 쉼표 목록 (최대 5개)')
    parser.add_argument('--exe', default=str(ROOT / 'cpppj/build/bin/Release/Netstorm.exe'), help='Netstorm.exe 경로')
    parser.add_argument('--game', default=str(ROOT / 'assets/game-data'), help='게임 자료 폴더 (--run 인자)')
    parser.add_argument('--out', default='', help='출력 폴더 (기본: extracted/cpp-campaign-autoplay/<대상>-<시각>)')
    parser.add_argument('--frames', type=int, default=24000, help='실행할 총 프레임 수 (기본 24000 = 고정 25ms 기준 게임 시간 600초)')
    parser.add_argument('--frame-ms', type=int, default=25, help='고정 시각 모드의 프레임당 ms (0이면 실제 OS 시계)')
    parser.add_argument('--unix-seconds', type=int, default=0, help='고정 시각 모드의 가상 유닉스 초 (0이면 기본값)')
    parser.add_argument('--dump-every', type=int, default=2000, help='session-dump/report 주기 (프레임)')
    parser.add_argument('--click-every', type=int, default=20, help='안내 버튼 클릭 주기 (프레임)')
    parser.add_argument('--snapshot-every', type=int, default=0, help='화면 캡처(BMP) 주기 (프레임, 0이면 끔)')
    parser.add_argument('--buttons', default='', help='기본 안내 버튼 목록 외에 항상 누를 버튼 이름 쉼표 목록')
    parser.add_argument('--when-click', action='append', default=[], help="조건부 클릭 '보고정규식;;버튼' (여러 번 지정 가능). 예: 'page\\tAi2PriestCaptured;;Continue'")
    parser.add_argument('--ally-ai', default='hard', help='아군(로컬·동맹) AI 난이도 easy/normal/hard, none 이면 아군 AI 없음')
    parser.add_argument('--enemy-ai', default='easy', help='적 AI 난이도 easy/normal/hard, option 이면 옵션(스커미시는 화면) 값 사용')
    parser.add_argument('--start-sp', type=int, default=None, help='로컬 시작 SP (기본: 미션 값, 스커미시 5000)')
    parser.add_argument('--enemy-start-sp', type=int, default=None, help='적 AI 시작 SP (기본: 미션 값, 스커미시 5000)')
    parser.add_argument('--no-follow', action='store_true', help='관전 카메라(주요 전투·희생 의식 제단으로 화면 이동)를 끈다. 기본은 켬')
    parser.add_argument('--language', default='english', help='게임 언어 (english 권장: 버튼 이름이 영어 기준이다)')
    parser.add_argument('--timeout', type=int, default=3600, help='실행 제한 시간 (초)')
    parser.add_argument('--no-early-stop', action='store_true', help='승패가 확정돼도 --frames 끝까지 계속 실행한다 (기본: 확정 뒤 보고가 --keep-reports 개 더 쌓이면 종료)')
    parser.add_argument('--keep-reports', type=int, default=1, help='승패 확정 뒤 더 지켜볼 보고 개수 (기본 1)')
    args = parser.parse_args()

    stamp = datetime.datetime.now().strftime('%Y%m%d%H%M%S')
    name = args.mission if args.mission else 'skirmish'
    out_dir = Path(args.out) if args.out else ROOT / 'extracted/cpp-campaign-autoplay' / f'{name}-{stamp}'
    out_dir.mkdir(parents=True, exist_ok=True)
    user_data = out_dir / 'user-data'
    user_data.mkdir(exist_ok=True)
    extra_buttons = [item.strip() for item in args.buttons.split(',') if item.strip()]
    when_clicks = []
    # "정규식;;버튼" 형식의 조건부 클릭을 읽는다(예: 'page\tAi2PriestCaptured;;Continue').
    for item in args.when_click:
        pattern, button = item.split(';;', 1)
        when_clicks.append((pattern, button))
    steps = build_steps(args.frames, args.dump_every, args.click_every, args.snapshot_every, args.skirmish, out_dir, extra_buttons, when_clicks)
    steps_path = out_dir / 'steps.tsv'
    steps_path.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8')

    settings = f'SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0;intro=1;currentLanguage={args.language}'
    if args.skirmish:
        settings += ';' + skirmish_settings(args)
    command = [args.exe, '--run', args.game, '--window', '--mute-audio', '--frames', str(args.frames), '--set', settings,
               '--ui-script', str(steps_path), '--ui-report', str(out_dir / 'report.tsv')]
    if args.mission:
        command[3:3] = ['--mission', args.mission]
    if args.frame_ms:
        command += ['--fixed-frame-ms', str(args.frame_ms)]
        if args.unix_seconds:
            command += ['--fixed-unix-seconds', str(args.unix_seconds)]
    # 테스트 전용 옵션: 아군 AI·적 난이도·시작 SP다.
    if args.ally_ai.lower() != 'none':
        command += ['--test-ally-ai', args.ally_ai]
    if args.enemy_ai.lower() != 'option':
        command += ['--test-enemy-ai', args.enemy_ai]
    if args.start_sp is not None:
        command += ['--test-start-sp', str(args.start_sp)]
    if args.enemy_start_sp is not None:
        command += ['--test-enemy-start-sp', str(args.enemy_start_sp)]
    if not args.no_follow:
        command += ['--test-follow-battle']

    env = dict(os.environ, NETSTORM_USER_DATA=str(user_data))
    (out_dir / 'run.json').write_text(json.dumps({'argv': command, 'NETSTORM_USER_DATA': str(user_data)}, ensure_ascii=False, indent=2), encoding='utf-8')
    stopped_early, timed_out = False, False
    with (out_dir / 'console.txt').open('w', encoding='utf-8') as console:
        process = subprocess.Popen(command, cwd=ROOT, env=env, stdout=console, stderr=subprocess.STDOUT)
        started, outcome_at = time.time(), None
        # 게임이 끝나거나 제한 시간이 될 때까지 보고 파일을 지켜본다. 결과가 나온 뒤 보고 --keep-reports 개가 더 쌓이면 남은 프레임을 기다리지 않고 종료한다.
        while process.poll() is None:
            time.sleep(2)
            if time.time() - started > args.timeout:
                timed_out = True
                process.kill()
                break
            if args.no_early_stop:
                continue
            try:
                reports_now = parse_reports(out_dir / 'report.tsv')
            except OSError:
                continue  # 게임이 보고 파일을 다시 쓰는 중이면 다음 확인으로 넘긴다.
            decided, _ = judge(reports_now)
            if decided != 'undecided' and outcome_at is None:
                outcome_at = len(reports_now)
            if outcome_at is not None and len(reports_now) >= outcome_at + args.keep_reports:
                stopped_early = True
                process.kill()  # 이 도구가 띄운 프로세스만 종료한다. 사용자 데이터는 출력 폴더에 격리되어 있다.
                break
        process.wait()
    result = process

    dumps = [parse_dump(path) for path in sorted(out_dir.glob('dump-*.tsv'))]
    reports = parse_reports(out_dir / 'report.tsv')
    outcome, outcome_frame = judge(reports)
    console_text = (out_dir / 'console.txt').read_text(encoding='utf-8', errors='replace')
    summary = {'target': name, 'exit_code': 0 if stopped_early else result.returncode, 'stopped_early': stopped_early, 'timed_out': timed_out,
               'outcome': outcome, 'outcome_frame': outcome_frame,
               'local_ai_attached': '로컬 owner' in console_text and 'AI 생성 완료' in console_text,
               'ai_diagnostics': console_text.count('AI 원본 진단'),
               'camera_moves': {'battle': console_text.count('테스트 관전 카메라: 전투'), 'ritual': console_text.count('테스트 관전 카메라: 희생 의식 제단')},
               'dumps': dumps, 'reports': reports, 'logs': [str(path) for path in sorted((user_data / 'logs').glob('*.log'))]}
    (out_dir / 'summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
    # 사람이 읽는 간단한 표를 출력한다.
    print(f'출력 폴더: {out_dir}')
    print(f"종료 코드: {summary['exit_code']}{' (결과 확정 뒤 조기 종료)' if stopped_early else ''}{' (제한 시간 초과)' if timed_out else ''} / 로컬 AI 부착: {summary['local_ai_attached']} / AI 원본 진단: {summary['ai_diagnostics']} / 오류 로그: {len(summary['logs'])}")
    for item in dumps:
        print(f"frame {item['frame']}: AI={item['ai_owners']} objects={dict(sorted(item['objects_by_owner'].items()))} sp={item['sp']}")
    for item in reports:
        print(f"report {item['frame']}: page={item.get('page')} popup={item.get('popup')} paused={item.get('paused')} done={item.get('mission_progress')}")
    print(f"관전 카메라 이동: 전투 {summary['camera_moves']['battle']}회 / 희생 의식 {summary['camera_moves']['ritual']}회")
    print(f'결과: {outcome} (프레임 {outcome_frame})')
    return summary['exit_code']


if __name__ == '__main__':
    sys.exit(main())
