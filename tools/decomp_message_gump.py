#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 메시지 생성자의 Pause(kind11) 분기를 실제 PE에서 격리 관측한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK, STOP, TEXT, digest, fbits
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW,
    UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

NODE, NAME, TRANSLATED = TEXT+0x1000, TEXT+0x3000, TEXT+0x4000  # 실제 노드와 명시 외부 문자열의 격리 주소다.
ENTRY = 0x4d0600  # 기존 TextGump를 호출하는 실제 파생 생성자다.
FIXTURE = ROOT/'cpppj/tests/fixtures/message-gump-x86.tsv'  # 제품 작성 전에 동결하는 실제 필드/효과다.
REPORT = ROOT/'extracted/cpp-message-gump-20261023-native.json'  # 현재 호스트와 실행 범위/해시다.
STRINGS = (b'~E~wGame Paused!', b'', b'~E~yPause\xff!')  # 명시 번역 반환의 원본 바이트다.


def inputs():
    """기대값을 계산하지 않고 challenge/검색키/번역/경계 변경 입력만 교차한다."""
    return list(itertools.product((0, 1, 0x80000000), (0, 1, 8, 0xffffffff),
        (0, 99, 0xffffffff), range(3), range(3)))


class Oracle(MemoryOracle):
    """실제 파생 몸체만 실행하며 TextGump/이름/번역/StyleText/측정은 명시 경계다."""
    def __init__(self):
        """원본 이미지를 읽기 전용 입력으로 재사용하고 허용 범위를 좁힌다."""
        super().__init__()
        self.mu.mem_map(0, 0x1000)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        for part in facts[ENTRY]['ranges'].split(';'):  # 실제 export의 불연속 명령 범위만 허용한다.
            low, high = (int(value, 16) for value in part.split('-'))
            self.allowed.append((low, high+1))
        self.events, self.calls, self.returns = [], Counter(), 0
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """노드/SEH/실제 큰 스택 밖의 쓰기를 거부한다."""
        ranges = ((NODE, NODE+0xc0), (0, 4), (STACK-0x1f000, STACK+0x1000))
        if not any(low <= address and address+size <= high for low, high in ranges):
            raise RuntimeError(f'Pause 생성 범위 밖 쓰기 {address:08x}+{size}')

    def event(self, *values):
        """실제로 도달한 경계와 인자만 순서대로 보존한다."""
        self.events.append(','.join(map(str, values)))

    def mutate(self, phase):
        """외부 경계의 명시 변경이 원문 저장 순서에 의해 보존되는지 관찰한다."""
        if self.case[4] == phase:
            if phase == 1:
                self.put(NODE+0xa8, 3)
                self.mu.mem_write(NODE+0xb0, struct.pack('<d', 42.5))
            else:
                self.put(NODE+0xa8, 8)
                self.put(NODE+0xb8, 123)

    def instruction(self, mu, address, size, data):
        """선택된 원문 전체를 실행하고 OS/CRT 진입은 실제 호출 규약만 재생한다."""
        if any(low <= address and address+size <= high for low, high in self.allowed):
            self.instructions += 1
            return
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp+4, 4)
        ecx = mu.reg_read(UC_X86_REG_ECX)
        self.calls[f'{address:08x}'] += 1
        if address == 0x4e4a40:
            size = mu.reg_read(UC_X86_REG_EAX)
            target = self.words(esp, 1)[0]
            assert size == 0xb404
            mu.reg_write(UC_X86_REG_ESP, esp+4-size)
            mu.reg_write(UC_X86_REG_EIP, target)
        elif address == 0x4bb170:
            assert ecx == NODE and args == (0, 0, 0, 0)
            self.event('Text')
            self.put(NODE, 0x5144d4)
            self.put(NODE+4, 0x400001)
            self.put(NODE+0x20, 45)
            self.put(NODE+0x2c, 41)
            self.put(NODE+0xa0, 0)
            mu.mem_write(NODE+0x80, struct.pack('<8I', 0, 0, 0, 0, 0, 0, 1, 1))
            self.ret(NODE, 16)
        elif address in (0x48f480, 0x42bbb0):
            assert address == (0x42bbb0 if self.case[0] else 0x48f480)
            assert args[0] == self.case[1]
            if address == 0x42bbb0:
                assert ecx == TEXT+0x5000
            self.event('ChallengeName' if address == 0x42bbb0 else 'PlayerName', args[0])
            self.mutate(1)
            self.ret(NAME, 4 if address == 0x42bbb0 else 0)
        elif address == 0x4de9a0:
            assert ecx == 0x5e22b8 and self.string(args[0]) == STRINGS[0]
            self.event('Translate', self.string(args[0]).hex())
            self.mutate(2)
            self.ret(TRANSLATED, 4)
        elif address == 0x4bc800:
            assert ecx == NODE
            self.event('Style', self.string(args[0]).hex())
            self.put(NODE+0xa0, 0x12408000)
            self.ret(purge=4)
        elif address == 0x4bb310:
            assert ecx == NODE and args[:2] == (0, 0)
            self.event('Fit', *args[:2])
            for offset, value in ((0x3c, 51.25), (0x40, 13.5), (0x44, 51.25), (0x48, 13.5)):
                self.put(NODE+offset, fbits(value))  # 명시 측정 장치가 현재 크기를 갱신한다.
            self.ret(purge=8)
        else:
            raise RuntimeError(f'Pause 생성 미등록 경계 {address:08x}')

    def sample(self, case, precision):
        """독립 입력/큰 스택/SEH/비휘발 레지스터/x87 정상 반환을 확인한다."""
        self.case, self.events = case, []
        mu = self.mu
        mu.mem_write(NODE, b'\xa5'*0xc0)
        mu.mem_write(NAME, b'player\0')
        mu.mem_write(TRANSLATED, STRINGS[case[3]]+b'\0')
        self.put(0, 0xffffffff)
        self.put(0x594fc8, case[0])
        self.put(0x5467d8, TEXT+0x5000)
        saved = {UC_X86_REG_EBX:0x12345678, UC_X86_REG_ESI:0x23456789,
            UC_X86_REG_EDI:0x3456789a, UC_X86_REG_EBP:0x456789ab}
        for register, value in saved.items():  # 실제 thiscall 비휘발 레지스터를 서로 다르게 둔다.
            mu.reg_write(register, value)
        for register, value in ((UC_X86_REG_ECX,NODE),(UC_X86_REG_EAX,0),
            (UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),
            (UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):
            mu.reg_write(register, value)  # 이전 관측의 플래그/x87 상태를 남기지 않는다.
        mu.mem_write(STACK, struct.pack('<5I', STOP, 11, case[1], case[2], NAME))
        mu.emu_start(ENTRY, STOP, count=100000)
        assert mu.reg_read(UC_X86_REG_EIP) == STOP
        assert mu.reg_read(UC_X86_REG_ESP) == STACK+20
        assert mu.reg_read(UC_X86_REG_EAX) == NODE and self.words(0,1) == (0xffffffff,)
        assert all(mu.reg_read(register) == value for register,value in saved.items())
        assert mu.reg_read(UC_X86_REG_FPCW) == precision and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.returns += 1
        return bytes(mu.mem_read(NODE, 0xc0)).hex(), ';'.join(self.events)


def main():
    """fixture 생성 또는 기존 원문 해시 검증을 수행하며 게임/창은 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for path, wanted in report['sha256'].items():  # 기대 원문을 다시 계산하지 않고 파일 바이트를 검증한다.
            assert digest(ROOT/path) == wanted, path
        assert report['rows'] == len(inputs())
        print('Pause 메시지 원문 SHA/입력 감사', report['rows'])
        return
    oracle, rows = Oracle(), []
    for case in inputs():  # 두 x87 정밀도의 독립 실행이 다르면 fixture를 쓰지 않는다.
        first = oracle.sample(case, 0x27f)
        assert first == oracle.sample(case, 0x37f), case
        rows.append((','.join(map(str,case)), *first))
    FIXTURE.write_text('# inputs node192 effects\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe','extracted/refined/originals/functions.tsv',
        'extracted/refined/originals/Netstorm.c','tools/decomp_priestmenu.py',
        'tools/decomp_message_gump.py',FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), lastDecompileHost='VM-W11-Codex',
        lastDecompileDate='2026-10-23', rows=len(rows), normalReturns=oracle.returns,
        instructions=oracle.instructions, nativeCalls=dict(oracle.calls), precisionDifferences=0,
        sha256={path:digest(ROOT/path) for path in paths}, limits=[
            '004d0600 전체 중 kind11(Pause) 선택 경로만 실행한다. 다른 kind의 복원을 주장하지 않는다.',
            'TextGump 생성/이름 조회/번역/StyleText/측정 및 CRT 큰 스택 확보는 명시 경계다.',
            '192바이트 파생 상태/순서를 동결한다. 실제 폰트/픽셀/루트20칸/Draw/Idle 통합은 별도 기존 모듈 범위다.',
            '보호 PE는 읽기만 했다. 실제 게임/GUI/OS 및 새 디컴파일은 실행하지 않았다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n', encoding='utf-8', newline='\n')
    print('Pause 메시지 원본 관측',len(rows),oracle.returns,oracle.instructions)


if __name__ == '__main__':
    main()
