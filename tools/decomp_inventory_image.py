#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""생산 항목의 실제 SHP Gump 생성·프레임·출력·투명 적중 인자를 독립 관측한다."""
import argparse
import collections
import json
import socket
from pathlib import Path
from decomp_factory_menu import Oracle as BaseOracle, ROOT, TYPES, TEXT, fbits, digest
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

ENTRIES=(0x419fc0,0x4198f0,0x419990,0x419cd0,0x419cb0,0x4642c0,0x49a840,0x40da40,0x4e49c0) # 실제 전체 몸체다.
NODE=TEXT+0x1000 # 원본 노드와 독립된 격리 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/inventory-image-x86.tsv' # C++ 전에 동결할 전체 출력이다.
REPORT=ROOT/'cpppj/inventory-image.json' # 실제 실행 범위/입력 출처다.


def inputs():
    """출력 판정 없이 헤더·타입·배율·모드·현재owner·좌표를 교차한다."""
    rows=[]
    # signed 헤더와 TYPE 기준점의 단정도 저장도 실제 생성/프레임 갱신으로 관측한다.
    for op in ('Construct','Reset'):
        for width,height,hx,hy,tx,ty in ((40,53,12,-7,2,3),(0,-1,-32768,32767,-2147483648,2147483647),(32767,-32768,32767,-32768,16777217,-16777217)):
            for scale in (1,0.63,-1):rows.append((op,','.join(map(str,(width,height,hx,hy,tx,ty,fbits(scale),5)))))
    # 모드4의 먼저 그리기/모드1 우선/owner0·음수·비0와 현재 표 존재를 독립 교차한다.
    for mode in (0,1,2,3,4,5,6,7,8,12):
        for scale in (1,0.63,0,-1):
            for owner,table in ((0,0),(1,0),(1,1),(-1,1)):
                rows.append(('Draw',','.join(map(str,(mode,fbits(scale),owner,table,fbits(12.25),fbits(-3.75),17,-9,5,5)))))
    rows.append(('Draw',','.join(map(str,(4,fbits(1),1,1,fbits(12.25),fbits(-3.75),2147483647,-2147483648,3,-4)))))
    rows.append(('Draw',','.join(map(str,(4,fbits(0.63),1,1,fbits(12.25),fbits(-3.75),-17,9,-3,8)))))
    # 적중 함수는 로컬 항목 좌표·그림 자식 위치·기준점/배율을 조합한다.
    for scale in (1,0.63,-1,0):
        for px,py in ((0,0),(7.5,-3.25),(199,200),(-1,-200),(1e9,-1e9)):
            for answer in (0,1):rows.append(('Hit',','.join(map(str,(fbits(scale),fbits(px),fbits(py),answer)))))
    rows.append(('Attach','0'));return rows


