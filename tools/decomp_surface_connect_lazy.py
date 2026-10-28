#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본441e40의첫연결마스크가0인경우두번째방향표를생략하는반환을관측한다."""
import argparse
import hashlib
import json
import socket
import struct
from pathlib import Path
from decomp_surface_oracle import SurfaceOracle

ROOT = Path(__file__).resolve().parents[1]  # 보호PE와기존격리실행기의기준루트다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/surface-connect-lazy-x86.tsv'  # 기존3016연결기대값과별도로보관하는새128입력이다.
REPORT = ROOT / 'cpppj/surface-connect-lazy-native-20261028.json'  # 최초원문반환/출처를보존하는문서다.


def digest(path):
    """읽은파일전체바이트의확정해시다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def observe():
    """지도/게임/OS없이검토된원본연결부모만실행하며첫마스크로입력을선택한다."""
    oracle = SurfaceOracle('originals')
    rows = []  # 출력은기계어실제반환값이며C++결과를입력으로쓰지않는다.
    for code in range(ord('A'), ord('P') + 1):  # 첫표면코드는원래정상16문자다.
        mask = struct.unpack('<H', oracle.mu.mem_read(0x532502 + code * 2, 2))[0]
        for direction in (0, 2, 4, 6):  # 실제표면finder의네직각방향이다.
            bit = struct.unpack('<I', oracle.mu.mem_read(0x52f8dc + direction * 4, 4))[0]
            if mask & bit:
                continue
            for unused in (0, 128, 204, 255):  # 첫조건이거부해마스크조회가필요없는두번째코드BYTE다.
                result = oracle.connect(code, code, 0x1000002, unused, unused, 0x1000002, direction)
                rows.append([code, code, 0x1000002, unused, unused, 0x1000002, direction, result])
    assert len(rows) == 128 and all(row[-1] == 0 for row in rows)
    return rows


def content(rows):
    """정수비트를손실없이저장하고원문기대값의UTF-8주석을붙인다."""
    return '# 원본441e40: 첫마스크0이면두번째방향표는미조회; 첫문자/변형/flags2/둘째문자/변형/flags2/방향/반환\n' + ''.join('\t'.join(map(str, row)) + '\n' for row in rows)


def main():
    """최초관측을새로저장하거나기존기대값과같은원문재실행을대조한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    rows = observe()
    text = content(rows)
    if args.verify:
        assert FIXTURE.read_text('utf-8') == text
        saved = json.loads(REPORT.read_text('utf-8'))
        for name, wanted in saved['sha256'].items():  # 과거도구/기대값/보호PE의핀도확인하며덮어쓰지않는다.
            assert digest(ROOT / name) == wanted, name
    else:
        assert not FIXTURE.exists() and not REPORT.exists()
        FIXTURE.write_text(text, 'utf-8', newline='\n')
        paths = (Path(__file__), FIXTURE, ROOT / 'originals/Netstorm.exe',
                 ROOT / 'tools/decomp_surface_oracle.py', ROOT / 'tools/decomp_bridge_oracle.py',
                 ROOT / 'tools/decomp_oracle.py', ROOT / 'extracted/refined/originals/functions.tsv')
        report = dict(host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23', outputsModified=False),
                      entry='00441e40', cases=len(rows), normalReturns=len(rows), gameExecutions=0,
                      sha256={path.relative_to(ROOT).as_posix(): digest(path) for path in paths},
                      limits=['첫마스크가0인표면/표면의직각연결이다. 두번째마스크가필요한미지원코드를허용하는증거가아니다.',
                              '기존코드/스택/ret/x87·쓰기검사를유지하며게임/창/OS호출과공중3종을실행하지않았다.'])
        REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print('원본 연결 단락128조건 일치')


if __name__ == '__main__':
    main()
