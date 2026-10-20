#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004b51e0의 전체 요청144BYTE/Zone512·복제86BYTE를 제품 수정 전에 격리 관측한다."""
import csv
import json
import socket
import struct
import sys
from pathlib import Path
from decomp_local_mission_transport import Oracle, ROOT, TEXT, HEAP, TABLE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

NEXT, COUNT, ADDRESS, PORT = 0x5c8878, 0x5c85c0, 0x513870, 0x5318e8  # 원본의 서로 다른 대기 블록/순번/9BYTE 주소/포트다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/mission-fort-request-x86.tsv'  # 구현 결과로 덮어쓰지 않을 첫 전체 출력이다.
REPORT = ROOT / 'cpppj/mission-fort-request-native.json'  # 제품/게임 실행과 구별한 독립 원문이다.


class RequestOracle(Oracle):
    """실제 요청/패킷 할당/네 슬롯/관찰기/복제를 실행하고 Fort 선택·힙만 경계로 공급한다."""
    def __init__(self):
        """부모의 PE/격리 메모리만 재사용하고 실행 허용 범위를 요청의 실제 함수들로 좁힌다."""
        super().__init__()
        selected = {0x4b51e0, 0x4df590, 0x40a220, 0x4dece0, 0x40a1e0, 0x4e4a40}
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []  # 알 수 없는 다른 본문 진입을 성공으로 통과시키지 않는다.
        # 현재 호스트 정밀 함수표에 기록된 실제 불연속 범위만 허용한다.
        for entry in selected:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """실제 명령의 쓰기를 스택/복제 이미지/현재 요청과 순번에 한정한다."""
        allowed = ((STACK - 0x7f000, STACK + 0x1000), (HEAP, HEAP + 86), (NEXT, NEXT + 144), (COUNT, COUNT + 4))
        assert any(low <= address and address + size <= high for low, high in allowed), (hex(address), size)

    def instruction(self, mu, address, size, data):
        """선택 뒤 현재 주소/포트와 복제 뒤 현재 순번의 재조회를 독립 변이로 확인한다."""
        esp = mu.reg_read(UC_X86_REG_ESP);args = self.words(esp + 4, 8)
        if address == 0x460980:
            self.events.append('Fort,' + self.string(args[0]).hex())
            if self.variant == 1:
                mu.mem_write(ADDRESS, bytes.fromhex('414243444546474849'));self.put(PORT, 12345);self.put(COUNT, 0xfffffffe)
            self.ret();return
        if address == 0x4e4391:
            assert args[0] == 86;self.events.append('Allocate,86')
            if self.variant == 2:
                self.put(COUNT, 0xffffffff);self.put(PORT, 9876);self.put(NEXT + 0x60, 0x76543210)
            self.ret(HEAP);return
        super().instruction(mu, address, size, data)

    def observe(self, inputs):
        """원문 전체 블록과 양쪽 이웃/전체 예약/복제를 같은 정상 반환에서 보존한다."""
        seed, address, port, count, name, self.variant = inputs
        self.events = [];self.instructions = 0;self.reserve = 0;self.packet_address = 0;self.kind = 'Request'
        before = bytes((index * 13 + seed) & 255 for index in range(160));self.mu.mem_write(NEXT - 8, before)
        self.mu.mem_write(TEXT, name.encode('ascii') + b'\0');self.mu.mem_write(ADDRESS, bytes.fromhex(address));self.put(PORT, port);self.put(COUNT, count)
        self.put(0x5e477c, TABLE);self.put(TABLE + 0x1a * 4, 0x40a1e0);self.put(0x544c30, 0)
        self.mu.mem_write(HEAP, b'\xa5' * 94)
        # 미사용 입력도 고유 값으로 채워 cdecl 보존 레지스터와 x87 상태를 확인한다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):self.mu.reg_write(register, 0x76543210 + register)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.mem_write(STACK, struct.pack('<III', STOP, TEXT, 0xa5a5a5a5))
        self.mu.emu_start(0x4b51e0, STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 and self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f
        # 모든 정상 반환에서 원본 callee-saved 레지스터를 검사한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):assert self.mu.reg_read(register) == 0x76543210 + register
        after = bytes(self.mu.mem_read(NEXT - 8, 160));assert before[:8] == after[:8] and before[-8:] == after[-8:]
        assert self.reserve == 512 and self.words(NEXT + 0x80)[0] == HEAP and bytes(self.mu.mem_read(HEAP + 86, 8)) == b'\xa5' * 8
        return [json.dumps(inputs, separators=(',', ':')), before.hex(), after.hex(), str(self.words(COUNT)[0]), bytes(self.mu.mem_read(self.packet_address, 512)).hex(), bytes(self.mu.mem_read(HEAP, 86)).hex(), ';'.join(self.events)]


def cases():
    """예약 영역·주소9BYTE·포트/순번 감김·경계 뒤 재조회 입력을 교차한다."""
    # 현재 요청의 상태/주소 주변 stale 바이트를 각 입력에서 다르게 유지한다.
    for seed in (0, 7, 91, 255):
        for address in ('7a6c6f63616c2c2a00', '000102030405060708'):
            for port, count in ((6799, 0), (0xffffffff, 0xffffffff), (0, 7)):
                for name in ('Tutorial1', 'MasterOfWhirligigs'):
                    for variant in (0, 1, 2):yield [seed, address, port, count, name, variant]


def main():
    """첫 관측을 한 번만 생성하거나 SHA/고유행을 읽기 전용으로 검증한다."""
    evidence = ['AGENTS.md', 'originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_local_mission_transport.py', 'tools/decomp_mission_fort_request.py', FIXTURE.relative_to(ROOT).as_posix()]
    if '--verify' in sys.argv:
        facts = json.loads(REPORT.read_text('utf-8'))
        # 최초 원문과 실행 도구·정밀 근거의 전체 바이트를 대조한다.
        for name, sha in facts['sha256'].items():assert digest(ROOT / name) == sha, name
        rows = [row for row in FIXTURE.read_text('utf-8').splitlines() if not row.startswith('#')]
        assert len(rows) == facts['uniqueInputs'] == len({row.split('\t')[0] for row in rows}) == 144
        print('미션 Fort 요청 최초 전체 원문144입력 보존 확인');return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 독립 원문은 덮어쓰지 않는다'
    oracle = RequestOracle();rows = [];instructions = 0
    # 제품과 무관한 원본 PE 입력을 먼저 끝까지 실행한다.
    for inputs in cases():rows.append(oracle.observe(inputs));instructions += oracle.instructions
    assert len(rows) == 144
    FIXTURE.write_text('# input\tbefore160\tafter160\tcount\tzone512\tcopy86\ttrace\n' + ''.join('\t'.join(row) + '\n' for row in rows), encoding='utf-8', newline='\n')
    facts = {'date': '2026-10-21', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon', 'lastDecompileDate': '2026-10-20', 'uniqueInputs': len(rows), 'normalFunctionReturns': len(rows), 'instructions': instructions, 'gameExecuted': False, 'osExecuted': False, 'boundaries': ['실제004b51e0/4df590/40a220/4dece0/40a1e0/4e4a40만 격리 실행', '큰00460980 Fort 선택과힙만 명시 효과이며 전체 State7/게임 실행이 아니다', '주소는DWORD2개와BYTE1개만 복사하며 뒤3BYTE/나머지144BYTE 필드는 보존한다', '복제 관찰기의86BYTE와전체512BYTE/양쪽8BYTE/호출 순서·현재 순번 감김/스택·레지스터·x87 보존'], 'sha256': {name: digest(ROOT / name) for name in evidence}}
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print('원본 Fort 요청', len(rows), '정상 반환', instructions, '명령')


if __name__ == '__main__':main()