class Oracle(BaseOracle):
    """이미지 Gump의 실제 계산을 실행하며 자산 헤더와 VFX/좌표 효과만 명시한다."""
    def __init__(self):
        """검토한 함수 구간과 SEH 저장 페이지를 준비한다."""
        super().__init__();self.mu.mem_map(0,0x1000);self.allowed=[];self.hits=collections.Counter();self.boundaries=collections.Counter()
        # 정밀 함수표의 불연속 구간을 끝 바이트까지 허용한다.
        for entry in ENTRIES:
            for part in self.facts[entry]['ranges'].split(';'):
                low,high=(int(word,16) for word in part.split('-'));self.allowed.append((low,high+1))

    def event(self,name,*args):
        """관측한 실제 인자만 기록한다."""
        self.events.append(','.join(map(str,(name,*args))))

    def instruction(self,mu,address,size,data):
        """허용 몸체는 그대로 실행하고 외부 효과만 입력 경계로 응답한다."""
        self.recent.append(hex(address))
        if any(low<=address and address+size<=high for low,high in self.allowed):
            self.instructions+=1
            if address in ENTRIES:self.hits[hex(address)]+=1
            return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,9);ecx=mu.reg_read(UC_X86_REG_ECX);self.boundaries[hex(address)]+=1
        if address==0x419850:
            self.event('Header',(ecx-TYPES)//500,*args[:2]);self.ret(TEXT+0xe000,8)
        elif address==0x464740:
            self.event('Position');self.put(args[0],self.point[0]);self.put(args[0]+4,self.point[1]);self.ret(args[0],4)
        elif address==0x4dabe0:self.event('Color',args[0]);self.ret()
        elif address in (0x4daa70,0x4daab0,0x4daaf0,0x4dab40):
            self.event(hex(address),*args[:(4 if address in (0x4daa70,0x4daab0) else 7)]);self.ret()
        elif address==0x4daca0:self.event('Hit',*args);self.ret(self.answer)
        elif address==0x466750:self.event('BaseAttach',*args[:4]);self.ret(purge=16)
        else:raise RuntimeError('생산 그림 미검토 명령 '+hex(address))

    def run(self,row,control):
        """명시 raw노드/TYPE/헤더만 준비하고 정상 반환과 전체 필드를 읽는다."""
        op,text=row;values=list(map(int,text.split(',')));self.prepare([85,1,1,128,0,3,0,800,0,'-','-','-','-',0]);self.mu.mem_write(TEXT+0x1000,b'\xa5'*0x100);self.mu.mem_write(0x563e38,bytes(4000));self.put(0x565dc8,0);self.put(0x540418,23);self.put(0,0xffffffff);self.events=[]
        self.put(0x59ab20,TYPES);self.put(TYPES+128*500+0xdc,0x16000000);self.put(0x557940+128*4,0)
        if op in ('Construct','Reset'):
            width,height,hx,hy,tx,ty,scale,mode=values;self.put(TEXT+0xe018,width,2);self.put(TEXT+0xe01a,height,2);self.put(TEXT+0xe01c,hx,2);self.put(TEXT+0xe01e,hy,2);self.put(TYPES+128*500+0x1dc,tx);self.put(TYPES+128*500+0x1e0,ty)
            self.put(NODE+56,scale)
            result,trace=self.invoke(0x419fc0 if op=='Construct' else 0x4198f0,(128,7,mode),NODE,12,control)
            result=str(result) if op=='Construct' else '-'
        else:
            self.put(NODE,0x501bb0);self.put(NODE+160,128);self.put(NODE+164,7);self.put(NODE+172,fbits(12.25));self.put(NODE+176,fbits(-3.75));self.put(NODE+76,fbits(2.5));self.put(NODE+80,fbits(-4.5))
            if op=='Draw':
                mode,scale,owner,table,hx,hy,px,py,sx,sy=values;self.point=(px,py);self.put(NODE+56,scale);self.put(NODE+168,mode);self.put(NODE+172,hx);self.put(NODE+176,hy);self.put(0x540c70,owner);self.put(0x557940+128*4,TEXT+0x8000 if table else 0);self.put(0x52f434,sx);self.put(0x52f444,sy)
                _,trace=self.invoke(0x419990,(),NODE,0,control);result='-'
            elif op=='Hit':
                scale,px,py,self.answer=values;self.put(NODE+56,scale);result,trace=self.invoke(0x419cd0,(px,py),NODE,8,control);result=str(result)
            else:
                _,trace=self.invoke(0x419cb0,(fbits(4),fbits(5),0x1234,7),NODE,16,control);result='-'
        assert self.words(0,1)==(0xffffffff,)
        return result,bytes(self.mu.mem_read(NODE,0xb4)).hex(),trace


def generate():
    """C++에 의존하지 않는 두 정밀도 관측만 저장한다."""
    oracle=Oracle();rows=[]
    # 각 입력은 두 원본 실행에서 초기 상태부터 재생한다.
    for row in inputs():
        first=oracle.run(row,0x27f);second=oracle.run(row,0x37f);assert first==second,row;rows.append([*row,*first])
    FIXTURE.write_text('# 연산 명시입력 반환 전체이미지노드 실제호출\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_factory_menu.py',ROOT/'tools/decomp_priestmenu.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE]
    r=dict(host=socket.gethostname(),date='2026-10-18',unit=176,rows=len(rows),normalReturns=oracle.returns,instructions=oracle.instructions,controls=[0x27f,0x37f],precisionDifferences=0,nativeCalls=dict(oracle.hits),boundaries=dict(oracle.boundaries),sha256={path.relative_to(ROOT).as_posix():digest(path) for path in paths},limits=['419fc0/4198f0/419990/419cd0/419cb0와 기본 생성·TYPE/localowner getter/CRT ftol의 실제 전체 명령이다. 헤더·현재 위치/색표/VFX 그리기·적중/기본 부착은 명시 경계다.','게임/OS/창 실행·원본 파일 변경/새 디컴파일 없음. VFX 픽셀 자체와 실제 Client 생산 패널 연결은 별도 검사다.'])
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('생산 그림 원본 관측',len(rows),oracle.returns,oracle.instructions)


def verify():
    """기대 원문을 재생성하지 않고 출처/입력/반환을 감사한다."""
    r=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    for path,wanted in r['sha256'].items():assert digest(ROOT/path)==wanted,path
    assert len(rows)==r['rows']==len(inputs()) and r['normalReturns']==len(rows)*2 and not r['precisionDifferences']
    for row,case in zip(rows,inputs()):assert row[:2]==list(case) and len(row)==5
    print('생산 그림 원본/SHA/입력 감사',len(rows))


# 새 관측과 기존 독립 근거 감사는 명시 옵션으로 구별한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
