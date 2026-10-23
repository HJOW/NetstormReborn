#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 c79b0의 실제 버튼/Enter/메뉴 종료 분기와 40칸 큐를 격리 관측한다."""
import argparse
import csv
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, POOL, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG)

NODE = TEXT + 0x1000  # 실제 부모+21e8 범위를 다른 가상 입력과 분리한다.
ENTRY = 0x4c79b0  # 실제 HTML 부모의 Handle 전체다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/dialog-command-lifetime-x86.tsv'  # 제품을 실행하지 않고 동결하는 원문 관측이다.
REPORT = ROOT / 'extracted/cpp-dialog-command-lifetime-20261023-native.json'  # 현재 PC와 원문 SHA/실행 범위를 보존한다.


def inputs(events):
    """20개 슬롯/즉시 플래그/입력 release/정수 극값과 부모 fallback 입력을 교차한다."""
    cases = []
    for slot in range(20):  # 원문은 버튼 수 필드와 무관하게 20개 serial 슬롯을 검사한다.
        for instant in (0, 4):  # 자식+ac의 Instant 비트이며 부모 c79b0은 이 필드를 읽지 않는다.
            for release in (0, 0x80000000):  # 부모에 직접 전달하는 명시 입력이며 자식의 press/release 판정과 구별한다.
                for command, parameter in ((0, 0), (0x02000095, 99), (0xffffffff, 0xffffffff)):  # 이미 해석된 DWORD의0/정상/최댓값을 입력한다.
                    cases.append(('Click', events['click'] | release, 700 + slot, 0, slot, instant, command, parameter, 1))
    for instant in (0, 4):  # Enter도 첫 슬롯의 메타와 같은 발화 경로를 사용한다.
        for release in (0, 0x80000000):  # Enter의 상위release도 부모 마스크 뒤에는 같은 사건이다.
            for command, parameter in ((0, 0), (0x02000095, 99), (0xffffffff, 0xffffffff)):  # 첫 슬롯의 명령/인자 극값을 그대로 교차한다.
                cases.append(('Enter', events['enter'] | release, 0, 0, 0, instant, command, parameter, 1))
    for event in (events['enter'], events['click'], 0x123456):  # 빈 슬롯과 serial 불일치는 기본 부모 Handle로 위임한다.
        cases.append(('Absent', event, 799, 0, 0, 0, 0x02000095, 99, 0))
    for parameter in (0, 123):  # tag는 nonzero 링크일 때만 부모를 먼저 Kill하고 링크 처리한다.
        cases.append(('Tag', events['tag'], 0, parameter, 0, 0, 0x02000095, 99, 1))
    cases.append(('Closed', events['closed'], 0, 0, 0, 0, 0, 0, 0))
    return cases


