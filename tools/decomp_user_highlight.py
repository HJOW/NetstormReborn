#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본10.78 Highlight/ResetMouse의 현재 전역·선택 보존과 실제 효과 순서를 격리 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ESP

ENTRIES = (0x4d5480, 0x4d5bb0)  # 단순 마우스 TYPE 초기화와 전체 강조 부모의 실제 진입점이다.
GLOBALS = (0x5cae90, 0x5caea0, 0x5caea4, 0x5cae94, 0x5cae98)  # 강조/선택/마우스 TYPE/X/Y의 독립 현재 저장소다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/user-highlight-x86.tsv'  # 기존 커서 fixture를 수정하지 않는 새 기대값이다.
REPORT = ROOT / 'cpppj/user-highlight.json'  # 실제 실행 함수/명시 경계/ABI/입력 수와 근거 SHA다.


def cases():
    """모드 전체 DWORD와 원본 경계가 변경한 현재 전역을 독립 입력으로 만든다."""
    modes = (0, 1, 2, 3, 4, 5, 6, 0x7fffffff, 0x80000000, 0xffffffff)  # 부호 경계를 bool/작은 enum으로 줄이지 않는다.
    base = [1, 0, 5, 0x87654321, 0x12345678, 0xfffffffd, 47, 0, 0x80000000, 0x7fc01234]
    rows = []  # operation/oldMode/newMode/selected/mouseType/X/Y/mutation/projectX/projectY의10열이다.
    # 모든 같은 모드의 조기 반환과 기본/1..5 전환을 따로 관측한다.
    for old, new in itertools.product(modes, modes):
        row = base.copy();row[1:3] = old, new;rows.append(row)
    # ResetMouse 자체는 선택/모드/마우스 위치와 무관하게 TYPE 한DWORD만 쓴다.
    for mode, mouse_type in itertools.product(modes, (0, 1, 0xffffffff)):
        row = base.copy();row[0:2] = 0, mode;row[4] = mouse_type;rows.append(row)
    # 모드5는 선택0/전체DWORD와 원문 signed 좌표/두float의 비트를 그대로 전달한다.
    for selected, mouse, point in itertools.product((0, 1, 0xffffffff), ((0, 0), (0x80000000, 0x7fffffff)), ((0, 0), (0x80000000, 0x7fc01234), (0x7f800000, 0xff800000))):
        row = base.copy();row[3] = selected;row[5:7] = mouse;row[8:10] = point;rows.append(row)
    # 실제 커서/역투영/이동 효과 뒤 변경된 mode/selected/type/X/Y를 보존한다.
    for new, mutation in itertools.product(modes, (1, 2, 3)):
        row = base.copy();row[2] = new;row[7] = mutation;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """두 실제 몸체만 실행하고 커서 선택/좌표 역투영/Carrier 이동 강조는 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE와 기존 스택/canary/비휘발GP/x87 균형 검사를 그대로 재사용한다."""
        super().__init__();self.allowed = [];ranges(self, ENTRIES);self.instructions = 0;self.calls = collections.Counter()

    def mutation(self, expected):
        """명시 외부 효과가 바꾸는 전역만 기록하고 부모의 선택 분기는 대신 계산하지 않는다."""
        if self.case[7] == expected:
            # 모든 저장소를 바꿔 후속 읽기/스냅샷/반환 뒤 덮어쓰기 유무를 구분한다.
            for address, value in zip(GLOBALS, (7, 202, 77, 0xffffffdf, 65)):
                self.put(address, value)

    def instruction(self, mu, address, size, data):
        """실제 부모가 선택한 경계의 전체 인자와 그 시점의 TYPE 초기화를 기록한다."""
        self.instructions += 1;esp = mu.reg_read(UC_X86_REG_ESP);args = struct.unpack('<4I', mu.mem_read(esp + 4, 16))
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1
        if address == 0x4a4410:
            self.event('K', args[0], self.get(0x5caea4));self.mutation(1);self.ret();return
        if address == 0x4971e0:
            self.event('P', args[1], args[2], self.get(0x5caea4));self.mutation(2)
            mu.mem_write(args[0], struct.pack('<2I', *self.case[8:10]));self.ret(args[0]);return
        if address == 0x4284a0:
            self.event('M', *args);self.mutation(3);self.ret();return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'사용자 강조 범위 밖 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """두 실제 쓰기 전역과 격리 스택 이외의 원본 쓰기를 거부한다."""
        if any(low <= address and address + size <= high for low, high in ((0x5cae90, 0x5cae94), (0x5caea4, 0x5caea8), (STACK - 0x7f000, STACK + 0x1000))):
            return
        raise RuntimeError(f'사용자 강조 범위 밖 쓰기 {address:08x}+{size}')

    def sample(self, case, precision):
        """각 독립 입력을 실제 함수에 주고 원본 반환 뒤의 전역과 경계 순서만 기대값으로 남긴다."""
        self.case = case;self.events = []
        # 이전 실행의 경계 변경을 다음 입력으로 넘기지 않는다.
        for address, value in zip(GLOBALS, (case[1], case[3], case[4], case[5], case[6])):
            self.put(address, value)
        arguments = b'' if case[0] == 0 else struct.pack('<I', case[2])
        self.call(ENTRIES[case[0]], 0, arguments, 0, precision);self.returns += 1
        return ';'.join(self.events) or '-', ','.join(str(self.get(address)) for address in GLOBALS)


