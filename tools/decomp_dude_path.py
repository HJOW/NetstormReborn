#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DudePath24의 실제 생성자·부착과 공유 가상 슬롯을 제품 작성 전에 격리 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import zlib
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle, POOL
import pefile
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

HEAP, SIZE = 0x14500000, 0x186f0  # 원본 new의 전체100080바이트다.
FIXTURE = ROOT/'cpppj/tests/fixtures/dude-path-x86.tsv'  # 새 독립 원문이며 기존 기대값은 변경하지 않는다.
VTABLES = ROOT/'cpppj/tests/fixtures/dude-path-vtables.tsv'  # 실제24/54와 부모70의 공유/고유 슬롯이다.
REPORT = ROOT/'cpppj/dude-path-native.json'  # 실행 몸체·경계·정상 반환·보호 해시다.


def tables():
    """보호PE에서 모든 경로 슬롯과 부모의 정지·도착·시작 통지를 직접 읽는다."""
    image = pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image()
    rows = []
    # 부착+2c만 다른 두 경로의13개 슬롯을 생략하지 않는다.
    for kind, table in ((24, 0x50e3d8), (54, 0x50e458), (70, 0x508cc8)):
        offsets = range(0, 52, 4) if kind != 70 else (0x38, 0x3c, 0x40, 0x44)
        rows.append('\t'.join(map(str, (kind, table, *(struct.unpack_from('<I', image, table-0x400000+offset)[0] for offset in offsets)))))
    return '# kind vtable path13slots_OR_parent_halt_arrival_started40_started44\n'+'\n'.join(rows)+'\n'


