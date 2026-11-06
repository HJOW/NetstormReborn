#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CD SID 할당의 진단·고정 후보 회수와 실제 Native 반납을 창 없이 관측한다."""
import csv
import hashlib
import json
import socket
import struct
import zlib
from pathlib import Path
from decomp_sid_oracle import SidOracle, ROOT, POOL, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

WORK = ROOT / 'extracted/cpp-resume-977/oracle'  # 정밀 범위·분석 중단·최종 원문 관측을 보관한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/sid-cd-allocation-recovery-x86.tsv'  # 기존 PATCH fixture와 분리한 독립 CD 관측이다.
FAKE_TABLE, FAKE_KILL, RELEASE_RETURN = SCRATCH + 0x1000, SCRATCH + 0x1100, SCRATCH + 0x1200  # 원문 VT 호출의 명시 외부 효과 경계다.
BOSS, BRIDGE, DISALLOW = 0x540a2c, 0x51ca8c, 0x5395c4  # CD 현재 권한·전체 DWORD 종류·할당 금지 전역이다.
CAPACITY, STRIDE, PREDICTABLE = 14005, 36, 14000  # 실제 CD 레이아웃의 작은 합법 Pool 입력이다.
DUMP, DETAIL, TELL, LOOP = 0x41f3e0, 0x4875b0, 0x4c3dc0, 0x4aaf45  # 원문 외부 부작용과 고정 후보 루프의 실제 입구다.


class CallbackStop(Exception):
    """명시 외부 장치 실패 또는 원문 비진전 관측을 부분 상태와 함께 끝낸다."""
    def __init__(self, kind):
        """kind1은 외부 실패, kind2는 같은 고정 후보의 무효과 반복이다."""
        super().__init__(kind)
        self.kind = kind  # 분석 중단 종류이며 원문 성공 반환값을 대신하지 않는다.