class Oracle(MemoryOracle):
    """c79b0과52f90만 실제 명령으로 실행하고 Kill/링크/기본 Handle은 명시 경계로 둔다."""
    def __init__(self):
        """현재 정밀 범위를 읽고 보호 PE의 실제 초기 사건 DWORD를 보존한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        for entry in (ENTRY, 0x452f90):  # 임의 인접 명령의 실행을 허용하지 않는다.
            for part in facts[entry]['ranges'].split(';'):  # 현재 정밀 분석의 불연속 범위를 실제 명령 단위로 허용한다.
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.events = {name: self.words(address, 1)[0] for name, address in
            (('enter', 0x5427d8), ('click', 0x5427a8), ('tag', 0x54278c), ('closed', 0x5429ec))}
        self.normal = 0

    def instruction(self, mu, address, size, data):
        """예약은 실제 고리 큐를 진행하고 모든 외부 효과는 실제 호출 순서대로 기록한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 3)
        if address == 0x452f90:
            self.trace.append('Queue,' + ','.join(map(str, args)))
        elif address == 0x466480:
            self.trace.append('Kill,' + str(mu.reg_read(UC_X86_REG_ECX)))
            self.ret()
            return
        elif address == 0x4a00c0:
            self.trace.append('Link,' + str(args[0]))
            self.ret()
            return
        elif address == 0x464990:
            self.trace.append('Base,' + ','.join(map(str, args)))
            self.ret(0x12345678, 12)
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'미등록 HTML 부모 명령 {address:08x}')
        self.instructions += 1

    def sample(self, case, precision):
        """실제 thiscall 반환/스택/비휘발 레지스터와 선택 큐/부모 반납을 독립 확인한다."""
        name, event, first, second, slot, instant, command, parameter, present = case
        self.trace = []
        self.mu.mem_write(NODE, bytes(0x21e8))
        self.put(0x55a508, 0)
        self.put(0x55a50c, 0)
        child = POOL + slot * 0x100
        self.mu.mem_write(child, bytes(0x100))
        self.put(child + 0x2c, 700 + slot)
        self.put(child + 0xac, instant)
        self.put(NODE + 0x20c4 + slot * 12, child if present else 0)
        self.put(NODE + 0x20c8 + slot * 12, command)
        self.put(NODE + 0x20cc + slot * 12, parameter)
        saved = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789,
            UC_X86_REG_EDI: 0x3456789a, UC_X86_REG_EBP: 0x456789ab}
        for register, value in saved.items():  # 새 호출마다 이전 함수 상태를 남기지 않는다.
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, NODE)
        self.mu.mem_write(STACK, struct.pack('<IIII', STOP, event, first, second))
        self.mu.emu_start(ENTRY, STOP, count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 16
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        self.normal += 1
        count = self.words(0x55a50c, 1)[0]
        queue = ','.join(map(str, self.words(0x55a510, 3))) if count else '-'
        return str(self.mu.reg_read(UC_X86_REG_EAX)), str(count), queue, ';'.join(self.trace) or '-'


def main():
    """읽기 전용 PE 관측 또는 동결 SHA 검증만 수행하며 게임/GUI를 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, wanted in report['sha256'].items():  # 제품 구현으로 기대 데이터를 재계산하지 않는다.
            assert digest(ROOT / path) == wanted, path
        assert len(FIXTURE.read_text(encoding='utf-8').splitlines()) - 1 == report['rows']
        print('HTML 부모 발화 원문 SHA 감사', report['rows'])
        return
    oracle, rows = Oracle(), []
    for case in inputs(oracle.events):  # 두 x87 정밀도는 동일한 초기 메모리/레지스터를 받는다.
        first = oracle.sample(case, 0x27f)
        assert first == oracle.sample(case, 0x37f), case
        rows.append((*case, *first))
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(['#kind', 'event', 'first', 'second', 'slot', 'instant', 'command', 'parameter', 'present',
            'result', 'queueCount', 'queue', 'effects'])
        writer.writerows(rows)
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c',
        'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py',
        'tools/decomp_dialog_command_lifetime.py', FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', lastDecompileDate='2026-10-23',
        rows=len(rows), normalReturns=oracle.normal, nativeInstructions=oracle.instructions, precisionDifferences=0,
        events=oracle.events, sha256={path: digest(ROOT / path) for path in paths}, limits=[
            'c79b0 전체와52f90 실제40칸큐를 실행한다. 자식 ButtonGump와 메타regex/HTML은 실행하지 않는다.',
            'Kill466480/링크4a00c0/기본Handle464990은 명시 외부 경계다. 실제 GUI와 창 삭제 전체를 증명하지 않는다.',
            'Instant+ac와release를 명시 입력으로 교차한다. 자식이 실제 사용자 입력을 발화하는 시점은 기존 ButtonGump가 담당한다.',
            '새 디컴파일/기존 원본 수정/게임/GUI/OS 실행/제품 소스 실행은 없다.'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('HTML 부모 실제 명령 관측', len(rows), oracle.normal, oracle.instructions)


if __name__ == '__main__':
    main()
