#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 Carrier42a750 전체와 좌표 제한/TYPE getter의 실제 결과를 관찰한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import pefile
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, digest
from decomp_cursor_position import ranges
from decomp_squid_selection import tables
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

POOL = 0x13900000  # 기존 PE/TYPE와 구별되는 부모/내용물/미리보기 raw 입력이다.
OWNER, POP, OTHER_POP, REFRESH = (STOP + offset for offset in (0x300, 0x320, 0x340, 0x360))  # 현재 가상 호출을 기록하는 서로 다른 경계다.
CALLER, RESULT, FLOAT_VALUE, FLOAT12, FLOAT4 = (STOP + offset for offset in (0x500, 0x580, 0x590, 0x600, 0x620))  # 실제 float10→caller float32와 하위 float 반환 스텁이다.
PARENT_VT, GHOST_VT, OTHER_VT = STOP + 0x800, STOP + 0x900, STOP + 0xa00  # 호스트 포인터 대신 격리한 현재 가상 표다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/carrier-regular-x86.tsv'  # 기존 표시 자료와 분리한 새 부모 관찰이다.
VT_FIXTURE = ROOT / 'cpppj/tests/fixtures/carrier-regular-vtables.tsv'  # 보호 PE에서 직접 읽은83VT의+5c다.
REPORT = ROOT / 'cpppj/carrier-regular.json'  # 두 정밀도/ABI/입력·보호 파일의 SHA다.
WATCH = (0, 50, 80, 81, 90, 91)  # 실제 부모와 두 내용물/두 생성 결과의50바이트 관측 범위다.


def bits(value):
    """명시 단정도 입력만 원문 비트로 만든다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def vtables():
    """기존 생성자 목록의 실제+5c DWORD를 보호 PE에서 직접 읽는다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    return [(row[0], struct.unpack('<I', pe.get_data(row[0] + 0x5c - 0x400000, 4))[0]) for row in tables()]


