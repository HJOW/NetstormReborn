#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 미니맵 셀/공간 해시/TYPE/CRT의 실제 픽셀 쓰기를 독립 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

ROOT = Path(__file__).resolve().parents[1]  # 새 원문 관측의 루트다.
PE_PATH = ROOT/'originals/Netstorm.exe'  # 변경하지 않는 보호 입력이다.
PE_SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 원문 식별자다.
FIXTURE = ROOT/'cpppj/tests/fixtures/minimap-cell-x86.tsv'  # 실제 픽셀 관측이다.
REPORT = ROOT/'extracted/cpp-minimap-cell-20261023-native.json'  # 범위/해시/한계다.
BASE, HEAP, STACK = 0x400000, 0x1000000, 0x2100000  # 원문과 격리 자료/스택이다.
BITMAP, FORMS, TYPES = HEAP, HEAP+0x10000, HEAP+0x20000  # 고정128×128 출력과50/500BYTE 자료다.
HASHES = (HEAP+0x40000, HEAP+0x60000, HEAP+0x68000, HEAP+0x6A000)  # 단계0..3의 실제 WORD 배열이다.
SIDES = (256, 128, 64, 16)  # 원본256/scale의 물리 배열 한 변이다.
ENTRY, STOP = 0x4808D0, HEAP+0x6F000  # 실제 cdecl 진입과 최상위 정상RET의 종료점이다.
COLORS = (0x5A4058, 0x5A4060, 0x5B5E58, 0x5BEC2C, 0x5B5DC4, 0x5B5DA0,
          0x59AFD8, 0x59AF80, 0x5BEC34, 0x5ACD1C, 0x59AFD0, 0x5ACD20)  # black/owner1..8/중립isle/중립받침/meta109다.
RANGES = ((0x4808D0, 0x480B79), (0x480B80, 0x48131E), (0x4B2B00, 0x4B2B47),
          (0x4AC200, 0x4AC214), (0x49A840, 0x49A853), (0x4E49C0, 0x4E4A35))  # 실제 본체와 좁은 하위 전부다.
CONTROLS = (0x27F, 0x37F)  # 두 x87 정밀도로 각각 정상함수를 실행한다.
SCHEMA = ('name', 'cx', 'cy', 'challenge', 'suppress', 'frame', 'colors', 'forms', 'heads',
          'completed', 'pixels', 'writes', 'trace', 'stackdelta', 'nativeinstructions', 'bitmapsha')  # 앞9열 입력/뒤7열 관측이다.


def digest(path):
    """보호 입력과 새 산출물을 변경 없이 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """명시 단정도 좌표의 원시 비트를 준비한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def records(text):
    """입력 레코드만 읽고 결과 픽셀은 계산하지 않는다."""
    return [] if text=='-' else [tuple(int(value) for value in row.split(':')) for row in text.split(';')]


def encoded(rows):
    """명시 입력 목록을 TSV의 줄/탭과 충돌하지 않게 표현한다."""
    return ';'.join(':'.join(str(value) for value in row) for row in rows) or '-'


