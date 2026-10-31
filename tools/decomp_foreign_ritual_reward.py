#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""비로컬 보상의 실제 PE 덧셈·정수 변환·Core 쓰기를 제품 수정 전에 고정한다."""
import csv
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, PLAYERS, TYPES, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_FPCW

# 실제 원문 주소와 별도 관측 창이며, 게임이나 Windows 창을 실행하지 않는다.
RECORDS, WINDOW = 0x595360, 0x595320
FIXTURE = ROOT / 'cpppj/tests/fixtures/foreign-ritual-reward-x86.tsv'
REPORT = ROOT / 'extracted/cpp-foreign-ritual-reward-20261031-native.json'


class Oracle(MemoryOracle):
    """장부/정수 계산은 CPU로 실행하고 TYPE·분산 SP의 장치 경계만 관측한다."""
    def __init__(self):
        """원본 PE와 정밀 함수 범위를 읽기 전용으로 빌린다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as file:
            facts = {int(row['entry'],16):row for row in csv.DictReader(file,delimiter='\t')}
        self.allowed = []
        for entry in (0x448260,0x40ef30,0x4e49c0,0x447810,0x48f380,0x42b280):  # 부모의 실제 하위만 허용한다.
            for part in facts[entry]['ranges'].split(';'):
                low,high = (int(value,16) for value in part.split('-'))
                self.allowed.append((low,high+1))
        self.running = False
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def write(self,mu,access,address,size,value,data):
        """장부 전체의 실제 쓰기 순서와 폭을 보존한다."""
        if self.running and WINDOW <= address < address+size <= WINDOW+256:
            self.events.append(f'W:{address-RECORDS}:{size}:{value&((1<<(size*8))-1)}')

    def instruction(self,mu,address,size,data):
        """SP getter/setter 경계의 원시 인자와 원본 assert를 기록한다."""
        self.instructions += 1
        if self.instructions > 1000:
            raise RuntimeError('비로컬 보상 원문 명령 상한')
        arguments = self.words(mu.reg_read(UC_X86_REG_ESP)+4,3)
        if address == 0x49a840:
            self.events.append(f'T:{arguments[0]}');self.ret(TYPES)
        elif address == 0x40ec50:
            self.events.append(f'G:{arguments[0]}');self.put(arguments[1],self.sp);self.ret()
        elif address == 0x40edb0:
            self.events.append(f'S:{arguments[0]}:{arguments[1]}');self.ret()
        elif address == 0x43dad0:
            self.events.append('U');self.ret()
        elif address == 0x4e0620:
            self.events.append(f'A:{arguments[2]}');self.ret()
        elif not any(low <= address < high for low,high in self.allowed):
            raise RuntimeError('비로컬 보상 범위 밖 명령:'+hex(address))

    def sample(self,case,control):
        """입력만 준비하고 전체 인접 장부와 효과의 기대값은 CPU에서 얻는다."""
        mode,owner,local,known,editor,sp,bit,seed = case
        self.running = False;self.events = [];self.instructions = 0;self.sp = sp
        self.mu.mem_write(WINDOW,bytes((seed+index*17)&255 for index in range(256)))
        self.mu.mem_write(PLAYERS,bytes([0xa5])*4096)
        for address,value in ((0x540c70,local),(0x595344,known),(0x5c85a4,editor),(0x59534c,PLAYERS),(0x5412c0,154),(0x5411cc,93),(TYPES+0x1ec,bit)):
            self.put(address,value)
        self.mu.mem_write(STACK-4096,bytes(4128));self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        arguments = (owner,85) if mode == 0 else (154,owner)
        self.mu.mem_write(STACK,struct.pack('<III',STOP,*arguments))
        self.running = True;self.mu.emu_start(0x48f380 if mode == 0 else 0x448260,STOP);self.running = False
        if self.mu.reg_read(UC_X86_REG_ESP) != STACK+4:
            raise RuntimeError('비로컬 보상 원문 스택 복귀')
        return '\t'.join((','.join(map(str,(control,*case))),bytes(self.mu.mem_read(WINDOW,256)).hex(),';'.join(self.events)))


def main():
    """새 제품 helper가 없는 최초에만 fixture를 생성하며 덮어쓰지 않는다."""
    assert not FIXTURE.exists() and not REPORT.exists(),'최초 관측 덮어쓰기 금지'
    assert not (ROOT/'cpppj/src/o/RawForeignRitualReward.cpp').exists(),'제품 수정 전 관측 필요'
    oracle = Oracle();cases = []
    for owner,bit,seed in itertools.product((0,1,8,9,0x10000000,0xffffffff),(-1,0,7,31,32,-2),(0,165)):
        cases.append((0,owner,2,1,0,0,bit,seed))
    # float32의 분수·음수·signed32/64 경계·NaN/무한대를 기대 계산 없이 입력한다.
    values = (0,0x42f78000,0xc2f78000,0x3f7fffff,0xbf7fffff,0x4effffff,0x4f000000,0xcf000000,0x5effffff,0x5f000000,0xdf000000,0x7f800000,0xff800000,0x7fc00001,0xaa800000)
    for owner,known,editor,sp in itertools.product((0,1,8),(0,1),(0,1),values):
        cases.append((1,owner,2,known,editor,sp,0,0))
    lines = ['# control,mode,owner,local,known,editor,spBits,technologyBit,seed / entire256ByteCoreWindow / nativeEffects']
    differences = 0
    for case in cases:  # 양 정밀도를 숨기지 않으며 기본 게임의 53비트와 확장 64비트를 구분한다.
        first,second = oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        differences += first.split('\t')[1:] != second.split('\t')[1:]
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths = ('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_foreign_ritual_reward.py','cpppj/tests/fixtures/foreign-ritual-reward-x86.tsv')
    report = {'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','sourceAbsentBeforeObservation':True,'uniqueInputs':len(cases),'observations':len(lines)-1,'precisionDifferences':differences,'spAddendAddress':'00507548','spAddendBits':bytes(oracle.mu.mem_read(0x507548,4)).hex(),'gameOrOsExecuted':False,'sha256':{path:digest(ROOT/path) for path in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(report,ensure_ascii=False))


if __name__ == '__main__':
    main()
