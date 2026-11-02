#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문459990 전체 디렉터리 getter와435cd0의 모듈 경로 부모를 CPU로 관측한다."""
import argparse
import json
from pathlib import Path
import socket
from decomp_file_name_leaf import LeafOracle, add_ranges
from decomp_file_name_formatter import Oracle as NameOracle, REGISTERS, RING, INDEX
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, digest
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW
import pefile

DIRECTORY = ROOT / 'cpppj/tests/fixtures/file-directory-x86.tsv'  # 전체256BYTE/부분 출력과 잘못된 입력의 접근 실패를 고정한다.
PARENT = ROOT / 'cpppj/tests/fixtures/file-module-directory-x86.tsv'  # 실제 부모의 출력/3칸 소모를 따로 고정한다.
MODULE, FILENAME, GAME_ROOT = STOP + 0x100, STOP + 0x200, 0x54d988  # 두 OS 경계와 실제 원문 목적지다.


class DirectoryOracle(LeafOracle):
    """실제459990/strchr를 실행하며 없는 separator를 빈 정상 출력으로 합성하지 않는다."""
    def __init__(self):
        """현재 고리 쓰기 계약에 실제 디렉터리 함수만 추가한다."""
        super().__init__()
        add_ranges(self, (0x459990,))

    def directory(self, value, level, separator, precision):
        """모드별 부분 버퍼·반환·접근 실패와 정상 함수 규약을 관측한다."""
        self.thrown = False
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, 0)
        self.put(0x532728, separator, 1)
        self.mu.mem_write(TEXT, value + b'\0')
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        for number, register in enumerate(REGISTERS):  # 정상 반환의 비영 보존 레지스터를 비교한다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        before = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_ECX, TEXT)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.put(STACK, STOP)
        self.put(STACK + 4, level)
        status = 'ok'
        try:
            self.mu.emu_start(0x459990, STOP, count=50000)
        except UcError as error:
            assert 'Invalid memory read' in str(error), str(error)
            status = 'read-fault'
        assert not self.thrown and self.words(INDEX, 1) == (1,)
        assert bytes(self.mu.mem_read(TEXT, len(value) + 1)) == value + b'\0'
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        if status == 'ok':
            assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 8
            assert self.mu.reg_read(UC_X86_REG_EAX) == RING + 256 and before == [self.mu.reg_read(register) for register in REGISTERS]
        return status, bytes(self.mu.mem_read(RING + 256, 256)).hex()


