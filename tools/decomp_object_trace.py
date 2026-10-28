#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""객체 trace의 재귀·현재 링크·들여쓰기를 원본 x86으로 독립 관측한다."""
import csv
import hashlib
import json
import random
import socket
from decomp_process_summary import SummaryOracle
from decomp_bridgedecay_oracle import (DecayOracle, ROOT, POOL, HEAP,
    UC_X86_REG_ECX, UC_X86_REG_ESP)

# 재귀 본체·모든 Form 탐색기·로그 들여쓰기와 기본 true 필터의 실제 주소다.
ENTRIES = (0x44dab0, 0x4b22d0, 0x4b1b80, 0x4c2600)
# 가상 trace 장치만 데이터 가상 표에서 이 별도 진입점으로 연결한다.
TRACE = 0x20000100
# 현재 진단 깊이와 로그의 실제 들여쓰기 전역이다.
DEPTH, INDENT = 0x55a4a0, 0x5ca8d4
# 이 단위의 원시 자료와 새 고정 기대값 경로다.
WORK = ROOT / 'extracted/cpp-object-trace-554-20261029'
FIXTURE = ROOT / 'cpppj/tests/fixtures/object-trace-x86.tsv'


class TraceOracle(SummaryOracle):
    """Form 필터/순회는 실제 코드로 실행하고 가상 출력·로그·assert만 기록한다."""
    def __init__(self):
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.entries = (*ENTRIES, self.number(0x5019e8))  # 실제 기본 finder 필터는 입력을 통과시키는 원문이다.
        for entry in self.entries:  # 필요한 몸체의 불연속 범위만 허용하며 PE 코드를 덮어쓰지 않는다.
            for part in rows[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed[low-self.base:high+1-self.base] = b'\x01'*(high+1-low)
                self.bodies.append((low, bytes(self.mu.mem_read(low, high+1-low))))
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], Trace=0x44dab0))

    def on_write(self, mu, access, address, size, value, data):
        """깊이·들여쓰기 두 DWORD만 이 함수의 추가 쓰기로 허용한다."""
        if address in (DEPTH, INDENT) and size == 4:
            return
        super().on_write(mu, access, address, size, value, data)

    def mutate(self, sid):
        """첫 가상 trace 반환의 명시 외부 효과로 현재 슬롯/전역만 변경한다."""
        if self.calls != 1:
            return
        if self.mutation == 1:
            self.number(POOL+sid*50+6, 7, 2)
        elif self.mutation == 2:
            self.number(POOL+3*50+4, 7, 2)
        elif self.mutation == 3:
            self.number(POOL+4*50+4, 7, 2)
        elif self.mutation == 4:
            self.number(DEPTH, 41)
        elif self.mutation == 5:
            self.number(POOL+3*50+11, 1, 1)
        elif self.mutation == 6:
            self.number(POOL+4*50+10, 70, 1)
        elif self.mutation == 7:
            self.number(0x5c847c, 4)

    def on_instruction(self, mu, address, size, data):
        """출력 경계의 인자·호출 시 현재 두 깊이와 선택된 가상 SID를 기록한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == TRACE:
            sid = self.sid_of(mu.reg_read(UC_X86_REG_ECX))
            self.events.append(f'V:{sid}:{self.number(DEPTH)}:{self.number(INDENT)}')
            self.calls += 1
            self.mutate(sid)
            # 자식 호출 뒤 저장된 next가 유지되는지도 별도 외부 변형으로 관측한다.
            if self.calls == 2 and self.mutation == 8:
                self.number(POOL+3*50+4, 7, 2)
            if self.calls == 2 and self.mutation == 9:
                self.number(POOL+4*50+4, 7, 2)
            self.returned()
            return
        if address == 0x4c2600:
            self.events.append(f'I:{self.number(sp+4)}')  # 실제 setter 본문은 계속 실행한다.
        if address == 0x4c2630:
            level, fmt = self.number(sp+4), self.text(self.number(sp+8))
            assert fmt in (b'sid %d is free\n', b'~1~yEND %04d\n')
            line = fmt % self.number(sp+12)
            self.events.append(f'L:{level}:{line.hex()}:{self.number(DEPTH)}:{self.number(INDENT)}')
            if self.mutation == 10:
                self.number(DEPTH, self.number(DEPTH)+2)
            self.returned()
            return
        if address == 0x4e0620:
            self.events.append('A:'+self.text(self.number(sp+4)).hex())
            self.returned()
            return
        DecayOracle.on_instruction(self, mu, address, size, data)

    def sample(self, case, control):
        """출력 계산 없이 독립 입력의 물리 슬롯·전역·외부 변경 번호를 설치한다."""
        sid, count, depth, indent, checking, metamap, boss, change, nodes = case
        self.calls, self.mutation = 0, change  # 가상 호출 횟수와 외부 변경의 명시 입력 번호다.
        self.mu.mem_write(POOL, bytes(24000*50))
        self.number(DEPTH, depth)
        self.number(INDENT, indent)
        self.number(0x5c847c, count)
        self.number(0x5e4794, checking)
        self.number(0x594fc8, metamap)
        self.number(0x540bc4, boss)
        self.number(HEAP+0x8000+0x70, TRACE)
        for number, kind, flags, head, next_sid in nodes:  # 입력에 적힌 슬롯만 원문50BYTE 배열에 설치한다.
            pointer = POOL+number*50
            self.number(pointer, HEAP+0x8000)
            self.number(pointer+4, next_sid, 2)
            self.number(pointer+6, head, 2)
            self.number(pointer+10, kind, 1)
            self.number(pointer+11, flags, 1)
        self.call('Trace', [sid], control=control)
        assert all(bytes(self.mu.mem_read(address, len(body))) == body for address, body in self.bodies)
        return ';'.join(self.events)+f'|{self.number(DEPTH)}|{self.number(INDENT)}'


def cases():
    """잎/다단/경계/상태/외부 변경과 고정seed의 유한 트리 입력을 반환한다."""
    nodes = [(2, 82, 0, 3, 0), (3, 46, 0, 5, 4), (4, 69, 0, 0, 6),
             (5, 6, 0, 0, 0), (6, 70, 0, 0, 7), (7, 0, 0, 0, 0)]
    basic = (2, 20, 0, 17, 0, 0, 0, 0, nodes)
    result = [basic, (0, *basic[1:]), (2, 20, 0xffffffff, 9, 0, 0, 1, 0, nodes)]
    for change in range(1, 11):  # 각 명시 콜백 변경은 기본 순회와 같은 입력에서 따로 실행한다.
        result.append((*basic[:7], change, nodes))
    for flags in (1, 2, 4, 8, 16, 255):  # free 이외 상태 비트가 가상 trace를 막지 않는지 구별한다.
        changed = [(n, k, flags if n == 2 else f, h, v) for n, k, f, h, v in nodes]
        result.append((2, 20, 12, 15, 1, 0, 0, 0, changed))
    result.extend([(2, 20, 12, 15, 1, 1, 0, 0, [(2, 82, 2, 0, 0)]),
                   (2, 20, 12, 15, 1, 0, 1, 0, [(2, 82, 2, 0, 0)]),
                   (2, 4, 12, 15, 0, 0, 1, 0, nodes),
                   (2, 20, 12, 15, 0, 0, 1, 0, [(2, 82, 0, 0, 0)])])
    rng = random.Random(0x44dab0)  # 입력만 고정하며 기대 출력에는 C++ 계산을 사용하지 않는다.
    for _ in range(80):  # 부모번호가 항상 작아 순환 없는 다단 종속 입력을 만든다.
        children = {number: [] for number in range(2, 18)}
        for number in range(3, 18):
            children[rng.randrange(2, number)].append(number)
        generated = []
        for number in range(2, 18):
            parent = next((owner for owner, values in children.items() if number in values), None)
            siblings = children[parent] if parent is not None else [number]
            index = siblings.index(number)
            generated.append((number, rng.choice((0, 1, 2, 6, 46, 63, 69, 70, 82, 187)),
                              rng.choice((0, 0, 0, 1, 2, 4, 8, 16)),
                              children[number][0] if children[number] else 0,
                              siblings[index+1] if index+1 < len(siblings) else 0))
        result.append((2, 20, rng.randrange(40), rng.randrange(40), 0, 0, 1, 0, generated))
    return result


def main():
    """독립53/64비트 실행의 전체 경계 기록을 비교한 뒤 고정 fixture를 저장한다."""
    oracle = TraceOracle()
    rows = ['# sid count depth indent checking metamap boss change nodes => 원문 호출/최종전역']
    inputs = cases()
    for case in inputs:  # 같은 입력을 두 정밀도로 새로 설치하여 상태 누적을 피한다.
        a, b = (oracle.sample(case, control) for control in (0x27f, 0x37f))
        assert a == b
        packed = '/'.join(','.join(map(str, node)) for node in case[-1])
        rows.append('\t'.join(map(str, (*case[:-1], packed, a))))
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), lastFullDecompilation='HJOW-Athlon / 2026-10-23',
                  uniqueInputs=len(inputs), nativeObservations=len(inputs)*2,
                  binarySha256=oracle.sha256, entries=[f'{value:08x}' for value in oracle.entries],
                  fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
                  boundary='0044dab0/004b22d0/004b1b80/004c2600/기본true 실제 실행; 가상+70·로그·assert만 명시 출력 경계. 게임/OS 실행0.')
    WORK.mkdir(parents=True, exist_ok=True)
    (WORK/'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
