#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Form 공통 해제의 두 PE 기계어만 격리 관측한다. getter/종속 분리/진단은 명시 장치다."""
import argparse
import hashlib
import itertools
import json
import socket
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

POOL, VT = 0x13000000, 0x14000000  # 원본과 호스트를 수정하지 않는 명시 입력 메모리다.
GETTER = SCRATCH + 0x1000  # 실제 자식으로 주장하지 않는 가상 부모 getter다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/form-unpop-current-x86.tsv'  # 최초 기계어 결과만 동결한다.
SPEC = {  # CD 함수는 정밀 목록 누락이므로 PE의 ret 4까지 직접 확인한 범위만 허용한다.
    'originals': (0x4ae140, 0x4ae1d4, 0x5c8464, 0x5c847c, 50, 0x4e0620, 0x4ac6b0),
    'originalCD': (0x4af270, 0x4af30a, 0x5395dc, 0x5395f4, 36, 0x490040, 0x4ac160)}


class FormUnpopOracle(Oracle):
    """공통 Form 부모 명령 외의 게임/OS/통신은 실행하지 않는다."""
    def __init__(self, edition):
        """보호 PE와 수동 확인한 두 순수 함수 범위만 가상 주소에 읽는다."""
        super().__init__(edition, [], helpers=[])
        self.entry, end, self.pool, self.capacity, self.stride, self.assertion, self.detach = SPEC[edition]  # 판본별 실제 주소/폭이다.
        self.entries['unpop'] = self.entry
        self.allowed.append((self.entry, end))
        self.mu.mem_map(POOL, 0x10000)
        self.mu.mem_map(VT, 0x1000)
        self.events = []  # 원문 진단과 선택된 가상 호출 순서다.
        self.profile = 0  # 장치가 명시적으로 변경하는 현재 입력의 프로필이다.
        self.object = POOL  # 이번 입력의 같은 물리 객체 주소다.

    def put(self, address, value, width=4):
        """명시 입력/장치의 정수 폭만 기록한다."""
        self.mu.mem_write(address, value.to_bytes(width, 'little'))

    def get(self, address, width=4):
        """원문 메모리의 현재 정수 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address, width), 'little')

    def resume(self, value):
        """명시 자식의 ret 0만 공급하며 원문 부모의 ret 4는 직접 실행한다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4)
        self.mu.reg_write(UC_X86_REG_EIP, self.get(sp))

    def on_instruction(self, mu, address, size, data):
        """선택된 현재 진단/getter/종속 분리만 명시 정상 복귀시킨다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == self.assertion:
            expression = self.read_string(self.get(sp + 4)).decode('ascii')
            file = self.read_string(self.get(sp + 8)).decode('ascii')
            self.events.append(f'D{expression}|{file}|{self.get(sp + 12)}')
            if self.profile == 1:
                self.put(self.object + 10, 6, 1)
                self.put(self.object + 11, 8, 1)
                self.put(self.object, VT + 0x100)
            self.resume(0)
            return
        if address in (GETTER, GETTER + 0x10):
            self.events.append(f'P{VT + (0x100 if address == GETTER + 0x10 else 0)}')
            if self.profile == 2:
                self.put(POOL + 9 * self.stride + 11, self.get(POOL + 9 * self.stride + 11, 1) ^ 1, 1)
            self.resume(9)
            return
        if address == self.detach:
            assert mu.reg_read(UC_X86_REG_ECX) == self.object
            self.events.append(f'U{self.get(self.object + 11, 1)}')
            self.resume(0)
            return
        super().on_instruction(mu, address, size, data)

    def observe(self, case):
        """합성 초기 바이트 뒤 실제 trace와 두 슬롯 전체를 기록한다."""
        sid, number, state, parent_state, self.profile = case
        self.events = []
        self.object = POOL + sid * self.stride
        self.mu.mem_write(POOL, bytes((i * 37 + 19) & 255 for i in range(16 * self.stride)))
        self.put(self.pool, POOL)
        self.put(self.capacity, 32768)
        self.put(VT, GETTER)
        self.put(VT + 0x100, GETTER + 0x10)
        self.put(self.object, VT)
        self.put(self.object + 10, number, 1)
        self.put(self.object + 11, state, 1)
        self.put(POOL + 9 * self.stride + 11, parent_state, 1)
        self.call('unpop', [0], this=self.object, purge=4)
        slots = ';'.join(bytes(self.mu.mem_read(POOL + i * self.stride, self.stride)).hex() for i in (sid, 9))
        return ';'.join(self.events), slots


def main():
    """최초 관측은 새 파일로만 쓰고 재관측은 동결 파일을 읽기만 한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    rows = ['# Form 공통 부모만 실행. 진단/getter/Detach는 명시 장치이며 게임/MP/실제 자식은 미실행.']
    editions = {}
    cases = list(itertools.product((0, 5), (1, 2, 3, 6, 69, 70), (0, 1, 4, 8, 10, 12), (0, 1, 2, 3), (0, 1, 2)))
    for edition in SPEC:  # 두 보호 PE의 원문 결과를 각각 기록한다.
        oracle = FormUnpopOracle(edition)
        for case in cases:  # 기대값 계산 없이 최초 trace/두 슬롯 바이트만 저장한다.
            rows.append('\t'.join((edition, ','.join(map(str, case)), *oracle.observe(case))))
        editions[edition] = {'samples': len(cases), 'binarySha256': oracle.sha256}
    content = '\n'.join(rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == content
    else:
        with FIXTURE.open('x', encoding='utf-8', newline='\n') as stream:
            stream.write(content)
    report = dict(host=socket.gethostname(), mode='verify' if args.verify else 'first', samples=len(rows)-1,
                  editions=editions, fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
                  toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), actualChildren=False, game=False, multiplayer=False)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(report['mode'], report['samples'], 'Form Unpop 부모 관측', flush=True)


if __name__ == '__main__':
    main()
