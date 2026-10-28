#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 배너 설정 쓰기와 최초 소유자 색표 등록을 실제 x86으로 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
import decomp_production_shortcuts as native
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 같은 보호 이미지와 격리 로더를 빌리되 실제 실행은 아래 두 함수만 허용한다.
ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'cpppj/tests/fixtures/banner-option-x86.tsv'
OUTPUT = ROOT / 'extracted/cpp-banner-option-546'
BEGIN, INIT, END = 0x43bc40, 0x43be30, 0x43c010
READY, ENABLED, REDRAW, TABLES = 0x557938, 0x5319c0, 0x59a8b0, 0x557940
TYPE_ADDRESSES = (0x5411ac, 0x541174, 0x541258, 0x5411dc, 0x5411d0)
DEFAULT_TYPES = (85, 71, 128, 97, 94)
TABLE_COUNT, STOP = 188, native.HEAP + 0x3000
SCHEMA = ('name', 'operation', 'ready', 'enabled', 'redraw', 'mask', 'types', 'requests',
          'skip', 'readyout', 'enabledout', 'redrawout', 'tablesout', 'trace', 'instructions')


def token(kind):
    """명시 할당 장치는 타입별 식별 토큰만 공급하며 실제 색표 계산과 구별한다."""
    return 0x100000 + kind * 2304


class Oracle(native.Oracle):
    """실제 관문·store·free 순서와 초기화 등록 순서를 관측한다."""

    def __init__(self):
        """허용 함수 전체 바이트를 보존하고 기존 입력 함수는 실행하지 않는다."""
        super().__init__()
        self.original[BEGIN] = bytes(self.mu.mem_read(BEGIN, END - BEGIN))

    def guard(self, mu, access, address, size, value, data):
        """원문의 쓰기는 스택과 현재 세 DWORD·타입별 색표 포인터로 제한한다."""
        if native.STACK - 0x8000 <= address and address + size <= native.STACK + 0x3000:
            return
        if any(low <= address and address + size <= high for low, high in
               [(READY, READY + 4), (ENABLED, ENABLED + 4), (REDRAW, REDRAW + 4),
                (TABLES, TABLES + TABLE_COUNT * 4)]):
            return
        raise RuntimeError(f'배너 원문의 미명시 쓰기 {address:x}/{size}')

    def instruction(self, mu, address, size, data):
        """색표 생성/메모리 반납은 명시 장치이며 관문과 현재 포인터 해제는 원문이다."""
        if address == STOP:
            self.completed = True
            mu.emu_stop()
            return
        if BEGIN <= address < END:
            self.instructions += 1
            return
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address in (0x43ba00, 0x43bbe0):
            kind = self.number(esp + 4)
            if address == 0x43ba00:
                self.event('standard', (kind,))
            else:
                offset = int.from_bytes(self.mu.mem_read(esp + 8, 8), 'little')
                sources = []
                # 실제 cdecl 스택의 낮은 BYTE 종료값까지 읽으며 기대 소스는 계산하지 않는다.
                for index in range(256):
                    value = self.number(esp + 16 + index * 4)
                    if value & 255 == 255:
                        break
                    sources.append(str(value))
                else:
                    raise RuntimeError('원본 가변 인자 종료값 부재')
                self.event('special', (kind, offset, ','.join(sources)))
            self.number(TABLES + kind * 4, token(kind))
            self.returned()
            return
        if address == 0x4e4354:
            self.event('free', (self.number(esp + 4),))
            self.returned()
            return
        raise RuntimeError(f'배너 원문의 미명시 실행 {address:x}')

    def run(self, case, control):
        """독립 시작 상태에서 요청열 또는 최초 등록을 실행하고 전체188포인터를 읽는다."""
        self.trace, self.instructions = [], 0
        self.number(READY, case['ready'])
        self.number(ENABLED, case['enabled'])
        self.number(REDRAW, case['redraw'])
        self.number(0x59ab28, case['skip'])
        kinds = [int(value) for value in case['types'].split(',')]
        # 같은 현재 TYPE 전역을 명시하며 변경된 타입 번호 사례도 직접 읽도록 한다.
        for address, kind in zip(TYPE_ADDRESSES, kinds):
            self.number(address, kind)
        self.mu.mem_write(TABLES, bytes(TABLE_COUNT * 4))
        # 다섯 해제 후보 외의 표도 보존돼야 하므로111을 별도 생존 입력으로 둔다.
        self.number(TABLES + 111 * 4, token(111))
        for index, kind in enumerate(kinds):
            self.number(TABLES + kind * 4, token(kind) if case['mask'] & (1 << index) else 0)
        requests = [int(value) for value in case['requests'].split(',')]
        # 요청 사이의 현재 설정·표를 재초기화하지 않아 반복/전환의 실제 효과를 보존한다.
        for requested in requests:
            self.mu.mem_write(native.STACK - 0x4000, bytes(0x8000))
            self.mu.mem_write(native.STACK, struct.pack('<2I', STOP, requested))
            for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                             UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
                self.mu.reg_write(register, 0)
            self.mu.reg_write(UC_X86_REG_ESP, native.STACK)
            self.mu.reg_write(UC_X86_REG_FPCW, control)
            self.mu.reg_write(UC_X86_REG_FPSW, 0)
            self.completed = False
            self.mu.emu_start(INIT if case['operation'] == 'init' else BEGIN, STOP + 1, count=50000)
            assert self.completed and self.mu.reg_read(UC_X86_REG_ESP) == native.STACK + 4
            assert self.mu.reg_read(UC_X86_REG_FPCW) == control
        assert bytes(self.mu.mem_read(BEGIN, END - BEGIN)) == self.original[BEGIN]
        return {'readyout': self.number(READY), 'enabledout': self.number(ENABLED),
                'redrawout': self.number(REDRAW),
                'tablesout': ','.join(str(self.number(TABLES + index * 4)) for index in range(TABLE_COUNT)),
                'trace': ';'.join(self.trace) or '-', 'instructions': self.instructions}


