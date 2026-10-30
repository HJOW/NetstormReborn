#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Alt/가운데 폴링의 현재 지도 거리·FTOL/8과 목표 쓰기를 격리 x86에서 관측한다."""
import argparse
import itertools
import json
import platform
import struct
from pathlib import Path
from decomp_edge_scroll import Oracle as EdgeOracle, ROOT, STACK, RENDERER
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EDI, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

ENTRY, PAN_END, QUEUE_END = 0x4d6b54, 0x4d6cda, 0x4d6da2  # 실제 가운데 폴링부터 두 큐 경로 직전까지다.
FIELDS = ('code','x','y','middle','shift','scrollOutside','dialog','active','clientWidth','clientHeight','viewX','viewY','viewWidth','viewHeight','cameraX','cameraY','targetX','targetY','revision','change')  # 출력과 무관한 명시 입력 열이다.
GLOBALS = ((0x5318d0,'scrollOutside'),(0x565dd4,'dialog'),(0x54dc50,'active'),(0x5c78fc,'clientWidth'),(0x5c7900,'clientHeight'),(0x5ca99c,'viewX'),(0x5ca9a0,'viewY'),(0x5ca9b4,'viewWidth'),(0x5ca9b8,'viewHeight'),(0x5c85dc,'revision'))  # 각 원문이 읽는 현재 DWORD 주소다.


def signed(value):
    """관측된 하위 DWORD를 signed 십진수로만 표시한다."""
    return struct.unpack('<i',struct.pack('<I',value & 0xffffffff))[0]


class Oracle(EdgeOracle):
    """FTOL/497e00은 실제 명령으로 실행하고 두 폴링/팝업 요청만 입력 경계로 둔다."""
    def __init__(self):
        """기존 명령 범위의 FTOL/목표 함수를 보존하고 부모 구간만 교체한다."""
        super().__init__();self.allowed[0]=(ENTRY,PAN_END)
        self.initialScroll=self.words(0x5318d0,1)[0]  # 원본 PE의 최초 scrollWhileMouseOut=1을 직접 관측한다.

    def instruction(self,mu,address,size,data):
        """반환 경로/실제 호출 인자를 기록하고 callback 뒤 현재 전역 변경도 공급한다."""
        if address in (PAN_END,QUEUE_END):
            self.completed=True;self.selected=address==PAN_END;mu.emu_stop();return
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,2)
        if address==0x452e00:
            code=args[1];assert code in (0x40000,0x100000);release=self.case['middle'] if code==0x40000 else self.case['shift'];self.events.append('P:'+str(code))
            mu.mem_write(args[0],struct.pack('<3I',code|(0x80000000 if release else 0),self.case['x']&0xffffffff,self.case['y']&0xffffffff))
            if code==0x100000 and self.case['change']==2:
                self.put(RENDERER+0x1c,900);self.put(RENDERER+0x20,901);self.put(0x5c85dc,0xffffffff)
            self.ret(args[0]);return
        if address==0x466710:
            self.events.append('C')
            if self.case['change']==1:
                for address,value in ((0x5ca99c,13),(0x5ca9a0,17),(0x5ca9b4,440),(0x5ca9b8,330),(0x5c78fc,880),(0x5c7900,660)): self.put(address,value)  # 팝업 요청 뒤에는 이전 지도 사본을 쓰지 않는다.
            self.ret();return
        if address==0x497e00: self.events.append('D:'+str(signed(args[0]))+':'+str(signed(args[1])))
        if not any(first<=address and address+size<=last for first,last in self.allowed): raise RuntimeError(f'카메라 pan 관측 밖 명령 {address:08x}')

    def run(self,case,control):
        """별도 스택·전역 입력으로 실제 선택/목표/현재 view와 호출 순서를 관측한다."""
        self.case=case;self.events=[];self.completed=False;self.selected=False  # 각 사례의 관측 상태를 별개로 보존한다.
        for address,key in GLOBALS: self.put(address,case[key])  # 이전 실행의 callback 변경을 이어받지 않는다.
        self.put(0x59a8bc,RENDERER);self.put(0x5c78f4,768);self.mu.mem_write(RENDERER,bytes(256))
        for offset,key in ((0x1c,'cameraX'),(0x20,'cameraY'),(0x50,'targetX'),(0x54,'targetY')): self.put(RENDERER+offset,case[key])  # 현재 view와 이전 목표를 구별한다.
        self.mu.mem_write(STACK,bytes(1024));self.put(STACK+0x14,case['code']);self.put(STACK+0x18,case['x']);self.put(STACK+0x1c,case['y'])
        for register,value in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_EDI,0),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)): self.mu.reg_write(register,value)  # 실제 부모 진입 계약과 빈 x87을 준비한다.
        self.mu.emu_start(ENTRY,QUEUE_END+1,count=10000);assert self.completed and self.mu.reg_read(UC_X86_REG_ESP)==STACK
        return [int(self.selected),*self.words(RENDERER+0x1c,2),*self.words(RENDERER+0x50,2),self.words(0x5c85dc,1)[0],';'.join(self.events)]


