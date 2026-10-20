#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""초기 공용 경계 가로채기를 배제하고 세 실제 함수 진입 횟수까지 확인하는 독립 재관측이다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle, ROOT, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADERS, MANAGERS = 0x5c89f0, 0x12100000  # 원본9×20BYTE와 외부 DataManager의 불투명 입력 주소다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/template-lifetime-v2-x86.tsv'  # 첫 전체 출력이며 제품 구현으로 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/template-lifetime-v2-native.json'  # 큰 DataManager 몸체/게임과 구별한 독립 관측이다.


class LifetimeOracle(Oracle):
    """원본 세 부모 전체를 실행하고 실제 DataManager 소멸/힙은 명시 효과로 관측한다."""
    def __init__(self):
        """현재 PC의 정밀 함수표로 실제 실행할 세 함수의 불연속 범위만 허용한다."""
        super().__init__();self.allowed = [];self.entryCounts = {0x4bfb00: 0, 0x4bf690: 0, 0x4bd2e0: 0}
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 큰 소멸/CRT/OS를 알 수 없는 성공으로 통과시키지 않는다.
        for entry in (0x4bfb00, 0x4bf690, 0x4bd2e0):
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """실제 명령은 같은180BYTE와 격리 스택 밖에 쓰지 못한다."""
        assert HEADERS <= address and address + size <= HEADERS + 180 or STACK - 0x7f000 <= address and address + size <= STACK + 0x1000, (hex(address), size)

    def instruction(self, mu, address, size, data):
        """소멸/free 당시 전체 현재 헤더와 효과 뒤 재조회를 보존한다."""
        if address in (0x4bd330, 0x4e4354):
            pointer = mu.reg_read(UC_X86_REG_ECX) if address == 0x4bd330 else self.words(mu.reg_read(UC_X86_REG_ESP) + 4)[0]
            slot = (pointer - MANAGERS) // 0x1000;assert 0 <= slot < 9
            self.events.append(('D' if address == 0x4bd330 else 'F') + ',' + str(pointer) + ',' + bytes(mu.mem_read(HEADERS - 8, 196)).hex())
            if address == 0x4bd330 and self.variant == 1:
                self.put(HEADERS + slot * 20 + 4, 0x87654321)
                if slot < 8:self.put(HEADERS + (slot + 1) * 20 + 4, 1)
            if address == 0x4e4354 and self.variant == 2:
                self.put(HEADERS + slot * 20, 0xdead0000);self.put(HEADERS + slot * 20 + 16, 0x76543210)
                if slot < 8:self.put(HEADERS + (slot + 1) * 20, 0)
            self.ret();return
        # 공용 실행기의 대체 호출을 사용하지 않고 선택한 세 본문의 실제 진입을 센다.
        if address in self.entryCounts:self.entryCounts[address] += 1
        if not any(low <= address < high for low, high in self.allowed):raise RuntimeError(f'unreviewed template instruction {address:08x}')
        self.instructions += 1
        if self.instructions > 100000:raise RuntimeError('instruction limit')

    def observe(self, inputs):
        """같은 입력에서 전체 헤더/양쪽8BYTE·소멸/free 직전 상태와 ABI를 남긴다."""
        kind, seed, present, borrowed, selected, self.variant = inputs
        self.events = [];self.instructions = 0;self.kind = kind
        before = bytearray((index * 17 + seed) & 255 for index in range(196))
        # 포인터가0인 칸에도 별도 stale 필드를 남겨 전역/단일 정리의 차이를 관측한다.
        for slot in range(9):
            struct.pack_into('<II', before, 8 + slot * 20, MANAGERS + slot * 0x1000 if present & (1 << slot) else 0, 0x80000000 if borrowed & (1 << slot) else 0)
        self.mu.mem_write(HEADERS - 8, bytes(before))
        # 모든 정상 반환에서 cdecl/thiscall의 같은 보존 레지스터를 대조한다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):self.mu.reg_write(register, 0x76543210 + register)
        self.mu.reg_write(UC_X86_REG_ECX, HEADERS + selected * 20);self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.mem_write(STACK, struct.pack('<II', STOP, 0xa5a5a5a5))
        self.mu.emu_start({'A': 0x4bfb00, 'S': 0x4bf690, 'I': 0x4bd2e0}[kind], STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 and self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f
        # 변경 대상이 아닌 네 callee-saved 레지스터를 전수 확인한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):assert self.mu.reg_read(register) == 0x76543210 + register
        after = bytes(self.mu.mem_read(HEADERS - 8, 196));assert before[:8] == after[:8] and before[-8:] == after[-8:]
        return [json.dumps(inputs, separators=(',', ':')), before.hex(), after.hex(), ';'.join(self.events) or '-']


def cases():
    """9칸·빈/stale/빌림/소유와 소멸/free 뒤 현재 필드 변이를 교차한다."""
    # 전역 정리는 비영 포인터인 칸만 비우며 뒤 칸의 현재 필드도 다시 읽는다.
    for seed in (0, 7, 255):
        for present in (0, 0x1ff, 0x155, 0x1ee):
            for borrowed in (0, 0x1ff, 0x155):
                for variant in (0, 1, 2):yield ['A', seed, present, borrowed, 0, variant]
    # 단일 반납은 포인터가0이어도 선택된20BYTE를 비운다.
    for seed in (0, 255):
        for present in (0, 0x1ff):
            for borrowed in (0, 0x1ff, 0x155):
                for selected in (0, 4, 8):
                    for variant in (0, 1, 2):yield ['S', seed, present, borrowed, selected, variant]
    # 생성 초기화는 이전 입력을 소멸하지 않고20BYTE만0으로 쓴다.
    for seed in (0, 255):
        for present in (0, 0x1ff):
            for borrowed in (0, 0x1ff):
                for selected in (0, 4, 8):yield ['I', seed, present, borrowed, selected, 0]


def main():
    """최초 관측만 한 번 생성하거나 전체 근거/원문을 읽기 전용으로 검증한다."""
    evidence = ['AGENTS.md', 'originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_local_mission_transport.py', 'tools/decomp_template_lifetime_v2.py', FIXTURE.relative_to(ROOT).as_posix()]
    if '--verify' in sys.argv:
        facts = json.loads(REPORT.read_text('utf-8'))
        # 첫 원문·실행 도구·정밀 근거의 전체 바이트를 대조한다.
        for name, sha in facts['sha256'].items():assert digest(ROOT / name) == sha, name
        rows = [line for line in FIXTURE.read_text('utf-8').splitlines() if not line.startswith('#')]
        assert len(rows) == facts['uniqueInputs'] == len({row.split('\t')[0] for row in rows}) == 240
        print('Template 수명 최초240입력 전체 원문 보존 확인');return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 최초 관측은 덮어쓰지 않는다'
    oracle = LifetimeOracle();rows = [];instructions = 0
    # 제품 코드를 읽거나 호출하지 않고 실제 원본 입력을 먼저 끝까지 실행한다.
    for inputs in cases():rows.append(oracle.observe(inputs));instructions += oracle.instructions
    assert len(rows) == 240 and oracle.entryCounts == {0x4bfb00: 108, 0x4bf690: 108, 0x4bd2e0: 24}
    FIXTURE.write_text('# input\tbefore196\tafter196\ttrace\n' + ''.join('\t'.join(row) + '\n' for row in rows), encoding='utf-8', newline='\n')
    facts = dict(date='2026-10-21', host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', lastDecompileDate='2026-10-20', uniqueInputs=len(rows), normalFunctionReturns=len(rows), instructions=instructions, gameExecuted=False, osExecuted=False,
        boundaries=['실제004bfb00/004bf690/004bd2e0의전체부모와20/180BYTE를격리실행', 'DataManager 소멸004bd330과CRT free는명시효과이며내부MemStream 목록/힙/OS/게임을실행하지않았다', '전체196BYTE·각소멸/free직전현재헤더·효과뒤재조회·소유/빌림/빈stale차이·ABI/보존레지스터/x87/쓰기범위를관측'], sha256={name: digest(ROOT / name) for name in evidence})
    facts['entryCounts'] = {f'{entry:08x}': count for entry, count in oracle.entryCounts.items()}
    facts['firstAttemptRejected'] = True;facts['capturedAfterProductEdits'] = True
    facts['firstAttemptSha256'] = {name: digest(ROOT / name) for name in ['tools/decomp_template_lifetime.py', 'cpppj/template-lifetime-native.json', 'cpppj/tests/fixtures/template-lifetime-x86.tsv']}
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print('원본 Template 수명', len(rows), '정상 반환', instructions, '명령')


if __name__ == '__main__':main()
