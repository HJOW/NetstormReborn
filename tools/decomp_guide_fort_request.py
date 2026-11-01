#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004b50b0/004b32f0의 현재 Subscriber와 미션 요청을 제품 작성 전에 고정한다."""
import csv
import itertools
import json
import socket
import struct
import sys
from decomp_mission_fort_request import RequestOracle, ROOT, NEXT, COUNT, ADDRESS, PORT, TEXT, HEAP, TABLE, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_FPCW

HANDLE, NAME, HEADERS, OWNER, MY, FORT = TEXT + 0x1000, TEXT + 0x2000, 0x5c89f0, 0x540cac, 0x540c74, 0x594fb8  # 원문 handle/입력/현재 전역의 서로 다른 물리다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/guide-fort-request-x86.tsv'  # 최초 관측을 구현 결과로 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/guide-fort-request-native-20261102.json'  # 실제 몸체와 명시 child 경계를 구별한다.


class GuideOracle(RequestOracle):
    """선택/미션 요청/주소 작성/패킷/복제를 실행하고 Config·제목·Template writer만 공급한다."""
    def __init__(self):
        """같은 보호 PE의 정확한 함수 범위를 추가한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        for entry in (0x4b50b0, 0x4b32f0, 0x463b30):  # 세 실제 몸체 외의 새 함수를 허용하지 않는다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))

    def write(self, mu, access, address, size, value, data):
        """Subscriber의 실제 handle/My 쓰기만 기존 요청의 허용 범위에 추가한다."""
        if HANDLE <= address and address + size <= HANDLE + 64 or MY <= address and address + size <= MY + 4:
            return
        super().write(mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """큰 외부 효과 뒤 바뀐 현재 owner/handle/주소/카운터를 원문이 다시 읽게 한다."""
        esp = mu.reg_read(UC_X86_REG_ESP);args = self.words(esp + 4, 8)
        if address == 0x4414d0:
            assert self.string(args[0]) == b'fort.mySubHandle'
            self.events.append('Config,' + self.string(args[1]).hex())
            if self.variant == 1:
                self.put(OWNER, 8);self.put(MY, 91);self.put(FORT, 0);mu.mem_write(HANDLE, b'AfterConfig\0')
                mu.mem_write(ADDRESS, bytes.fromhex('414243444546474849'));self.put(PORT, 12345);self.put(COUNT, 0xfffffffe)
            self.ret();return
        if address == 0x460730:
            self.events.append('Title');self.ret();return
        if address == 0x4bd400:
            owner = (mu.reg_read(UC_X86_REG_ECX) - HEADERS) // 20
            self.events.append(f'Subscriber,{owner},{args[0]},{self.string(args[1]).hex()}')
            if self.variant == 2:
                self.put(OWNER, 7);self.put(MY, 0xabababab);mu.mem_write(HANDLE, b'AfterSubscriber\0')
            self.ret(purge=8);return
        if address == 0x4bd360:
            self.events.append('Notify,' + str((mu.reg_read(UC_X86_REG_ECX) - HEADERS) // 20));self.ret();return
        if address == 0x4e4391:
            assert args[0] == 86;self.events.append('Allocate,86')
            if self.variant == 3:
                self.put(COUNT, 0xffffffff);self.put(PORT, 9876);self.put(NEXT + 0x60, 0x76543210)
            self.ret(HEAP);return
        if address == 0x460980:
            raise AssertionError('Guide 요청은 Fort 이름 선택을 다시 호출하지 않는다')
        super().instruction(mu, address, size, data)

    def observe_guide(self, kind, inputs):
        """전체 요청/이웃/64BYTE 이름/현재 전역/복제와 실제 호출 순서를 반환한다."""
        seed, address, port, count, gate, self.variant, name, subscriber, supplied = inputs
        self.events = [];self.instructions = 0;self.reserve = 0;self.packet_address = 0;self.kind = kind
        before = bytes((index * 13 + seed) & 255 for index in range(160));self.mu.mem_write(NEXT - 8, before)
        self.mu.mem_write(ADDRESS, bytes.fromhex(address));self.put(PORT, port);self.put(COUNT, count)
        self.mu.mem_write(HANDLE, b'\xa5' * 64);self.mu.mem_write(HANDLE, name.encode('ascii') + b'\0');self.put(0x540ca8, HANDLE)
        self.mu.mem_write(NAME, b'Selected\0');self.put(OWNER, 1);self.put(MY, 0xffffffff);self.put(FORT, int(gate != 1))
        self.mu.mem_write(HEADERS, b'\0' * 180)
        for owner in (1, 7, 8):  # callback로 선택될 manager도 입력으로 존재시키며 원문 관문만 바꾼다.
            self.put(HEADERS + owner * 20, 0 if gate == 0 else 0x12340000 + owner)
        self.put(0x5e477c, TABLE);self.put(TABLE + 0x1a * 4, 0x40a1e0);self.put(0x544c30, 0);self.mu.mem_write(HEAP, b'\xa5' * 94)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_FPCW, 0x27f)
        self.mu.mem_write(STACK, struct.pack('<III', STOP, subscriber, NAME if supplied else 0))
        self.mu.emu_start(0x4b50b0 if kind == 'R' else 0x4b32f0, STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 and self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f
        after = bytes(self.mu.mem_read(NEXT - 8, 160));assert before[:8] == after[:8] and before[-8:] == after[-8:]
        if kind == 'R':
            assert self.reserve == 512 and self.words(NEXT + 0x80)[0] == HEAP and bytes(self.mu.mem_read(HEAP + 86, 8)) == b'\xa5' * 8
        state = [self.words(field)[0] for field in (COUNT, MY, OWNER, FORT, PORT)]
        return [kind, json.dumps(inputs, separators=(',', ':')), before.hex(), after.hex(), ','.join(map(str, state)), bytes(self.mu.mem_read(HANDLE, 64)).hex(), bytes(self.mu.mem_read(self.packet_address, 512)).hex() if kind == 'R' else '-', bytes(self.mu.mem_read(HEAP, 86)).hex() if kind == 'R' else '-', ';'.join(self.events) or '-']


def cases():
    """조건 단락 평가·현재값 재조회·카운터 감김과 오래 남는 예약 바이트를 교차한다."""
    for index, (seed, address, values, gate, variant) in enumerate(itertools.product((0, 91), ('7a6c6f63616c2c2a00', '000102030405060708'), ((6799, 0), (0xffffffff, 0xffffffff), (0, 7)), range(3), range(4))):
        yield 'R', [seed, address, *values, gate, variant, ('', 'Old', 'A' * 40, 'B' * 63)[index % 4], 1, True]
    for subscriber, supplied, gate, variant in itertools.product((0, 1, 0xffffffff), (False, True), range(3), range(3)):
        yield 'S', [7, '7a6c6f63616c2c2a00', 6799, 7, gate, variant, 'Kept', subscriber, supplied]


def main():
    """첫 원문만 생성하며 이후에는 모든 근거의 물리 SHA를 읽기 검증한다."""
    if '--verify' in sys.argv:
        facts = json.loads(REPORT.read_text('utf-8'))
        for name, sha in facts['sha256'].items():  # 최초 도구/보호 원문/fixture를 수정해 대조 결과를 바꾸지 않는다.
            assert digest(ROOT / name) == sha, name
        print('Guide 요청 최초 원문', facts['inputs'], '입력 보존');return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 fixture 덮어쓰기 금지'
    oracle = GuideOracle();rows = [];instructions = 0
    for kind, inputs in cases():  # C++ 기대값 생성기를 사용하지 않고 실제 PE 명령만 관측한다.
        rows.append(oracle.observe_guide(kind, inputs));instructions += oracle.instructions
    FIXTURE.write_text('# kind\tinput\tbefore160\tafter160\tcount,my,owner,fort,port\thandle64\tzone512\tcopy86\ttrace\n' + ''.join('\t'.join(row) + '\n' for row in rows), encoding='utf-8', newline='\n')
    evidence = ['AGENTS.md', 'originals/Netstorm.exe', 'extracted/refined/originals/raw.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_local_mission_transport.py', 'tools/decomp_mission_fort_request.py', 'tools/decomp_guide_fort_request.py', FIXTURE.relative_to(ROOT).as_posix()]
    facts = dict(unit=713, date='2026-11-02', host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', lastDecompileDate='2026-10-23', inputs=len(rows), requestInputs=sum(row[0] == 'R' for row in rows), subscriberInputs=sum(row[0] == 'S' for row in rows), instructions=instructions, gameExecutions=0, multiplayerTests=0, boundaries=['004b50b0/4b32f0/463b30와 기존 패킷 할당·슬롯·관찰기·복제 몸체만 격리 실행', 'Config/제목/Template Subscriber 쓰기·Notify/힙은 명시 효과이며 실제 GUI/전체 State1·7/32비트 FILE ABI가 아니다', '단독 미션의 Guide 요청이며 TCP/멀티플레이/Outpust 새 검사는 없다'], fixtureSha256=digest(FIXTURE), sha256={name: digest(ROOT / name) for name in evidence})
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print('Guide 요청 원문', len(rows), '입력', instructions, '명령')


if __name__ == '__main__':
    main()
