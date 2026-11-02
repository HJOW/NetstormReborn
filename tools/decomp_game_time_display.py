#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""T/t와kind16의실제기계어를격리관측한다. 제품/게임/멀티플레이는실행하지않는다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

PARSER = argparse.ArgumentParser(description=__doc__)  #준비본/정식도구에서같은명시저장소와출력을받는다.
PARSER.add_argument('--repo', type=Path, required=True)
PARSER.add_argument('--output', type=Path, required=True)
ARGS = PARSER.parse_args()  #실제원본위치는다른파일의추측탐색대신명시인자로받는다.
sys.path.insert(0, str(ARGS.repo / 'tools'))
from decomp_message_gump import Oracle as PauseOracle, NODE, NAME, TRANSLATED, ENTRY
from decomp_lost_position_key import Oracle as LostOracle, FIELDS, bits
from decomp_priestmenu import STACK, STOP, TEXT
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW,
    UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

KEY_FLAG, CLOCK_STAMP, CLOCK_ORIGIN, WALL = 0x594fc0, 0x594fb0, 0x594fb4, TEXT+0x6000  #원문세DWORD와명시벽시각저장소다.
FORMAT = b'~E~yGame Time~r:~w %d:%02d:%02d'  #PE00516024의번역키를직접검증한다.
FORMATS = (FORMAT, b'~ETime %d/%02d/%02d', b'')  #현재번역반환은명시입력이고기대시간값은기계어가계산한다.


def signed(value):
    """기록된DWORD서식인자를signed32로해석한다."""
    return struct.unpack('<i', struct.pack('<I', value))[0]


class KeyOracle(LostOracle):
    """이미확정된일반입력부모를재사용하고실제T플래그의store만추가허용한다."""
    def guard(self, mu, access, address, size, value, data):
        """쓰기는기존스택또는실제표시DWORD한칸에만허용한다."""
        if address == KEY_FLAG and size == 4:
            return
        super().guard(mu, access, address, size, value, data)

    def sample(self, event, paused, modal, initial, control):
        """선행뗌/Console/정지/문자분배를같은실제명령으로관측한다."""
        self.dword(KEY_FLAG, initial)
        case = dict(zip(FIELDS, (event, bits(41.25), bits(73.5), 256, paused, modal, 0, bits(111.25), bits(121.5), 0)))
        result = super().run(case, control)
        return self.dword(KEY_FLAG), result['trace'], result['gate']


