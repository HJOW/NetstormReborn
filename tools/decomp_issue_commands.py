#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 초기 등록/flash/cheatorama와 실제 토큰·atol을 창 없이 격리 관측한다."""
import argparse
import csv
import json
import platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

THREAD = TEXT+0x4000  # 원본 C locale을 빌리는 합성 TLS의 독립 저장소다.


class Oracle(MemoryOracle):
    """선택된 원문/실제 CRT만 실행하고 등록·화면·음향은 명시 경계로 기록한다."""
    def __init__(self):
        """같은 보호 PE와 정밀 범위를 사용하며 런타임 입력에 제품 코드를 쓰지 않는다."""
        super().__init__()
        facts = {int(row['entry'], 16): row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        self.allowed = []
        for entry in (0x4d5300, 0x4d5360, 0x4d53a0, 0x487620, 0x487710, 0x4e63e0, 0x4e6656, 0x4e66de):
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high+1))  # 실제 불연속 함수 범위만 허용한다.
        self.put(THREAD+0x64, self.words(0x543efc, 1)[0])

    def instruction(self, mu, address, size, data):
        """원문 효과 순서와 입력을 기록하고 CRT 숫자/토큰 계산은 실제 명령으로 진행한다."""
        if address == STOP:
            self.completed = True
            mu.emu_stop()
            return
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4ea61d:
            self.ret(THREAD)
            return
        if address == 0x43f660:
            name, callback, flag = self.words(esp+4, 3)
            self.events.append(f'R,{self.string(name).hex()},{callback},{flag}')
            self.ret(1)
            return
        if address == 0x470fa0:
            self.events.append('F,'+str(self.words(esp+4, 1)[0]))
            self.ret()
            return
        if address == 0x4a9cb0:
            args = self.words(esp+4, 6)
            self.events.append('S,'+self.string(args[0]).hex()+','+','.join(map(str, args[1:])))
            self.ret()
            return
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'초기 명령 관측 밖 {address:08x}')

    def run(self, operation, source, before):
        """입력/기존 치트 값만 설정하고 정상 반환 뒤 결과와 효과를 읽는다."""
        self.events, self.completed = [], False
        self.put(0x5cab30, before)
        self.mu.mem_write(TEXT, (source or b'')+b'\0')
        self.mu.mem_write(STACK-0x1000, bytes(0x1100))
        self.put(STACK, STOP)
        self.put(STACK+4, 0 if source is None else TEXT)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, 0)
        self.mu.emu_start({'Init':0x4d53a0, 'Flash':0x4d5300, 'Cheat':0x4d5360}[operation], STOP+1, count=100000)
        assert self.completed
        return [operation, '-' if source is None else source.hex() or '~', before,
                self.mu.reg_read(UC_X86_REG_EAX), self.words(0x5cab30, 1)[0], ';'.join(self.events) or '-']


def main():
    """입력 교차와 독립 재관측만 저장하고 기존 fixture를 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    texts = [b'', b'0', b'64', b'-1', b'+7', b'garbage', b'2147483648', b'-2147483649', b'4294967296',
             b'8675309', b'  8675309 trailing', b'"8675309"', b'"8675309 tail"', b'+8675309', b'86753090', b'4303642605', b'\n8675309', b'\t8675309', b'0x845fed', b'1'*79]
    cases = [('Init', None, 17), ('Flash', None, 17)]
    cases += [(operation, text, before) for operation in ('Flash', 'Cheat') for text in texts for before in (0, 17, 0x845fed)]  # 원문 결과와 독립인 정상/경계/인용 입력이다.
    oracle = Oracle()
    first = [oracle.run(*case) for case in cases]
    replay = [oracle.run(*case) for case in cases]
    assert first == replay
    with (out/'issue-commands-x86.tsv').open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# operation/inputHex(~빈값,-NULL)/기존치트/반환DWORD/최종치트/실제외부호출\n')
        for row in first:
            stream.write('\t'.join(map(str, row))+'\n')  # 계산한 기대값 대신 실제 CPU 관측만 보존한다.
    report = dict(unit=649, host=platform.node(), lastFullDecompileHost='HJOW-Athlon', lastFullDecompileDate='2026-10-23', cases=len(cases), observations=len(cases)*2, differences=0,
                  scope='실제 초기 등록/flash/cheatorama·토큰/atol. TLS C locale과 등록/화면/음향만 명시 경계. desc/MP·OS·GUI·게임 실행 없음.')
    (out/'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
