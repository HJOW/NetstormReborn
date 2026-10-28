#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 판본의 실제 TYPE 전체 후처리에서 생략/0 비용의 기본 가격과 제외 규칙을 동결한다."""
import itertools
import json
import socket
import struct

from decomp_archertype_oracle import ArcherTypeOracle, ROOT, TYPES, float_bits, digest

# 기존 fixture는 덮어쓰지 않고 이번 비용 관찰만 별도로 보존한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/type-default-cost-x86.tsv'
REPORT = ROOT / 'cpppj/type-default-cost-native-20261028.json'


def main():
    """현재 가격은 계산하지 않고 실제 PE가 저장한 float32 비트만 읽는다."""
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 독립 관측을 덮어쓰지 않는다'
    rows, editions, files = [], {}, set()
    cases = list(itertools.product((71, 133, 158), (0, 3, 7, 8, 10),
                                  (0, 0x10000, 0x30000, 0x400, 0x200000),
                                  (-2, -1, 0, 1, 2), (0, 0x80000000, float_bits(500), float_bits(-50))))
    for edition in ('originals', 'originalCD'):  # 각 PE의 다른 함수/상수·사제 전역을 실제로 사용한다.
        oracle = ArcherTypeOracle(edition)
        oracle.initialize()
        oracle.run('-', 0)
        stride = oracle.type_stride  # PATCH500/CD468의 실제 TYPE 간격을 구별한다.
        prototype = bytes(oracle.mu.mem_read(TYPES + 71 * stride, stride))
        before = oracle.normal
        for number, group, flags, level, cost in cases:  # 경계 입력은 동일한 유효35프레임 TYPE에 명시 공급한다.
            body = bytearray(prototype)
            struct.pack_into('<i', body, 0x94, level)
            struct.pack_into('<i', body, 0x9c, group)
            struct.pack_into('<I', body, 0xec, flags)
            struct.pack_into('<I', body, 0xc4, cost)
            body[0xa0:0xa2] = b's\0'
            target = TYPES + number * stride
            oracle.mu.mem_write(target, bytes(body))
            oracle.invoke(oracle.m['post'], this=target)
            result = int.from_bytes(oracle.mu.mem_read(target + 0xc4, 4), 'little')
            rows.append((edition, number, group, flags, level, cost, result))
        assert oracle.normal - before == len(cases)
        editions[edition] = dict(entry=f"{oracle.m['post']:08x}", inputs=len(cases),
                                 normalReturns=oracle.normal - before, boundaries=dict(oracle.boundaries),
                                 nativeCalls=dict(oracle.native_calls), assertions=oracle.assertions)
        files.update(oracle.exports)
        files.update((ROOT / oracle.spec['binary'], oracle.shapeFile, ROOT / edition / 'netstorm.tarc'))
    FIXTURE.write_text('# edition TYPE group flags2 level costBeforeBits costAfterBits\n' +
                       '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    files.update((FIXTURE, ROOT / 'tools/decomp_type_default_cost.py', ROOT / 'tools/decomp_archertype_oracle.py',
                  ROOT / 'tools/decomp_owner_oracle.py'))
    facts = dict(host=socket.gethostname(), lastFullDecompileHost='HJOW-Athlon', lastFullDecompileDate='2026-10-23',
                 observations=len(rows), editions=editions, gameExecuted=False, osCalls=0,
                 boundaries='초기화 접두/보호35클러스터 callback과 전체 후처리는 실제 PE다. 비용 입력은 명시 raw 속성으로 주입하며 파일/힙/CRT 스레드만 기존 실행기 경계다. 전체 게임 실행은 아니다.',
                 sha256={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(files)})
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('원본 TYPE 기본 비용:', len(rows), '입력')


if __name__ == '__main__':
    main()