class Oracle:
    """외부 함수 대체 없이 실제 셀과 해시/TYPE/절삭을 실행한다."""
    def __init__(self):
        """원문 이미지와 고정 물리 자료를 별도 주소에 적재한다."""
        pe=pefile.PE(str(PE_PATH))  # 보호 입력 파서다.
        self.mu=Uc(UC_ARCH_X86, UC_MODE_32)  # 실제32비트 명령 실행기다.
        self.mu.mem_map(BASE, (pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
        self.mu.mem_write(BASE, pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP, 0x70000)
        self.mu.mem_map(STACK-0x10000, 0x20000)
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.original={low: bytes(self.mu.mem_read(low, high-low)) for low,high in RANGES}  # 본체 불변 근거다.
        self.totals, self.write_totals, self.visited={}, Counter(), set()  # 정밀도/쓰기/명령 집계다.

    def number(self, address, value=None, width=4):
        """비정렬 원시 입력을 읽고 허용한 준비 단계에만 쓴다."""
        if value is not None:
            self.mu.mem_write(address, (value&((1<<(width*8))-1)).to_bytes(width, 'little'))
        return int.from_bytes(self.mu.mem_read(address, width), 'little')

    def guard(self, mu, access, address, size, value, data):
        """CPU 쓰기는 같은 bitmap 또는 실제 스택만 허용한다."""
        if BITMAP<=address and address+size<=BITMAP+16384:
            if size!=1: raise RuntimeError('미니맵 픽셀의 BYTE 외 저장')
            self.pixel_writes.append(f'{address-BITMAP}:{value&255}')
            self.write_totals['bitmap']+=1
        elif STACK-0x1000<=address and address+size<=STACK+12:
            self.write_totals['stack']+=1
        else:
            raise RuntimeError(f'미명시 CPU 쓰기 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """명시 하위까지 실제 실행하며 진입 인자만 관측한다."""
        if address==STOP:
            self.completed=True
            mu.emu_stop()
            return
        if not any(low<=address<high for low,high in RANGES):
            raise RuntimeError(f'미명시 실행 {address:08x}')
        self.instructions+=1
        self.visited.add(address)
        if address==0x4B2B00:
            esp=mu.reg_read(UC_X86_REG_ESP)  # 실제 thiscall이 받은x/y/level이다.
            x,y,level=(self.number(esp+offset) for offset in (4,8,12))
            self.trace.append(f'hash:{level}:{x}:{y}')
        elif address==0x4AC200:
            raw=mu.reg_read(UC_X86_REG_ECX)  # 실제 flags2 getter가 읽을 현재 슬롯이다.
            self.trace.append(f'flags:{(raw-FORMS)//50}:{self.number(raw+10,width=1)}')

    def run(self, case, control):
        """독립 입력만 쓰고 정상RET 뒤 전체 bitmap과 실제 store 순서를 읽는다."""
        self.trace, self.pixel_writes, self.instructions, self.completed=[], [], 0, False  # 실행별 관측이다.
        self.mu.mem_write(BITMAP, bytes([0xA5])*16384)
        self.mu.mem_write(FORMS, bytes([0xA5])*(256*50))
        self.mu.mem_write(TYPES, bytes(188*500))
        # 네 단계 실제 배열을 초기화하고 원본 객체의 포인터 필드에 연결한다.
        for level,(address,side) in enumerate(zip(HASHES,SIDES)):
            self.mu.mem_write(address, bytes(side*side*2))
            self.number(0x542508+12+level*4, address)
        self.number(0x5C84BC, HASHES[0])
        self.number(0x5C8464, FORMS)
        self.number(0x59AB20, TYPES)
        self.number(0x5949C4, BITMAP)
        self.number(0x5949CC, 128)
        self.number(0x594FC8, case['challenge'])
        self.number(0x59A8A8, case['suppress'])
        self.number(0x540BB0, case['frame'])
        # 원본 팔레트 로더가 제공하는 색DWORD의 낮은 BYTE를 실제 코드가 선택하게 한다.
        for address,value in zip(COLORS, (int(value) for value in case['colors'].split(':'))):
            self.number(address,value)
        seen_types={}  # 같은 현재 TYPE에 서로 다른flags를 주는 모순 입력을 거부한다.
        for sid,kind,owner,flags,x,y,next_sid,state,extra in records(case['forms']):
            if not 0<sid<256 or not 0<=kind<188: raise RuntimeError('명시 Pool/TYPE 범위')
            if kind in seen_types and seen_types[kind]!=flags: raise RuntimeError('같은TYPE flags2 모순')
            seen_types[kind]=flags
            raw=FORMS+sid*50  # 원문과 같은 실제 stride다.
            self.number(raw+4,next_sid,2)
            self.number(raw+10,kind,1)
            self.number(raw+11,state,1)
            self.number(raw+14,x)
            self.number(raw+18,y)
            self.number(raw+34,owner,1)
            self.number(raw+40,extra,1)
            self.number(TYPES+kind*500+0xEC,flags)
        # 실제Hash.Cell 입력 배열의 지정 머리만 바꾸며 연결 순서는 raw next가 결정한다.
        for level,x,y,sid in records(case['heads']):
            if not 0<=level<4 or not 0<=x<SIDES[level] or not 0<=y<SIDES[level]: raise RuntimeError('명시 Hash 범위')
            self.number(HASHES[level]+(y*SIDES[level]+x)*2,sid,2)
        self.mu.mem_write(STACK-0x1000,bytes(0x2000))
        self.mu.mem_write(STACK,struct.pack('<3I',STOP,case['cx']&0xFFFFFFFF,case['cy']&0xFFFFFFFF))
        preserved=(UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)  # 정상cdecl의 보존 레지스터다.
        for index,register in enumerate(preserved): self.mu.reg_write(register,0x12345670+index)  # 서로 다른 원시 표식이다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(ENTRY,STOP+1,count=300000)
        if not self.completed: raise RuntimeError('정상RET 미도달')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control: raise RuntimeError('x87 제어어 복원 실패')
        for index,register in enumerate(preserved):
            if self.mu.reg_read(register)!=0x12345670+index: raise RuntimeError('cdecl 레지스터 보존 실패')
        raw=bytes(self.mu.mem_read(BITMAP,16384))  # 실제 전체 출력이며 Python 래스터를 실행하지 않는다.
        output={'completed':int(self.completed),'pixels':encoded((index,value) for index,value in enumerate(raw) if value!=0xA5),
                'writes':';'.join(self.pixel_writes) or '-','trace':';'.join(self.trace) or '-',
                'stackdelta':self.mu.reg_read(UC_X86_REG_ESP)-(STACK+4),'nativeinstructions':self.instructions,
                'bitmapsha':hashlib.sha256(raw).hexdigest()}  # 실제 픽셀/경계/ABI다.
        # 모든 실행에서 보호 본체를 변경하지 않았음을 확인한다.
        for low,expected in self.original.items():
            if bytes(self.mu.mem_read(low,len(expected)))!=expected: raise RuntimeError('보호 본체 변경')
        total=self.totals.setdefault(f'{control:03x}',{'runs':0,'instructions':0})  # 제어어별 집계다.
        total['runs']+=1;total['instructions']+=self.instructions
        return output


def cases():
    """지면/단계별 체인/덮기/색/절삭의 입력을 기대 픽셀 없이 만든다."""
    rows,seen=[],set()  # 독립 입력의 중복만 제거한다.
    palette=(3,17,29,43,61,79,97,113,131,149,167,181)  # 각 색 분기를 구별할 명시 DWORD다.

    def add(name,cx=8,cy=12,forms=(),heads=(),**changes):
        """임의 입력 자료만 등록하며 래스터 결과나 색 선택을 계산하지 않는다."""
        case=dict(name=name,cx=cx,cy=cy,challenge=0,suppress=0,frame=0x12345678,
                  colors=':'.join(map(str,palette)),forms=encoded(forms),heads=encoded(heads))
        case.update(changes)
        key=tuple(case[field] for field in SCHEMA[1:9])  # 결과와 이름을 제외한 원시 입력 키다.
        if key not in seen: seen.add(key);rows.append(case)

    def form(sid,kind,owner=0,flags=0,x=32.9,y=48.9,next_sid=0,state=0,extra=0):
        """원문이 읽을50BYTE 객체의 명시 필드만 만든다."""
        return (sid,kind,owner,flags,bits(x),bits(y),next_sid,state,extra)

    # 정상 네 모서리/내부의 초기화와 비영 색 억제/두 모드를 교차한다.
    for cell,mode,suppress,frame in itertools.product(((0,0),(1,1),(8,12),(63,63)),(0,1),(0,1),(0,0xFFFFFFFF)):
        add(f'empty-{cell}-{mode}-{suppress}-{frame}',cx=cell[0],cy=cell[1],challenge=mode,suppress=suppress,frame=frame)
    # 모든4×4 지면 위치에서 두 실제 지형/무관타입과 owner 경계/억제를 관측한다.
    for dx,dy,kind,owner,suppress in itertools.product(range(4),range(4),(97,157,71),(0,1,2,3,4,5,6,7,8,9,255),(0,1)):
        add(f'ground-{dx}-{dy}-{kind}-{owner}-{suppress}',forms=(form(1,kind,owner,2),),heads=((0,32+dx,48+dy,1),),suppress=suppress)
    # 같은 최종 픽셀을 여러 지면 칸이 덮을 때 실제 y/x 순서가 결과를 정한다.
    for flip in (0,1):
        forms=[form(index+1,97,(15-index if flip else index)%10,2) for index in range(16)]
        heads=[(0,32+index%4,48+index//4,index+1) for index in range(16)]
        add(f'ground-overlap-{flip}',forms=forms,heads=heads)
    # 단계2/1/0 각각의 동일한 후보가 flags2와 모드/타입에 따라 표시되는지 실제 분기를 관측한다.
    for level,kind,flags,owner,mode,suppress in itertools.product((0,1,2),(82,97,108,109),(0,2,4,6),(0,1,8,9,255),(0,1),(0,1)):
        hx,hy=(32,48) if level==0 else (16,24) if level==1 else (8,12)  # 각 단계의 같은4×4 월드 영역 입력이다.
        add(f'hash-{level}-{kind}-{flags}-{owner}-{mode}-{suppress}',forms=(form(1,kind,owner,flags),),heads=((level,hx,hy,1),),challenge=mode,suppress=suppress)
    # 상태/free/void/추상/매몰은 이 원문 셀의 추가 필터가 아님을 독립 입력으로 확인한다.
    for state,extra in itertools.product((0,1,2,4,8,0xFF),(0,1,8,9,0xFF)):
        add(f'state-{state}-{extra}',forms=(form(1,82,2,4,state=state,extra=extra),),heads=((2,8,12,1),))
    # 체인 다음과 단계2→1→0의 중첩 덮기를 같은 픽셀/다른 위치에서 관측한다.
    for owners,reverse in itertools.product(((1,2,3),(8,0,9)),(0,1)):
        forms=(form(1,82,owners[0],4,next_sid=2 if reverse else 0),form(2,82,owners[1],4,next_sid=0 if reverse else 1),form(3,82,owners[2],4))
        add(f'chain-{owners}-{reverse}',forms=forms,heads=((2,8,12,1 if reverse else 2),(1,16,24,3)))
        add(f'layers-{owners}-{reverse}',forms=forms,heads=((2,8,12,3),(1,16,24,1 if reverse else 2),(0,32,48,3)))
    # 실제 CRT절삭과 signed/2가 경계의 픽셀을 결정하게 하며 좌표를 출력으로 재계산하지 않는다.
    for x,y,level,mode,kind in itertools.product((32.0,32.9,33.9,35.9),(48.0,48.9,49.9,51.9),(0,1,2),(0,1),(82,109)):
        hx,hy=(32,48) if level==0 else (16,24) if level==1 else (8,12)
        add(f'fraction-{x}-{y}-{level}-{mode}-{kind}',forms=(form(1,kind,1,4,x,y),),heads=((level,hx,hy,1),),challenge=mode)
    # -0.75의 실제0방향 절삭과255.9의 마지막 물리 픽셀을 별도 끝 셀에서 관측한다.
    for cell,x,y in (((0,0),-0.75,-0.25),((0,0),0.75,1.75),((63,63),255.9,254.9)):
        add(f'edge-{cell}-{x}-{y}',cx=cell[0],cy=cell[1],forms=(form(1,82,1,4,x,y),),heads=((2,cell[0],cell[1],1),))
    # 모든 색DWORD는 실제 낮은BYTE만 사용하며 억제는 frameDWORD의 낮은BYTE를 선택한다.
    high_colors=':'.join(str(value|0xABCDEF00) for value in palette)
    for kind,owner,mode,suppress in itertools.product((97,157,82,108,109),(0,1,5,8,255),(0,1),(0,0xFFFFFFFF)):
        add(f'colors-{kind}-{owner}-{mode}-{suppress}',forms=(form(1,kind,owner,4),),heads=((0,32,48,1),),colors=high_colors,challenge=mode,suppress=suppress,frame=0x7FEDCBA5)
    return rows


def main():
    """보호 원문 확인 뒤 새 실제 관측과 실행 범위만 저장한다."""
    if digest(PE_PATH)!=PE_SHA: raise RuntimeError('보호 PE SHA256 불일치')
    oracle,inputs,rows=Oracle(),cases(),[]  # 독립 실행기/입력/관측 행이다.
    for index,case in enumerate(inputs):  # 같은 입력을 두 정밀도에서 각각 온전하게 실행한다.
        previous=None
        for control in CONTROLS:
            try: output=oracle.run(case,control)
            except Exception as error: raise RuntimeError(f'{case["name"]}: {error}') from error
            if previous is not None and previous!=output: raise RuntimeError('정밀도 차이: '+case['name'])
            previous=output
            values=dict(case,name=case['name']+f'-cw{control:03x}',**output)  # 관측 결과만 입력 옆에 보존한다.
            rows.append('\t'.join(str(values[field]) for field in SCHEMA))
        if index%1000==0: print('observed',index,flush=True)
    FIXTURE.write_text('# '+'\t'.join(SCHEMA)+'\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report={'host':socket.gethostname(),'lastDecompileHost':'VM-W11-Codex / 2026-10-23','inputs':len(inputs),'observations':len(rows),
            'schema':SCHEMA,'precisionDifferences':0,'precisions':oracle.totals,'writes':dict(oracle.write_totals),
            'ranges':[[hex(low),hex(high)] for low,high in RANGES],'distinctInstructionAddresses':len(oracle.visited),
            'inputGroups':dict(Counter(case['name'].split('-',1)[0] for case in inputs)),
            'scope':'004808d0 전체와 실제4b2b00/4ac200/49a840/CRT4e49c0을 실행한다. 하위 장치/반환값 대체는 없다. 실제Pool/TYPE/단계0..3 Hash 배열과128×128 bitmap을 공급한다.',
            'limits':['GUI/원본게임/미니맵Gump/카메라/Refresh00481320/Pop/Unpop은 실행하지 않는다. 마지막 Refresh의 소비/시각 연결은 기존dirty 근거와 별개다.',
                      '정상 셀좌표0..63, 물리Pool256/TYPE188 및 유효 무순환 체인만 입력한다. 잘못된셀/풀/출력주소를 원본메모리손상으로 실행하지 않는다.',
                      'pixels는 초기A5와 달라진 전체bitmap 위치이며 writes는 실제BYTE store 순서다. 주변bitmap 불변과 전체bitmapsha도 보존한다.'],
            'colorAddresses':[hex(address) for address in COLORS],'gameExecuted':False,'guiExecuted':False,
            'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE_PATH,ROOT/'extracted/refined/originals/Netstorm.c',Path(__file__),FIXTURE)}}  # 원문/도구/산출물 식별자다.
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:report[key] for key in ('inputs','observations','precisionDifferences')}),flush=True)


if __name__=='__main__':
    main()
