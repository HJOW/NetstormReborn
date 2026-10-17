#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""커서의 실제 TYPE 번호 반환과 사정거리 판정을 외부 계산 대체 없이 격리 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_particle_gradient import Oracle as CallOracle, ROOT, STACK, STOP, digest
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX

TYPES = 0x12000000  # 실제 TYPE188칸을 원본 포인터 크기와500바이트 간격으로 공급하는 격리 주소다.
ENTRIES = (0x49adc0, 0x474240, 0x49a840)  # 번호 반환·사정거리·번호에서 TYPE 포인터 조회의 실제 함수다.
FIXTURE = ROOT/'cpppj/tests/fixtures/cursor-type-range-x86.tsv'  # 기존565수명 원문을 변경하지 않는 별도 관측이다.
REPORT = ROOT/'cpppj/cursor-type-range.json'  # 현재 호스트·정상 반환·실행 범위와 바이트 근거다.


def cases():
    """전체188번호와 그룹/플래그 경계를 독립 입력으로 만들며 판정 결과를 계산하지 않는다."""
    result = [(number, 0xffffffff, 0) for number in range(188)]  # 내장/자산 번호의 기본 입력이다.
    groups = (0x80000000, 0xffffffff, *range(11), 0x7fffffff)  # 음수·네 허용 그룹·인접 불허 그룹과 signed 끝이다.
    flags = (0, 1, 0x100, 0x200, 0x400, 0x800, 0x80000000, 0xffffffff)  # 단독 비트와 조합 입력이다.
    # 번호0/69/70/187은 내장과 자산의 경계이며92/128은 비용 그림과 썬 캐논이다.
    for number, group, flag in itertools.product((0, 69, 70, 92, 128, 187), groups, flags):
        result.append((number, group, flag))
    return list(dict.fromkeys(result))


class Oracle(CallOracle):
    """세 원본 몸체만 실행하며 TYPE 입력과 프로그램 바이트는 실행 중 쓰지 못한다."""
    def __init__(self):
        """보호 PE를 읽고 정밀 함수표가 지정한 실제 명령 범위만 허용한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # OS·창·실제 게임 프로세스 없이 CPU 명령만 실행한다.
        image = pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image()  # 보호 파일의 읽기 전용 바이트다.
        self.mu.mem_map(0x400000, (len(image)+4095)&~4095); self.mu.mem_write(0x400000, image)
        # 원본 주소와 겹치지 않는 TYPE·호출 스택·정상 반환 지점이다.
        for address, size in ((TYPES, 0x20000), (STACK-0x7f000, 0x80000), (STOP, 0x1000)):
            self.mu.mem_map(address, size)
        self.mu.mem_write(0x59ab20, struct.pack('<I', TYPES))
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []  # 명령의 끝 바이트도 범위 안에 있어야 한다.
        # 외부 경계로 반환을 공급하지 않고 세 함수의 실제 본문을 모두 허용한다.
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high+1))
        self.instructions = 0  # 실행한 실제 명령의 누적 수다.
        self.calls = Counter()  # 세 실제 함수의 진입 횟수다.
        self.returns = 0  # ABI/x87/정상 복귀 검사를 모두 통과한 호출 수다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction); self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """허용 범위 밖 실행을 즉시 거부하고 실제 진입·명령만 집계한다."""
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'커서 TYPE 범위 밖 실행 {address:08x}')
        self.instructions += 1
        if address in ENTRIES: self.calls[f'{address:08x}'] += 1

    def write(self, mu, access, address, size, value, data):
        """원본이 호출 스택 밖의 TYPE/전역/PE를 변경하면 관측을 거부한다."""
        if not STACK-0x7f000 <= address <= STACK+0x1000-size:
            raise RuntimeError(f'커서 TYPE 범위 밖 쓰기 {address:08x}')

    def sample(self, case, precision):
        """현재 TYPE의 두 입력 필드를 주고 getter와 predicate의 실제 EAX를 읽는다."""
        number, group, flags = case  # 결과와 분리한 명시 입력이다.
        pointer = TYPES+number*500  # 원본 TYPE 포인터 입력의500바이트 간격이다.
        self.mu.mem_write(pointer+0x9c, struct.pack('<I', group)); self.mu.mem_write(pointer+0xec, struct.pack('<I', flags))
        self.call(0x49adc0, pointer, b'', 0, precision); self.returns += 1
        actual_number = self.mu.reg_read(UC_X86_REG_EAX)  # 원본 getter가 반환한 값을 다음 실제 함수에 그대로 준다.
        self.call(0x474240, 0, struct.pack('<I', actual_number), 0, precision); self.returns += 1
        return actual_number, self.mu.reg_read(UC_X86_REG_EAX)


def main():
    """두 x87 정밀도의 원문 결과를 비교하여 새 근거를 저장하거나 현재 SHA만 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if args.verify:
        facts = json.loads(REPORT.read_text(encoding='utf-8'))
        # 현재 근거의 모든 바이트를 검사하며 저장된 기대값은 재계산하지 않는다.
        for path, wanted in facts['sha256'].items(): assert digest(ROOT/path) == wanted, path
        rows = [line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == facts['uniqueInputs'] == len(set(rows)); print('커서 TYPE 번호/사정거리 SHA 감사:', len(rows)); return
    oracles = [Oracle(), Oracle()]; rows = []  # 두 실행기는 입력 이외의 메모리를 공유하지 않는다.
    # 기대값은 C++/Python 판정식 대신 두 실제 PE 실행의 반환값으로만 만든다.
    for case in cases():
        results = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x027f, 0x037f))]
        assert results[0] == results[1]; rows.append((*case, *results[0]))
    FIXTURE.write_text('# type group flags2 actualTypeNumber actualRange\n'+'\n'.join('\t'.join(map(str, row)) for row in rows)+'\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_cursor_type_range.py', FIXTURE.relative_to(ROOT).as_posix(), 'cpppj/src/client/RawCursorLifecycle.h', 'cpppj/src/client/RawCursorLifecycle.cpp', 'cpppj/tests/CursorLifecycleTests.cpp', 'cpppj/tests/PreviewDrawTests.cpp')  # 관측과 제품 연결의 현재 바이트 근거다.
    facts = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', entries=[f'{entry:08x}' for entry in ENTRIES], uniqueInputs=len(rows), normalReturns=sum(oracle.returns for oracle in oracles), instructions=sum(oracle.instructions for oracle in oracles), actualEntries=dict(sum((oracle.calls for oracle in oracles), Counter())), precisionDifferentInputs=[], sha256={path:digest(ROOT/path) for path in paths}, limitations=['49adc0·474240·49a840의 실제 몸체와 호출 ABI를 실행했다. 전체 게임/GUI 실행은 아니다.', '기존565수명 관측의 getter/predicate 경계 입력은 그대로 보존한다.', '마나/사정거리 프로세스 생성·프레임·표시·Client 입력 연결은 남아 있다.'])
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n'); print('커서 TYPE 번호/사정거리:', len(rows), '고유 입력, 두 정밀도 차이0, 정상 반환', facts['normalReturns'])


if __name__ == '__main__': main()
