#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Warning kind17생성자와실제문자열CRT경계를게임없이격리관측한다."""
import argparse
import csv
import itertools
import json
from pathlib import Path
import struct
import sys

PARSER = argparse.ArgumentParser(description=__doc__)  #제품밖준비본의입력/출력을명시한다.
PARSER.add_argument('--repo', type=Path, required=True)
PARSER.add_argument('--output', type=Path, required=True)
ARGS = PARSER.parse_args()  #보호PE는입력저장소에서읽기만한다.
sys.path.insert(0, str(ARGS.repo/'tools'))
from decomp_message_gump import Oracle as PauseOracle, NODE, NAME, TRANSLATED, ENTRY
from decomp_priestmenu import STACK, STOP, TEXT
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

SOURCE, WALL = TEXT+0x6000, TEXT+0x7000  #원문포인터의텍스트와명시벽시각을별도저장한다.
SOURCES = (b'Warning', b'', b'A\0tail', b'~B~E~yWarning\xff', None)  #NULL/빈문자열/첫NUL/확장문자바이트를구별한다.
FORMATS = (b'%s', b'~E~w[%s]', b'', b'%.3s', b'%% %s')  #번역반환서식의별도입력이다.


class Oracle(PauseOracle):
    """생성자/서식기/기한을실행하고현재UI/OS장치만명시경계로둔다."""
    def __init__(self):
        """원문순수문자열계산의허용몸체를정밀표범위로추가한다."""
        super().__init__()
        with (ARGS.repo/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16):row for row in csv.DictReader(stream, delimiter='\t')}
        for address in (0x4a89c0,0x4de490,0x4e7990,0x4eaac6,0x418070,0x4e8790,0x4eaa38,0x4eaa6b,0x4eaa8f,0x4f0d0e):  #자체서식/CRT출력/기한의실제명령만추가실행한다.
            # 정밀 디컴파일의 각 몸체 범위만 실행 허용 목록에 더한다.
            for part in facts[address]['ranges'].split(';'):
                low, high = (int(value,16) for value in part.split('-'))
                self.allowed.append((low,high+1))
        self.helpers = set()

    def write(self, mu, access, address, size, value, data):
        """자체서식기의실제전역scratch외에는기존노드/스택쓰기만허용한다."""
        if 0x5e18b8 <= address and address+size <= 0x5e22b8:
            return
        super().write(mu,access,address,size,value,data)

    def instruction(self, mu, address, size, data):
        """큰명시효과의순서와실제서식반환을기록하고미등록자식은거부한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e6fa1:
            character = self.words(sp+4,1)[0]
            self.ret(4 if ord('0') <= character <= ord('9') else 0)  #서식기의isdigit만현재ASCII분류장치로공급하며TLS/OS초기화를실행하지않는다.
            return
        if address == 0x4a9cb0:
            args = self.words(sp+4,6)
            assert self.string(args[0]) == b'newWarnMessage.wav' and args[1:] == (0,0,0,0,0)
            self.event('Sound')
            self.ret()
            return
        if address == 0x4de9a0:
            assert self.string(self.words(sp+4,1)[0]) == b'%s'
            self.event('Translate','2573')
            self.ret(TRANSLATED,4)
            return
        if address == 0x460d70:
            self.event('Wall',struct.pack('<d',self.wall).hex())
            self.ret(WALL)
            return
        if address == 0x465330:
            assert self.words(sp+4,1) == (1,)
            self.event('Idle',1)
            self.put(NODE+4,self.words(NODE+4,1)[0]|0x80)
            self.ret(purge=4)
            return
        if address == 0x4bc800:
            self.event('Style',self.string(self.words(sp+4,1)[0]).hex())
            self.put(NODE+0xa0,0x12408000)
            self.ret(purge=4)
            return
        super().instruction(mu,address,size,data)

    def sample(self, source, translation, wall, control):
        """독립입력에서정상thiscall반환과원문192BYTE·효과순서를읽는다."""
        self.case, self.wall, self.events = (0,0,0,0,0), wall, []
        mu = self.mu
        mu.mem_write(NODE,b'\xa5'*0xc0)
        mu.mem_write(NAME,b'player\0')
        mu.mem_write(TRANSLATED,FORMATS[translation]+b'\0')
        mu.mem_write(SOURCE,(SOURCES[source] or b'')+b'\0')
        mu.mem_write(WALL,struct.pack('<d',wall))
        self.put(0,0xffffffff)
        self.put(0x594fc8,0)
        for register,value in ((UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab),(UC_X86_REG_ECX,NODE),(UC_X86_REG_EAX,0),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):  #이전관측의레지스터/x87상태를남기지않는다.
            mu.reg_write(register,value)
        mu.mem_write(STACK,struct.pack('<5I',STOP,17,0,0,0 if SOURCES[source] is None else SOURCE))
        mu.emu_start(ENTRY,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP) == STOP and mu.reg_read(UC_X86_REG_ESP) == STACK+20
        assert mu.reg_read(UC_X86_REG_EAX) == NODE and self.words(0,1) == (0xffffffff,)
        assert mu.reg_read(UC_X86_REG_FPCW) == control and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        return bytes(mu.mem_read(NODE,0xc0)).hex(),';'.join(self.events)


def main():
    """원문정밀도둘과재실행을대조하고기존fixture를건드리지않는다."""
    ARGS.output.mkdir(parents=True,exist_ok=True)
    oracle, rows = Oracle(), []
    print('Warning 원문 장치 준비',flush=True)
    # 본문·번역·시계의 독립 조합을 두 x87 정밀도와 반복 실행으로 대조한다.
    for source, translation, wall in itertools.product(range(len(SOURCES)),range(len(FORMATS)),(0.0,17.25)):
        first = oracle.sample(source,translation,wall,0x27f)
        assert first == oracle.sample(source,translation,wall,0x37f) == oracle.sample(source,translation,wall,0x27f)
        rows.append('\t'.join(map(str,(source,translation,struct.pack('<d',wall).hex(),*first))))
    path = ARGS.output/'warning-message-x86.tsv'
    assert not path.exists()
    path.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report = dict(inputs=len(rows),nativeExecutions=len(rows)*3,gameExecutions=0,multiplayerTests=0,lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-23')
    (ARGS.output/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
