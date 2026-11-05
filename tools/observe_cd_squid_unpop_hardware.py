#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CD 공간 해제의 현재 진단/부분 쓰기만 실제 x86 CPU로 관측한다. 게임·GUI·통신은 시작하지 않는다."""
import argparse
import collections
import csv
import hashlib
import itertools
import json
import socket
import subprocess
from pathlib import Path
from observe_squid_unpop_hardware import inputs

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 최초 관측의 저장소 기준이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/cd-squid-unpop-current-hardware-x86.tsv'  # 기존 관측과 다른 새 파일에만 동결한다.


def matrix():
    """기존 명시 입력에 self/범위 밖 next·비균일 지도를 더하며 결과는 계산하지 않는다."""
    rows = inputs()
    for layout, genus, mutation, flags in itertools.product((5, 6, 7), (0, 4, 0x202), (0, 3, 7), (0, 0x2000)):  # 실제 해시 쓰기 뒤의 진단/현재 상태를 구별한다.
        values = [0, 50, 70, 0, 0, 0x41820000, 0x41a40000, 3, 3, genus, 0, layout, flags, mutation, 0x35, 1]
        rows.append([str(len(rows)), *map(str, values)])
    for genus, foot, layout, mutation in itertools.product((1, 4, 6, 0x202), (1, 3), (1, 2), (0, 4, 7)):  # 셀마다 다른 초기 byte로 지도 읽기/쓰기 위치를 검사한다.
        values = [0, 50, 70, 0, 0, 0x41820000, 0x41a40000, foot, foot, genus, 0x800 if genus == 1 else 0, layout, 0x2000, mutation, 0x135, 1]
        rows.append([str(len(rows)), *map(str, values)])
    return rows


def main():
    """별도 창 없는 순수 관측기만 실행하고 최초 바이트 또는 기존 파일 일치를 확인한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if not args.verify and FIXTURE.exists():
        raise FileExistsError('기존 관측은 덮어쓰지 않습니다')
    rows = matrix()
    binary = ROOT / 'originalCD/NETSTORM.EXE'
    run = subprocess.run([str(args.probe.resolve()), str(binary)],
                         input=''.join(' '.join(row)+'\n' for row in rows), text=True, encoding='utf-8',
                         capture_output=True, timeout=60, creationflags=subprocess.CREATE_NO_WINDOW)
    if run.returncode:
        raise RuntimeError(f'CD 순수 CPU 관측 실패{run.returncode}: {run.stderr}')
    observed = list(csv.reader(run.stdout.splitlines(), delimiter='\t'))
    assert len(observed) == len(rows)
    for original, row in zip(rows, observed):  # 기대값을 계산하지 않고 원래 입력/출력 필드 수만 확인한다.
        assert len(row) == 29 and row[:17] == original
    data = ('\n'.join('\t'.join(row) for row in observed)+'\n').encode('utf-8')
    if args.verify:
        assert FIXTURE.read_bytes() == data
    else:
        with FIXTURE.open('xb') as stream:
            stream.write(data)
    counts = collections.Counter()
    for row in observed:  # 실제 발생한 원문 진단만 집계한다.
        for event in row[-1].split(';'):
            if event.startswith('A,'):
                counts[event.split(',', 1)[1].split(':', 1)[0]] += 1
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()  # 관측에 사용한 실제 바이트만 식별한다.
    report = dict(host=socket.gethostname(), lastFullDecompile='HJOW-Athlon / 2026-11-03',
                  backend='Windows x86 CPU / CD004ad0b0와 실제 TYPE·EffectiveGenus·HashBucket·CRT·공간 쓰기',
                  mode='verify' if args.verify else 'first', rows=len(observed), diagnosticCallsByLine=dict(counts),
                  fixtureSha256=digest(FIXTURE), binarySha256=digest(binary), probeSha256=digest(args.probe),
                  probeSourceSha256=digest(ROOT/'tools/native_cd_squid_unpop_probe.cpp'),
                  sharedSourceSha256=digest(ROOT/'tools/native_board_angle_probe.cpp'),
                  driverSha256=digest(Path(__file__)), actualChildren=False, game=False, multiplayer=False,
                  boundary='미니맵·옆면 통지·가상 표시·진단·종속 분리/UI는 명시 장치이며 실제 자식 실행을 주장하지 않음')
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'mode': report['mode'], 'rows': len(observed), 'diagnostics': dict(counts), 'sha256': digest(FIXTURE)}), flush=True)


if __name__ == '__main__':
    main()
