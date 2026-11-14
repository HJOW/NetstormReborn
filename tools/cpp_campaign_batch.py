# -*- coding: utf-8 -*-
"""
cpppj 캠페인 전 미션 아군 AI 자동 플레이 일괄 실행 도구 (2026-11-14 신규)

tools/cpp_campaign_autoplay.py 를 미션마다 한 번씩 실행해(동시에 --workers 개) 결과를 한 표로 모은다.
기본 대상은 캠페인 1~3(메뉴 offical3~5)의 16개 미션이며 메뉴 순서다. 튜토리얼 성격의 Early Missions·Priest Training은 --groups 로 명시할 때만 포함하고, 사용자 제작 캠페인은 대상이 아니다.
게임 창이 뜨므로 AGENTS.md 의 실제 게임 실행 확인 규칙(예외 호스트 포함)을 따른다.

사용법 예:
    python -X utf8 tools/cpp_campaign_batch.py --out extracted/cpp-campaign-batch/run1
    python -X utf8 tools/cpp_campaign_batch.py --missions TheWarBegins,Tutorial1 --frames 48000 --start-sp 20000 --enemy-start-sp 1000 --workers 2

설명서: docs/testing/cpp-autoplay-test-guide.md
"""
import argparse
import json
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

# 저장소 최상위 경로다.
ROOT = Path(__file__).resolve().parents[1]
# 캠페인 메뉴 순서의 미션 키다(offical1~offical5 메뉴의 MissionBegin 인자, 사용자 제작 캠페인 offical6은 제외).
CAMPAIGN_GROUPS = [
    ('Early Missions', ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']),
    ('Priest Training', ['bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76']),
    ('Campaign 1', ['TheWarBegins', 'MasterOfWhirligigs', 'SaveTheIsland', 'FragileFortune', 'ThunderingPower', 'DissolvedAlliance']),
    ('Campaign 2', ['TheNoose', 'RainVsRain', 'RunForIt', 'GuardMyBack', 'Surrounded']),
    ('Campaign 3', ['BreakingThrough', 'ToTheRescue', 'Vicious', 'EnemyTerritory', 'FinalConfrontation']),
]


# 기본 실행 대상 그룹이다. 튜토리얼 성격의 Early Missions·Priest Training과 사용자 제작 캠페인은 --groups 로 명시할 때만 포함한다.
DEFAULT_GROUPS = ('Campaign 1', 'Campaign 2', 'Campaign 3')
# 미션별 조건부 클릭(보고 정규식;;버튼)이다. EnemyTerritory 의 Ai2PriestCaptured 안내 창은 'Continue'(DoNothing)로 닫아야 게임이 계속되지만,
# 같은 이름의 'Continue'가 실패 창([Failed])에도 있어 그 안내 창이 떠 있을 때만 누른다.
MISSION_WHEN_CLICKS = {'EnemyTerritory': [r'page\tAi2PriestCaptured;;Continue']}


def all_missions(groups=None):
    """메뉴 순서의 (그룹, 미션 키) 목록을 돌려준다. groups 가 있으면 그 그룹만 포함한다."""
    return [(group, key) for group, keys in CAMPAIGN_GROUPS if groups is None or group in groups for key in keys]


def run_one(mission, args, out_root):
    """미션 하나를 실행 도구로 돌리고 summary.json 의 요약 한 줄을 돌려준다."""
    out_dir = out_root / mission
    command = [sys.executable, '-X', 'utf8', str(ROOT / 'tools/cpp_campaign_autoplay.py'), '--mission', mission,
               '--frames', str(args.frames), '--dump-every', str(args.dump_every), '--snapshot-every', str(args.snapshot_every),
               '--ally-ai', args.ally_ai, '--enemy-ai', args.enemy_ai, '--timeout', str(args.timeout), '--out', str(out_dir)]
    # 지정한 선택 인자만 그대로 전달한다.
    if args.start_sp is not None:
        command += ['--start-sp', str(args.start_sp)]
    if args.enemy_start_sp is not None:
        command += ['--enemy-start-sp', str(args.enemy_start_sp)]
    if args.no_follow:
        command += ['--no-follow']
    for item in MISSION_WHEN_CLICKS.get(mission, []):
        command += ['--when-click', item]
    started = time.time()
    with (out_root / f'{mission}.runner.txt').open('w', encoding='utf-8') as log:
        completed = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    seconds = round(time.time() - started)
    summary_path = out_dir / 'summary.json'
    if not summary_path.exists():
        return {'mission': mission, 'outcome': 'no-summary', 'runner_exit': completed.returncode, 'seconds': seconds}
    summary = json.loads(summary_path.read_text(encoding='utf-8'))
    dumps = summary.get('dumps', [])
    last = dumps[-1] if dumps else {}
    return {'mission': mission, 'outcome': summary['outcome'], 'outcome_frame': summary['outcome_frame'], 'exit_code': summary['exit_code'],
            'stopped_early': summary.get('stopped_early'), 'timed_out': summary.get('timed_out'), 'local_ai_attached': summary['local_ai_attached'],
            'ai_diagnostics': summary['ai_diagnostics'], 'error_logs': len(summary['logs']), 'camera_moves': summary.get('camera_moves'),
            'last_frame': last.get('frame'), 'last_objects': last.get('objects_by_owner'), 'last_ai_owners': last.get('ai_owners'),
            'last_page': (summary['reports'][-1].get('page') if summary.get('reports') else None), 'seconds': seconds, 'out': str(out_dir)}


def write_tables(results, groups, out_root):
    """결과를 JSON 과 사람이 읽는 Markdown 표로 저장한다."""
    (out_root / 'batch-summary.json').write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
    lines = ['| 그룹 | 미션 | 결과 | 확정 프레임 | 종료 | AI 부착 | AI 진단 | 오류 로그 | 카메라(전투/의식) | 마지막 page | 걸린 시간(초) |', '| --- | --- | --- | ---: | --- | --- | ---: | ---: | --- | --- | ---: |']
    # 미션 순서대로 한 줄씩 쓴다.
    for result in results:
        cam = result.get('camera_moves') or {}
        lines.append(f"| {groups.get(result['mission'], '')} | {result['mission']} | {result['outcome']} | {result.get('outcome_frame')} | "
                     f"{'조기' if result.get('stopped_early') else ('시간초과' if result.get('timed_out') else result.get('exit_code'))} | {result.get('local_ai_attached')} | "
                     f"{result.get('ai_diagnostics')} | {result.get('error_logs')} | {cam.get('battle')}/{cam.get('ritual')} | {result.get('last_page')} | {result.get('seconds')} |")
    (out_root / 'batch-summary.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')


def main():
    """명령줄을 해석해 미션들을 일괄 실행한다."""
    parser = argparse.ArgumentParser(description='cpppj 캠페인 전 미션 아군 AI 자동 플레이 일괄 실행')
    parser.add_argument('--missions', default='', help='실행할 미션 키 쉼표 목록 (지정하면 --groups 보다 우선)')
    parser.add_argument('--groups', default=','.join(DEFAULT_GROUPS), help=f"실행할 그룹 쉼표 목록 (기본: 캠페인 1~3의 16개). 선택지: {', '.join(group for group, _ in CAMPAIGN_GROUPS)}")
    parser.add_argument('--out', default='', help='출력 폴더 (기본: extracted/cpp-campaign-batch/<시각>)')
    parser.add_argument('--workers', type=int, default=3, help='동시에 실행할 게임 수 (기본 3, 코어 수보다 작게)')
    parser.add_argument('--frames', type=int, default=48000, help='미션당 총 프레임 (고정 25ms 기준 게임 시간 1200초)')
    parser.add_argument('--dump-every', type=int, default=4000, help='덤프·보고 주기 (프레임)')
    parser.add_argument('--snapshot-every', type=int, default=0, help='화면 캡처 주기 (0이면 끔)')
    parser.add_argument('--ally-ai', default='hard')
    parser.add_argument('--enemy-ai', default='easy')
    parser.add_argument('--start-sp', type=int, default=None)
    parser.add_argument('--enemy-start-sp', type=int, default=None)
    parser.add_argument('--no-follow', action='store_true', help='관전 카메라 끄기')
    parser.add_argument('--timeout', type=int, default=2400, help='미션당 제한 시간 (초)')
    args = parser.parse_args()

    groups = {key: group for group, key in all_missions()}
    selected = {item.strip() for item in args.groups.split(',') if item.strip()}
    missions = [item.strip() for item in args.missions.split(',') if item.strip()] or [key for _, key in all_missions(selected)]
    out_root = Path(args.out) if args.out else ROOT / 'extracted/cpp-campaign-batch' / time.strftime('%Y%m%d%H%M%S')
    out_root.mkdir(parents=True, exist_ok=True)
    results = {}
    started = time.time()
    # 미션을 동시에 workers 개까지 실행하고 끝나는 대로 결과를 기록한다(중간에 끊겨도 batch-summary 가 남는다).
    with ThreadPoolExecutor(max_workers=max(1, args.workers)) as pool:
        futures = {pool.submit(run_one, mission, args, out_root): mission for mission in missions}
        for future in as_completed(futures):
            mission = futures[future]
            try:
                results[mission] = future.result()
            except Exception as error:  # 한 미션의 실행 실패가 전체를 막지 않게 기록만 하고 계속한다.
                results[mission] = {'mission': mission, 'outcome': 'tool-error', 'error': str(error)}
            ordered = [results[key] for key in missions if key in results]
            write_tables(ordered, groups, out_root)
            print(f"[{len(results)}/{len(missions)}] {mission}: {results[mission]['outcome']} ({results[mission].get('seconds')}초)", flush=True)
    print(f'완료: {round(time.time() - started)}초, 출력 {out_root}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
