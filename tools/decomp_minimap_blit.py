#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 미니맵 Draw와 원시 복사·클립·VFX 외곽선을 실제 PE로 관측한다."""
import csv
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_GDTR, UC_X86_REG_CS, UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES

ROOT=Path(__file__).resolve().parents[1]  # 보호 입력과 독립 산출물의 루트다.
PE=ROOT/'originals/Netstorm.exe'  # 읽기 전용 10.78 보호 PE다.
PE_SHA='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 원본 입력의 바이트 신원이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/minimap-blit-x86.tsv'  # 원문 결과만 기록하는 새 자료다.
REPORT=ROOT/'extracted/cpp-minimap-blit-20261023-native.json'  # 명령 범위와 입력·출력 SHA의 보고서다.
NODE,SCREEN,BITMAP,CANVAS,STACK,RETURN,GDT=0x1000100,0x1001000,0x1020000,0x1040100,0x2100000,0x1002000,0x40000000  # 격리 객체/표면/소스/대상/스택/반환/평면 세그먼트다.
WIDTH,HEIGHT,GUARD=192,160,0x8000  # 명시 대상 크기와 앞뒤 읽기 관측용 공간이다. guard는 원문 할당의 일부가 아니다.
ENTRIES=(0x480660,0x464200,0x464290,0x445290,0x4a1580,0x4a1fe0,0x4a0c10,0x4a24b0,0x4dade0,0x4011f1,0x4e49c0)  # 대체 없이 실행하는 전체 본체다.
INPUTS=('name','control','widthBits','heightBits','cropX','cropY','destX','destY','clipLeft','clipTop','clipRight','clipBottom','cameraXBits','cameraYBits','viewLeft','viewTop','viewWidth','viewHeight','pixelsX','pixelsY','bitmapSeed','guardSeed','background','ink')  # 원문 기대값과 독립적인24열 입력이다.
OUTPUTS=('completed','changedRuns','readRanges','guardReads','blit','frame','clipout','instructions','stackdelta')  # 실제 CPU의9열 출력이다.


def sha(path):
    """지정한 파일의 실제 바이트 신원을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """입력 float를 실제 DWORD로 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def signed(value):
    """현재 DWORD를 비트 변경 없이 signed32로 읽는다."""
    return struct.unpack('<i',struct.pack('<I',value&0xffffffff))[0]


def pattern(index,seed):
    """원시 입력 메모리만 만드는 BYTE 패턴이다. 복사·클립 기대값은 계산하지 않는다."""
    return (index*73+(index>>7)*19+seed)&255


def ranges(values):
    """실제 읽은 BYTE 집합을 정렬한 연속 구간으로 압축한다."""
    result=[];first=None;last=None  # 현재 연속 관측 구간이다.
    for value in sorted(values):  # CPU가 읽은 주소만 보존한다.
        if first is None:first=last=value
        elif value==last+1:last=value
        else:result.append(f'{first}:{last-first+1}');first=last=value
    if first is not None:result.append(f'{first}:{last-first+1}')
    return ';'.join(result) or '-'


def changed(raw,background):
    """전체 대상의 변경 BYTE를 위치·연속 hex로 무손실 기록한다."""
    result=[];index=0  # 이전 배경과 다른 BYTE만 저장하되 재생 시 전체 표면과 비교한다.
    while index<len(raw):  # 선/블릿 밖 배경도 전체 비교에 포함된다.
        if raw[index]==background:index+=1;continue
        first=index
        while index<len(raw) and raw[index]!=background:index+=1  # 연속 변경 구간의 끝이다.
        result.append(f'{first}:{raw[first:index].hex()}')
    return ';'.join(result) or '-'


