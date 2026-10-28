#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본4daff0의 실제 RGB 곱셈/ftol을 고정 primary 입력에서 독립 관측한다."""
import hashlib
import json
import socket
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

# 원문 루프와 실제 CRT 변환 본체 외에는 명시 RGB 할당 장치만 사용한다.
ROOT = Path(__file__).resolve().parents[1]  # 변경하지 않는 보호 입력과 새 자료의 루트다.
PE = ROOT/'originals/Netstorm.exe'  # 읽기 전용10.78 기계어 입력이다.
OUTPUT = ROOT/'extracted/cpp-screenshot-key-548'  # 이번 단위의 정밀도별 관측 폴더다.
FIXTURE = ROOT/'cpppj/tests/fixtures/screenshot-palette-x86.tsv'  # 입력 double 비트와 두 원문 결과를 함께 담는다.
PRIMARY, STACK = 0x1000000, 0x2100000  # 격리한 raw 기록과 원문 호출 스택 주소다.


def observe(control):
    """256개의 명시 primary 기록을 같은 원문 루프에서 RGB 정수로 읽는다."""
    pe = pefile.PE(str(PE))
    mu = Uc(UC_ARCH_X86, UC_MODE_32)
    mu.mem_map(0x400000, (pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
    mu.mem_write(0x400000, pe.get_memory_mapped_image())
    mu.mem_map(PRIMARY, 0x4000)
    mu.mem_map(STACK-0x10000, 0x20000)
    records, result = [], []
    unit = struct.unpack('<d', struct.pack('<Q', 0x3f70101010101010))[0]
    # 원문 primary에 이미 저장된 각 채널의 binary64를 외부 입력으로 제공한다.
    for index in range(256):
        values = [index*unit, (255-index)*unit, ((index*73)&255)*unit]
        records.append(values)
        mu.mem_write(PRIMARY+index*56+24, struct.pack('<3d', *values))
    mu.mem_write(0x555d88, struct.pack('<I', PRIMARY))
    mu.reg_write(UC_X86_REG_ESP, STACK)
    mu.reg_write(UC_X86_REG_EBX, 0)
    mu.reg_write(UC_X86_REG_EBP, PRIMARY+0x3900)
    mu.reg_write(UC_X86_REG_FPCW, control)

    def instruction(machine, address, size, data):
        """원문 CALL의 RGB 할당 인자를 읽고 원래 복귀주소로 돌아간다."""
        if address == 0x4db07a:
            machine.emu_stop()
            return
        if address == 0x461230:
            esp = machine.reg_read(UC_X86_REG_ESP)
            arguments = struct.unpack('<5I', machine.mem_read(esp, 20))
            result.append(arguments[2:5])
            machine.reg_write(UC_X86_REG_EAX, len(result)-1)
            machine.reg_write(UC_X86_REG_ESP, esp+4)
            machine.reg_write(UC_X86_REG_EIP, arguments[0])
            return
        if 0x4db022 <= address < 0x4db07a or 0x4e49c0 <= address < 0x4e4a40:
            return
        raise RuntimeError(f'미명시 원문 실행 {address:08x}')

    mu.hook_add(UC_HOOK_CODE, instruction)
    mu.emu_start(0x4db022, 0x4db07a, count=1000000)
    assert len(result) == 256
    return records, result


def main():
    """기본53비트 기대값과 확장64비트의 차이를 숨기지 않고 함께 보존한다."""
    assert hashlib.sha256(PE.read_bytes()).hexdigest() == 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    records, precision53 = observe(0x27f)
    _, precision64 = observe(0x37f)
    rows = ['index\tredbits\tgreenbits\tbluebits\tred53\tgreen53\tblue53\tred64\tgreen64\tblue64']
    # 입력 double은 십진수 변환으로 바꾸지 않고 저장된64비트 그대로 보존한다.
    for index, values in enumerate(records):
        bits = [struct.unpack('<Q', struct.pack('<d', value))[0] for value in values]
        rows.append('\t'.join(map(str, (index, *bits, *precision53[index], *precision64[index]))))
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), inputs=256, nativeObservations=512,
                  precisionDifferences=sum(a != b for a, b in zip(precision53, precision64)),
                  productPrecision='53 bits, original default', externalDevices=['gdImageColorAllocate'],
                  fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest())
    (OUTPUT/'palette-native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
