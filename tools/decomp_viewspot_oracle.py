#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 ViewSpot 433ad0의 실제 기계어로 인자를 읽어 497f20에 넘어가는 두 float를 기록한다(단위676).

실제 실행: 433ad0 몸체, 487710/487620 토큰 읽기, x87 변환.
명시 경계: 497f20 진입에서 인자만 읽고 멈춘다. 카메라 효과는 tools/decomp_camera_immediate.py가 이미 대조했다.
4e66de atol은 CRT 잠금/힙 초기화에 의존해 격리 실행하지 못하므로 진입에서 ConfigParseLong과 같은 규칙으로 대신 계산한다.
--verify는 저장된 fixture를 같은 실행으로 다시 만들어 바이트를 비교한다.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'extracted/oracle-python'))
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW

PE = ROOT/'originals/Netstorm.exe'
FIXTURE = ROOT/'cpppj/tests/fixtures/viewspot-x86.tsv'
REPORT = ROOT/'cpppj/viewspot-native.json'
ENTRY, CENTER, ATOL, STOP = 0x433ad0, 0x497f20, 0x4e66de, 0x0f000000  # 대상 함수·두 경계·호출자 복귀 표식 주소다.
STACK, INPUT = 0x10080000, 0x20000000  # 합성 스택과 입력 문자열 주소다.
CASES = ['(139x88)', '(05x05)', '(250x05)', '(05x250)', '(125x125)', '(144x60)', '(40x200)', '(117x130)', '(0x0)', '139x88', '(139x88', '(139)',
         '(x88)', '()', '', '(', ')', 'x', '( 7 x 8 )', '(-3x-4)', '(+5x+6)', '(1x2x3)', '(7x8)junk', '((3x4))', '(0x10x4)', '(2147483648x1)',
         '(99999999999x1)', '("5"x6)', '(5x"6")', '(\t9x\t10)', '(12abcx34def)', '(' + '1'*90 + 'x5)', '(5x' + '2'*90 + ')', '(1 2x3 4)']  # 실제 대본 형식과 경계 입력이다.


def atol(raw):
    """MSVC atol 규칙이다. 앞 공백·부호 뒤 숫자만 읽고 넘침은 32비트로 감긴다(o::ConfigParseLong과 같다)."""
    index, sign, value = 0, 1, 0
    # C 로캘 isspace의 여섯 문자를 건너뛴다.
    while index < len(raw) and raw[index] in b' \t\n\v\f\r':
        index += 1
    if index < len(raw) and raw[index] in b'+-':
        sign = -1 if raw[index] == 0x2d else 1
        index += 1
    # 숫자가 아닌 첫 바이트에서 멈춘다.
    while index < len(raw) and 0x30 <= raw[index] <= 0x39:
        value = (value*10+raw[index]-0x30) & 0xffffffff
        index += 1
    return (value*sign) & 0xffffffff


def load():
    """보호 PE의 섹션을 읽기 전용 원본에서 합성 메모리에 올린다."""
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0, 0x2000)  # SEH fs:[0] 읽기용 빈 페이지다.
    machine.mem_map(0x400000, 0x400000)
    machine.mem_map(STOP, 0x1000)
    machine.mem_map(STACK-0x40000, 0x80000)
    machine.mem_map(INPUT, 0x2000)
    data = PE.read_bytes()
    header = struct.unpack_from('<I', data, 0x3c)[0]
    count, optional = struct.unpack_from('<H', data, header+6)[0], struct.unpack_from('<H', data, header+20)[0]
    base = struct.unpack_from('<I', data, header+52)[0]
    # 각 섹션의 파일 원시 크기만 복사하고 나머지 BSS는 0으로 남긴다.
    for index in range(count):
        offset = header+24+optional+40*index
        virtual, raw_size, raw = struct.unpack_from('<III', data, offset+12)
        machine.mem_write(base+virtual, data[raw:raw+raw_size])
    return machine


def run(machine, text):
    """한 입력을 실제 함수로 실행하고 497f20이 받은 두 float 비트를 돌려준다."""
    seen = []

    def hook(emulator, address, _size, _data):
        """경계 진입에서 스택 인자만 읽고 실행을 멈춘다."""
        esp = emulator.reg_read(UC_X86_REG_ESP)
        if address == CENTER:
            seen.extend(struct.unpack('<2I', emulator.mem_read(esp+4, 8)))
            emulator.emu_stop()
        elif address == ATOL:  # cdecl 인자 문자열을 읽어 호출자에게 바로 돌려준다.
            back, pointer = struct.unpack('<2I', emulator.mem_read(esp, 8))
            raw = bytes(emulator.mem_read(pointer, 256))
            emulator.reg_write(UC_X86_REG_EAX, atol(raw[:raw.index(0)]))
            emulator.reg_write(UC_X86_REG_ESP, esp+4)
            emulator.reg_write(UC_X86_REG_EIP, back)

    handle = machine.hook_add(UC_HOOK_CODE, hook)
    machine.mem_write(INPUT, text.encode('latin-1')+b'\0')
    machine.mem_write(STACK, struct.pack('<2I', STOP, INPUT))
    machine.reg_write(UC_X86_REG_ESP, STACK)
    machine.reg_write(UC_X86_REG_EAX, 0)
    machine.reg_write(UC_X86_REG_FPCW, 0x27f)
    machine.emu_start(ENTRY, STOP, count=2000000)
    machine.hook_del(handle)
    if len(seen) != 2:
        raise RuntimeError(f'경계 미도달: {text!r}')
    return seen


def generate():
    """모든 입력의 실제 결과를 탭 표로 만든다."""
    machine = load()
    lines = ['input_hex\tx_bits\ty_bits']
    # 입력은 16진수로 저장해 탭/공백/따옴표를 fixture 구분자와 분리한다.
    for text in CASES:
        x, y = run(machine, text)
        lines.append(f"{text.encode('latin-1').hex() or '-'}\t{x:08x}\t{y:08x}")
    return '\n'.join(lines)+'\n'


def main():
    """fixture와 근거 보고를 만들거나 저장본과 비교한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    arguments = parser.parse_args()
    text = generate()
    if arguments.verify:
        if FIXTURE.read_text(encoding='utf-8') != text:
            raise SystemExit('fixture 불일치')
        print(f'대조 {len(CASES)}행 일치')
        return
    FIXTURE.write_text(text, encoding='utf-8', newline='\n')
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    REPORT.write_text(json.dumps({'unit': 676, 'function': '00433ad0', 'rows': len(CASES), 'pe': digest(PE), 'fixture': digest(FIXTURE),
                                  'tool': digest(Path(__file__)), 'boundary': '497f20 진입에서 스택 인자를 읽고 멈춘다.'}, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(f'{len(CASES)}행 작성')


if __name__ == '__main__':
    main()
