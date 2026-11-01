#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""MoveDialog 004c8df0과 실제strchr의 좌표/순서를 원문으로 동결한다. 게임은 실행하지 않는다."""
import argparse
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_viewspot_oracle import load, INPUT, STACK, STOP, PE
from decomp_gump_redraw import digest
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

ENTRY, PUSH, ATTACH = 0x4c8df0, STOP+0x10, 0x466750  # 가상Push/공통Attach만 외부 효과 경계다.
NODE, OTHER, VTABLE = INPUT+0x1000, INPUT+0x1100, INPUT+0x1800  # 같은 현재 노드/교체된포인터/가상표의 별도 메모리다.


def bits(value):
    """명시float 입력을십진수손실없이DWORD로보존한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def plan():
    """문자순서·대소문자·NUL·화면/창치수·Push뒤변경을기대좌표계산없이교차한다."""
    texts = ('', 'br', 'tl', 'tr', 'bl', 'tm', 'mc', 'bm', 'mr', 'ml', 'bt', 'tb', 'rl', 'lr', 'btrl', 'TR', ' b r ', 'b\0tl', 'x', 'm', 'c')
    cases = []
    for text, dimensions in itertools.product(texts, ((1024, 768, 320.5, 240.25), (640, 480, 900.25, 800.5), (16777217, 16777219, 0.5, 0.25))):
        w, h, width, height = dimensions
        cases.append((text, 1, w, h, bits(width), bits(height), bits(33.25), bits(44.5), 0))
    cases.extend((text, 0, 1024, 768, bits(320), bits(240), bits(33.25), bits(44.5), 0) for text in texts[:5])
    cases.extend((text, 1, 1024, 768, bits(320), bits(240), bits(33.25), bits(44.5), 1) for text in ('br', 'tl', 'tm', 'mc', 'btrl'))
    return cases


def run(machine, case, control):
    """원문은실제strchr/산술을실행하고두가상외부효과의인자와노드전체를관측한다."""
    text, present, screen_w, screen_h, width, height, x, y, changed = case
    raw = bytearray([0x71]*0xa0)
    struct.pack_into('<I', raw, 0, VTABLE)
    for at, value in ((0x3c, width), (0x40, height), (0x4c, x), (0x50, y)):
        struct.pack_into('<I', raw, at, value)
    machine.mem_write(NODE, bytes(raw))
    machine.mem_write(VTABLE+0x18, struct.pack('<I', PUSH))
    machine.mem_write(0x565dd4, struct.pack('<I', NODE if present else 0))
    machine.mem_write(0x531860, struct.pack('<ii', screen_w, screen_h))
    machine.mem_write(INPUT, text.encode('latin-1')+b'\0')
    events = []

    def hook(mu, address, size, data):
        """Push 효과와 Attach 실제 인자만 외부 경계로 두며 기대 좌표를 계산하지 않는다."""
        if address not in (PUSH, ATTACH):
            return
        esp = mu.reg_read(UC_X86_REG_ESP)
        back = struct.unpack('<I', mu.mem_read(esp, 4))[0]
        assert mu.reg_read(UC_X86_REG_ECX) == NODE
        if address == PUSH:
            events.append('P')
            if changed:
                mu.mem_write(0x565dd4, struct.pack('<I', OTHER))
                mu.mem_write(0x531860, struct.pack('<ii', 1920, 1080))
                for at, value in ((0x3c, 123.5), (0x40, 222.25), (0x4c, -10.5), (0x50, 12.25)):
                    mu.mem_write(NODE+at, struct.pack('<f', value))
            mu.reg_write(UC_X86_REG_ESP, esp+4)
        else:
            events.append('A:'+','.join(f'{value:08x}' for value in struct.unpack('<4I', mu.mem_read(esp+4, 16))))
            mu.reg_write(UC_X86_REG_ESP, esp+20)
        mu.reg_write(UC_X86_REG_EIP, back)

    handle = machine.hook_add(UC_HOOK_CODE, hook)
    machine.mem_write(STACK, struct.pack('<2I', STOP, INPUT))
    machine.reg_write(UC_X86_REG_ESP, STACK)
    machine.reg_write(UC_X86_REG_EAX, 0)
    machine.reg_write(UC_X86_REG_FPCW, control)
    machine.reg_write(UC_X86_REG_FPSW, 0)
    machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
    machine.emu_start(ENTRY, STOP, count=100000)
    machine.hook_del(handle)
    assert machine.reg_read(UC_X86_REG_FPCW) == control and not machine.reg_read(UC_X86_REG_FPSW) & 0x3800
    return bytes(machine.mem_read(NODE, 0xa0)).hex(), ';'.join(events) or '-'


def main():
    """기본/보조x87및재관측을확인한원문만새파일에쓴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    machine, cases = load(), plan()
    first = [run(machine, case, 0x27f) for case in cases]
    assert first == [run(machine, case, 0x27f) for case in cases] == [run(machine, case, 0x37f) for case in cases]
    fixture = args.output/'move-dialog-x86.tsv'
    with fixture.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# text_hex\tinput_dwords\tfinal_node\tevents\n')
        for case, (raw, events) in zip(cases, first):
            stream.write('\t'.join((case[0].encode('latin-1').hex() or '-', ','.join(str(value & 0xffffffff) for value in case[1:]), raw, events))+'\n')
    report = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23',
              'cases': len(cases), 'observations': len(cases)*3, 'differences': 0, 'sourceSha256': digest(PE), 'toolSha256': digest(Path(__file__)),
              'fixtureSha256': digest(fixture), 'scope': '실제004c8df0/strchr·float좌표. 가상Push·Attach만명시효과. 게임/포획/희생/공격/승리/MP검사없음.'}
    (args.output/'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
