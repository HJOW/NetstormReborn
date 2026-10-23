#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004c1f20의 실제 세 DWORD 쓰기를 제품 작성 전에 독립 CPU 실행으로 고정한다."""
import argparse
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 인접 동기화 기록·시계 전체·별도 난수 저장소를 함께 관측한다.
SYNC, SYNC_BYTES, CLOCK, CLOCK_BYTES = 0x5c8be0, 96, 0x55b4a0, 96
STORES = (0x5c8bf4, 0x5c8c00, 0x55b4ac)
RANDOMS = (0x532710, 0x5c89a4, 0x5c89a8, 0x5c89b4)
ENTRY, END = 0x4c1f20, 0x4c1f32
FIXTURE = ROOT / 'cpppj/tests/fixtures/clock-sync-reset-x86.tsv'
REPORT = ROOT / 'cpppj/clock-sync-reset-native.json'


class Oracle(MemoryOracle):
    """18BYTE 전체만 실행하며 외부 자식·가짜 명령 큐 효과를 공급하지 않는다."""
    def __init__(self):
        """읽기 전용 원본 이미지를 가상 메모리에 복사하고 쓰기 감사를 설치한다."""
        super().__init__()
        self.running = False
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """실제 세 DWORD 이외 쓰기를 거부하고 각 store의 주소·폭·값 순서를 기록한다."""
        if self.running:
            if address not in STORES or size != 4:
                raise RuntimeError('ClockSyncReset 예상 밖 쓰기: ' + hex(address))
            self.trace.append(f'{address:08x}:{size}:{value:08x}')

    def instruction(self, mu, address, size, data):
        """허용한 원문 전체 명령만 세며 호출·OS·CRT 경계는 허용하지 않는다."""
        self.instructions += 1
        if not ENTRY <= address < END or self.instructions > 5:
            raise RuntimeError('ClockSyncReset 원문 범위 또는 명령 수 차이')

    def run(self, control):
        """현재 전역을 유지해 한 번 실행하고 cdecl·비휘발·FPU 불변을 확인한다."""
        self.instructions = 0
        self.trace = []
        self.running = False
        self.mu.mem_write(STACK, struct.pack('<I', STOP))
        registers = ((UC_X86_REG_EAX, 0x89abcdef), (UC_X86_REG_ECX, 0x98765432), (UC_X86_REG_EDX, 0x87654321), (UC_X86_REG_EBX, 0x12345678), (UC_X86_REG_ESI, 0x23456789), (UC_X86_REG_EDI, 0x3456789a), (UC_X86_REG_EBP, 0x456789ab))
        for register, value in registers:  # 일부 0 전역 입력을 레지스터 초기 0으로 가장하지 않는다.
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.running = True
        self.mu.emu_start(ENTRY, STOP)
        self.running = False
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('ClockSyncReset cdecl 반환 차이')
        for register, value in registers[1:]:  # 실제 몸체가 건드리지 않는 ECX/EDX와 비휘발 레지스터도 대조한다.
            if self.mu.reg_read(register) != value:
                raise RuntimeError('ClockSyncReset 레지스터 보존 차이')
        if self.mu.reg_read(UC_X86_REG_EAX) != 0 or self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) != 0 or self.mu.reg_read(UC_X86_REG_FPTAG) != 0xffff:
            raise RuntimeError('ClockSyncReset EAX 또는 FPU 차이')
        return bytes(self.mu.mem_read(SYNC, SYNC_BYTES)), bytes(self.mu.mem_read(CLOCK, CLOCK_BYTES)), ';'.join(self.trace), self.instructions


