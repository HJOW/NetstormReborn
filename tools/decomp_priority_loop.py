#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""WinMain 우선순위 국소 원문을 비멀티 입력만으로 CPU 관측하고 새 파일에 저장한다."""
import argparse
import csv
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import sys

DEFAULT_ROOT = Path('H:/T5Workspace/NetstormReborn')  # 읽기 전용 원문과 정밀 범위표의 저장소다.
ENTRY, END = 0x439983, 0x4399ef  # refresh 뒤 priority 판단부터 다음 프레임 자식 호출 직전까지다.
CHALLENGE, ACTIVE, REQUESTED = 0x594fc8, 0x54dc50, 0x54db2c  # 원문에서 순서대로 읽는 실제 DWORD다.
CURRENT_IAT, GET_IAT, SET_IAT = 0x5001d8, 0x5001d4, 0x5001dc  # 실제 Win32 간접 호출 표다.
ARENA, STACK = 0x1000000, 0x2100000  # OS와 분리한 명시 호출 장치와 스택이다.
CURRENT, GET, SET = ARENA+0x100, ARENA+0x200, ARENA+0x300  # 실제 OS API를 실행하지 않는 세 경계다.
PE_SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 보호된 10.78 PE 식별자다.
SCHEMA = ('name', 'challenge', 'active', 'requested', 'current', 'firstThread', 'secondThread', 'setResult',
          'rawWanted', 'normalized', 'called', 'selected', 'trace', 'completed', 'stackdelta',
          'globalsout', 'nativeinstructions')  # 앞8열은 독립 입력이고 뒤9열은 원문 관측이다.