def main():
    """명시 요청에서 새 CPU 기대값을 만들거나 기존 SHA/고유 입력과 실행 경계를 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'));rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        # 실제 출력값을 재계산하지 않고 근거 바이트/고유 입력/기록 폭을 확인한다.
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        assert [row[0] for row in rows] == [','.join(map(str, case)) for case in cases()]
        assert len(rows) == report['uniqueInputs'] and report['normalReturns'] == len(rows) * 2
        assert not report['precisionDifferentInputs'] and report['controls'] == [0x27f, 0x37f]
        assert set(report['nativeCalls']) == {f'{entry:08x}' for entry in ENTRIES}
        for row in rows:
            assert len(row) == 3 and len(row[2].split(',')) == 5
        print('사용자 강조 근거 감사:', len(rows), '입력/', report['normalReturns'], '정상 반환');return
    oracles = [Oracle(), Oracle()];rows = []
    # 두 실제 x87 정밀도 출력을 대조하며 C++/Python으로 부모의 기대 분기를 만들지 않는다.
    for case in cases():
        outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert outputs[0] == outputs[1], case;rows.append((','.join(map(str, case)), *outputs[0]))
    FIXTURE.write_text('# input events globals\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_previews.py', 'tools/decomp_cursor_position.py', 'tools/decomp_user_highlight.py', FIXTURE.relative_to(ROOT).as_posix(), 'cpppj/src/client/RawUserHighlight.h', 'cpppj/src/client/RawUserHighlight.cpp', 'cpppj/tests/UserHighlightTests.cpp']
    report = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', lastDecompileDate='2026-10-16', date='2026-10-17', controls=[0x27f, 0x37f], entries=[f'{entry:08x}' for entry in ENTRIES], uniqueInputs=len(rows), normalReturns=sum(oracle.returns for oracle in oracles), instructionObservations=sum(oracle.instructions for oracle in oracles), precisionDifferentInputs=[], nativeCalls=dict(sum((oracle.calls for oracle in oracles), collections.Counter())), sha256={path: digest(ROOT / path) for path in paths}, limitations=['실제4d5bb0/4d5480 전체 몸체와 보존GP/스택/canary/x87 균형을 실행했다. 커서4a4410/역투영4971e0/이동 강조4284a0는 명시 경계다.', '실제 선택4d5c90/다중선택4d5490 및 Carrier 이동 강조의 내부 몸체/GUI/게임은 실행하지 않는다.', '10.78 전용이다. CD Highlight0041ad10은 정적 대응 기록이며 이번 신규 실행이 아니다.'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('사용자 강조:', len(rows), '입력/', report['normalReturns'], '정상 반환/정밀도 차이0')


if __name__ == '__main__':
    main()  # 명시 호출에서만 격리 CPU 실행 또는 근거 감사를 수행한다.