class Oracle(BaseOracle):
    """실제48bf30/48b2c0/48bab0을 실행하고 할당·Host 부착·진단을 경계로 둔다."""
    def __init__(self):
        """OS나 게임 창 없이 보호 이미지와 별도 heap/부모 슬롯을 준비한다."""
        super().__init__()
        self.mu.mem_map(HEAP, 0x19000)  # 생성자의 전체 실제 저장소다.
        self.mu.mem_map(POOL, 0x10000)  # 호출자의 this 토큰과 부착 부모51의 격리 슬롯이다.
        self.allowed = []  # 실제 명령 범위만 허용한다.
        ranges(self, (0x48bf30, 0x48b2c0, 0x48bab0))
        self.instructions = 0  # 실제 실행한 명령 수다.
        self.calls = collections.Counter()  # 세 원본 몸체 진입 횟수다.
        self.events = []  # 호출 순서대로 저장한 명시 경계 인자다.
        self.allocation = False  # 이번 생성자 관측의 명시 new 결과다.
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def instruction(self, mu, address, size, data):
        """원본 계산을 그대로 실행하며 세 외부 효과의 전체 인자를 기록한다."""
        self.instructions += 1
        if address in (0x48bf30, 0x48b2c0, 0x48bab0):
            self.calls[f'{address:08x}'] += 1
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e4391:
            assert self.get(esp+4) == SIZE
            self.ret(HEAP if self.allocation else 0)
            return
        if address == 0x41bd20:
            assert mu.reg_read(UC_X86_REG_ECX) == POOL+50*50
            self.events.append('attach:'+':'.join(map(str, struct.unpack('<4I', mu.mem_read(esp+4, 16)))))
            self.ret(0, 16)
            return
        if address == 0x4e0620:
            self.events.append('assert:'+str(self.get(esp+12)))
            self.ret()
            return
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'DudePath 범위 밖 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """실제 몸체 쓰기는 생성자 heap과 호출 스택으로 제한한다."""
        if HEAP <= address and address+size <= HEAP+SIZE or STACK-0x7f000 <= address and address+size <= STACK+0x1000:
            return
        raise RuntimeError(f'DudePath 범위 밖 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """보호 이미지·스택·heap·명시 부모 슬롯 외 읽기를 거부한다."""
        if any(low <= address and address+size <= high for low, high in ((0x400000, 0x600000), (HEAP, HEAP+SIZE), (POOL, POOL+0x10000), (STACK-0x7f000, STACK+0x1000), (STOP, STOP+0x1000))):
            return
        raise RuntimeError(f'DudePath 범위 밖 읽기 {address:08x}+{size}')

    def sample(self, case, precision):
        """전체 생성자 checksum 또는 실제 부착·진단 순서를 두 정밀도에 독립 관측한다."""
        operation, first, second, third = case
        self.events = []
        if operation == 0:
            self.allocation = bool(second)
            self.mu.mem_write(HEAP, bytes([first])*SIZE)
            PreviousOracle.invoke(self, 0x48bf30, b'', precision, False)
            result = self.mu.reg_read(UC_X86_REG_EAX)
            assert result in (0, HEAP)
            output = str(int(bool(result)))+':'+str(zlib.adler32(bytes(self.mu.mem_read(HEAP, SIZE))))
        else:
            self.put(0x5c8464, POOL)
            self.put(0x541170, 70)
            self.put(0x5410d0, 24)
            self.put(POOL+51*50+10, first, 1)
            PreviousOracle.invoke(self, 0x48bab0, struct.pack('<3I', 51, second, third), precision, False)
            output = ';'.join(self.events)
        self.returns += 1
        return output


def cases():
    """세 heap 패턴/할당 실패와 현재 부모 타입/flags/Form을 독립 입력으로 만든다."""
    return [(0, fill, allocation, 0) for fill, allocation in itertools.product((0, 0xa5, 0xff), (0, 1))]+[(1, kind, flags, form) for kind, flags, form in itertools.product((70, 71, 158), (0, 0x10, 0x50, 0x80), (0, 65535))]


def main():
    """최초 원문을 수집하거나 파일을 변경하지 않고 해시·공유 슬롯·정상 반환을 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        # 실제 관측 자료와 도구·보호PE를 각각 확인한다.
        for path, wanted in report['sha256'].items():
            assert digest(ROOT/path) == wanted, path
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == 30 and report['normalReturns'] == 60 and all(row[1] == row[2] for row in rows)
        assert [row[0] for row in rows] == [','.join(map(str, case)) for case in cases()]
        assert VTABLES.read_text('utf-8') == tables()
        print('DudePath24 원본 감사:30입력/60정상 반환/가상표3행')
        return
    oracles = [Oracle(), Oracle()]
    rows = []
    # 제품 구현이나 기존 C++ 기대값을 사용하지 않고 두 정밀도를 관측한다.
    for case in cases():
        outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert outputs[0] == outputs[1]
        rows.append((','.join(map(str, case)), *outputs))
    FIXTURE.write_text('# input output53 output64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', encoding='utf-8', newline='\n')
    VTABLES.write_text(tables(), encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'tools/decomp_dude_path.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_carrier_falling.py', 'extracted/refined/originals/functions.tsv', 'cpppj/tests/fixtures/dude-path-x86.tsv', 'cpppj/tests/fixtures/dude-path-vtables.tsv')
    report = dict(host=socket.gethostname(), date='2026-10-21', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-20'), uniqueInputs=30, normalReturns=sum(oracle.returns for oracle in oracles), instructions=sum(oracle.instructions for oracle in oracles), nativeCalls=dict(sum((oracle.calls for oracle in oracles), collections.Counter())), controls=[0x27f, 0x37f], boundaries='48bf30/48b2c0 생성자 전체와48bab0 부착 전체. new/41bd20 Host 부착/복귀 진단은 명시 경계. 생성자100080바이트 전체와 부착 전체 인자·진단 순서. GP/retN/canary/x87/코드·읽기·쓰기 검사. OS/게임/창 실행 없음. 공유 Init/Run/Arrive의 재관측을 주장하지 않는다.', sha256={path: digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}))


if __name__ == '__main__':
    main()
