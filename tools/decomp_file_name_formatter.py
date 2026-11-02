#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78의 전체459c60 경로 작성기와 실제 strchr을 격리 CPU로 관측한다."""
import argparse
import csv
import itertools
import json
import socket
import struct

from decomp_priestmenu import Oracle as Base, ROOT, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                              UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
                              UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW)

RING, INDEX = 0x55a760, 0x55b160  # 원문 10×256BYTE 출력과 signed 현재 번호의 실제 주소다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/file-name-formatter-x86.tsv'  # 최초 CPU 기대값은 별도 이름으로만 만든다.
REPORT = ROOT / 'cpppj/file-name-formatter-native-20261102.json'  # 제품 구현과 독립인 원문 관측 요약이다.
REGISTERS = (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)  # 정상 반환 시 유지할 비영 레지스터다.


class Oracle(Base):
    """파일·OS·게임을 실행하지 않고 전체 작성기/strchr만 허용한다."""
    def __init__(self):
        """보호 PE 사본을 준비하고 실제 함수표의 불연속 범위만 허용한다."""
        super().__init__()
        facts = {int(row['entry'], 16): row for row in csv.DictReader(
            (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        self.allowed = []  # 원문 분기/CRT 밖의 임의 성공 반환을 금지할 실행 경계다.
        for entry in (0x459c60, 0x4e63e0):  # 문자열 탐색도 실제 CRT 명령을 사용한다.
            for part in facts[entry]['ranges'].split(';'):  # 각 실제 코드 범위를 독립적으로 보존한다.
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """원문 스택·같은 고리/번호 외 쓰기를 거부하며 입력/PE를 보호한다."""
        allowed = ((STACK - 0x2000, STACK + 4), (RING, RING + 2560), (INDEX, INDEX + 4))
        if not any(low <= address and address + size <= high for low, high in allowed):
            raise RuntimeError(f'경로 작성기 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """정상 반환과 C++ throw 진입을 구별하고 그 외 하위를 합성하지 않는다."""
        if address == STOP:
            mu.emu_stop()
            return
        if address == 0x4e5a09:
            esp = mu.reg_read(UC_X86_REG_ESP)
            source, info = self.words(esp + 4, 2)
            assert bytes(mu.mem_read(source, 1)) == b'\0' and info == 0x522e00
            self.thrown = True
            mu.emu_stop()
            return
        if not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError(f'경로 작성기 실행 범위 밖 {address:08x}')
        self.instructions += 1

    def sample(self, parts, separator, index, precision):
        """명시 부품과 현재 고리를 넣고 반환/throw·현재256BYTE·보존 계약을 읽는다."""
        self.thrown = False
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, index)
        self.put(0x532728, separator, 1)
        self.mu.mem_write(TEXT, bytes(0x10000))
        offset, words = 0, []
        for kind, value in parts:  # 가변 인자는 kind/실제 NUL 문자열 포인터 순서다.
            words.append(kind)
            if not kind:
                break
            assert len(value) < 256
            self.mu.mem_write(TEXT + offset, value + b'\0')
            words.append(TEXT + offset)
            offset += 256
        words.append(0)
        inputs = bytes(self.mu.mem_read(TEXT, 0x10000))
        self.mu.mem_write(STACK - 0x1000, bytes([0x65]) * 0x1000)
        for number, register in enumerate(REGISTERS):  # 보존 대상의 비영 표식을 매번 새로 둔다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        before = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.put(STACK, STOP)
        self.mu.mem_write(STACK + 4, struct.pack('<' + 'I' * len(words), *(word & 0xffffffff for word in words)))
        self.mu.emu_start(0x459c60, STOP, count=20000)
        after_index = self.words(INDEX, 1)[0]
        assert after_index == (index + 1) % 10
        assert bytes(self.mu.mem_read(TEXT, 0x10000)) == inputs
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        if not self.thrown:
            assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
            assert [self.mu.reg_read(register) for register in REGISTERS] == before
            assert self.mu.reg_read(UC_X86_REG_EAX) == RING + after_index * 256
        ring = bytes(self.mu.mem_read(RING, 2560))
        assert not any(ring[:after_index * 256]) and not any(ring[(after_index + 1) * 256:])
        return int(self.thrown), after_index, ring[after_index * 256:(after_index + 1) * 256]


def cases():
    """각 문법 순서·빈 부품·루트·선후 구분자·확장자와 물리 경계를 독립 입력으로 만든다."""
    values = [b'', b'd', b'C:', b'C:\\', b'\\', b'//', b'\\dir\\\\', b'a/b//',
              b'.', b'..col...', b'file.name', b'\xe9cole', b'first\0ignored']
    result = [[]]
    for kind, value in itertools.product((-1, -2, -3, -4, -5, 7), values):  # 잘못된 첫 토큰의 throw도 관찰한다.
        result.append([(kind, value)])
    for first, second in itertools.product((-1, -2, -3, -4), repeat=2):  # 원문 허용/거부 순서16개를 모두 포함한다.
        for value in (b'', b'a', b'\\b\\\\', b'..ext..'):  # 후속 문자열에 따른 앞선 부분 효과도 남긴다.
            result.append([(first, b'root'), (second, value)])
    for name in values:  # 실제 Uber의 root/d/filename/extension 조합이다.
        result.append([(-2, b''), (-2, b'd'), (-3, name), (-4, b'col')])
    for count in (0, 1, 2, 3, 8):  # 원문 후행 slash의 실제 반복 방향을 추측으로 정규화하지 않는다.
        for kind in (-1, -2, -3):
            result.append([(kind, b'part' + b'\\' * count)])
    result.extend([[(-1, b'C'), (-2, b'd'), (-3, b'file'), (-4, b'..col...')],
                   [(-3, b'a'), (0, b''), (-4, b'ignored')], [(-3, b'x' * 253)],
                   [(-1, b'R' * 254)], [(-2, b'x' * 252)]])
    return result


def main():
    """최초 관측을 고정하고 --verify는 같은 원문을 다시 실행해 비교만 한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()):
        raise RuntimeError('경로 작성기 최초 관측 덮어쓰기 거부')
    oracle, rows, observations = Oracle(), ['# separator index parts throw after_index current_slot_hex'], 0
    for number, parts in enumerate(cases()):  # 제품 로직을 사용하지 않고 두 구분자/정밀도의 CPU 결과만 비교한다.
        for separator in (92, 47):
            index = number % 10
            normal = oracle.sample(parts, separator, index, 0x027f)
            extended = oracle.sample(parts, separator, index, 0x037f)
            assert normal == extended
            encoded = ';'.join(f'{kind}:{value.hex() or "-"}' for kind, value in parts) or '-'
            rows.append(f'{separator}\t{index}\t{encoded}\t{normal[0]}\t{normal[1]}\t{normal[2].hex()}')
            observations += 2
    data = '\n'.join(rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text('utf-8') == data
    else:
        FIXTURE.write_text(data, 'utf-8', newline='\n')
        save = dict(unit=727, date='2026-11-02', host=socket.gethostname(),
                    lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'),
                    entry='00459c60', actualCrt='004e63e0 strchr', samples=len(rows)-1,
                    observations=observations, instructions=oracle.instructions,
                    gameExecutions=0, multiplayerTests=0,
                    boundary='004e5a09 throw 진입의 char0/형식만 관찰하며 SEH 전개를 실행하지 않는다',
                    sha256={str(path.relative_to(ROOT)): digest(path) for path in
                            (ROOT / 'AGENTS.md', ROOT / 'originals/Netstorm.exe', FIXTURE, ROOT / 'tools/decomp_file_name_formatter.py')})
        REPORT.write_text(json.dumps(save, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps(dict(samples=len(rows)-1, observations=observations, instructions=oracle.instructions)))


if __name__ == '__main__':
    main()