def main():
    """기존 최초 자료 덮어쓰기와 제품 선작성은 거부하고 SHA 전용 감사도 제공한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    arguments = parser.parse_args()
    if arguments.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for relative, expected in report['sha256'].items():  # 후속 공유 소스 SHA를 과거 최초 관측에 섞지 않는다.
            if digest(ROOT / relative) != expected:
                raise RuntimeError('ClockSyncReset 최초 SHA 차이: ' + relative)
        print(json.dumps({'verified': True, 'observations': report['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('ClockSyncReset 최초 자료 덮어쓰기 금지')
    for relative in ('cpppj/src/o/RawClockSyncReset.h', 'cpppj/src/o/RawClockSyncReset.cpp', 'cpppj/tests/ClockSyncResetTests.cpp'):  # 이 관측 전에는 제품을 만들지 않는다.
        if (ROOT / relative).exists():
            raise RuntimeError('ClockSyncReset 제품 선작성 금지')
    oracle = Oracle()
    rows = ['# control / input / repeat / peer / replyMode / ticks / initialSync96 / initialClock96 / sync96 / clock96 / random16 / stores / instructions']
    values = (0, 1, 0xffffffff, 0x80000000, 0x7fffffff, 0x12345678, 0x87654321, 0x5c8bf4)
    different = []
    instructions = 0
    for index in range(16):  # 모두 0인 입력뿐 아니라 서로 다른 비영 비트·극단 DWORD를 사용한다.
        peer, reply, ticks = values[index % 8], values[(index * 3 + 1) % 8], values[(index * 5 + 2) % 8]
        initial_sync = bytearray((index * 19 + byte * 13 + 37) & 255 for byte in range(SYNC_BYTES))
        initial_clock = bytearray((index * 23 + byte * 11 + 61) & 255 for byte in range(CLOCK_BYTES))
        struct.pack_into('<I', initial_sync, STORES[0] - SYNC, peer)
        struct.pack_into('<I', initial_sync, STORES[1] - SYNC, reply)
        struct.pack_into('<I', initial_clock, STORES[2] - CLOCK, ticks)
        expected_sync, expected_clock = bytearray(initial_sync), bytearray(initial_clock)
        struct.pack_into('<I', expected_sync, STORES[0] - SYNC, 0)
        struct.pack_into('<I', expected_sync, STORES[1] - SYNC, 0)
        struct.pack_into('<I', expected_clock, STORES[2] - CLOCK, 0)
        pair = []
        for control in (0x27f, 0x37f):  # FPU 미사용도 두 제어어에서 보존되는지 함께 관측한다.
            oracle.mu.mem_write(SYNC, bytes(initial_sync))
            oracle.mu.mem_write(CLOCK, bytes(initial_clock))
            for position, address in enumerate(RANDOMS):  # 난수 실행 없이 실제 네 DWORD 주변 효과만 감시한다.
                oracle.put(address, 0x89abcdef + position * 17)
            observed = []
            for repeat in range(2):  # 두 번째 호출은 임의 원점·카운터 초기화를 추가하지 않는지 확인한다.
                sync, clock, trace, count = oracle.run(control)
                randoms = ''.join(bytes(oracle.mu.mem_read(address, 4)).hex() for address in RANDOMS)
                if sync != expected_sync or clock != expected_clock or randoms != 'efcdab8900ceab8911ceab8922ceab89':
                    raise RuntimeError('ClockSyncReset 세 DWORD 이외 물리 차이')
                if trace != ';'.join(f'{address:08x}:4:00000000' for address in STORES) or count != 5:
                    raise RuntimeError('ClockSyncReset 실제 store 순서 차이')
                outputs = (sync.hex(), clock.hex(), randoms, trace, str(count))
                observed.append(outputs)
                rows.append('\t'.join(map(str, (control, index, repeat, peer, reply, ticks, initial_sync.hex(), initial_clock.hex(), *outputs))))
                instructions += count
            pair.append(observed)
        if pair[0] != pair[1]:
            different.append(index)
    FIXTURE.write_text('\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originalCD/Netstorm.c', 'extracted/refined/originalCD/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_clock_sync_reset.py', FIXTURE.relative_to(ROOT).as_posix())
    report = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '004c1f20', 'bodyBytes': 18, 'cdSourceMapAddress': '004ec490', 'cdExecuted': False, 'inputs': 16, 'repeatsPerInputAndPrecision': 2, 'observations': len(rows) - 1, 'instructions': instructions, 'precisionDifferentInputs': different, 'productAbsentBeforeObservation': True, 'externalChildren': [], 'scope': '18BYTE 전체 PE 실행. 동기화96/clock96/난수16BYTE와 실제 3store 순서 대조. 명령큐/공통Client/시간동기화 큰부모/공간해시 복원이나 게임 실행은 아님.', 'sha256': {relative: digest(ROOT / relative) for relative in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('observations', 'instructions', 'precisionDifferentInputs')}))


if __name__ == '__main__':
    main()
