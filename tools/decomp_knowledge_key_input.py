#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""F6 지식 키의 원문 공통 정지/수정키 검사와 일반 명령 예약을 격리 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_selection_key_input import Oracle


def main():
    """기존 fixture를 덮지 않고 실제 10.78 입력 분기의 독립 결과16개를 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists()
    oracle = Oracle()
    assert struct.unpack('<I', oracle.mu.mem_read(0x542974, 4))[0] == 0x020000a0
    rows = ['# input3 trace']  # 실제 코드/정지/owner 입력과 PE가 결정한 trace다.
    # 원문 Shift/Control/Alt의 모든 조합과 정지 여부를 독립적으로 공급한다.
    for modifiers, paused in itertools.product(range(8), (0, 1)):
        code = 0x750000 | ((modifiers & 1) * 0x20000000) | ((modifiers & 2) * 0x08000000) | ((modifiers & 4) * 0x02000000)
        rows.append('\t'.join(oracle.run((code, paused, 0))))
    args.fixture.write_text('\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    record = dict(unit=741, date='2026-11-02', host=socket.gethostname(),
                  lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'),
                  inputs=len(rows)-1, nativeRange='004d96b6..004d9bba', stopBefore='004d9bbb', stackDelta=0,
                  explicitBoundaries=['paused00460de0', 'queue00452fe0'], releaseGuardOutsideRange=True,
                  commandAddress='00542974', command=0x020000a0, gameExecutions=0,
                  peSha256=oracle.sha256,
                  fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
                  toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  sharedOracleSha256=hashlib.sha256(Path(__file__).with_name('decomp_selection_key_input.py').read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(record, ensure_ascii=False))


if __name__ == '__main__':
    main()
