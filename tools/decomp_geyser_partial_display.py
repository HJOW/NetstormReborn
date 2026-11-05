#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""간헐천 부분 표시/고정 외형 getter/사각형/dirty의 실제 x86만 격리 실행한다."""
import argparse
import collections
import csv
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, STACK, STOP
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW

FIXTURE = ROOT/'cpppj/tests/fixtures/geyser-partial-display-x86.tsv'  # 최초 입력/원본 변경표를 고정하고 재관측은 덮어쓰지 않는다.
ENTRY = 0x4854b0  # VT0050da88의 실제 가상8c다.
VTABLE = 0x50da88  # 가상78은 현재 TYPE+128을 반환하는004acfe0이다.
CACHE = 0x595110  # 네 inset DWORD와 직후 초기화 비트 DWORD가 있는 원문 저장소다.
FRAMES = [(100,180,-3,5),(130,60,7,-9),(50,260,2,3),(80,90,4,6)]  # 고정 프레임/잘린 빈 사각형/그림자 구별용 입력이다.
POSITIONS = [(20.,20.),(0.25,0.5),(38.,40.),(30.25,25.5)]  # 정상/좌상/우하 클리핑과 현재 다른 TYPE를 구별한다.
POOL,TYPES,SHAPE,DIRTY = 0x11000000,0x12000000,0x13000000,0x14000000  # 서로 겹치지 않는 raw 인자/합성TYPE/SHP/변경표다.