def cases():
    """현재 준비/설정·전체 포인터 조합과 DWORD 감김을 독립 입력으로 준비한다."""
    rows = []
    # 준비0의 지연 처리와 같음/다름, 모든 다섯 표 존재 조합 및 dirty 감김을 교차한다.
    for ready, enabled, requested, redraw, mask in itertools.product(
            (0, 1), (0, 1, 7, 0xffffffff), (0, 1, 2, 0xffffffff), (17, 0xffffffff), range(32)):
        rows.append(dict(name=f'write-{ready}-{enabled}-{requested}-{redraw}-{mask}', operation='write',
                         ready=ready, enabled=enabled, redraw=redraw, mask=mask,
                         types=','.join(map(str, DEFAULT_TYPES)), requests=str(requested), skip=0))
    # 최초 등록의 강제-1 쓰기와 선택 두 표 생략, 요청열과 실제 타입 전역 재조회도 확인한다.
    for enabled, mask, skip in itertools.product((0, 1, 7, 0xffffffff), (0, 31), (0, 1)):
        rows.append(dict(name=f'init-{enabled}-{mask}-{skip}', operation='init', ready=0, enabled=enabled,
                         redraw=0xffffffff, mask=mask, types=','.join(map(str, DEFAULT_TYPES)), requests='0', skip=skip))
    for kinds in (DEFAULT_TYPES, (171, 71, 128, 170, 169)):
        rows.append(dict(name='sequence-'+'-'.join(map(str, kinds)), operation='write', ready=1, enabled=1,
                         redraw=0xfffffffd, mask=31, types=','.join(map(str, kinds)), requests='0,0,1,0,4294967295', skip=0))
    return rows


def main():
    """제품 작성 전 실제 관측과 두 x87의 일치를 새 fixture/보고서에 고정한다."""
    assert native.digest(native.PE_PATH) == native.PE_SHA
    OUTPUT.mkdir(parents=True, exist_ok=True)
    oracle = Oracle()
    rows = cases()
    observed = []
    # 각 명시 입력마다 두 정밀도를 별도로 실행하며 원문 출력만 직렬화한다.
    for row in rows:
        first = oracle.run(row, 0x27f)
        second = oracle.run(row, 0x37f)
        assert first == second, row['name']
        observed.append({**row, **first})
    FIXTURE.write_bytes(('\t'.join(SCHEMA)+'\n'+'\n'.join(
        '\t'.join(str(row[field]) for field in SCHEMA) for row in observed)+'\n').encode('utf-8'))
    report = {'host': socket.gethostname(), 'peSha256': native.PE_SHA, 'inputs': len(rows),
              'uniqueInputs': len({tuple(str(row[field]) for field in SCHEMA[1:9]) for row in rows}),
              'nativeObservations': len(rows)*2, 'precisionDifferences': 0,
              'ranges': [[hex(BEGIN), hex(END)]], 'nativeBodiesUnchanged': True,
              'externalDevices': ['43ba00 standard color map', '43bbe0 special color map', 'CRT free'],
              'fixtureSha256': hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
              'scope': '배너 설정/색표 최초 등록만 격리 실행. 게임/OS/멀티플레이/유닛 생성 실행 없음'}
    (OUTPUT/'native.json').write_bytes((json.dumps(report, ensure_ascii=False, indent=2)+'\n').encode('utf-8'))
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
