#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 읽기 부모의 경로 고리 소모와 저장 상태 부모→전체 작성기를 CPU로 관측한다."""
import argparse
import csv
import json
from pathlib import Path
import socket

from decomp_basefile_read_policy import Oracle as ReadOracle
from decomp_file_name_formatter import Oracle as NameOracle, REGISTERS, RING, INDEX
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

FIXTURE = ROOT / 'cpppj/tests/fixtures/file-name-device-x86.tsv'  # 장치 getter의 현재 전체256BYTE 최초 출력이다.


class ReadCounter(ReadOracle):
    """기존 실제 읽기 부모에서 두 종류의 고리 소비 진입을 각각 기록한다."""
    def __init__(self):
        """현재 호출의 진입 목록은 원문 실행 중에만 추가한다."""
        self.operations = []  # 장치 문자열 getter와 가변 경로 작성기를 구별하는 실제 진입 순서다.
        super().__init__()

    def instruction(self, mu, address, size, data):
        """결과 번호를 formatter 호출 수라고 가정하지 않고 실제 함수 진입을 센다."""
        if address in (0x459900, 0x459c60):
            self.operations.append(f'{address:08x}')
        super().instruction(mu, address, size, data)


class DeviceOracle(NameOracle):
    """FileSpec의 이미 작성된 문자열에서 실제459900/strchr 전체만 실행한다."""
    def __init__(self):
        """같은 고리/쓰기 계약에 장치 getter의 실제 코드 범위만 더한다."""
        super().__init__()
        facts = {int(row['entry'], 16): row for row in csv.DictReader(
            (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        for part in facts[0x459900]['ranges'].split(';'):  # 별도 사본의 문자열이나 OS 판정을 합성하지 않는다.
            low, high = (int(value, 16) for value in part.split('-'))
            self.allowed.append((low, high + 1))

    def device(self, value, separator, precision):
        """선증가/clear·복사·첫BYTE/colon/UNC 절삭 뒤 NUL 바깥 바이트까지 읽는다."""
        self.thrown = False  # 정상 경계 밖 throw가 발생하면 바로 관측 실패다.
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, 0)
        self.put(0x532728, separator, 1)
        self.mu.mem_write(TEXT, value + b'\0')
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        for number, register in enumerate(REGISTERS):  # 실제 thiscall의 비영 보존 레지스터를 대조한다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        saved = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_ECX, TEXT)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.put(STACK, STOP)
        self.mu.emu_start(0x459900, STOP, count=20000)
        assert not self.thrown and self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert saved == [self.mu.reg_read(register) for register in REGISTERS]
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision and self.words(INDEX, 1) == (1,)
        assert self.mu.reg_read(UC_X86_REG_EAX) == RING + 256
        assert bytes(self.mu.mem_read(TEXT, len(value) + 1)) == value + b'\0'
        return bytes(self.mu.mem_read(RING + 256, 256)).hex()


class SavedNameOracle(NameOracle):
    """실제4b5470과459c60/strchr를 실행하고 고정 sprintf만 외부 장치로 남긴다."""
    def __init__(self):
        """같은 쓰기/코드 화이트리스트에 원문 부모의 실제 범위만 더한다."""
        super().__init__()
        facts = {int(row['entry'], 16): row for row in csv.DictReader(
            (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        for part in facts[0x4b5470]['ranges'].split(';'):  # 부모의 떨어진 코드 범위를 임의로 넓히지 않는다.
            low, high = (int(value, 16) for value in part.split('-'))
            self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """원문이 작성한 state와 실제 서식 인자만 읽어 지역 출력에 돌려준다."""
        if address == 0x4a89c0:
            target, pattern, name, zero = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 4)
            assert self.string(pattern) == b'!%s.dat' and self.string(name) == b'state' and zero == 0
            assert STACK - 0x2000 <= target <= STACK - 16
            mu.mem_write(target, b'!state.dat\0')
            self.ret(target)
            return
        super().instruction(mu, address, size, data)

    def parent(self, precision):
        """0번호의 고리에서 실제 부모를 호출해 경로·번호·정상 반환을 관찰한다."""
        self.thrown = False  # 같은 throw 관측기는 정상 부모 반환과 예외 경계를 구별한다.
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, 0)
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        for number, register in enumerate(REGISTERS):  # 실제 보존 규약은 비영 값으로 확인한다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        saved = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.put(STACK, STOP)
        self.mu.emu_start(0x4b5470, STOP, count=30000)
        assert not self.thrown and self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert saved == [self.mu.reg_read(register) for register in REGISTERS] and self.mu.reg_read(UC_X86_REG_FPCW) == precision
        index = self.words(INDEX, 1)[0]
        return dict(index=index, path=self.string(RING + index * 256).decode('ascii'),
                    slotHex=bytes(self.mu.mem_read(RING + index * 256, 256)).hex())


def main():
    """기존 읽기 오라클의 경계와 새 전체 이름 부모 관측을 별도 새 보고서에 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists()
    if not args.verify:
        assert not FIXTURE.exists()
    oracle = ReadCounter()
    cases = [('primary', b'd\\one.dat', 2), ('secondary', b'd\\two.dat', 8),
             ('missing', b'missing.dat', 0), ('direct', b'C:\\base\\one.dat', 1),
             ('archive', b'd\\archive.dat', 4)]
    rows = []
    for label, name, mask in cases:  # 각 실제 읽기 분기에서 초기0 뒤의 현재 번호를 읽는다.
        oracle.operations.clear()
        result = oracle.sample((b'C:\\base', name, mask, 1))
        advances = oracle.words(INDEX, 1)[0]
        assert advances == len(oracle.operations)
        rows.append(dict(label=label, ringAdvances=advances, operations=list(oracle.operations), trace=result))
    parent = SavedNameOracle()
    normal, extended = parent.parent(0x027f), parent.parent(0x037f)
    assert normal == extended
    device = DeviceOracle()
    values = [b'', b'.\\', b'.\\d\\one.dat', b'C:\\base\\file', b'C:file', b'\\\\server\\share\\file',
              b'\\\\server', b'\\\\', b'\\\\\\share', b':file', b'\\\\x\\', b'/raw/slash',
              b'abc:def:ghi', b'foo\\bar', b'raw\0ignored', b'X' * 255]
    fixture = ['# separator input_hex current_slot_hex']
    for value in values:  # 이미 정규화된 FileSpec과 명시 BYTE 경계 입력을 구별해 그대로 공급한다.
        for separator in (92, 47):
            result = device.device(value, separator, 0x027f)
            assert result == device.device(value, separator, 0x037f)
            fixture.append(f'{separator}\t{value.hex() or "-"}\t{result}')
    content = '\n'.join(fixture) + '\n'
    if args.verify:
        assert FIXTURE.read_text('utf-8') == content
    else:
        FIXTURE.write_text(content, 'utf-8', newline='\n')
    record = dict(unit=728, date='2026-11-02', host=socket.gethostname(),
                  lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'),
                  readRows=rows, savedState=normal, deviceRows=32, observations=71,
                  readBoundary='기존 읽기 부모/실제FileSpec·장치판정·전체459c60을 실행하며 파일/TAFF·CRT조회는 기존 명시 장치다',
                  savedBoundary='실제4b5470→459c60/strchr이며 고정!%s.dat/state의sprintf만 명시 장치다',
                  gameExecutions=0, multiplayerTests=0,
                  sha256={str(path.relative_to(ROOT)): digest(path) for path in (
                      ROOT / 'originals/Netstorm.exe', ROOT / 'tools/decomp_file_path_consumers.py',
                      ROOT / 'tools/decomp_basefile_read_policy.py', ROOT / 'tools/decomp_file_name_formatter.py', FIXTURE)})
    args.output.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps(dict(readAdvances=[row['ringAdvances'] for row in rows], deviceRows=32, savedIndex=normal['index'], savedPath=normal['path'])))


if __name__ == '__main__':
    main()
