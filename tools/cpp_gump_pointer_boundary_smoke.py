#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 미니맵 capture에서 창 밖 버튼 기록과 재진입 합성 release를 확인한다."""
import argparse
import json
from pathlib import Path

from cpp_bridge_ritual_runtime import save, sha
from cpp_game_menu_smoke import protected_run
from cpp_world_smoke import states


def main():
    """폴링만 변경한 구간에는 release를 큐에 주입하지 않고 실제 앞단이 예약하게 한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    run = out / 'scenario'  # 실제 프로세스와 보호 원본 해시·허용 설정 복원 자료를 보존한다.
    steps = [
        (10, 'click', 'Play Mission'), (20, 'report', 'initial'),
        (25, 'down', 'minimap:40,37'), (28, 'report', 'held'),
        (30, 'pointer', '0x04010000,40,768'), (32, 'report', 'outsidePressed'),
        (35, 'pointer', '0x84010000,40,768'), (37, 'report', 'releasedOutside'),
        (40, 'pointer', '0x80010000,40,730'), (42, 'report', 'reenteredReleased'),
        (42, 'snapshot', str(run / 'reentered-released.bmp')),
        (45, 'down', 'minimap:40,37'), (48, 'report', 'heldAgain'),
        (50, 'pointer', '0x04010000,40,768'),
        (54, 'pointer', '0x00010000,40,730'), (56, 'report', 'reenteredPressed'),
        (60, 'up', 'minimap:40,37'), (64, 'report', 'normalRelease'),
    ]
    report, result = protected_run(run, 1024, 768, steps, 72)
    observed = states(report)
    capture = {}  # 읽기 전용 보고서의 실제 Gump capture와 미니맵 scrolling을 함께 기록한다.
    for label in ('held', 'outsidePressed', 'releasedOutside', 'reenteredReleased',
                  'heldAgain', 'reenteredPressed', 'normalRelease'):
        row = observed[label]
        capture[label] = dict(token=int(row['RawButtons'][3]), scrolling=int(row['minimap'][7]))
    for label in ('held', 'outsidePressed', 'releasedOutside', 'heldAgain', 'reenteredPressed'):
        assert capture[label]['token'] != 0 and capture[label]['scrolling'] == 1, (label, capture[label])
    assert capture['held']['token'] == capture['outsidePressed']['token'] == capture['releasedOutside']['token'], capture
    for label in ('reenteredReleased', 'normalRelease'):
        assert capture[label] == dict(token=0, scrolling=0), (label, capture[label])
    assert capture['heldAgain']['token'] == capture['reenteredPressed']['token'], capture
    summary = dict(passed=True, originalsUnchanged=result['originalsUnchanged'],
                   binarySha256=result['binarySha256'], toolSha256=sha(Path(__file__)),
                   capture=capture, reentryReleaseQueuedByProduct=True,
                   limitations=['폴링 code/x/y와 실제 큐를 사용하는 Client 결합 검사이며 OS 자체의 마우스 메시지 생성은 범위 밖이다.',
                                '현재 기본 HUD의 미니맵이며 전체 dock 배치·전체 미션 완주를 판정하지 않는다.'])
    save(out / 'summary.json', summary)
    print(json.dumps(summary, ensure_ascii=False))


if __name__ == '__main__':  # 명시 CLI 실행에서만 클론 창을 연다.
    main()