class CdAllocationOracle(SidOracle):
    """현재 CD 정밀 범위의 Reset/Allocate/Release/기록 CRT만 실제 실행한다."""
    def __init__(self):
        """제품 코드를 읽지 않고 canonical 범위와 보호 원본 PE만 준비한다."""
        WORK.mkdir(parents=True, exist_ok=True)
        with (ROOT / 'extracted/refined/originalCD/functions.tsv').open(encoding='utf-8') as source:
            rows = {int(row['entry'], 16): row for row in csv.DictReader(source, delimiter='\t')}
        wanted = (0x4aaad0, 0x4aae40, 0x4ab250, 0x4cff40, 0x4f1a60)  # 재할당 malloc·진단·UI를 실행하지 않는 실제 원문 집합이다.
        facts = WORK / 'allocation-functions.tsv'
        with facts.open('w', encoding='utf-8', newline='') as output:
            writer = csv.DictWriter(output, fieldnames=['entry', 'ranges'], delimiter='\t', lineterminator='\n')
            writer.writeheader()
            for entry in wanted:  # 확인한 함수의 현재 불연속 몸체 범위를 그대로 기록한다.
                writer.writerow({key: rows[entry][key] for key in ('entry', 'ranges')})
        super().__init__('originalCD', facts)
        self.code_base = min(low for low, _ in self.allowed)  # 같은 허용 집합을 빠르게 검사하기 위한 기준 주소다.
        self.code_allowed = bytearray(max(high for _, high in self.allowed) - self.code_base)  # 원문 코드의 실제 허용 바이트 집합이다.
        for low, high in self.allowed:  # 선별 범위만 표시하며 다른 PE 함수의 실행은 거부한다.
            self.code_allowed[low - self.code_base:high - self.code_base] = b'\1' * (high - low)
        self.events = []  # 단언·dump·detail·Tell·가상 삭제의 실제 호출 순서다.
        self.mode = 0  # 명시 raw 입력과 외부 장치의 효과 선택값이다.
        self.kills = 0  # 같은 후보를 반복하는 원문 가상 삭제의 진입 횟수다.
        self.tracing = False  # 준비 Reset/Allocate는 관측 사건에 포함하지 않는다.
        self.stall = None  # 효과 없는 같은 루프 입력의 직전 값이며 제품 동작을 바꾸지 않는다.
        self.reset_cache = {}  # 같은 권한 입력의 Native Reset 전체 메모리를 재사용하여 반복 초기화 비용만 줄인다.

    def number(self, address, value=None, width=4):
        """가상 원문 메모리의 현재 little endian 정수만 읽거나 쓴다."""
        if value is None:
            return int.from_bytes(self.mu.mem_read(address, width), 'little')
        self.mu.mem_write(address, (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little'))

    def text(self, address):
        """원문 NUL 종료 문자열의 실제 바이트를 복사한다."""
        result = bytearray()
        while (value := self.number(address + len(result), width=1)) != 0:  # 문자열 범위를 NUL까지만 읽는다.
            result.append(value)
        return bytes(result)

    def returned(self):
        """명시 외부 cdecl 경계만 정상 복귀시키고 인자 정리는 호출자에 맡긴다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EIP, self.number(sp))
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4)
        self.mu.reg_write(UC_X86_REG_EAX, 0)

    def candidate(self, sid, type_id=82):
        """명시 외부 삭제 장치가 받는 live/void 슬롯과 고정 가상 표를 놓는다."""
        self.number(POOL + sid * STRIDE, FAKE_TABLE)
        self.number(POOL + sid * STRIDE + 10, type_id, 1)
        self.number(POOL + sid * STRIDE + 11, 4, 1)

    def on_instruction(self, mu, address, size, data):
        """외부 경계를 기록하고 나머지 현재 정밀 함수의 실제 명령만 허용한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == self.spec['assert']:
            expression, file, line = self.text(self.number(sp + 4)), self.text(self.number(sp + 8)), self.number(sp + 12)
            if self.tracing:
                self.events.append(f'A:{line}:{expression.hex()}:{file.hex()}')
            if self.mode == 1 and line == 0xe8:
                self.number(DISALLOW, 0)
            if self.mode == 11 and line == 0xf0:
                self.number(POOL + PREDICTABLE * STRIDE + 11, 1, 1)
            if self.mode == 20 and line == 0x118:
                self.number(BOSS, 0)
            self.returned()
            return
        if address in (DUMP, DETAIL, TELL):
            event = 'D' if address == DUMP else ('V:' if address == DETAIL else 'T:') + self.text(self.number(sp + 4)).hex()
            if self.tracing:
                self.events.append(event)
            if (self.mode == 18 and address == DUMP) or (self.mode == 19 and address == DETAIL):
                raise CallbackStop(1)
            self.returned()
            return
        if address == FAKE_KILL:
            sid = (mu.reg_read(UC_X86_REG_ECX) - POOL) // STRIDE
            assert self.number(sp + 4) == 0
            self.kills += 1
            if self.tracing:
                self.events.append(f'K:{sid}:0')
            if self.mode == 6 and self.kills > 1:
                mu.reg_write(UC_X86_REG_EIP, self.number(sp))
                mu.reg_write(UC_X86_REG_ESP, sp + 8)
                return  # 같은 후보의 나머지49회는 명시 멱등 외부 효과이며 Release를 거듭 호출하지 않는다.
            self.mu.mem_write(sp - 8, struct.pack('<II', RELEASE_RETURN, sid))
            mu.reg_write(UC_X86_REG_ESP, sp - 8)
            mu.reg_write(UC_X86_REG_EIP, self.entries['Release'])
            return
        if address == RELEASE_RETURN:
            original_sp = sp + 4  # Native cdecl Release의 인자를 원래 ret4 가상 호출과 구별한다.
            sid = self.number(sp)
            if self.mode == 6:
                self.candidate(sid)  # 첫 Native Release 뒤 VT/void를 보존하는 명시 외부 효과이며 자연 삭제가 아니다.
            if self.kills == 1:
                if self.mode == 8:
                    self.number(BRIDGE, 70)
                elif self.mode == 9:
                    raise CallbackStop(1)
            mu.reg_write(UC_X86_REG_EIP, self.number(original_sp))
            mu.reg_write(UC_X86_REG_ESP, original_sp + 8)
            return
        if address == LOOP:
            from unicorn.x86_const import UC_X86_REG_EBX, UC_X86_REG_EDX  # 원문 EBX 고정 offset과 현재 Pool 기준을 읽는다.
            candidate = mu.reg_read(UC_X86_REG_EDX) + mu.reg_read(UC_X86_REG_EBX)
            current = (candidate, self.number(candidate + 10, width=1), self.number(BRIDGE), self.number(self.spec['globals'][6]))
            if current[1] != current[2] and current == self.stall:
                raise CallbackStop(2)  # 동일 후보·종류·용량의 두 무효과 반복만 관측 중단하며 성공값을 만들지 않는다.
            self.stall = current if current[1] != current[2] else None
        self.instructions += 1
        if address == self.spec['copy']:
            _ret, dest, source, length = struct.unpack('<4I', mu.mem_read(sp, 16))
            assert length == 152 and source in self.spec['logs'] and dest == source + 8
            self.copies += 1
        offset = address - self.code_base
        if not 0 <= offset <= len(self.code_allowed) - size or not all(self.code_allowed[offset:offset + size]):
            raise RuntimeError(f'CD 할당 정밀 범위 밖 명령 {address:08x}')

    def state_without_result(self):
        """원문 정상 반환이 없는 중단에서는 반환값0과 실제 부분 상태만 기록한다."""
        glob = self.spec['globals']
        return [0, self.number(glob[5]), self.number(glob[7]), self.number(POOL + 4, width=2), (self.number(glob[2]) - POOL) // STRIDE, self.number(POOL + 18, width=2), (self.number(glob[4]) - POOL) // STRIDE, zlib.adler32(self.mu.mem_read(POOL, CAPACITY * STRIDE)), zlib.adler32(b''.join(bytes(self.mu.mem_read(a, 160)) for a in self.spec['logs']))]

    def observe(self, mode, flags, server, boss):
        """같은 합법 Pool을 초기화한 뒤 명시 raw 입력/외부 효과만 적용한다."""
        self.mode = 0
        self.tracing = False
        self.begin(CAPACITY, bool(server))
        self.number(DISALLOW, 0)
        self.number(BOSS, boss)
        self.number(BRIDGE, 82)
        if server not in self.reset_cache:
            self.step('Reset')
            self.reset_cache[server] = [(POOL, bytes(self.mu.mem_read(POOL, CAPACITY * STRIDE))), *((address, bytes(self.mu.mem_read(address, 4))) for address in self.spec['globals'][:8]), *((address, bytes(self.mu.mem_read(address, 160))) for address in self.spec['logs'])]
        else:
            for address, content in self.reset_cache[server]:  # 같은 Native Reset의 전체 Pool·전역·기록을 원본 바이트 그대로 복구한다.
                self.mu.mem_write(address, content)
        self.number(FAKE_TABLE + 0x10, FAKE_KILL)
        client = bool(flags & 2)
        head = POOL + (0 if client else 14)
        tail = (self.number(self.spec['globals'][2 if client else 4]) - POOL) // STRIDE
        if mode == 15:
            for _ in range(2):  # 예측 커서의 준비 증가도 Native 정상 할당으로 만든다.
                self.step('Allocate', 1)
        if mode == 1:
            self.number(DISALLOW, 1)
        if mode == 3:
            self.number(POOL + self.number(head + 4, width=2) * STRIDE + 11, 0, 1)
        if mode in (4, 5, 6, 7, 8, 9, 17, 18, 19, 20):
            self.number(head + 4, tail, 2)
        if mode in (6, 7, 8, 9, 17):
            self.candidate(10, 70 if mode == 7 else 82)
        if mode == 17:
            self.number(BRIDGE, 0x10052)
        if mode == 11:
            self.number(POOL + PREDICTABLE * STRIDE + 11, 0, 1)
        if mode == 12:
            self.number(POOL + (PREDICTABLE + 1) * STRIDE + 11, 0, 1)
        if mode in (13, 14):
            self.number(POOL + (PREDICTABLE + 1) * STRIDE + 4, PREDICTABLE + (4 if mode == 13 else 0), 2)
        self.mode, self.events, self.kills, self.tracing, self.stall = mode, [], 0, True, None
        stopped = 0
        try:
            result = self.step('Allocate', flags)
        except CallbackStop as error:
            stopped = error.kind
            result = self.state_without_result()
        self.tracing = False
        return [stopped, *result, self.number(BRIDGE), self.number(DISALLOW), ';'.join(self.events) or '-']


def main():
    """CD 진단·고정 후보·정상 예측의 독립 관측을 동결하거나 기존 결과와 비교한다."""
    oracle = CdAllocationOracle()
    cases = []
    for mode in (0, 1, 3, 4, 5, 6, 7, 8, 9, 17, 18, 19):  # 두 일반 목록과 선택한 외부 실패·비진전 경계다.
        for flags in (0, 2):  # server/client 소진의 다른 선행 dump·진단을 각각 관측한다.
            cases.append((mode, flags, 1, 0 if mode == 4 else 1))
    cases.extend(((0, 0, 0, 0), (1, 2, 0, 0), (6, 2, 0, 1), (20, 2, 1, 1)))
    for mode in (10, 11, 12, 13, 14, 15):  # 예측 할당은 두 플래그 우선순위와 서버 분기를 구별한다.
        for flags in (1, 3):  # bit2가 있어도 predictable bit1이 먼저 선택된다.
            for server in (0, 1):  # next 검사/머리 쓰기는 서버 현재값에만 의존한다.
                cases.append((mode, flags, server, 1))
    rows = ['# mode\tflags\tserver\tboss\tstop\tresult\tfree\tcursor\tch\tct\tsh\tst\tpoolAdler\tlogsAdler\tbridge\tdisallow\tevents']
    for case in cases:  # 모든 명시 입력의 반환/부분 상태/호출 순서를 빠짐없이 고정한다.
        rows.append('\t'.join(map(str, (*case, *oracle.observe(*case)))))
    content = ('\n'.join(rows) + '\n').encode('utf-8')
    if FIXTURE.exists():
        assert FIXTURE.read_bytes() == content, '기존 CD 할당 fixture와 원문 관측 불일치'
    else:
        FIXTURE.write_bytes(content)
    report = dict(host=socket.gethostname(), lastFullDecompilation='HJOW-Athlon / 2026-11-03', inputs=len(cases), binarySha256=oracle.sha256, fixtureSha256=hashlib.sha256(content).hexdigest(), fakeTable=FAKE_TABLE, stopKinds={'0': '원문 정상 반환', '1': '명시 외부 장치 예외', '2': '현재 고정 후보의 동일 무효과 루프 두 번 관측'}, boundary='CD Reset/Allocate/Release/기록 CRT 실제 원문. dump/detail/Tell/단언은 호출 기록 및 명시 외부 효과. 같은 후보50회는첫Native Release후VT/void복원과나머지49회멱등입력. 게임/MP/자연삭제 아님.')
    (WORK / 'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