class MessageOracle(PauseOracle):
    """kind16/CRT정수나눗셈/ftol/0.1기한몸체는실행하고큰UI경계만관측한다."""
    def __init__(self):
        """원문생성자외의세작은함수를검토한실제바이트범위로추가한다."""
        super().__init__()
        self.allowed += [(0x4a68b0, 0x4a68dd), (0x4e49c0, 0x4e4a35), (0x424930, 0x424952)]

    def write(self, mu, access, address, size, value, data):
        """현재CRTstamp쓰기만기존노드/스택경계밖에서허용한다."""
        if address == CLOCK_STAMP and size == 4:
            return
        super().write(mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """실제시간/이름/번역/서식/Style/Idle/측정의순서와정수인자를보존한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e78d0:
            self.event('Clock', self.now)
            self.ret(self.now)
            return
        if address == 0x4de9a0:
            pointer = self.words(sp+4, 1)[0]
            assert self.string(pointer) == FORMAT
            self.event('Translate', FORMAT.hex())
            self.ret(TRANSLATED, 4)
            return
        if address == 0x4a89c0:
            target, fmt, hours, minutes, seconds = self.words(sp+4, 5)
            numbers = tuple(map(signed, (hours, minutes, seconds)))
            self.event('Format', *numbers)
            pattern = self.string(fmt)
            result = pattern % numbers if b'%' in pattern else pattern
            mu.mem_write(target, result+b'\0')
            self.ret(target)
            return
        if address == 0x460d70:
            self.event('Wall', struct.pack('<d', self.wall).hex())
            self.ret(WALL)
            return
        if address == 0x465330:
            assert mu.reg_read(UC_X86_REG_ECX) == NODE and self.words(sp+4, 1) == (1,)
            self.event('Idle', 1)
            self.put(NODE+4, self.words(NODE+4, 1)[0] | 0x80)
            self.ret(purge=4)
            return
        super().instruction(mu, address, size, data)

    def sample(self, now, origin, wall, translation, control):
        """실제thiscall정상반환/192BYTE/CRTstamp/효과를각독립입력에서읽는다."""
        self.now, self.wall, self.case, self.events = now, wall, (0, 1, 0, translation, 0), []
        mu = self.mu
        mu.mem_write(NODE, b'\xa5'*0xc0)
        mu.mem_write(NAME, b'player\0')
        mu.mem_write(TRANSLATED, FORMATS[translation]+b'\0')
        mu.mem_write(WALL, struct.pack('<d', wall))
        self.put(0, 0xffffffff)
        self.put(0x594fc8, 0)
        self.put(CLOCK_STAMP, 0x87654321)
        self.put(CLOCK_ORIGIN, origin)
        for register, value in ((UC_X86_REG_EBX, 0x12345678), (UC_X86_REG_ESI, 0x23456789), (UC_X86_REG_EDI, 0x3456789a), (UC_X86_REG_EBP, 0x456789ab), (UC_X86_REG_ECX, NODE), (UC_X86_REG_EAX, 0), (UC_X86_REG_ESP, STACK), (UC_X86_REG_EFLAGS, 2), (UC_X86_REG_FPCW, control), (UC_X86_REG_FPSW, 0), (UC_X86_REG_FPTAG, 0xffff)):  #직전표본의레지스터/x87를재사용하지않는다.
            mu.reg_write(register, value)
        mu.mem_write(STACK, struct.pack('<5I', STOP, 16, 1, 0, NAME))
        mu.emu_start(ENTRY, STOP, count=100000)
        assert mu.reg_read(UC_X86_REG_EIP) == STOP and mu.reg_read(UC_X86_REG_ESP) == STACK+20
        assert mu.reg_read(UC_X86_REG_EAX) == NODE and self.words(0, 1) == (0xffffffff,)
        assert mu.reg_read(UC_X86_REG_FPCW) == control and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        return bytes(mu.mem_read(NODE, 0xc0)).hex(), self.words(CLOCK_STAMP, 1)[0], ';'.join(self.events)


def main():
    """최초독립관측을준비출력에만기록하고기존fixture를덮어쓰지않는다."""
    ARGS.output.mkdir(parents=True, exist_ok=True)
    key, message = KeyOracle(), MessageOracle()
    print('원문 격리 장치 준비 완료', flush=True)
    key_rows, message_rows = [], []
    for code, paused, modal, initial in itertools.product((ord('T'), ord('t')), (0, 1), (0, 1), (0, 1, 0xffffffff)):  #문자대소문자와현재비영플래그의실제분기를독립으로관측한다.
        for flags in (0, 0x20000000, 0x10000000, 0x08000000, 0x40000000, 0x04000000, 0x80000000):  #수정키/pressed/outside/뗌은서로다른명시입력이다.
            inputs = (code|flags, paused, modal, initial)
            first = key.sample(*inputs, 0x27f)
            assert first == key.sample(*inputs, 0x37f) == key.sample(*inputs, 0x27f)
            key_rows.append('\t'.join(map(str, (*inputs, *first))))
    print('원문 T 입력 관측 완료', len(key_rows), flush=True)
    for now, origin in ((0, 0), (999, 0), (1000, 0), (59999, 0), (60000, 0), (3599999, 0), (3600000, 0), (0xffffffff, 0), (0, 999), (0, 1000), (0, 60000), (0x7fffffff, 0), (0x80000000, 0), (0x100, 0xffffff00), (0xffffff00, 0x100)):
        for wall, translation in itertools.product((0.0, 17.25), range(len(FORMATS))):  #0.1더하기와빈/다른번역서식도실제명령으로관측한다.
            first = message.sample(now, origin, wall, translation, 0x27f)
            assert first == message.sample(now, origin, wall, translation, 0x37f) == message.sample(now, origin, wall, translation, 0x27f)
            message_rows.append('\t'.join(map(str, (now, origin, struct.pack('<d', wall).hex(), translation, *first))))
    for name, rows in (('game-time-key-x86.tsv', key_rows), ('game-time-message-x86.tsv', message_rows)):  #준비폴더의독립fixture만처음한번만든다.
        path = ARGS.output / name
        assert not path.exists()
        path.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(keyInputs=len(key_rows), messageInputs=len(message_rows), nativeExecutions=3*(len(key_rows)+len(message_rows)), gameExecutions=0, multiplayerTests=0, deadlineConstant=struct.unpack('<d', message.mu.mem_read(0x501960, 8))[0], lastDecompileHost='HJOW-Athlon', lastDecompileDate='2026-10-23')
    (ARGS.output / 'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