def cases():
    """명시 화면/포인터/관문·감김 입력만 만들며 기대 결과는 계산하지 않는다."""
    base=dict(zip(FIELDS,(0x120000,600,400,1,1,1,0,1,1024,768,128,26,896,680,100,200,300,400,17,0)));rows=[]
    for x,y,middle,shift in itertools.product((-10,128,129,575,576,577,1024,1200),(-10,26,365,366,367,706,900),(0,1),(0,1)): rows.append(dict(base,x=x,y=y,middle=middle,shift=shift))  # 중심/양끝·범위밖과 Alt/가운데/Shift를 독립 입력한다.
    for key,values in (('code',(0x80120000,0x4120000,0x84120000)),('scrollOutside',(0,2)),('dialog',(1,-1)),('active',(0,2)),('clientWidth',(0,1280,-1)),('viewWidth',(0,1,-1,-3,2147483647)),('viewHeight',(0,1,-1,-3)),('x',(-1,-2147483648,2147483647)),('y',(-1,-2147483648,2147483647)),('change',(1,2))):
        for value in values: rows.append(dict(base,**{key:value}))  # 한 관문/물리 입력씩 바꾸어 조기 반환을 구별한다.
    for width,height in ((1280,720),(1280,800),(1366,768),(1920,1080)):
        rows.append(dict(base,clientWidth=width,clientHeight=height,viewWidth=width-128,viewHeight=height-88,x=width,y=height))  # 확장 화면비의 실제 signed 크기 입력이다.
    rows.append(dict(base,code=0x84120000,middle=1,scrollOutside=0));rows.append(dict(base,cameraX=2147483647,cameraY=-2147483648,revision=0xffffffff))
    return [dict(zip(FIELDS,row)) for row in dict.fromkeys(tuple(row[key] for key in FIELDS) for row in rows)]  # 입력 중복만 제거하고 최초 순서를 보존한다.


def main():
    """최초 관측을 새 경로에 쓰고 기본53bit와64bit 탐침을 별개로 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    inputs=cases();oracle=Oracle();first=[oracle.run(case,0x27f) for case in inputs];replay=[oracle.run(case,0x27f) for case in inputs];assert first==replay and oracle.initialScroll==1
    extended=[oracle.run(case,0x37f) for case in inputs];precision=[index for index,(normal,extra) in enumerate(zip(first,extended)) if normal!=extra]  # 정밀도 차이가 있으면 삭제/혼합하지 않고 별도 한계로 남긴다.
    for name,values in (('first.json',first),('replay.json',replay),('extended.json',extended)): (out/name).write_text(json.dumps(values,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')  # 최초 원시 관측을 덮어쓰지 않는다.
    fixture=out/'camera-pan-x86.tsv'
    with fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# '+'\t'.join((*FIELDS,'selected','currentX','currentY','targetX','targetY','revision','events'))+'\n')
        for case,value in zip(inputs,first): stream.write('\t'.join(map(str,[*(case[key] for key in FIELDS),*value]))+'\n')  # 실제 원문 결과만 fixture로 쓴다.
    report={'unit':615,'host':platform.node(),'lastFullDecompilation':'HJOW-Athlon / 2026-10-23','entry':hex(ENTRY),'ends':[hex(PAN_END),hex(QUEUE_END)],'cases':len(inputs),'observations':len(inputs)*3,'replayDifferences':[],'extendedPrecisionDifferences':precision,'initialScrollWhileMouseOut':oracle.initialScroll,'scope':'기본53bit 원문/재관측과64bit 탐침. 두 폴링/팝업요청만 명시 경계이며 FTOL/카메라목표는실제명령이다. 게임GUI/MP/공격/포획/희생/승리 실행은 없다.'}
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