def cases():
    """교대/만료/현재 재조회/casttime 감김/회피/낙하/fallback 입력을 만든다."""
    base = (0x491, 0, bits(0.2), 3, 0x100, 0x200000, 3, 0, 1, 1, 0, 54, 0, 0, bits(0.2), bits(-1))
    rows = []
    # event,count,payload,contentsMask,flags0,flags1,casttime,positionVariant,allocate,boss,terrain,movementType,path,mutation,damageReturn,fallReturn다.
    for count, payload, present in itertools.product((0, 1, 2, 7, 8, 10, 11, 0x80000000, 0x7fffffff, 0xffffffff), (0, 0x80000000, bits(0.2), bits(-0.2), bits(1.5), 1, 0x7f800000, 0x7fc01234), range(4)):
        row = list(base);row[1:4] = count, payload, present;rows.append(tuple(row))
    # 외부 효과가 현재 raw를 바꾸는 경우와 TYPE 필터·할당 실패·좌표 상하한을 구별한다.
    for count, mutation, variant, allocate in itertools.product((0, 1, 2, 3), range(8), range(4), (0, 1)):
        row = list(base);row[1] = count;row[7:9] = variant, allocate;row[13] = mutation;rows.append(tuple(row))
    for flag0, flag1, present in itertools.product((0, 1, 0x100, 0x8000, 0x200000, 0x208100), (0, 1, 0x100, 0x200000), range(4)):
        row = list(base);row[3:6] = present, flag0, flag1;rows.append(tuple(row))
    #493은 form0의 WORDkind도 읽으며 casttime*10의 signed DWORD 감김을 보존한다.
    for count, cast, present in itertools.product((0, 1, 29, 30, 31, 0x7fffffff, 0x80000000, 0xffffffff), (0, 1, 3, 0xffffffff, 0x40000000, 0x7fffffff), (0, 2)):
        row = list(base);row[0:2] = 0x493, count;row[3] = present;row[6] = cast;rows.append(tuple(row))
    for boss, terrain, path, mutation in itertools.product((0, 1), (0, 1), (0, 1), (0, 5)):
        row = list(base);row[0] = 0x492;row[9:11] = boss, terrain;row[12:14] = path, mutation;rows.append(tuple(row))
    for event, count, returned in itertools.product((0, 0x494, 0x5105, 0x80000000, 0xffffffff), (0, 100, 0x80000000), (0, bits(-1), bits(3), 0x7fc01234)):
        row = list(base);row[0:2] = event, count;row[14:16] = returned, returned;rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """42a750/41d0d0/실제TYPE getter를 전체 실행하며 큰 생성/이동/가상 효과만 기록한다."""
    def __init__(self):
        """실제 호출자 fstpfloat32와 명시 하위 float 반환 스텁을 격리한다."""
        super().__init__();self.mu.mem_map(POOL, 0x10000);self.allowed = [];ranges(self, (0x42a750, 0x41d0d0, 0x49a840, 0x4ac550));self.instructions = 0;self.calls = collections.Counter()
        self.mu.mem_write(CALLER, b'\xd9\x1d' + struct.pack('<I', RESULT) + b'\xe9' + struct.pack('<i', STOP - (CALLER + 11)))
        # fld/retN은 실제 FPU 스택으로 반환값을 전달한다. ST0 레지스터를 Python 숫자로 합성하지 않는다.
        for address, purge in ((FLOAT12, 12), (FLOAT4, 4)):
            self.mu.mem_write(address, b'\xd9\x05' + struct.pack('<I', FLOAT_VALUE) + b'\xc2' + struct.pack('<H', purge))
        self.put(PARENT_VT + 0x88, REFRESH);self.put(GHOST_VT + 0x74, OWNER);self.put(GHOST_VT + 0x90, POP);self.put(OTHER_VT + 0x74, OWNER);self.put(OTHER_VT + 0x90, OTHER_POP);self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def instruction(self, mu, address, size, data):
        """원본이 선택한 현재 인자/가상 표만 관찰하며 표시 루프 분기는 대체하지 않는다."""
        self.instructions += 1;esp = mu.reg_read(UC_X86_REG_ESP);args = struct.unpack('<4I', mu.mem_read(esp + 4, 16));this = mu.reg_read(UC_X86_REG_ECX);sid = (this - POOL) // 50
        if address in (0x42a750, 0x41d0d0, 0x49a840, 0x4ac550):
            self.calls[f'{address:08x}'] += 1
        if CALLER <= address < CALLER + 11 or FLOAT12 <= address < FLOAT12 + 9 or FLOAT4 <= address < FLOAT4 + 9:
            return
        if address == 0x4ac9f0:
            self.event('F', sid, *args[:3]);mask = args[0];value = 80 if mask == 0x20 and self.case[3] & 1 else 81 if mask == 1 and self.case[3] & 2 else 0
            if self.case[13] == 5:
                self.put(POOL + 50 * 50 + 35, 17, 1)
            self.ret(value, 12);return
        if address == 0x4af530:
            ghost = 90 + self.created;self.created += 1;self.event('C', *args[:2], ghost);self.put(POOL + ghost * 50, GHOST_VT);self.put(POOL + ghost * 50 + 10, args[0] + (2 if self.case[13] == 1 else 0), 1);self.put(POOL + ghost * 50 + 11, 4, 1)
            if self.case[13] == 1:
                self.put(POOL + 50 * 50 + 35, 5, 1)
            self.ret(POOL + ghost * 50);return
        if address == 0x4ac220:
            self.event('W', sid, args[0]);self.put(this + 12, args[0], 2)
            if self.case[13] == 2:
                self.put(POOL + 50 * 50 + 34, 7, 1)
            self.ret(0, 4);return
        if address == OWNER:
            self.event('O', sid, args[0]);self.put(this + 34, args[0], 1)
            if self.case[13] == 3:
                self.put(POOL + 50 * 50 + 14, bits(30.5));self.put(POOL + 50 * 50 + 18, bits(40.75));self.put(this, OTHER_VT)
            self.ret(0, 4);return
        if address in (POP, OTHER_POP):
            self.event('P' if address == POP else 'X', sid, *args[:3]);self.put(this + 14, args[0]);self.put(this + 18, args[1])
            if self.case[13] == 4:
                self.put(TYPES + 101 * 500 + 0xec, 0)
            self.ret(0, 12);return
        if address == 0x4e4391:
            self.event('A', args[0]);self.ret(0x13a00000 if self.case[8] else 0);return
        if address == 0x428700:
            self.event('T', *args[:2]);self.ret(this, 8);return
        if address == REFRESH:
            self.event('R', sid)
            if self.case[13] == 6:
                self.put(POOL + 50 * 50 + 40, 0x12, 1)
            self.ret();return
        if address == 0x4225b0:
            self.event('E', *args[:2]);self.ret(self.case[10]);return
        if address == 0x488fe0:
            self.event('M', *args[:3]);self.ret(self.case[11]);return
        if address == 0x4b1a90:
            self.event('H', *args[:2]);self.ret(self.case[12]);return
        if address == 0x429010:
            self.event('V', sid, *args[:2]);self.ret(0, 8);return
        if address in (0x44b900, 0x426ef0):
            self.event('D' if address == 0x44b900 else 'B', sid, *args[:3 if address == 0x44b900 else 1])
            if self.case[13] == 7:
                self.put(POOL + 50 * 50 + 40, 0x33, 1)
            self.put(FLOAT_VALUE, self.case[14 if address == 0x44b900 else 15]);mu.reg_write(UC_X86_REG_EIP, FLOAT12 if address == 0x44b900 else FLOAT4);return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'Carrier 부모 범위밖 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """실제 명령의 허용 쓰기는 새 미리보기 세 필드·SEH·caller결과·스택뿐이다."""
        if STACK - 0x7f000 <= address <= STACK + 0x1000 - size or address == 0 and size == 4 or address == RESULT and size == 4:
            return
        if any(address == POOL + sid * 50 + at and size == width for sid in (90, 91) for at, width in ((35, 1), (36, 4), (40, 1))):
            return
        raise RuntimeError(f'Carrier 부모 범위밖 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """원본 이미지/명시 TYPE/raw/호출자/스택 외 읽기를 차단한다."""
        regions = ((0, 4), (0x400000, 0x600000), (POOL, POOL + 0x10000), (TYPES, TYPES + 188 * 500), (STOP, STOP + 0x1000), (STACK - 0x7f000, STACK + 0x1000))
        if not any(low <= address and address + size <= high for low, high in regions):
            raise RuntimeError(f'Carrier 부모 범위밖 읽기 {address:08x}+{size}')

    def invoke(self, args, precision):
        """정상 ret12와 호출자 float32 저장 뒤 GP/canary/SEH/x87를 모두 검사한다."""
        registers = {UC_X86_REG_EBX:0x12345678, UC_X86_REG_ESI:0x23456789, UC_X86_REG_EDI:0x3456789a, UC_X86_REG_EBP:0x456789ab}
        self.mu.mem_write(STACK, struct.pack('<4I', CALLER, *args) + b'\xc0\xde\xc0\xde')
        # 모든 입력은 독립 레지스터/FPU 상태에서 시작한다.
        for register, value in registers.items():
            self.mu.reg_write(register, value)
        for register, value in ((UC_X86_REG_EAX, 0), (UC_X86_REG_EDX, 0), (UC_X86_REG_ECX, POOL + 50 * 50), (UC_X86_REG_ESP, STACK), (UC_X86_REG_EFLAGS, 2), (UC_X86_REG_FPCW, precision), (UC_X86_REG_FPSW, 0), (UC_X86_REG_FPTAG, 0xffff)):
            self.mu.reg_write(register, value)
        self.mu.emu_start(0x42a750, STOP, count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 16
        assert bytes(self.mu.mem_read(STACK + 16, 4)) == b'\xc0\xde\xc0\xde'
        for register, value in registers.items():
            assert self.mu.reg_read(register) == value
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision and not self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800 and self.mu.reg_read(UC_X86_REG_FPTAG) == 0xffff

    def sample(self, case, precision):
        """분리된 입력만 배치하며 float반환/raw/효과는 실제 실행에서 읽는다."""
        self.case = case;self.events = [];self.created = 0;self.put(0, 0x87654321);self.put(0x5c8464, POOL);self.put(0x59ab20, TYPES);self.put(0x540bc4, case[9])
        # 무관한 바이트를a5로 채우고 원문이 실제로 읽는 입력만 명시한다.
        for sid in WATCH:
            self.mu.mem_write(POOL + sid * 50, b'\xa5' * 50)
        self.put(POOL + 18, 102, 2);parent = POOL + 50 * 50;self.put(parent, PARENT_VT);self.put(parent + 10, 158, 1);self.put(parent + 34, 1, 1);self.put(parent + 35, 0, 1)
        x, y = ((bits(10.5), bits(20.25)), (0x7fc01234, bits(-10)), (0x7f800000, bits(255.9)), (bits(-5), 0x7fc01234))[case[7]];self.put(parent + 14, x);self.put(parent + 18, y)
        for sid, kind, word in ((80, 100, 0xa123), (81, 101, 0x8001)):
            self.put(POOL + sid * 50 + 10, 3, 1);self.put(POOL + sid * 50 + 18, kind, 2);self.put(POOL + sid * 50 + 12, word, 2)
        for kind in range(100, 104):
            self.put(TYPES + kind * 500 + 0x118, kind - 93);self.put(TYPES + kind * 500 + 0x1b8, case[6]);self.put(TYPES + kind * 500 + 0xec, case[4 if kind == 100 else 5])
        self.invoke(case[:3], precision);self.returns += 1;assert self.get(0) == 0x87654321
        return str(self.get(RESULT)), ';'.join(self.events) or '-', ''.join(bytes(self.mu.mem_read(POOL + sid * 50, 50)).hex() for sid in WATCH)


def main():
    """새 원본 관찰을 저장하거나 입력/SHA/정밀도/VT만 변경 없이 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'));rows = [row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')]
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        assert report['vtables'] == [list(row) for row in vtables()]
        assert [row[0] for row in rows] == [','.join(map(str, row)) for row in cases()] and len(rows) == report['uniqueInputs'] and report['normalReturns'] == 2 * len(rows)
        assert [index for index, row in enumerate(rows) if row[1:4] != row[4:7]] == report['precisionDifferentInputs']
        print('Carrier Regular 원본 감사:', len(rows), '입력/', report['normalReturns'], '정상 반환');return
    oracles = [Oracle(), Oracle()];rows = [];different = []
    # 실제 PE를 각각 실행하며 제품 C++나 기대 분기 계산은 사용하지 않는다.
    for index, case in enumerate(cases()):
        outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        if outputs[0] != outputs[1]:
            different.append(index)
        rows.append((','.join(map(str, case)), *outputs[0], *outputs[1]))
    FIXTURE.write_text('# input return53 events53 raw53 return64 events64 raw64\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    vt = vtables();VT_FIXTURE.write_text('# vtable actualRegularEntry\n' + '\n'.join(f'{v}\t{entry}' for v, entry in vt) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'tools/decomp_carrier_regular.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_particle_gradient.py', 'tools/decomp_squid_selection.py', 'cpppj/src/o/DerivedConstructors.inc', 'extracted/refined/originals/functions.tsv', 'cpppj/tests/fixtures/carrier-regular-x86.tsv', 'cpppj/tests/fixtures/carrier-regular-vtables.tsv')
    report = dict(host=socket.gethostname(), date='2026-10-17', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-16'), controls=[0x27f, 0x37f], uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles), nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())), precisionDifferentInputs=different, vtables=vt, boundaries='Carrier42a750/41d0d0/TYPE getter전체. 내용물검색/생성/WORD/소유자/Pop/할당/타이머/표시/492지형·경로·회피/5105낙하/Damageable는명시경계. 실제callerfstpfloat32까지관측. 원본/OS창실행없음.', sha256={path:digest(ROOT / path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls', 'precisionDifferentInputs')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
