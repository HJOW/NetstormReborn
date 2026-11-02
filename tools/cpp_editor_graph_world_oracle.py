#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 편집 월드의 같은 입력으로 원본 Graph 재구성을 격리 실행하고 전체 출력 바이트를 대조한다."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct

from decomp_graphrebuild_oracle import RebuildOracle, REBUILD
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL
from decomp_pop_oracle import HEADS, SPOTS
from decomp_rawgraph_oracle import TABLE, FLOOD
from decomp_oracle import ROOT

CODE_BASE = 0x1d000000  # 실제 프레임 코드의 격리 주소이며 원본/호스트 포인터와 구별한다.
CODE_BYTES = 0x100000  # 모든188개 TYPE의 현재 코드 배열을 담을 명시 상한이다.
BLOBS = ('pool.bin', 'spots.bin', 'heads.bin', 'records.bin', 'stack.bin')  # 전체 입력/출력을 부분 checksum으로 대신하지 않는다.
LENGTHS = (32768 * 50, 65536, 86272 * 2, 255 * 6, 4096 * 4)  # 현재 실제 저장소와 원본 Graph 물리 폭이다.


def digest(data):
    """관측/도구/PE의 실제 바이트만 식별한다."""
    return hashlib.sha256(data).hexdigest()


def load(directory):
    """잘린 풀/표와 잘못된 용량·분기를 실행 전에 거부한다."""
    blobs = {name: (directory / name).read_bytes() for name in BLOBS}
    for name, size in zip(BLOBS, LENGTHS):  # 기대 물리 크기는 파일 내용이나 현재 결과에서 계산하지 않는다.
        if len(blobs[name]) != size:
            raise ValueError('실제 월드 관측 크기 오류: ' + name)
    meta = dict(line.split('\t') for line in (directory / 'meta.tsv').read_text('utf-8').splitlines())
    if meta['capacity'] != '32768' or meta['reset'] not in ('0', '1'):
        raise ValueError('실제 월드 관측 용량/분기 오류')
    return blobs, meta


def input_world(oracle, directory, blobs):
    """현재 게임 계산을 호출하지 않고 관측한 풀/공간/Graph·타입 필드를 원본의 격리 입력으로 옮긴다."""
    for name, address in zip(BLOBS, (POOL, SPOTS, HEADS, TABLE, FLOOD)):
        oracle.mu.mem_write(address, blobs[name])
    oracle.mu.mem_write(TYPES, bytes(188 * 500))
    cursor = CODE_BASE
    with (directory / 'types.tsv').open(encoding='utf-8', newline='') as stream:
        rows = list(csv.DictReader(stream, delimiter='\t'))
    if [int(row['type']) for row in rows] != list(range(188)):
        raise ValueError('실제 Graph TYPE188개 관측 순서 오류')
    for row in rows:  # Graph가 읽는 실제 flags/발자국/프레임 수와 코드 배열만 원래 필드 위치에 놓는다.
        base = TYPES + int(row['type']) * 500
        code = bytes.fromhex(row['codes']) if row['codes'] != '-' else b''
        if len(code) % 4 or cursor + len(code) > CODE_BASE + CODE_BYTES:
            raise ValueError('실제 프레임 코드 관측 폭/공간 오류')
        for offset, value in ((0xe8, int(row['flags1'])), (0xec, int(row['flags2'])),
                              (0x1d4, int(row['footX'])), (0x1d8, int(row['footY'])),
                              (0x114, len(code) // 4), (0x124, cursor)):
            oracle.mu.mem_write(base + offset, struct.pack('<I', value & 0xffffffff))
        if code:
            oracle.mu.mem_write(cursor, code)
            cursor += len(code)
    oracle.mu.mem_write(oracle.delete[3], struct.pack('<I', 1))  # 기존 오라클과 같은 동결 시계 입력으로00462ac0이 OS를 호출하지 않고 반환하게 한다.


def compare_case(directory, control):
    """원본 명령의 정상 반환 뒤 전체 풀/표/스택과 읽기 전용 공간의 바이트 일치를 판정한다."""
    before, meta = load(directory / 'before')
    after, after_meta = load(directory / 'after')
    if meta != after_meta:
        raise ValueError('실제 Graph 앞뒤 관측 metadata 불일치')
    oracle = RebuildOracle('originals')
    oracle.mu.mem_map(CODE_BASE, CODE_BYTES)
    oracle.start_graph(control)
    input_world(oracle, directory / 'before', before)
    oracle.invoke(REBUILD['originals'], [int(meta['reset'])])
    if oracle.assertions:
        raise ValueError('원본 Graph assert 도달')
    observed = {name: bytes(oracle.mu.mem_read(address, len(before[name])))
                for name, address in zip(BLOBS, (POOL, SPOTS, HEADS, TABLE, FLOOD))}
    comparisons = {}
    for name in BLOBS:  # 첫 차이 위치와 전체 SHA를 모두 남기며 차이를 정상화하거나 삭제하지 않는다.
        left, right = observed[name], after[name]
        first = next((index for index, (a, b) in enumerate(zip(left, right)) if a != b), None)
        comparisons[name] = dict(bytes=len(left), matches=left == right,
                                 nativeSha256=digest(left), cppSha256=digest(right),
                                 firstDifference=first,
                                 nativeByte=None if first is None else left[first],
                                 cppByte=None if first is None else right[first])
        (directory / ('native-' + str(control) + '-' + name)).write_bytes(left)
    return dict(case=directory.name, reset=int(meta['reset']), control=control,
                originalExeSha256=oracle.sha256, assertions=oracle.assertions,
                instructions=oracle.instructions, calls=dict(oracle.rebuild_calls),
                inputSha256={name: digest(data) for name, data in before.items()},
                typesSha256=digest((directory / 'before/types.tsv').read_bytes()),
                outputs=comparisons, passed=all(row['matches'] for row in comparisons.values()))


def main():
    """명시 extracted 자료에서만 원본을 격리 실행하고 검토용 결과를 같은 단위 경로에 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    directory, output = args.input_root.resolve(), args.output.resolve()
    allowed = (ROOT / 'extracted').resolve()
    if not directory.is_relative_to(allowed) or not output.is_relative_to(allowed):
        raise ValueError('Graph 오라클 입력/출력은 extracted 내부만 허용')
    rows = []
    for name in ('incremental', 'reset', 'joined', 'owner', 'cut'):  # 두 섬과 다리 연결·소유자 변경·절단 뒤의 실제 입력을 두 x87 정밀도로 각각 실행한다.
        for control in (0x027f, 0x037f):
            result = compare_case(directory / name, control)
            rows.append(result)
            print(json.dumps(dict(case=name, control=control, passed=result['passed'],
                                  mismatches=[key for key, row in result['outputs'].items() if not row['matches']]), ensure_ascii=False), flush=True)
            output.write_text(json.dumps(dict(results=rows), ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    if not all(row['passed'] for row in rows):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
