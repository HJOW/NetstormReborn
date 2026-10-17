#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의83VT 선택 비트/표시 순서·사정거리 종류·491 생성 입구를 격리 관찰한다."""
import argparse
import collections
import itertools
import json
import re
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, digest
from decomp_cursor_position import ranges
import pefile
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

POOL, PROCESS = 0x13000000, 0x13400000  # 보호 PE와 분리된 물리 객체/할당 결과 입력이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/squid-selection-x86.tsv'  # 기존 선택/표시 fixture와 분리한다.
REPORT = ROOT / 'cpppj/squid-selection.json'  # 원본 관찰 및 VT 범위와 SHA의 근거다.
TABLE = ROOT / 'cpppj/src/o/SelectionVtables.inc'  # C++가 비교용으로 읽는83VT의 실제 두 슬롯이다.


def tables():
    """기존 생성자 목록과 보호 PE에서 실제+7c/+88 DWORD만 읽는다."""
    source = (ROOT / 'cpppj/src/o/DerivedConstructors.inc').read_text('utf-8').split('kCdRecipes')[0]
    values = {int(v, 16) for _, v in re.findall(r'\{(0x[0-9a-f]+)U?,(0x[0-9a-f]+)U?,', source, re.I)} | {0x501dd0}
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    return [(v, *(struct.unpack('<I', pe.get_data(v + offset - 0x400000, 4))[0] for offset in (0x7c, 0x88))) for v in sorted(values)]