def sha(path):
    """원본/정밀 함수 범위/도구의 같은 바이트 정체성을 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class GeyserPartialOracle(Oracle):
    """추가 허용 범위는 현행 정밀 두 함수와 PE에서 직접 확인한 짧은 getter뿐이다."""
    def __init__(self):
        """기존 실제 표시 몸체를 재사용하고 함수 대체나 OS 진입을 추가하지 않는다."""
        entries=(ENTRY,0x499240,0x498ff0,0x497580,0x445290,0x49a840,0x419850,0x49adc0,0x4e49c0,0x4834c0,0x4970a0)
        super().__init__('originals',[dict(name=f'{address:08x}',originals=f'{address:08x}') for address in entries],helpers=[])
        for address,size in ((POOL,4096),(TYPES,0x20000),(SHAPE,4096),(DIRTY,4096)):  # 실제 입력과 쓰기 대상만 격리해서 확보한다.
            self.mu.mem_map(address,size)
        self.stride,self.sid=50,1  # this 인자의 물리 슬롯 크기와 합성 번호이며 할당 API를 실행하지 않는다.
        self.sha256=sha(ROOT/'originals/Netstorm.exe')  # 실제 읽은 보호 PE의 같은 바이트 정체성이다.
        self.write_ranges=[(0x30000000,0x30010000),(DIRTY,DIRTY+0x900),(CACHE,CACHE+20),(0x59a8b0,0x59a8b4)]  # 호출 스택/변경표/유일 cache/표시 억제만 쓰도록 제한한다.
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)
        self.selected = []  # 현행 전체 정밀 내보내기의 두 새 호출 경계다.
        self.calls = collections.Counter()  # 실제 가상8c/가상78/부분 helper 도달 횟수다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 시작 주소로 선택하며 다른 함수를 일괄 허용하지 않는다.
                if int(row['entry'],16) not in (ENTRY,0x499240):
                    continue
                self.selected.append(row)
                for part in row['ranges'].split(';'):  # 불연속 몸체 사이의 명령을 허용하지 않는다.
                    low,high=(int(value,16) for value in part.split('-'))
                    self.allowed.append((low,high+1))
        assert len(self.selected)==2
        self.allowed.append((0x4acfe0,0x4acff4))  # 정밀 함수 색인에 빠진 실제 movzx/call/get baseFrame/ret 20바이트다.

    def on_instruction(self,mu,address,size,data):
        """실제 호출을 세고 부모의 범위/OS/진단/쓰기 검사를 그대로 적용한다."""
        if hasattr(self,'calls') and address in (ENTRY,0x4acfe0,0x499240):
            self.calls[f'{address:08x}']+=1
        if address==0x4e0620:
            raise RuntimeError('부분 표시에서 원문 진단에 도달했습니다')
        super().on_instruction(mu,address,size,data)

    def on_write(self,mu,access,address,size,value,data):
        """원본 코드/TYPE/SHP/raw와 그 밖의 전역 쓰기는 허용하지 않는다."""
        if not any(low<=address and address+size<=high for low,high in self.write_ranges):
            raise RuntimeError(f'부분표시의 허용되지 않은 쓰기 {address:08x}+{size}')

    def start(self,control):
        """실제 게임/할당/Reset 없이 유효한 raw 인자와 합성 표 포인터만 제공한다."""
        self.control=control  # 각 Native 관측에서 복귀까지 유지해야 하는 x87 제어어다.
        self.mu.mem_write(0x59ab20,struct.pack('<I',TYPES))
        self.mu.mem_write(0x5e4794,bytes(4))

    def shape(self,flags,count,loaded,first,frames):
        """독립 입력 TYPE와 SHP 헤더를 만들며 기대 좌표/사각형은 계산하지 않는다."""
        raw=bytearray(500)
        for offset,value in [(0,first),(0xdc,SHAPE if loaded else 0),(0xe8,flags),(0x114,count)]:  # 필요한 원본 DWORD 필드만 채운다.
            struct.pack_into('<I',raw,offset,value)
        self.mu.mem_write(TYPES+74*500,bytes(raw))
        for index,frame in enumerate(frames):  # 픽셀 크기와 signed hotspot은 실제 SHP와 같은 헤더 위치다.
            offset=0x100+index*0x80
            self.mu.mem_write(SHAPE+8+index*8,struct.pack('<I',offset))
            self.mu.mem_write(SHAPE+offset-36,struct.pack('<2f',1,1))
            self.mu.mem_write(SHAPE+offset-12,struct.pack('<4h',*frame))

    def view(self,width,height,camera_x,camera_y,zoom,viewport,suppressed):
        """카메라/Q16 확대율/viewport와 빈 실제 Renderer 변경표의 입력만 저장한다."""
        self.mu.mem_write(DIRTY,bytes(0x900))
        self.mu.mem_write(0x59a91c,struct.pack('<2i',camera_x,camera_y))
        self.mu.mem_write(0x5ca9cc,struct.pack('<4i',*viewport))
        for address,value in [(0x59a918,zoom),(0x59a8bc,DIRTY),(0x5c78f0,width),(0x5c78f4,height),(0x59a8d0,0),(0x59a8b0,suppressed)]:  # 병합을 끄고 관측 순서를 보존한다.
            self.mu.mem_write(address,struct.pack('<I',value))

    def slot(self,frame,extra,state,x,y):
        """생성자/메타맵 없이 해당 가상 함수의 유효한 물리 this 인자만 만든다."""
        raw=bytearray(50)
        struct.pack_into('<I',raw,0,VTABLE);raw[10]=74;raw[11]=state
        struct.pack_into('<2f',raw,14,x,y);struct.pack_into('<i',raw,36,frame);raw[40]=extra
        self.mu.mem_write(POOL+self.sid*self.stride,bytes(raw))

    def raw(self):
        """같은 raw this의 전체50바이트를 관찰한다."""
        return bytes(self.mu.mem_read(POOL+self.sid*self.stride,self.stride)).hex()

    def invoke(self,address,this):
        """실제 thiscall의 ret/ESP/x87 제어어와 스택 복귀를 검증한다."""
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 이전 호출의 레지스터를 새 입력으로 숨기지 않는다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ECX,this);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_FPCW,self.control);top=self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.emu_start(address,STOP,count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control and self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==top

    def dirty_output(self):
        """출력 변경표의 전체 사각형/flags와 표시 억제 DWORD를 순서대로 읽는다."""
        count=struct.unpack('<I',self.mu.mem_read(DIRTY+100,4))[0]
        result=[]
        for index in range(count):  # 원본이 실제 기록한 항목만 손실 없이 직렬화한다.
            rect=struct.unpack('<4i',self.mu.mem_read(DIRTY+0x68+index*16,16))
            flag=struct.unpack('<I',self.mu.mem_read(DIRTY+0x6a8+index*4,4))[0]
            result.append(','.join(map(str,(*rect,flag))))
        return struct.unpack('<I',self.mu.mem_read(0x59a8b0,4))[0],';'.join(result) or '-'


def generate():
    """기대 사각형을 계산하지 않고 모든 반환/raw/변경표를 원본 명령에서 읽는다."""
    oracle=GeyserPartialOracle()
    rows=['# 실제 간헐천8c/가상78/부분bounds/dirty. 합성TYPE/SHP/카메라 입력이며 대체 함수/게임/통신 없음.']
    cases=list(itertools.product(range(3),(-1,1,0x7fffffff),(0,0x8000),(32768,65536,98304),range(4)))
    for control in (0x027f,0x037f):  # 같은 float 입력을 두 정상 x87 정밀도에서 관찰한다.
        oracle.start(control)
        oracle.write_ranges.append((CACHE,CACHE+20))
        oracle.mu.mem_write(CACHE,struct.pack('<5I',91,92,93,94,0x80000000))
        for index,(base,frame,word,zoom,position) in enumerate(cases):  # 원본 출력과 무관한 입력 교차표다.
            flags=0x440000 if index%2 else 0
            extra=(0,4,12)[(index//2)%3]
            suppressed=int(index%11==0)
            oracle.shape(flags,4,True,91,FRAMES)
            typ=TYPES+74*500
            oracle.mu.mem_write(typ+0x128,struct.pack('<i',base))
            oracle.mu.mem_write(TYPES+75*500,bytes(oracle.mu.mem_read(typ,500)))
            oracle.mu.mem_write(TYPES+75*500+0x128,struct.pack('<i',(base+1)%3))
            oracle.view(640,480,7,9,zoom,(12,9,620,450),suppressed)
            oracle.slot(frame,extra,4,*POSITIONS[position])
            raw=POOL+oracle.sid*oracle.stride
            oracle.mu.mem_write(raw,struct.pack('<I',VTABLE))
            oracle.mu.mem_write(raw+12,struct.pack('<H',word))
            oracle.mu.mem_write(raw+10,bytes([75 if position==3 else 74]))
            before=oracle.raw()
            oracle.invoke(ENTRY,this=raw)
            assert before==oracle.raw()
            assert struct.unpack('<5I',oracle.mu.mem_read(CACHE,20))==(0,50,0,50,0x80000001)
            full,dirty=oracle.dirty_output()
            assert full==suppressed
            rows.append('\t'.join(map(str,(control,base,frame,word,zoom,position,flags,extra,suppressed,before,full,dirty))))
    assert oracle.assertions==0
    return '\n'.join(rows)+'\n',dict(samples=len(rows)-1,uniqueInputs=len(cases),controls=[0x027f,0x037f],
        calls=dict(oracle.calls),assertions=oracle.assertions,selected=oracle.selected,binarySha256=oracle.sha256,
        refinedIndexSha=sha(ROOT/'extracted/refined/originals/functions.tsv'),
        limitations=['실제 격리 x86 표시만 실행한다. 게임/윈도우/MP/OS API/공중3종은 실행하지 않는다.',
            '합성으로 로드한 SHP/TYPE와 유한 좌표만 관찰했다. null/손상 SHP/실제 디스플레이 출력은 별도다.',
            '네 inset/비트 저장소는 이 함수의 초기화/읽기 외에 다른 원문 소비자/작성자가 없다.'])


def main():
    """최초 파일을 고정하거나 같은 원본을 재관측해 일치만 확인한다."""
    parser=argparse.ArgumentParser()
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    content,report=generate()
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(content)
    report.update(host=socket.gethostname(),mode='verify' if args.verify else 'first',fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)))
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(report['mode'],report['samples'],'원문 입력/전체 변경표 일치',flush=True)


if __name__=='__main__':
    main()