class ModuleOracle(DirectoryOracle):
    """435cd0의 전체 정상 부모를 실제 FileSpec/디렉터리/장치/작성기로 실행한다."""
    def __init__(self):
        """두 모듈 Win32 import만 명시 OS 응답에 연결한다."""
        super().__init__()
        self.mu.mem_map(0, 0x1000)
        self.executable = b''  # 현재 GetModuleFileName이 공급할 원래 C문자열이다.
        self.events = []  # 실제 OS와 세 고리 함수의 선택 순서다.
        add_ranges(self, (0x435cd0, 0x45a220, 0x459610, 0x4595f0, 0x459900, 0x4e7620))
        pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
        connected = set()
        for library in pe.DIRECTORY_ENTRY_IMPORT:  # 실제 IAT 이름/주소에 있는 두 함수만 바꾼다.
            for item in library.imports:  # 다른 import는 성공으로 넘기지 않는다.
                if item.name in (b'GetModuleHandleA', b'GetModuleFileNameA'):
                    self.put(item.address, MODULE if item.name == b'GetModuleHandleA' else FILENAME)
                    connected.add(item.name)
        assert len(connected) == 2

    def guard(self, mu, access, address, size, value, data):
        """원문의 SEH와 모듈 루트 목적지만 추가 허용한다."""
        if (address == 0 and size == 4) or (GAME_ROOT <= address and address + size <= GAME_ROOT + 256):
            return
        NameOracle.guard(self, mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """OS의 실제 호출 인자와 Directory→Device→Format 진입만 기록한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == MODULE:
            assert self.words(esp + 4, 1) == (0,)
            self.events.append('module')
            self.ret(0x1234, 4)
            return
        if address == FILENAME:
            module, output, capacity = self.words(esp + 4, 3)
            assert module == 0x1234 and capacity == 260
            mu.mem_write(output, self.executable + b'\0')
            self.events.append('filename')
            self.ret(len(self.executable), 12)
            return
        if address in (0x459990, 0x459900, 0x459c60):
            self.events.append(f'{address:08x}')
        NameOracle.instruction(self, mu, address, size, data)

    def parent(self, supplied, executable, precision):
        """명시 인자는 복사만, 빈 인자는 실제 모듈 위치의 세 칸을 소비한다."""
        self.events.clear()
        self.executable = executable
        self.thrown = False
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, 0)
        self.put(0x532728, 92, 1)
        self.put(0, 0)
        self.mu.mem_write(GAME_ROOT, bytes([0x65]) * 256)
        self.mu.mem_write(TEXT, supplied + b'\0')
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        for number, register in enumerate(REGISTERS):  # 부모 전체의 정상 반환 규약도 확인한다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        before = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.put(STACK, STOP)
        self.put(STACK + 4, TEXT)
        self.mu.emu_start(0x435cd0, STOP, count=50000)
        assert not self.thrown and self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert before == [self.mu.reg_read(register) for register in REGISTERS]
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision and self.words(0, 1) == (0,)
        assert bytes(self.mu.mem_read(TEXT, len(supplied) + 1)) == supplied + b'\0'
        return dict(pathHex=self.string(GAME_ROOT).hex(), advances=self.words(INDEX, 1)[0],
                    events=list(self.events), ringHex=bytes(self.mu.mem_read(RING, 2560)).hex())


def main():
    """두 최초 fixture를 덮지 않고 동일 재관측을 지원한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists()
    if not args.verify:
        assert not DIRECTORY.exists() and not PARENT.exists()
    oracle = DirectoryOracle()
    values = [b'.\\d\\file.ext', b'C:\\d\\file.ext', b'\\\\server\\share\\dir\\file', b'\\file', b'd\\file',
              b'..\\d\\file', b'...\\d\\file', b'.\\one\\two\\file', b'\\one\\two\\file', b'C:file', b'file', b'',
              b'\\\\server\\', b'.\\..\\file', b'\\..\\file', b'.\\' + b'x' * 251 + b'\\f']
    directory = ['# separator level input_hex status current_slot_hex']
    faults = 0
    for raw in values:  # 두 separator에서 같은 구조의 이미 작성된 FileSpec을 공급한다.
        for separator in (92, 47):  # 전역 비교 문자와 고정 역슬래시 연결을 분리해 관측한다.
            value = raw.replace(b'\\', bytes([separator]))
            for level in (-2, -1, 0, 1, 2, 7):  # 모든 실제 모드와 범위를 벗어나는 레벨을 구별한다.
                result = oracle.directory(value, level, separator, 0x027f)
                assert result == oracle.directory(value, level, separator, 0x037f)
                faults += result[0] != 'ok'
                directory.append(f'{separator}\t{level}\t{value.hex() or "-"}\t{result[0]}\t{result[1]}')
    module = ModuleOracle()
    cases = [(b'', b'C:\\Game\\Netstorm.exe'), (b'', b'C:/Game/Netstorm.exe'),
             (b'', b'\\\\server\\share\\game\\Netstorm.exe'), (b'', b'C:\\Netstorm.exe'),
             (b'', b'D:\\a.b\\c\\Netstorm.exe'), (b'given/root', b'C:\\ignored.exe')]
    parent = ['# supplied_hex executable_hex output_hex advances ring_hex']
    rows = []
    for supplied, executable in cases:  # 실제 무인자 부모와 명시 경로의 고리 미사용 분기를 구별한다.
        result = module.parent(supplied, executable, 0x027f)
        assert result == module.parent(supplied, executable, 0x037f)
        parent.append('\t'.join((supplied.hex() or '-', executable.hex(), result['pathHex'] or '-', str(result['advances']), result['ringHex'])))
        rows.append(dict(suppliedHex=supplied.hex(), executableHex=executable.hex(), pathHex=result['pathHex'], advances=result['advances'], events=result['events']))
    for path, lines in ((DIRECTORY, directory), (PARENT, parent)):  # 최초 출력만 생성하고 verify에서는 바이트 단위로 비교한다.
        content = '\n'.join(lines) + '\n'
        if args.verify:
            assert path.read_text('utf-8') == content
        else:
            path.write_text(content, 'utf-8', newline='\n')
    report = dict(unit=734, date='2026-11-02', host=socket.gethostname(),
                  lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'),
                  directoryRows=192, directoryReadFaultRows=faults, parentRows=rows, observations=396,
                  boundary='실제459990/strchr전체와435cd0/FileSpec/디렉터리/장치/전체작성기. 모듈Win32두경계만명시응답. 없는separator의Native read fault는정상출력으로합성하지않음',
                  gameExecutions=0, multiplayerTests=0,
                  sha256={str(path.relative_to(ROOT)):digest(path) for path in (ROOT/'originals/Netstorm.exe', Path(__file__), DIRECTORY, PARENT)})
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', 'utf-8')
    print(json.dumps(dict(directoryRows=192, faults=faults, parentAdvances=[row['advances'] for row in rows], observations=396)))


if __name__ == '__main__':
    main()
