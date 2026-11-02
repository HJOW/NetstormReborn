#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""비멀티일반명령5이름을실제명령표/토큰/큐에서격리관측한다."""
import argparse
import csv
import hashlib
import itertools
import json
from pathlib import Path
import socket
import decomp_script_command as base
from decomp_cursor_position import ranges


class Oracle(base.Oracle):
    """옛부분export없이 현호스트 전체정밀함수표의 동일몸체/CRT만 허용한다."""

    def __init__(self):
        """기존 명령관측기의 장치/수명을 유지하고 실제 함수범위를 전체표에서 선택한다."""
        base.MemoryOracle.__init__(self)
        self.allowed = []
        functions = json.loads((base.ROOT/'tools/ghidra/script-command-functions.json').read_text('utf-8'))['editions']['originals']
        ranges(self, tuple(int(entry, 16) for entry in functions)+(0x4e63e0, 0x4e6fa1, 0x4e6656, 0x4e65ed, 0x4ef510))
        self.put(base.THREAD+0x64, self.words(0x543efc, 1)[0])
        self.reset_ring()
        self.allocations = 0


def main():
    """48개독립이름/인자에서실제예약과문자열고리를새fixture에만고정한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists()
    oracle, lines = Oracle(), ['# operation source delimiter existed value next queue ring events']
    names = tuple(value for name in ('TakeScreenShot', 'URLAlias', 'Run', 'LookAtSid') for value in (name, name.lower(), name.upper()))+('MissingUtilityCommand',)  #MP 전용 MoveIsland는 분류 후의 신규 관측에서 제외한다.
    # 대소문자와 숫자/문자열 인자 조합을 실제 이름조회·큐 파서에 공급한다.
    for name, argument in itertools.product(names, ('0', '3', 'section')):
        lines.append('\t'.join(oracle.run('Command', (name+','+argument+'>').encode('ascii'))))
    args.fixture.write_text('\n'.join(lines)+'\n', encoding='utf-8', newline='\n')
    record = dict(unit=760, host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'), inputs=len(lines)-1,
                  nativeLookup='004d52c0', nativeQueueParser='004c9090', nativeTable='00542b90',
                  commands=['TakeScreenShot 00542768/0200001c', 'URLAlias 00542860/0200005a', 'Run 00542a5c/020000da', 'MoveIsland 00542714/02000006', 'LookAtSid 0054299c/020000aa'],
                  gameExecutions=0, multiplayerTests=0, nativeInstructions=oracle.instructions,
                  fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(), toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  limitations=['명령조회/토큰/실제큐만실행하며GUI몸체는별도관측이다', '실제대본태그호출/기본메뉴노출을확인한것은아니다'])
    args.report.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(inputs=len(lines)-1, fixtureSha256=record['fixtureSha256'])))


if __name__ == '__main__':
    main()
