#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""감시 쌍 생성 부모의 두 PE 명령만 격리 관측한다. 실제 Create/부착·게임·통신은 실행하지 않는다."""
import argparse
import hashlib
import itertools
import json
import socket
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

POOL, VT, ATTACH = 0x13000000, 0x14000000, SCRATCH+0x1000  # 같은 객체/가상 호출을 표현하는 격리 입력 주소다.
FIXTURE = ROOT/'cpppj/tests/fixtures/depend-create-parent-current-x86.tsv'  # 첫 유효 관측 바이트만 새 파일에 동결한다.
SPEC = {  # 현재 정밀 소스와 보호 PE의 실제 함수/전역이다.
    'originals': dict(entry=0x4b1510, pool=0x5c8464, stride=50, type=0x541074, server=0x540bc0, create=0x4af530, assertion=0x4e0620),
    'originalCD': dict(entry=0x4af940, pool=0x5395dc, stride=36, type=0x51c964, server=0x540a28, create=0x4ab390, assertion=0x490040)}


class DependOracle(Oracle):
    """원문 부모 외의 Create·가상 부착·진단은 명시 장치로 정상 복귀한다."""
    def __init__(self, edition):
        """함수 범위와 별도 객체/VT 입력만 준비한다."""
        self.s = SPEC[edition]  # 원문 판본별 주소/슬롯 폭이다.
        super().__init__(edition, [dict(name='pair', **{edition: f'{self.s["entry"]:08x}'})], helpers=[])
        self.mu.mem_map(POOL, 0x10000)
        self.mu.mem_map(VT, 0x1000)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_word)
        self.events = []  # 부모의 진단/두 Create/WORD/부착 순서다.
        self.profile = 0  # 정상 복귀/쓰기/첫 부착 중 외부 변경 입력이다.
        self.creates = 0  # 명시 Create가 두 객체를 순서대로 반환한다.

    def put(self, address, value, width=4):
        """명시 장치의 정수 폭만 입력 메모리에 쓴다."""
        self.mu.mem_write(address, (value & ((1 << (width*8))-1)).to_bytes(width, 'little'))

    def get(self, address, width=4):
        """계산 모델 없이 원문 메모리의 현재 정수를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address, width), 'little')

    def resume(self, value=0, purge=0):
        """선택된 명시 자식의 cdecl/thiscall 스택 복구만 공급한다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp+4+purge)
        self.mu.reg_write(UC_X86_REG_EIP, self.get(sp))

    def on_word(self, mu, access, address, size, value, data):
        """원문 링크 WORD 쓰기만 관측하고 지정 프로필의 VT 변경을 입력한다."""
        for sid in (5, 9):  # 결과 두 객체 외의 메모리 쓰기는 기대값으로 만들지 않는다.
            offset = address-POOL-sid*self.s['stride']
            if size == 2 and offset in (18, 20):
                self.events.append(f'W{sid},{offset},{value}')
                if self.profile == 3 and sid == 9 and offset == 18:
                    self.put(POOL+5*self.s['stride'], VT+0x200)

    def on_instruction(self, mu, address, size, data):
        """두 Create의 현재 DWORD와 저장 VT/다음 현재 VT의 가상 호출을 관측한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == self.s['assertion']:
            expression = self.read_string(self.get(sp+4)).decode('ascii')
            file = self.read_string(self.get(sp+8)).decode('ascii')
            self.events.append(f'D{expression}|{file}|{self.get(sp+12)}')
            if self.profile == 2:
                self.put(self.s['type'], 0xffffffff)
                self.put(self.s['server'], 1)
            self.resume()
            return
        if address == self.s['create']:
            number, flags = self.get(sp+4), self.get(sp+8)
            sid = 5 if self.creates == 0 else 9
            self.events.append(f'C{number},{flags},{sid}')
            self.put(POOL+sid*self.s['stride'], VT+(0 if sid == 5 else 0x100))
            self.put(POOL+sid*self.s['stride']+10, number, 1)
            self.put(POOL+sid*self.s['stride']+11, 4, 1)
            if self.profile == 1 and self.creates == 0:
                self.put(self.s['type'], 0x12345678)
            self.creates += 1
            self.resume(POOL+sid*self.s['stride'])
            return
        if address in (ATTACH, ATTACH+0x10, ATTACH+0x20):
            sid = (mu.reg_read(UC_X86_REG_ECX)-POOL)//self.s['stride']
            table = VT+(address-ATTACH)*16
            self.events.append(f'A{sid},{table},{self.get(sp+4)},{self.get(sp+8)}')
            if self.profile == 4 and sid == 5:
                self.put(POOL+9*self.s['stride'], VT+0x200)
            self.resume(purge=8)
            return
        super().on_instruction(mu, address, size, data)

    def observe(self, case):
        """합성 입력 뒤 원문 trace/현재 전역/두 슬롯 전체를 읽는다."""
        watched, watcher, server, self.profile = case
        self.events, self.creates = [], 0
        self.mu.mem_write(POOL, bytes((index*37+19) & 255 for index in range(16*self.s['stride'])))
        self.put(self.s['pool'], POOL)
        self.put(self.s['type'], 7)
        self.put(self.s['server'], server)
        for index in range(3):  # 세 명시 가상 장치의 실제 VT+74 토큰만 입력한다.
            self.put(VT+index*0x100+0x74, ATTACH+index*0x10)
        self.call('pair', [watched, watcher])
        slots = ';'.join(bytes(self.mu.mem_read(POOL+sid*self.s['stride'], self.s['stride'])).hex() for sid in (5, 9))
        return str(self.get(self.s['type'])), str(self.get(self.s['server'])), ';'.join(self.events), slots


def main():
    """첫 관측은 새 파일로만 쓰고 재관측은 기존 바이트를 검증한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    parents = (0, 4, 5, 5999, 6000, 14999, 15000, 32768, 0xffffffff)
    cases = list(itertools.product(parents, parents, (0, 1), range(5)))
    rows = ['# 감시 쌍 생성 부모만 실행. Create/진단/가상 부착과 쓰기 중 VT 변경은 명시 장치. 게임/MP 미실행.']
    binaries = {}
    for edition in SPEC:  # 두 보호 PE에서 같은 부모/현재 전역 입력을 각각 실행한다.
        oracle = DependOracle(edition)
        for case in cases:  # 반환값 모델 없이 실제 trace/현재 전역/두 raw 슬롯만 동결한다.
            rows.append('\t'.join((edition, ','.join(map(str, case)), *oracle.observe(case))))
        binaries[edition] = oracle.sha256
    data = ('\n'.join(rows)+'\n').encode('utf-8')
    if args.verify:
        assert FIXTURE.read_bytes() == data
    else:
        with FIXTURE.open('xb') as stream:
            stream.write(data)
    report = dict(host=socket.gethostname(), rows=len(rows)-1, rowsPerEdition=len(cases), binaries=binaries,
                  fixtureSha256=hashlib.sha256(data).hexdigest(), toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  mode='verify' if args.verify else 'first', actualCreate=False, actualAttach=False, game=False, multiplayer=False)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(report['mode'], report['rows'], 'Depend 생성 부모 관측', flush=True)


if __name__ == '__main__':
    main()