class Oracle:
    """Bounds 장치만 입력으로 공급하고 원문 픽셀 복사와 VFX 선을 실행한다."""
    def __init__(self):
        """실제 PE/함수 범위/격리 표면과32BIT 평면 세그먼트를 준비한다."""
        pe=pefile.PE(str(PE));self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(0x400000,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);self.mu.mem_write(0x400000,pe.get_memory_mapped_image())
        for address,size in ((0,4096),(0x1000000,0x60000),(STACK-0x10000,0x20000),(GDT,4096)):self.mu.mem_map(address,size)  # 분리된 입력과 스택이다.
        self.mu.mem_write(GDT,struct.pack('<QQQ',0,0x00cf9a000000ffff,0x00cf92000000ffff));self.mu.reg_write(UC_X86_REG_GDTR,(0,GDT,23,0));self.mu.reg_write(UC_X86_REG_CS,8)
        for register in (UC_X86_REG_SS,UC_X86_REG_DS,UC_X86_REG_ES):self.mu.reg_write(register,16)  # VFX push/pop ES의 실제 평면 스택이다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.original={};self.allowed=[];self.total=Counter();self.calls=Counter()  # 실제 코드 불변·전체 명령의 계수다.
        for entry in ENTRIES:  # 함수표의 불연속 범위도 빠뜨리지 않는다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1));self.original[low]=bytes(self.mu.mem_read(low,high+1-low))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def number(self,address,value=None):
        """명시 입력 DWORD를 쓰거나 현재 원문 DWORD를 읽는다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def instruction(self,mu,address,size,data):
        """Bounds 반환 외에는 실제 선택 본체의 명령만 허용한다."""
        if address==RETURN:self.completed=True;mu.emu_stop();return
        if address==0x4648e0:
            esp=mu.reg_read(UC_X86_REG_ESP);destination=self.number(esp+4);case=self.case
            if mu.reg_read(UC_X86_REG_ECX)!=NODE:raise RuntimeError('Bounds this 불일치')
            w=struct.unpack('<f',struct.pack('<I',case['widthBits']))[0];h=struct.unpack('<f',struct.pack('<I',case['heightBits']))[0]
            mu.mem_write(destination,struct.pack('<4i',case['destX'],case['destY'],case['destX']+int(w),case['destY']+int(h)))
            mu.reg_write(UC_X86_REG_EIP,self.number(esp));mu.reg_write(UC_X86_REG_ESP,esp+8);mu.reg_write(UC_X86_REG_EAX,destination);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미명시 코드 {address:08x}')
        self.count+=1;self.total[f'{address:08x}']+=1
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address in (0x4a1fe0,0x4a24b0):
            esp=mu.reg_read(UC_X86_REG_ESP);count=8 if address==0x4a1fe0 else 5;values=[signed(self.number(esp+4+index*4)) for index in range(count)]  # 실제 전달 인자다.
            if address==0x4a1fe0:values[2]-=BITMAP;self.blit=':'.join(map(str,values))
            else:self.frame=':'.join(map(str,values))

    def read(self,mu,access,address,size,value,data):
        """명시 원시 소스 안에서 실제 읽은 모든 BYTE 주소와 guard 횟수를 보존한다."""
        if BITMAP-GUARD<=address and address+size<=BITMAP+0x4000+GUARD:
            self.reads.update(range(address-BITMAP,address-BITMAP+size))
            if address<BITMAP or BITMAP+0x4000<address+size:self.guard_reads+=1

    def write(self,mu,access,address,size,value,data):
        """정상 스택/SEH/clip 전역/현재 대상 표면 외의 원문 쓰기를 거부한다."""
        regions=((STACK-0x4000,STACK+40),(0,4),(CANVAS,CANVAS+WIDTH*HEIGHT),(0x5c7a44,0x5c7a54),(0x5b5de0,0x5b5df0))  # 원문이 변경하는 정확한 저장소다.
        if not any(low<=address and address+size<=high for low,high in regions):raise RuntimeError(f'미명시 쓰기 {address:08x}/{size}')

    def run(self,case,control):
        """전체 Draw 진입에서 정상 RET까지 실행해 전체 픽셀과 원문 읽기를 반환한다."""
        self.case=case;self.count=0;self.completed=False;self.reads=set();self.guard_reads=0;self.blit=self.frame='-'
        self.mu.mem_write(NODE,bytes(0xac));self.number(NODE+0x3c,case['widthBits']);self.number(NODE+0x40,case['heightBits']);self.number(NODE+0xa0,case['cropX']);self.number(NODE+0xa4,case['cropY'])
        self.mu.mem_write(SCREEN,bytes(0x8c));self.number(SCREEN+0x78,1);self.number(SCREEN+0x74,CANVAS);self.number(SCREEN+0x14,WIDTH);self.number(SCREEN+0x10,HEIGHT);self.number(SCREEN+0x18,WIDTH)
        self.number(0x5c7948,SCREEN);self.number(0x5949c4,BITMAP);self.number(0x5949cc,128);self.number(0x5bec2c,case['ink'])
        for address,key in ((0x59a900,'cameraXBits'),(0x59a904,'cameraYBits'),(0x59a908,'pixelsX'),(0x59a90c,'pixelsY'),(0x5ca99c,'viewLeft'),(0x5ca9a0,'viewTop'),(0x5ca9b4,'viewWidth'),(0x5ca9b8,'viewHeight')):self.number(address,case[key])  # 실제 현재 카메라와 배율 전역이다.
        clip=tuple(case[key] for key in ('clipLeft','clipTop','clipRight','clipBottom'))
        self.mu.mem_write(0x5c7a44,struct.pack('<4i',*clip));self.mu.mem_write(0x5b5de0,struct.pack('<4i',clip[0],clip[1],clip[2]-1,clip[3]-1));self.number(0x5b5ddc,SCREEN+0x100)
        self.mu.mem_write(SCREEN+0x100,struct.pack('<4I',CANVAS,WIDTH-1,HEIGHT-1,1))
        self.mu.mem_write(BITMAP-GUARD,bytes(pattern(index-GUARD,case['guardSeed']) for index in range(GUARD*2+0x4000)));self.mu.mem_write(BITMAP,bytes(pattern(index,case['bitmapSeed']) for index in range(0x4000)))
        self.mu.mem_write(CANVAS-32,bytes([0x7d])*(WIDTH*HEIGHT+64));self.mu.mem_write(CANVAS,bytes([case['background']])*(WIDTH*HEIGHT));self.number(0,0);self.mu.mem_write(STACK-0x4000,bytes([0xcc])*0x4028);self.number(STACK,RETURN)
        registers={UC_X86_REG_EAX:0x10111213,UC_X86_REG_EBX:0x20212223,UC_X86_REG_ECX:NODE,UC_X86_REG_EDX:0x40414243,UC_X86_REG_ESI:0x50515253,UC_X86_REG_EDI:0x60616263,UC_X86_REG_EBP:0x70717273,UC_X86_REG_ESP:STACK,UC_X86_REG_FPCW:control,UC_X86_REG_FPSW:0}
        for register,value in registers.items():self.mu.reg_write(register,value)  # 이전 실행의 잔여 레지스터를 제거한다.
        self.mu.emu_start(0x480660,RETURN+1,count=1000000)
        delta=self.mu.reg_read(UC_X86_REG_ESP)-STACK-4
        if not self.completed or delta or self.number(0):raise RuntimeError('정상 RET/스택/SEH 불일치')
        for register in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 원문 callee-save 계약이다.
            if self.mu.reg_read(register)!=registers[register]:raise RuntimeError('callee-save 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('x87 균형 불일치')
        if bytes(self.mu.mem_read(CANVAS-32,32))+bytes(self.mu.mem_read(CANVAS+WIDTH*HEIGHT,32))!=bytes([0x7d])*64:raise RuntimeError('대상 guard 변경')
        for address,raw in self.original.items():  # 실제 실행 본문의 원본 바이트가 보존됐는지 확인한다.
            if bytes(self.mu.mem_read(address,len(raw)))!=raw:raise RuntimeError('실행 코드 변경')
        return dict(completed=1,changedRuns=changed(bytes(self.mu.mem_read(CANVAS,WIDTH*HEIGHT)),case['background']),readRanges=ranges(self.reads),guardReads=self.guard_reads,blit=self.blit,frame=self.frame,clipout=':'.join(str(signed(self.number(0x5c7a44+i*4))) for i in range(4)),instructions=self.count,stackdelta=delta)


def cases():
    """합법 폭130과 음수/행넘김/화면 clip/외곽선/미지 guard를 독립 입력으로 교차한다."""
    base=dict(widthBits=bits(130),heightBits=bits(75),cropX=-2,cropY=0,destX=8,destY=7,clipLeft=0,clipTop=0,clipRight=WIDTH,clipBottom=HEIGHT,cameraXBits=bits(0),cameraYBits=bits(0),viewLeft=85,viewTop=0,viewWidth=939,viewHeight=672,pixelsX=16,pixelsY=11,bitmapSeed=31,guardSeed=197,background=211,ink=155)
    result=[]  # 기대 픽셀과 독립인 입력 목록이다.
    for size,crop,clip in itertools.product(((80,75),(128,128),(130,75),(130,130),(160,140)),((-2,0),(-2,1),(0,0),(7,9)),((0,0,WIDTH,HEIGHT),(11,12,101,82))):  # 같은 선행주소라도 destination clip에 따라 guard 읽기가 제거되는 경계를 확인한다.
        result.append(dict(base,name=f'cross-{len(result):03d}',widthBits=bits(size[0]),heightBits=bits(size[1]),cropX=crop[0],cropY=crop[1],clipLeft=clip[0],clipTop=clip[1],clipRight=clip[2],clipBottom=clip[3]))
    for crop,guard in itertools.product(((-2,0),(-2,1),(0,-2),(0,1),(130,0)),(0,1,255)):  # bitmap 밖·앞/뒤 행과 같은 입력 guard 값만 바꿔 실제 의존을 드러낸다.
        result.append(dict(base,name=f'guard-{len(result):03d}',cropX=crop[0],cropY=crop[1],guardSeed=guard))
    for width,height,dest in ((129,1,(-2,0)),(127.75,74.25,(20,20)),(130,75,(-2,-1)),(130,130,(70,60)),(160,140,(8,7))):  # 폭128 뒤 기존 배경과 bitmap 밖 outline/화면 잘림을 확인한다.
        result.append(dict(base,name=f'edge-{len(result):03d}',widthBits=bits(width),heightBits=bits(height),destX=dest[0],destY=dest[1],cameraXBits=bits(220),cameraYBits=bits(130)))
    return result


def main():
    """등록 제품·원본을 변경하지 않고 새 독립 관측과 범위 보고서만 쓴다."""
    if socket.gethostname().lower()!='vm-w11-codex' or sha(PE)!=PE_SHA:raise RuntimeError('현재 호스트/보호 입력 불일치')
    oracle=Oracle();observations=[];differences=[];inputs=cases()
    for case in inputs:  # 동일 입력을 두 x87 정밀도로 실제 재실행한다.
        previous=None
        for control in (0x27f,0x37f):
            try:output=oracle.run(case,control)
            except Exception as error:raise RuntimeError(case['name']) from error
            if previous is not None and previous!=output:differences.append(case['name'])
            previous=output;values=dict(case,control=control,**output);observations.append('\t'.join(str(values[key]) for key in (*INPUTS,*OUTPUTS)))
    FIXTURE.write_text('# 실제 PE의 전체 Draw→복사→VFX선. guard BYTE는 명시 입력이며 실제 게임 heap 관측값이 아니다.\n#'+'\t'.join((*INPUTS,*OUTPUTS))+'\n'+'\n'.join(observations)+'\n',encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),last_decompile_host='VM-W11-Codex',date='2026-10-23',inputs=len(inputs),observations=len(observations),schema=[*INPUTS,*OUTPUTS],precision_differences=differences,instructions=sum(oracle.total.values()),entry_calls=oracle.calls,native_ranges=[dict(first=f'{first:08x}',end_exclusive=f'{first+len(raw):08x}',sha256=hashlib.sha256(raw).hexdigest()) for first,raw in oracle.original.items()],sha256={str(path.relative_to(ROOT)).replace('\\','/'):sha(path) for path in (PE,Path(__file__),FIXTURE)},scope=['00480660/464200/464290/445290/4a1580/4a1fe0/4a0c10/4a24b0/4dade0/4011f1/4e49c0은 대체 없이 실행한다.','4648e0 Bounds만 입력 destination와 현재 width/height를 절삭해 반환하는 명시 장치다.','Screen.type=1이며 이미 locked인 DIB8BIT 표면이다. DirectDraw IsLost/restore 분기는 실행하지 않는다.','bitmap 앞뒤의 guard는 명시 패턴 입력이다. 원본 new(0x4000)의 실제 allocator 인접 BYTE를 이 관측이 복원하지 않는다.','전용 게임/GUI/실제 frame순서/전체 HUD 생성·카메라·Refresh는 실행하지 않는다.'],new_games=0,new_decompilations=0)
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('inputs','observations','instructions','precision_differences')},ensure_ascii=False))


if __name__=='__main__':main()