def cases():
    """83개 VT·전체 extra BYTE·하위 인자·callback 변경과 종류/로딩/생성 실패 입력이다."""
    rows = [];entries = tables();damage = next(row[0] for row in entries if row[1] == 0x44b2b0)
    # op,vtable,extra,value,mutation,group,flags2,type,loading,allocation이다.
    base = [0, entries[0][0], 0, 0, 0, 0, 0, 158, 0, 1]
    # 모든 원본 생성 VT를 실제 코드에 직접 넣는다. 부모 함수의 경로는 계산하지 않는다.
    for index, extra, value, mutation in itertools.product(range(len(entries)), (0, 255), (0, 1), (0, 1)):
        row = base.copy();row[1:5] = entries[index][0], extra, value, mutation;rows.append(row)
    # Damageable은 표시가 바꾼 현재 extra/VT를 보존하고 bit2만 바꿔야 한다.
    for extra, value, mutation in itertools.product(range(256), (0, 1, 2, 3, 0x100, 0xffffffff), (0, 2, 3)):
        row = base.copy();row[1:5] = damage, extra, value, mutation;rows.append(row)
    # 사정거리 종류는 group 네 값과 bomb 비트이며 이동 가능 여부가 아니다.
    for group, flags in itertools.product((-0x80000000, -1, *range(10), 0x7fffffff), (0, 0x100, 0x300, 0xffffffff)):
        row = base.copy();row[0] = 1;row[5:7] = group & 0xffffffff, flags;rows.append(row)
    # 491 생성 입구의 로딩/추상·들린상태와 진단 후 현재TYPE/할당 실패를 교차한다.
    for kind, extra, loading, allocation, mutation in itertools.product((0, 3, 10, 69, 70, 158, 255), (0, 1, 4, 8, 9), (0, 1), (0, 1), (0, 4)):
        row = base.copy();row[0] = 2;row[2] = extra;row[4] = mutation;row[7:10] = kind, loading, allocation;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """두+7c/474240/426080은 전체 실행하고 표시·할당/Regular ctor·진단만 명시 경계다."""
    def __init__(self):
        """정밀 표 누락41c0b0은 실제 명령의 ret4까지22바이트만 허용한다."""
        super().__init__();self.mu.mem_map(POOL, 0x400000);self.mu.mem_map(PROCESS, 0x1000);self.entries = tables()
        self.allowed = [(0x41c0b0, 0x41c0c6)];ranges(self, (0x44b2b0, 0x474240, 0x426080));self.instructions = 0;self.calls = collections.Counter();self.refresh = {row[2] for row in self.entries}

    def instruction(self, mu, address, size, data):
        """원본이 선택한 현재 표시 시점/입자/인자·진단을 기록한다."""
        self.instructions += 1;esp = mu.reg_read(UC_X86_REG_ESP);args = struct.unpack('<3I', mu.mem_read(esp + 4, 12));this = mu.reg_read(UC_X86_REG_ECX)
        if address in (0x41c0b0, 0x44b2b0, 0x474240, 0x426080):
            self.calls[f'{address:08x}'] += 1
        if address in self.refresh:
            self.event('R', (this - POOL) // 50, self.get(this + 40) & 255, self.get(this))
            if self.case[4] in (1, 2, 3):
                self.put(this + 40, {1:0xa5, 2:0x12, 3:0xff}[self.case[4]], 1)
            if self.case[4] == 2:
                self.put(this, 0xdeadbeef)
            self.ret();return
        if address == 0x49a840:
            self.ret(TYPES + args[0] * 500);return
        if address == 0x4e0620:
            self.event('!', self.string(args[0]), args[2])
            if self.case[4] == 4:
                self.put(POOL + 50 * 50 + 10, 158, 1)
            self.ret();return
        if address == 0x4e4391:
            self.event('A', args[0]);self.ret(PROCESS if self.case[9] else 0);return
        if address == 0x496e80:
            self.event('C', *args);self.ret(PROCESS, 12);return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'선택 효과 범위밖 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """원본 선택은 extra1바이트, SEH 래퍼는 격리FS0/스택에만 쓴다."""
        if STACK - 0x7f000 <= address <= STACK + 0x1000 - size or address == 0 and size == 4 or address == POOL + 50 * 50 + 40 and size == 1:
            return
        raise RuntimeError(f'선택 효과 범위밖 쓰기 {address:08x}+{size}')

    def sample(self, case, precision):
        """전체 부모 대신 독립 네 몸체의 정상 반환/현재 extra/VT/TYPE만 읽는다."""
        self.case = case;self.events = [];vtable, entry, _ = next(row for row in self.entries if row[0] == case[1]);raw = bytearray(50);struct.pack_into('<I', raw, 0, vtable);raw[10] = case[7];raw[11] = 0xff;raw[40] = case[2]
        self.mu.mem_write(POOL + 50 * 50, bytes(raw));self.put(0x5c8464, POOL);self.put(0x5c847c, 24000);self.put(0x5c89b8, case[8]);self.put(TYPES + 158 * 500 + 0x9c, case[5]);self.put(TYPES + 158 * 500 + 0xec, case[6]);self.put(0, 0x87654321)
        op = case[0];address = entry if op == 0 else 0x474240 if op == 1 else 0x426080
        args = struct.pack('<I', case[3] if op == 0 else 158 if op == 1 else 50)
        self.call(address, POOL + 50 * 50 if op == 0 else 0, args, 4 if op == 0 else 0, precision);self.returns += 1
        assert self.get(0) == 0x87654321
        current = [self.get(POOL + 50 * 50), self.get(POOL + 50 * 50 + 40) & 255, self.get(POOL + 50 * 50 + 10) & 255]
        return str(self.mu.reg_read(UC_X86_REG_EAX) if op == 1 else 0), ';'.join(self.events) or '-', ','.join(map(str, current))


def main():
    """새 독립 관찰을 기록하거나 기존 입력/출력/SHA만 변경 없이 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'));rows = [row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')]
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        assert report['vtables'] == [list(row) for row in tables()] and len(tables()) == 83
        assert [row[0] for row in rows] == [','.join(map(str, row)) for row in cases()]
        assert len(rows) == report['uniqueInputs'] and report['normalReturns'] == len(rows) * 2 and not report['precisionDifferentInputs']
        print('객체 선택 효과 근거 감사:', len(rows), '입력/', report['normalReturns'], '정상 반환');return
    oracles = [Oracle(), Oracle()];rows = []
    # 실제 PE만 두 정밀도로 실행한다. 제품 C++를 기대값 계산에 사용하지 않는다.
    for case in cases():
        outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert outputs[0] == outputs[1], case;rows.append((','.join(map(str, case)), *outputs[0]))
    FIXTURE.write_text('# input result events current\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    entries = tables();TABLE.write_text('// 보호10.78 생성자83VT의 실제+7c/+88이다. 호스트 함수 포인터가 아니다.\nconstexpr std::array<SelectionVtable,83> kSelectionVtables{{\n' + ''.join('    {' + ','.join(f'0x{x:08x}U' for x in row) + '},\n' for row in entries) + '}};\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'cpppj/src/o/DerivedConstructors.inc', 'tools/decomp_squid_selection.py', 'cpppj/tests/fixtures/squid-selection-x86.tsv', 'cpppj/src/o/SelectionVtables.inc')
    report = dict(host=socket.gethostname(), date='2026-10-17', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-16'), vtables=entries, controls=[0x27f, 0x37f], uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles), nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())), precisionDifferentInputs=[], boundaries='표시+88/할당/Regular496e80/진단은명시입력. 선택bit/사정거리종류/491생성입구전체실행. 원본/OS창실행없음.', sha256={p:digest(ROOT / p) for p in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
