#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론의 메뉴 전역 다리 스캔과 실제 Q→배치→자연 수명 쓰기를 별도 자료 사본에서 관측한다."""
import argparse
import re
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def run_case(directory, data, bridge):
    """상태를 직접 쓰지 않고 실제 입력 큐와 자연 프레임으로 현재 스캔/슬롯을 보고한다."""
    directory.mkdir(parents=True, exist_ok=False)
    frames = 1500 if bridge else 700  # 배치 뒤 최소12초를 확보하고 메뉴에서는 대여 없는 진행만 검사한다.
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK')]
    if bridge:
        steps.extend([(10, 'click', 'Play Mission'), (700, 'report', 'before'),
                      (710, 'key', 'q'), (711, 'keyup', 'q'), (714, 'key', 'right'), (715, 'keyup', 'right'),
                      (720, 'up', 'placement:110,97'),
                      (725, 'report', 'prepared'), (730, 'click', 'placement:110,97'),
                      (735, 'report', 'placed')])
        for frame in range(800, frames, 100):  # 현재 동일 SID의 실제 수명 변화를 중간에 빠뜨리지 않는다.
            steps.append((frame, 'report', str(frame)))
    else:
        steps.extend([(20, 'report', 'menu20'), (350, 'report', 'menu350')])
    steps.extend([(frames-1, 'report', 'final'), (frames-1, 'snapshot', str(directory/'final.bmp'))])
    steps.sort(key=lambda row: row[0])  # 같은 프레임의 원래 입력/보고 순서를 유지한다.
    plan = directory/'steps.tsv'  # 실패해도 정확한 입력을 남긴다.
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', str(frames),
               '--set', 'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=1',
               '--ui-script', str(plan), '--ui-report', str(directory/'states.tsv')]
    if bridge:
        command.extend(['--mission', 'thewarbegins', '--test-bridge-patterns', '0'])  # 1칸 패턴0만 고정하는 명시 검사 입력이며 원본 RNG 증거가 아니다.
    result = execute(command, directory, GAME, 360)
    record = dict(result=result, testOnlyBridgePatternOverride=bridge, passed=False)
    save(directory/'result.json', record)
    observed = states(directory/'states.tsv')
    fields = {}  # 반복 raw_bridge_fields를 SID별로 보존한다. 일반 TSV parser의 마지막 행으로 대체하지 않는다.
    for block in re.split(r'(?m)^frame\t', (directory/'states.tsv').read_text('utf-8'))[1:]:
        header, body = block.split('\n', 1)
        label = header.split('\t', 1)[1]
        fields[label] = {int(row[1]): [int(row[2]), int(row[3])] for line in body.splitlines()
                         if (row := line.split('\t'))[0] == 'raw_bridge_fields'}
    final = observed['final']
    checks = dict(normalExit=result['returnCode'] == 0 and not result['timedOut'])
    scans = [state['bridge_scan_global'] for state in observed.values()]
    checks['scanProgress'] = (len({row[2] for row in scans}) > 1 or len({row[3] for row in scans}) > 1) and any(float(row[3]) > 0 for row in scans)  # 초기 BSS의 빈 범위도 원문 기한 갱신으로 확인한다.
    if bridge:
        before, prepared, placed = (observed[label] for label in ('before', 'prepared', 'placed'))
        expected = ['82', '1', '41', '110', '97']  # 원본 패턴0 J1을 우회전한 실제 K1 프레임/소유자/격자 위치다.
        candidates = [sid for sid, row in placed['raw_objects'].items() if row == expected and sid not in before['raw_objects']]
        checks.update(playing=final.get('phase') == ['4'] and final.get('paused') == ['0'],
                      correctMission=final.get('mission', [''])[0].lower() == 'thewarbegins',
                      actualSession=final.get('raw_session') == ['1', '1'],
                      legalPreview=prepared['raw_cursor'][1] == '1' and prepared['raw_cursor'][4:6] == ['110', '97'],
                      uniquePlayerBridge=len(candidates) == 1,
                      noBridgeCost=before['raw_sp'][1] == placed['raw_sp'][1],
                      sharedWorldScan=all(state['bridge_scan_global'][:5] == state['bridge_decay'] for state in observed.values()))
        if len(candidates) == 1:
            sid = candidates[0]  # 검증한 소유자/좌표/신규 SID만 추적하며 AI 다리를 대체 증거로 삼지 않는다.
            life = [(label, (values[sid][0] & 0x78) >> 3, values[sid][1]) for label, values in fields.items() if sid in values]
            record.update(bridgeSid=sid, bridgeFields=life)
            checks['naturalLifeWrite'] = any(value[1] == 7 for value in life)  # 배치가 미리 수명을 쓰지 않고 실제 원문 스캔이 최초7을 기록해야 한다.
            checks['samePlayerBridgeSurvives'] = final['raw_objects'].get(sid, [])[0:2] == ['82', '1']
    else:
        checks.update(menuOnly=all(state.get('phase') == ['0'] for state in observed.values()),
                      noWorld=all(row[8:10] == ['0', '0'] for row in scans))
    record.update(checks=checks, scans=scans, passed=all(checks.values()))
    save(directory/'result.json', record)
    print(directory.name, record['passed'], checks, flush=True)
    return record


def main():
    """원본과 사본의 전후 해시를 별도로 검증하며 실패 결과도 새 경로에 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--game-dir', type=Path, required=True)
    args = parser.parse_args()
    directory = args.output.resolve();directory.mkdir(parents=True, exist_ok=False)
    data = args.game_dir.resolve()
    if data == (ROOT/'originals').resolve() or data == (ROOT/'originalCD').resolve():
        raise RuntimeError('이 검사는 보호 원본이 아닌 별도 자료 사본을 요구합니다')
    before = snapshots(data);original = snapshots(ROOT/'originals');records = []
    try:
        with preserve_game_settings(data, 'd', directory/'settings-backup'):
            for bridge in (False, True):  # 두 실제 창을 순차 실행해 활성 커서 경로를 유지한다.
                records.append(run_case(directory/('bridge' if bridge else 'menu'), data, bridge))
    finally:
        unchanged = before == snapshots(data);protected = original == snapshots(ROOT/'originals')
        save(directory/'results.json', dict(runs=records, gameDataUnchanged=unchanged, originalsUnchanged=protected,
             scope='메뉴/단일 고정 패턴의 입력·스캔 연결이며 사용자 완주·전체 동일성·음향 청취 검사가 아니다.'))
    if not unchanged or not protected or len(records) != 2 or not all(row['passed'] for row in records):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