def digest(path):
    """보호 입력과 새 관측 파일의 바이트 SHA256을 계산한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def cases():
    """원문 결과를 계산하지 않고 raw DWORD와 signed OS 반환 경계만 고른다."""
    rows = []  # 명시 입력을 추가한 순서가 fixture 순서다.

    def add(active, requested, current, set_result=1, first=0xfffffffe, second=0xfffffffe):
        """challenge는 항상0이며 두 GetCurrentThread 응답을 독립 경계 입력으로 둔다."""
        rows.append(dict(name='priority-loop-'+str(len(rows)), challenge=0, active=active,
                         requested=requested, current=current, firstThread=first,
                         secondThread=second, setResult=set_result))

    # 지원 레벨과 범위 밖 raw 요청을 비활성/활성 및 정상 OS 레벨과 독립 교차한다.
    for active, requested, current in itertools.product((0, 1), (0, 1, 2, 3, 7, 0x7fffffff, 0x80000000, 0xffffffff), (0, 1, 2, 15)):
        add(active, requested, current)
    # OS의 음수·기타 양수·오류 반환은 요청0/1/2와 각각 교차한다.
    for requested, current in itertools.product((0, 1, 2), (-0x80000000, -15, -2, -1, 3, 4, 14, 16, 0x7fffffff)):
        add(1, requested, current)
    # bool의 비정규 DWORD, OS 실패 반환 및 두 번째 실제 핸들 재조회를 따로 관측한다.
    for arguments in ((0x80000000, 3, 0, 1), (0xffffffff, 2, 15, 1), (1, 1, 0, 0), (1, 3, 1, 0), (1, 2, 0, 1, 0x13572468, 0x24681357)):
        add(*arguments)
    assert len(rows) == 96 and all(case['challenge'] == 0 for case in rows)
    return rows


def main():
    """원문96행을 두 x87 정밀도와 반복으로 관측하며 기존 파일은 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=DEFAULT_ROOT)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    arguments = parser.parse_args()
    assert not arguments.fixture.exists() and not arguments.report.exists()
    root = arguments.root.resolve()  # 원문 저장소를 출력 경로와 구별한다.
    sys.path.insert(0, str(root/'extracted/oracle-python'))
    import pefile
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
        UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
        UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPTAG)

    class Oracle:
        """국소 PE 명령만 실행하고 OS API 세 개는 인자·순서 기록 장치로 둔다."""

        def __init__(self):
            """보호 PE를 읽고 국소 명령과 명시 스택/호출 장치만 준비한다."""
            self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 실제 x86 원문 실행기다.
            pe = pefile.PE(str(root/'originals/Netstorm.exe'))
            image = pe.get_memory_mapped_image()
            self.mu.mem_map(0x400000, (len(image)+4095)&~4095)
            self.mu.mem_write(0x400000, image)
            self.mu.mem_map(ARENA, 0x1000)
            self.mu.mem_map(STACK-0x1000, 0x2000)
            self.original = bytes(self.mu.mem_read(ENTRY, END-ENTRY))  # 실행 전후 동일해야 하는 국소 몸체다.
            self.mu.hook_add(UC_HOOK_CODE, self.instruction)
            self.mu.hook_add(UC_HOOK_MEM_READ, self.read)
            self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

        def number(self, address, value=None):
            """원시 DWORD를 읽고 명시 입력만 같은 폭으로 설치한다."""
            if value is not None:
                self.mu.mem_write(address, struct.pack('<I', value&0xffffffff))
            return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

        def returned(self, value, cleanup=0):
            """실제 CALL 복귀주소와 stdcall 인자 폭만 소비한다."""
            sp = self.mu.reg_read(UC_X86_REG_ESP)
            target = self.number(sp)
            self.mu.reg_write(UC_X86_REG_EAX, value&0xffffffff)
            self.mu.reg_write(UC_X86_REG_ESP, sp+4+cleanup)
            self.mu.reg_write(UC_X86_REG_EIP, target)

        def read(self, mu, access, address, size, value, data):
            """전역의 실제 CPU 조회 순서를 OS 경계와 함께 보존한다."""
            labels = {CHALLENGE: 'challenge', ACTIVE: 'active', REQUESTED: 'requested'}
            if address in labels:
                assert size == 4
                self.trace.append(labels[address]+':'+str(self.number(address)))

        def write(self, mu, access, address, size, value, data):
            """원문 CPU 쓰기는 CALL/push의 국소 스택 범위로만 제한한다."""
            if not STACK-0x1000 <= address <= address+size <= STACK+0x1000:
                raise RuntimeError(f'미명시 priority 루프 쓰기 {address:08x}/{size}')

        def instruction(self, mu, address, size, data):
            """원문 선택·비교는 대체하지 않고 Win32 인자와 반환 경계만 명시한다."""
            sp = mu.reg_read(UC_X86_REG_ESP)
            if address == END:
                self.completed = 1
                mu.emu_stop()
                return
            if address == CURRENT:
                value = self.case['firstThread'] if self.thread_calls == 0 else self.case['secondThread']
                self.thread_calls += 1
                self.trace.append('current-thread:'+str(value))
                self.returned(value)
                return
            if address == GET:
                self.trace.append('get-priority:'+str(self.number(sp+4))+':'+str(self.case['current']))
                self.returned(self.case['current'], 4)
                return
            if address == SET:
                self.selected = self.number(sp+8)
                self.called += 1
                self.trace.append('set-priority:'+str(self.number(sp+4))+':'+str(self.selected)+':'+str(self.case['setResult']))
                self.returned(self.case['setResult'], 8)
                return
            if not ENTRY <= address < END:
                raise RuntimeError(f'미검토 priority 루프 명령 {address:08x}')
            if address == 0x43999c:
                self.wanted = mu.reg_read(UC_X86_REG_ESI)
            if address == 0x4399c3:
                self.normalized = mu.reg_read(UC_X86_REG_EAX)
            self.instructions += 1

        def run(self, case, control):
            """선행 WinMain의 EBX=0/EDI=current IAT 상태를 명시하고 국소 반환을 관측한다."""
            assert case['challenge'] == 0
            self.case = case  # 현재 명시 경계 입력이다.
            self.trace = []  # 전역 읽기와 실제 API 인자를 같은 순서로 보존한다.
            self.thread_calls = 0  # GetCurrentThread는 첫/둘째 응답을 구별한다.
            self.called = 0  # 실제 SetThreadPriority 도달 횟수다.
            self.selected = '-'  # 호출하지 않은 경우 인자를 합성하지 않는다.
            self.wanted = None  # 원문 ESI에 선택된 raw 요청을 직접 읽는다.
            self.normalized = None  # 비교 직전 원문 EAX의 OS 정규화 결과다.
            self.instructions = 0  # 허용 원문 몸체만 실행한 명령 수다.
            self.completed = 0  # 다음 프레임 자식 직전까지 도달했는지 나타낸다.
            # 원문 전역과 실제 IAT 슬롯에는 독립 입력 및 격리 경계 주소만 공급한다.
            for address, value in ((CHALLENGE, 0), (ACTIVE, case['active']), (REQUESTED, case['requested']),
                                   (CURRENT_IAT, CURRENT), (GET_IAT, GET), (SET_IAT, SET)):
                self.number(address, value)
            self.mu.mem_write(STACK-0x1000, bytes(0x2000))
            # 필요 선행 EBX/EDI를 제외한 일반 레지스터는 독립 독성값에서 시작한다.
            for register, value in ((UC_X86_REG_EAX, 0x11111111), (UC_X86_REG_EBX, 0),
                (UC_X86_REG_ECX, 0x33333333), (UC_X86_REG_EDX, 0x44444444), (UC_X86_REG_ESI, 0x55555555),
                (UC_X86_REG_EDI, CURRENT), (UC_X86_REG_EBP, STACK+0x500), (UC_X86_REG_ESP, STACK),
                (UC_X86_REG_EFLAGS, 0x202), (UC_X86_REG_FPCW, control), (UC_X86_REG_FPTAG, 0xffff)):
                self.mu.reg_write(register, value)
            self.mu.emu_start(ENTRY, END+1, count=500)
            assert bytes(self.mu.mem_read(ENTRY, END-ENTRY)) == self.original
            return dict(rawWanted=self.wanted, normalized=self.normalized, called=self.called,
                        selected=self.selected, trace='>'.join(self.trace), completed=self.completed,
                        stackdelta=self.mu.reg_read(UC_X86_REG_ESP)-STACK,
                        globalsout=':'.join(str(self.number(address)) for address in (CHALLENGE, ACTIVE, REQUESTED)),
                        nativeinstructions=self.instructions)

    assert digest(root/'originals/Netstorm.exe') == PE_SHA
    oracle, inputs, raw, lines = Oracle(), cases(), [], ['\t'.join(SCHEMA)]
    # 두 x87 정밀도와 첫 정밀도의 별도 반복에서 완전히 같은 관측만 채택한다.
    for case in inputs:
        observations = [oracle.run(case, control) for control in (0x27f, 0x37f, 0x27f)]
        assert observations[0] == observations[1] == observations[2], case['name']
        result = observations[0]
        assert result['completed'] == 1 and result['stackdelta'] == 0, case['name']
        row = dict(case, **result)
        lines.append('\t'.join(str(row[name]) for name in SCHEMA))
        raw.append(dict(input=case, observations=observations))
    # 출력은 호출자가 지정한 새 경로에만 생성하고 기존 fixture는 덮어쓰지 않는다.
    for path in (arguments.fixture, arguments.report):
        path.parent.mkdir(parents=True, exist_ok=True)
    arguments.fixture.write_text('\n'.join(lines)+'\n', encoding='utf-8', newline='\n')
    record = dict(unit=762, host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'),
                  entry=f'{ENTRY:08x}', endExclusive=f'{END:08x}', inputs=len(inputs), observations=len(inputs)*3,
                  differences=0, gameExecutions=0, multiplayerTests=0, actualOsCalls=0, nativeBodiesUnchanged=True,
                  peSha256=PE_SHA, bodySha256=hashlib.sha256(oracle.original).hexdigest(), toolSha256=digest(__file__),
                  fixtureSha256=digest(arguments.fixture), controls=[0x27f, 0x37f, 0x27f],
                  preconditions=['WinMain의 EBX=0', 'EDI는 선행00439966에서 GetCurrentThread IAT를 읽은 값', 'challenge00594fc8=0 고정'],
                  boundaries=['GetCurrentThread:실제 CALL 순서별 명시 핸들', 'GetThreadPriority:명시 signed 반환', 'SetThreadPriority:실제 핸들/우선순위 인자 및 명시 BOOL 반환'],
                  limits=['국소 원문 CPU이며 실제 WinMain/OS/프레임/게임 실행 검사가 아니다', 'challenge 양수 분기와 멀티플레이 실행은 관측하지 않았다', 'GetCurrentThread 장치는 인자/순서만 관측하고 호스트 스레드를 조회하지 않는다'], raw=raw)
    arguments.report.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('inputs', 'observations', 'differences', 'fixtureSha256', 'toolSha256')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
