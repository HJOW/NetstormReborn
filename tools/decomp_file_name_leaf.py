#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문459b20 이름 getter 전체와 옵션 저장 부모의 추가 경로 고리 소비를 관측한다."""
import argparse
import csv
import json
import socket
from pathlib import Path

from decomp_options_path_parent import OptionsOracle
from decomp_file_name_formatter import Oracle as NameOracle, REGISTERS, RING, INDEX
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

FIXTURE = ROOT / 'cpppj/tests/fixtures/file-name-leaf-x86.tsv'  # 실제 반환 offset과 전체256BYTE를 최초 고정한다.


def add_ranges(oracle, entries):
    """전체 함수표에 있는 실제 범위만 기존 코드 화이트리스트에 더한다."""
    facts = {int(row['entry'], 16): row for row in csv.DictReader(
        (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
    for entry in entries:  # 떨어진 범위 사이 코드를 임의로 허용하지 않는다.
        for part in facts[entry]['ranges'].split(';'):  # 각 원본 함수의 실제 끝 BYTE도 포함한다.
            low, high = (int(value, 16) for value in part.split('-'))
            oracle.allowed.append((low, high + 1))


class LeafOracle(NameOracle):
    """같은 고리의 파일명 반환과 선택된 확장자 제거459550을 실제 CPU로 실행한다."""
    def __init__(self):
        """기존 고리/스택 쓰기 제한을 유지하고 두 실제 함수만 더한다."""
        super().__init__()
        add_ranges(self, (0x459b20, 0x459550))

    def leaf(self, value, extension, separator, precision):
        """구분자 없는 경우의 빈 반환·내부 포인터·확장자/NUL 뒤 바이트를 구별한다."""
        self.thrown = False  # 정상 getter의 throw를 성공으로 바꾸지 않는다.
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, 0)
        self.put(0x532728, separator, 1)
        self.mu.mem_write(TEXT, value + b'\0')
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        for number, register in enumerate(REGISTERS):  # 비영 보존 레지스터로 실제 규약을 확인한다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        before = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_ECX, TEXT)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.put(STACK, STOP)
        self.put(STACK + 4, extension)
        self.mu.emu_start(0x459b20, STOP, count=20000)
        assert not self.thrown and self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 8
        assert before == [self.mu.reg_read(register) for register in REGISTERS]
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision and self.words(INDEX, 1) == (1,)
        assert bytes(self.mu.mem_read(TEXT, len(value) + 1)) == value + b'\0'
        target = self.mu.reg_read(UC_X86_REG_EAX)
        assert RING + 256 <= target < RING + 512
        return dict(offset=target - RING - 256, valueHex=self.string(target).hex(), slotHex=bytes(self.mu.mem_read(RING + 256, 256)).hex())


class SaveOracle(OptionsOracle):
    """441d10→440f30→실제FileSpec/이름getter/41b4c0의 경로만 실행한다."""
    def __init__(self):
        """설정 내용 조회/실제 파일/key 경계만 분리하고 전체 경로 하위는 허용한다."""
        super().__init__()
        self.mu.mem_map(0, 0x1000)
        self.events = []  # 실행 중 선택한 실제 getter/formatter/설정 조회/디스크 인자다.
        add_ranges(self, (0x440f30, 0x45a220, 0x459610, 0x4595f0, 0x459900,
                          0x459b20, 0x459550, 0x41b4c0, 0x41a5a0, 0x41b410, 0x4e7620))

    def guard(self, mu, access, address, size, value, data):
        """원문 SEH 첫 슬롯과 changed 쓰기를 명시 허용하고 나머지 고리 제한을 유지한다."""
        if (address == 0 and size == 4) or (address == 0x558dfc and size == 4):
            return
        NameOracle.guard(self, mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """설정/디스크/key의 실제 진입만 경계로 남기고 이름·부모 분기는 실행한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address in (0x459c60, 0x459b20, 0x459900):
            self.events.append(dict(operation=f'{address:08x}'))
        if address == 0x440570:
            name, length = self.words(esp + 4, 2)
            self.events.append(dict(section=self.string(name).hex()))
            self.put(length, 0)
            self.ret(0, 8)
            return
        if address == 0x41a5b0:
            name, mode, create = self.words(esp + 4, 3)
            assert mode == 2 and create == 1
            self.events.append(dict(disk=self.string(name).hex(), mode=mode, create=create))
            self.ret(1, 12)
            return
        if address == 0x41a620:
            name = self.words(esp + 4, 1)[0]
            assert self.string(name) == b'mydoghasfleas'
            self.events.append(dict(key=True))
            self.ret(0, 4)
            return
        if address == 0x4e59a3:
            self.ret()
            return
        NameOracle.instruction(self, mu, address, size, data)

    def save(self, root, precision):
        """실제 저장 부모를 정상 primary-open 경계로 실행해 추가 한 칸들을 센다."""
        self.events.clear()
        self.thrown = False
        self.put(0, 0)
        self.put(0x558dfc, 1)
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, 0)
        self.put(0x532728, 92, 1)
        self.mu.mem_write(0x544d80, b'C:\\base\0')
        self.mu.mem_write(0x544e80, b'D:\\disc\0')
        value = root or b''  # 실제 부모 인자는 NULL과 빈 문자열 포인터를 구별한다.
        self.mu.mem_write(TEXT, value + b'\0')
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        for number, register in enumerate(REGISTERS):  # 정상 반환의 장기 레지스터 수명을 확인한다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        before = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.put(STACK, STOP)
        self.put(STACK + 4, 0 if root is None else TEXT)
        self.mu.emu_start(0x441d10, STOP, count=40000)
        assert not self.thrown and self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert before == [self.mu.reg_read(register) for register in REGISTERS] and self.mu.reg_read(UC_X86_REG_FPCW) == precision
        assert self.words(0, 1) == (0,) and self.words(0x558dfc, 1) == (0,)
        advances = self.words(INDEX, 1)[0]
        assert advances == sum('operation' in event for event in self.events)
        return dict(rootHex=None if root is None else root.hex(), advances=advances, events=list(self.events))


def main():
    """새 이름 getter의 독립 출력과 설정 저장 경로 진입 순서를 함께 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists()
    if not args.verify:
        assert not FIXTURE.exists()
    oracle = LeafOracle()
    values = [b'', b'file.ext', b'.\\file.ext', b'd\\file.name.ext', b'C:file.ext', b'\\\\server\\share\\.hidden',
              b'd\\.hidden', b'd\\file.', b'd\\.', b'd\\..', b'd\\', b'd/file.ext', b'a:b:c.ext', b'd\\a\0ignored', b'd\\\xe9cole.ext', b'd\\' + b'x' * 253]
    fixture = ['# separator extension input_hex output_hex offset current_slot_hex']
    for value in values:  # 원문은 구분자 없는 bare 이름을 빈 문자열로 반환한다.
        for separator in (92, 47):  # 현재 비교 BYTE와 고정 접두를 혼동하지 않는다.
            for extension in (0, 1):  # 확장자 제거는 선두 dot을 보존한다.
                result = oracle.leaf(value, extension, separator, 0x027f)
                assert result == oracle.leaf(value, extension, separator, 0x037f)
                fixture.append(f'{separator}\t{extension}\t{value.hex() or "-"}\t{result["valueHex"] or "-"}\t{result["offset"]}\t{result["slotHex"]}')
    saved = SaveOracle()
    saves = []
    for root in (None, b'', b'relative', b'F:\\custom'):  # 같은 부모의 상대 성공과 직접 장치 성공 소모량을 구별한다.
        result = saved.save(root, 0x027f)
        assert result == saved.save(root, 0x037f)
        saves.append(result)
    content = '\n'.join(fixture) + '\n'
    if args.verify:
        assert FIXTURE.read_text('utf-8') == content
    else:
        FIXTURE.write_text(content, 'utf-8', newline='\n')
    record = dict(unit=730, date='2026-11-02', host=socket.gethostname(),
                  lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'),
                  fixtureRows=64, observations=136, saves=saves,
                  leafBoundary='실제459b20과선택된459550 전체. 출력offset/전체256BYTE·입력/보존레지스터/FPCW/ESP 확인',
                  saveBoundary='실제441d10→440f30/FileSpec/이름getter/41b4c0. 설정section은없음/길이0, disk mode2/create1은성공, key저장은인자관측만. 내용쓰기/실제OS/예외unwind는실행하지않음',
                  gameExecutions=0, multiplayerTests=0,
                  sha256={str(path.relative_to(ROOT)): digest(path) for path in (
                      ROOT / 'originals/Netstorm.exe', Path(__file__), ROOT / 'tools/decomp_options_path_parent.py', FIXTURE)})
    args.output.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps(dict(fixtureRows=64, observations=136, saveAdvances=[row['advances'] for row in saves])))


if __name__ == '__main__':
    main()
