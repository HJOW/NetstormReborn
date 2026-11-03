#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""표면 이웃 탐색 중 지도/후보/원천을 바꾸고 두 원본의 다음 반환 순서를 관측한다."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from decomp_surface_oracle import SurfaceOracle, FINDER, POOL, MAP, ROOT, node

# 생성 직후 첫 반환 다음에 네 가지 현재값 변경을 넣는다. 여섯 번째 객체는 서쪽의 대체 후보다.
OBJECTS = [node(1,20,20), node(2,20,19), node(3,19,20), node(4,21,20), node(5,20,21), node(6,19,20)]


def observe(oracle, flags, action):
    """명시 입력 변경 외에는 실제 생성/Next/가상 필터 기계어가 이웃 목록을 결정한다."""
    oracle.setup(OBJECTS, (), [(19,20,3)])
    oracle.call('Find', [1, flags], FINDER, 8)
    first = struct.unpack('<I', oracle.mu.mem_read(FINDER + 0x34, 4))[0]
    assert first == 2, '원문 최초 북쪽 후보 입력 오류'
    if action == 1:
        oracle.mu.mem_write(MAP + (20*256 + 19)*2, struct.pack('<H', 6))
        oracle.mu.mem_write(MAP + (19*256 + 20)*2, struct.pack('<H', 5))
    elif action == 2:
        oracle.mu.mem_write(POOL + 3*oracle.squid_stride + 11, b'\x02')
        oracle.mu.mem_write(POOL + 4*oracle.squid_stride + (40 if oracle.edition == 'originals' else 35), b'\x01')
    elif action == 3:
        oracle.mu.mem_write(POOL + oracle.squid_stride + 14, struct.pack('<ff', 80, 80))
    result = [first]  # 첫 반환도 포함하여 생성과 Next의 실제 순서를 비교한다.
    # 물리 지도 셀보다 적은 정상 입력이며 무한 순회는 결과로 저장하지 않는다.
    for _ in range(20):
        oracle.call('Next', [], FINDER)
        current = struct.unpack('<I', oracle.mu.mem_read(FINDER + 0x34, 4))[0]
        if not current:
            return ','.join(map(str, result))
        result.append(current)
    raise RuntimeError('원문 표면 이웃이 제한 내에 종료하지 않음')


def main():
    """새 파일만 생성하거나 기존 관측을 읽기 대조하며 원본 파일은 읽기만 한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--fixture', type=Path, default=ROOT / 'cpppj/tests/fixtures/surface-neighbor-current-x86.tsv')
    parser.add_argument('--report', type=Path, default=ROOT / 'cpppj/surface-neighbor-current-native-20261103.json')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    rows, binaries = [], {}  # 실제 PE 출력과 원본 바이너리 해시다.
    # 두 판본에서 각 flags 조합의 동일한 현재값 변경을 실행한다.
    for edition in ('originals', 'originalCD'):
        oracle = SurfaceOracle(edition);binaries[edition] = oracle.sha256
        # flags8의 접합 생략/abstract 제외 조합과 네 가지 변경 입력을 열거한다.
        for flags in (8, 9, 12, 15):
            # 변경 종류마다 탐색기와 입력 공간을 새로 초기화한다.
            for action in range(4):
                rows.append(f'{edition}\t{flags}\t{action}\t{observe(oracle, flags, action)}')
    output = ('\n'.join(rows) + '\n').encode()
    if args.verify:
        assert args.fixture.read_bytes() == output, '표면 이웃 현재값 원문 재관측 불일치'
    else:
        if args.fixture.exists() or args.report.exists():
            raise FileExistsError('기존 산출물에는 --verify를 사용하십시오')
        args.fixture.write_bytes(output)
        args.report.write_text(json.dumps({'host': 'HJOW-Athlon', 'lastFullDecompile': 'HJOW-Athlon / 2026-11-03',
            'binaries': binaries, 'rows': len(rows), 'fixtureSha256': hashlib.sha256(output).hexdigest(),
            'boundary': '게임/GUI/OS 미실행·실제 flags8 생성/Next/필터·순회 사이 명시 입력 변경'}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'rows': len(rows), 'verify': args.verify}))


if __name__ == '__main__':
    main()
