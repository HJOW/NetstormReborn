#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 DataManager/섹션 MemStream 소멸의 전체 명령과 free 직전 바이트를 격리 관측한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle, ROOT, IMAGE, HEAP, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

MANAGER, NODES, STRIDE, COUNT = IMAGE + 8, IMAGE + 0x1000, 76, 5  # manager16BYTE와 섹션60BYTE의 양쪽8BYTE를 보존한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/datamanager-lifetime-x86.tsv'  # 제품 변경 전 최초 관측이며 다시 생성하지 않는다.
REPORT = ROOT / 'cpppj/datamanager-lifetime-native.json'  # 실제 게임/OS/CRT 힙 실행과 격리 명령 실행을 구분한다.


class ManagerOracle(Oracle):
    """004bd330의 실제 점프·0044da60·0044d040만 실행하고 CRT free만 명시 경계로 둔다."""
    def __init__(self):
        """현재 PC 함수표의 불연속 범위와 각 본문 진입 횟수를 고정한다."""
        super().__init__()
        self.entries = {0x4bd330: 0, 0x44da60: 0, 0x44d040: 0}
        self.allowed = []
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 공용 Oracle의 대체 호출을 사용하지 않고 선택한 세 본문만 허용한다.
        for entry in self.entries:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def snapshot(self):
        """해제된 메모리도 지우지 않고 manager/모든 섹션과 보호 바이트를 기록한다."""
        return bytes(self.mu.mem_read(MANAGER - 8, 32)) + bytes(self.mu.mem_read(NODES, COUNT * STRIDE))

    def write(self, mu, access, address, size, value, data):
        """실제 원본 쓰기는 vptr/head와 MemStream 첫16BYTE 및 격리 스택으로 제한한다."""
        valid = MANAGER <= address and address + size <= MANAGER + 8
        # 이름/next/양쪽 보호 바이트는 원본 소멸 명령이 변경하지 못한다.
        for index in range(COUNT):
            node = NODES + index * STRIDE + 8
            valid |= node <= address and address + size <= node + 16
        assert valid or STACK - 0x7f000 <= address and address + size <= STACK + 0x1000, (hex(address), size)

    def instruction(self, mu, address, size, data):
        """free 직전 같은 전체 바이트와 효과 뒤 현재 head 재조회를 관측한다."""
        if address == 0x4e4354:
            pointer = self.words(mu.reg_read(UC_X86_REG_ESP) + 4)[0]
            is_buffer = HEAP <= pointer < HEAP + COUNT * 0x1000
            index = (pointer - HEAP) // 0x1000 if is_buffer else (pointer - NODES - 8) // STRIDE
            assert 0 <= index < COUNT
            assert pointer == (HEAP + index * 0x1000 if is_buffer else NODES + index * STRIDE + 8)
            self.events.append(('B' if is_buffer else 'N') + ',' + str(pointer) + ',' + self.snapshot().hex())
            node = NODES + index * STRIDE + 8
            if self.variant == 1 and is_buffer:
                self.put(node, 0xabcdef01)
                self.put(node + 4, 0x87654321)
                self.put(node + 0x38, 0x98765432)
            if self.variant == 2 and not is_buffer:
                self.put(MANAGER + 4, 0)
                self.put(MANAGER + 8, 0x87654321)
            if self.variant == 3 and is_buffer:
                following = self.words(MANAGER + 4)[0]
                if following:
                    self.put(following + 0x38, 0)
            self.ret()
            return
        if address in self.entries:
            self.entries[address] += 1
        if not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError(f'미검토 DataManager 명령 {address:08x}')
        self.instructions += 1
        if self.instructions > 10000:
            raise RuntimeError('DataManager 소멸 명령 상한')

    def observe(self, inputs):
        """서로 다른 목록 순서·빈 버퍼·현재 포인터·경계 변이에 대해 전체 결과와 ABI를 저장한다."""
        kind, seed, count, reverse, mask, self.variant = inputs
        self.events = []
        self.instructions = 0
        manager = bytes((index * 13 + seed) & 255 for index in range(32))
        nodes = bytes((index * 17 + seed) & 255 for index in range(COUNT * STRIDE))
        self.mu.mem_write(MANAGER - 8, manager)
        self.mu.mem_write(NODES, nodes)
        order = list(range(count))
        if reverse:
            order.reverse()
        self.put(MANAGER + 4, NODES + order[0] * STRIDE + 8 if order else 0)
        # 선택되지 않은 노드도 같은 전체 입력에 남겨 원본 밖 쓰기를 검사한다.
        for index in range(COUNT):
            self.put(NODES + index * STRIDE + 8, HEAP + index * 0x1000 if mask & (1 << index) else 0)
            self.put(NODES + index * STRIDE + 8 + 0x38, 0)
        # 생성/파일 읽기 알고리즘을 대체하지 않고 소멸의 명시 목록 입력만 구성한다.
        for at, index in enumerate(order):
            self.put(NODES + index * STRIDE + 8 + 0x38, NODES + order[at + 1] * STRIDE + 8 if at + 1 < count else 0)
        before = self.snapshot()
        # x87 제어어와 네 보존 레지스터를 모든 정상 반환에서 확인한다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0x76543210 + register)
        self.mu.reg_write(UC_X86_REG_ECX, NODES + 8 if kind == 'S' else MANAGER)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x27f)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<I', STOP))
        self.mu.emu_start({'S': 0x44d040, 'D': 0x44da60, 'T': 0x4bd330}[kind], STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 and self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f
        # 실제 본문이 보존해야 하는 레지스터만 비교한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            assert self.mu.reg_read(register) == 0x76543210 + register
        return [','.join(map(str, inputs)), before.hex(), self.snapshot().hex(), ';'.join(self.events) or '-']


def cases():
    """직접/Template 점프 진입과 단일 MemStream의 서로 다른 입력을 각각 관측한다."""
    # 동일 제품 결과를 사용하지 않고 소유 버퍼·목록 길이·효과 뒤 변이를 교차한다.
    for kind in ('D', 'T'):
        for seed in (0, 255):
            for count in (0, 1, 3, 5):
                for reverse in (0, 1):
                    for mask in (0, 0x15, 0x1f):
                        for variant in (0, 1, 2, 3):
                            yield [kind, seed, count, reverse, mask, variant]
    # 단일 소멸은 manager/연결/이름을 보존하고 버퍼가 없을 때 free를 부르지 않는다.
    for seed in (0, 255):
        for mask in (0, 1):
            for variant in (0, 1):
                yield ['S', seed, 1, 0, mask, variant]


def main():
    """최초 원문만 한 번 생성하며 --verify는 기계어/게임을 실행하지 않고 전체 SHA를 검사한다."""
    if '--verify' in sys.argv:
        facts = json.loads(REPORT.read_text('utf-8'))
        # 최초 도구·원문·정밀 근거의 전체 바이트를 다시 읽기만 한다.
        for name, expected in facts['sha256'].items():
            assert digest(ROOT / name) == expected, name
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if not line.startswith('#')]
        assert len(rows) == facts['uniqueInputs'] == len({row[0] for row in rows}) == 392
        print('DataManager/MemStream 최초392입력 전체 원문/SHA 보존 확인')
        return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 관측을 덮어쓰지 않는다'
    oracle = ManagerOracle()
    rows = []
    instructions = 0
    # C++ 제품을 읽거나 호출하지 않고 원본 PE의 세 본문만 실행한다.
    for inputs in cases():
        rows.append(oracle.observe(inputs))
        instructions += oracle.instructions
    assert len(rows) == 392 and oracle.entries[0x4bd330] == 192 and oracle.entries[0x44da60] == 384
    FIXTURE.write_text('# input\tbefore412\tafter412\ttrace\n' + ''.join('\t'.join(row) + '\n' for row in rows), encoding='utf-8', newline='\n')
    evidence = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_local_mission_transport.py', 'tools/decomp_datamanager_lifetime.py', FIXTURE.relative_to(ROOT).as_posix()]
    facts = dict(date='2026-10-21', host=socket.gethostname(), lastDecompileHost='VM-W11-CODEX', lastDecompileDate='2026-10-21', uniqueInputs=len(rows), normalFunctionReturns=len(rows), instructions=instructions,
        entryCounts={f'{entry:08x}': count for entry, count in oracle.entries.items()}, gameExecuted=False, osExecuted=False, cppExecuted=False, capturedBeforeProductEdits=True,
        boundaries=['004bd330의 실제 점프와0044da60/0044d040 전체 본문을 실행; CRT free만 명시 경계', '현재 head를 먼저 unlink한 뒤 버퍼/노드를 반납하고 경계 뒤 head를 재조회; current/이름/next/양쪽8BYTE 보존', '실제 CRT 힙/파일/OS/게임 실행과 가상 scalar 소멸은 범위 밖'], sha256={name: digest(ROOT / name) for name in evidence})
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('DataManager/MemStream 최초', len(rows), '입력', instructions, '명령', oracle.entries)


if __name__ == '__main__':
    main()
