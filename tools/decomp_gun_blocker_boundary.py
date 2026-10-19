#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 원본690행을 유지하며 장애물의 발사기 최대 반경 경계를 별도 PE 입력으로 고정한다."""
import argparse
import itertools
import json
import socket
from decomp_gun_blocker import Oracle, KEYS, ROOT, FPU_CONTROLS, cases, bits, sha

FIXTURE = ROOT / 'cpppj/tests/fixtures/gun-blocker-boundary-x86.tsv'  # 기존690행과 분리한6개 경계 입력이다.
REPORT = ROOT / 'cpppj/gun-blocker-boundary-native.json'  # 실제 float 반경의 포함/제외 관측 출처다.


def main():
    """제곱거리400의 바로 아래/동일/바로 위 입력을 두 정밀도로 실행하거나 저장 해시를 검증한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 기존 수집기와 새 경계 입력의 출처를 모두 그대로 대조한다.
        for name, expected in report['sources'].items():
            assert sha(ROOT/name) == expected, name
        print('gun blocker native radius boundary verified');return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 관측을 덮어쓰지 않는다.'
    oracle = Oracle();rows = [];outputs = []  # 기대값은 실제 PE 반환값만 저장한다.
    # 현재 발사기 중심48.5/44.5와 새 사각형 중심의 거리를20 전후로 교차한다.
    for kind, point_y in itertools.product((0, 1), (65.375, 65.5, 65.625)):
        case = cases()[0].copy();case.update(kind=kind, group=0, x=bits(49.5), y=bits(point_y))
        result = [oracle.run(case, control) for control in FPU_CONTROLS]
        assert result[0] == result[1]
        outputs.append(result[0]);rows.append('\t'.join([','.join(str(case[key]) for key in KEYS), *result[0]]))
    FIXTURE.write_text('# 독립 원본 최대 반경20의 아래/동일/위 수집·통지. 기존690행은 유지한다.\n'+'\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    names = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_gun_blocker.py',
             'tools/decomp_gun_blocker_boundary.py', FIXTURE.relative_to(ROOT).as_posix())
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(), lastDecompileHost='VM-W11-Codex', lastDecompileDate='2026-10-19',
        uniqueInputs=len(rows), normalReturns=oracle.returns, precisionDifferences=0, outputs=outputs,
        sources={name:sha(ROOT/name) for name in names}, limitations=['독립 좌표 입력이며 실제 게임/OS 실행이 아니다. 기존 Form/Kernel 경계는 본 수집기 관측과 같다.']),
        ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(outputs))


if __name__ == '__main__':
    main()
