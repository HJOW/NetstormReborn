#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 생산 격자·내용물 항목 생성/부착/통지의 실제 x86을 독립 관측한다."""
import argparse
import collections
import json
import socket
import struct
from pathlib import Path
from decomp_factory_menu import Oracle as MenuOracle, ROOT, POOL, TEXT, digest, fbits
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_MXCSR

# 전체 격자와 항목 생성·점멸 시작, 기존 기본 생성자/CRT만 실행한다.
ENTRIES=(0x43d790,0x43e120,0x43d690,0x43e1e0,0x43d850,0x43d9c0,0x4642c0,0x4e6e10,0x4e49c0)
FIXTURE=ROOT/'cpppj/tests/fixtures/inventory-grid-x86.tsv' # 새 C++ 전에 동결할 독립 출력이다.
REPORT=ROOT/'cpppj/inventory-grid.json' # 실제 범위·호출·정밀도·입력 SHA다.
NODE=TEXT+0x1000 # 격자/단일 항목의 안정된 격리 저장소다.


def inputs():
    """원본 결과를 추측하지 않고 생성·배치·입력 축을 명시한다."""
    rows=[]
    # 0열 진단 뒤의 정상 생성과 부호 있는 최소 행/열도 관측한다.
    for columns,minimum,mask in ((0,6,8),(1,6,8),(2,4,64),(-1,-2,0xffffffff)):
        rows.append(('Grid',f'{columns},{minimum},{mask}'))
    # flags200의 점멸 시계/idle 등록과 float 비트 복사를 분리한다.
    for flags in (0,8,0x100,0x200,0x208,0x300):
        for width,height,now in ((40,250,0),(19.5,0.25,1.25),(-2,0,100.01)):
            rows.append(('Item',f'{fbits(width)},{fbits(height)},128,{flags},{now}'))
    # 실제 슬롯 번호의 빈 칸/중복/최댓값과 필터에 걸리지 않는 내용물이다.
    forms=('-', '8:0', '8:1', '8:0;8:1', '8:0;8:5', '8:7;64:255;8:3', '64:255', '8:255', '8:0;8:0')
    for width,height in ((40,250),(40,144),(250,40),(100,100),(0,40)):
        for contents in forms:
            # fixed는 원본의 별도 열/행 입력을 선택한다.
            for fixed in (0,1):
                rows.append(('Attach',f'2,6,8,{fixed},0,1,{fbits(width)},{fbits(height)},{fbits(width*1.5)},{fbits(height*1.5)},0|{contents}'))
    # 빈 목록의 실제 attach mode와 c0 배율/클릭조건의 전달을 따로 관측한다.
    for mode in (0,1,2,8):
        for contents in ('-','64:255','8:5'):
            rows.append(('Attach',f'1,6,8,0,{fbits(0.63)},0,{fbits(40)},{fbits(250)},{fbits(40)},{fbits(250)},{mode}|{contents}'))
            rows.append(('Attach',f'1,1,8,0,0,1,{fbits(250)},{fbits(40)},{fbits(250)},{fbits(40)},{mode}|{contents}'))
    # 통지는 전체 사건 상위 비트/부모 일치/내용물 flags와 독립 수신값을 교차한다.
    for event in (0x02000031,0x82000031,0x0200003f,0x0200005f,0x0200005e,0x10000,0x80010000,0x02000003):
        for parent,flags,inside,cursor in ((50,8,1,0x41),(51,8,1,0x41),(50,64,0,0),(50,8,1,2)):
            rows.append(('Handle',f'{event},{parent},{flags},{inside},{cursor},1,2147483657,17'))
    for sid in (50,51):rows.append(('Gone',str(sid)))
    return rows


class Oracle(MenuOracle):
    """기본 생성/격자 계산은 실제 명령, 저장소·GUI 효과는 명시 입력 경계다."""
    def __init__(self):
        """현재 정밀 함수표의 실제 불연속 구간만 선택한다."""
        super().__init__();self.mu.mem_map(0,0x1000);self.allowed=[];self.hits=collections.Counter();self.boundaries=collections.Counter()
        # 끝 명령의 모든 바이트를 허용한다.
        for entry in ENTRIES:
            for part in self.facts[entry]['ranges'].split(';'):
                low,high=(int(word,16) for word in part.split('-'));self.allowed.append((low,high+1))

    def event(self,kind,*args):
        """현재 호출 인자만 보존하며 기대 분기는 계산하지 않는다."""
        self.events.append(','.join(map(str,(kind,*args))))

    def instruction(self,mu,address,size,data):
        """실제 계산을 실행하고 외부효과 인자/순서와 명시 응답만 기록한다."""
        self.recent.append(hex(address));args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,8);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x43e1e0 and self.op in ('Handle','Gone'):
            self.event('V',*args[:4]);self.ret(purge=16);return
        if any(low<=address and address+size<=high for low,high in self.allowed):
            self.instructions+=1
            if address in ENTRIES:self.hits[hex(address)]+=1
            return
        self.boundaries[hex(address)]+=1
        if address==0x464c60:self.event('Clear');self.ret()
        elif address==0x466750:self.event('BaseAttach',*args[:4]);self.ret(purge=16)
        elif address==0x4b2270:
            assert args[0]==50;self.iterators[ecx]=0;self.put(ecx+8,self.forms[0] if self.forms else 0);self.event('Begin',args[0]);self.ret(purge=4)
        elif address==0x4b1b20:
            self.iterators[ecx]+=1;index=self.iterators[ecx];self.put(ecx+8,self.forms[index] if index<len(self.forms) else 0);self.event('Next');self.ret()
        elif address==0x4e4391:
            assert args[0]==0xc0;token=TEXT+0x3000+len(self.items)*0x100;self.items.append(token);self.event('New',args[0],token);self.ret(token)
        elif address==0x43ce40:self.event('ItemAttach',ecx,*args[:4]);self.ret(purge=16)
        elif address==0x465330:
            assert args[0]==1;self.event('Idle',ecx,1);self.put(ecx+4,self.words(ecx+4,1)[0]|0x80);self.ret(purge=4)
        elif address==0x460d70:
            self.event('Now');mu.mem_write(args[0],struct.pack('<d',self.now));self.ret(args[0])
        elif address==0x4e0620:self.event('Assert',self.string(args[0]).hex());self.ret()
        elif address==0x20000100:self.event('Parent',100);self.ret(self.parent)
        elif address==0x464e90:self.event('Forward',*args[:3]);self.ret(self.forward,12)
        elif address==0x464560:self.event('Push');self.ret()
        elif address==0x4644e0:self.event('Bind',args[0]);self.put(ecx+36,args[0]);self.ret(purge=4)
        elif address==0x4650a0:
            self.event('Local',*args[1:4]);self.put(args[0],fbits(7.5));self.put(args[0]+4,fbits(-3.25));self.ret(purge=16)
        elif address==0x4248e0:self.event('Inside',*args[:2]);self.ret(self.inside,8)
        elif address==0x446e70:self.event('Cancel');self.ret()
        elif address==0x464990:self.event('BaseHandle',*args[:3]);self.ret(self.baseReturn,12)
        else:raise RuntimeError('생산 격자 미검토 명령 '+hex(address))

    def run(self,row,control):
        """raw 내용물과 노드/현재 입력을 준비하고 전체 정상 반환을 관측한다."""
        self.op,text=row;self.prepare([85,1,1,128,0,3,0,800,0,'-','-','-','-',0]);self.forms=[];self.items=[];self.iterators={};self.now=0.0;self.parent=50;self.forward=0;self.baseReturn=0;self.inside=0
        self.mu.mem_write(TEXT+0x1000,b'\xa5'*0x8000);self.mu.mem_write(0x563e38,bytes(4000));self.put(0x565dc8,0);self.put(0x540418,23);self.put(0,0xffffffff);self.put(0x5e5340,1);self.mu.reg_write(UC_X86_REG_MXCSR,0x1f80) # CRT의 명시 SSE2/기본 예외 마스크 입력이다.
        if self.op=='Grid':
            args=tuple(int(x)&0xffffffff for x in text.split(','));result,trace=self.invoke(0x43d790,args,NODE,12,control);assert result==NODE
        elif self.op=='Item':
            width,height,sid,flags,now=text.split(',');self.now=float(now);self.put(POOL+int(sid)*50+20,int(flags),2)
            result,trace=self.invoke(0x43e120,(int(width),int(height),int(sid)),NODE,12,control);assert result==NODE
        else:
            self.events=[];self.invoke(0x43d790,(2,6,8),NODE,12,control);self.events=[];self.put(NODE+36,50);self.put(NODE+76,fbits(1.25));self.put(NODE+80,fbits(-2.5))
            if self.op=='Attach':
                fields,contents=text.split('|');columns,minimum,mask,fixed,scale,click,bw,bh,width,height,mode=map(int,fields.split(','))
                # 선언한 원본 필드와 목록만 쓰며 열·행 결과는 공급하지 않는다.
                for offset,value in ((160,columns),(164,minimum),(176,mask),(188,fixed),(192,scale),(184,click),(60,bw),(64,bh),(68,width),(72,height)):self.put(NODE+offset,value)
                if contents!='-':
                    # 실제 slot BYTE와flags WORD는 별도의 Form에 둔다.
                    for index,content in enumerate(contents.split(';')):
                        flags,slot=map(int,content.split(':'));sid=100+index;self.forms.append(sid);self.put(POOL+sid*50+20,flags,2);self.put(POOL+sid*50+23,slot,1)
                _,trace=self.invoke(0x43e1e0,(fbits(5),fbits(6),0,mode),NODE,16,control);result='-'
            elif self.op=='Handle':
                event,self.parent,flags,self.inside,cursor,accepted,self.forward,self.baseReturn=map(int,text.split(','));self.put(POOL+100*50,TEXT+0xf000);self.put(TEXT+0xf000,0x20000100);self.put(POOL+100*50+20,flags,2);self.put(NODE+180,accepted);self.put(0x55a2ac,cursor);self.put(0x55a2f8,7);self.put(0x55a2fc,-3)
                result,trace=self.invoke(0x43d850,(event,100,9),NODE,12,control)
            else:
                _,trace=self.invoke(0x43d9c0,(int(text),),NODE,4,control);result='-'
        assert self.words(0,1)==(0xffffffff,)
        size=0xc0 if self.op=='Item' else 0xc4
        nodes=';'.join(str(token)+'='+bytes(self.mu.mem_read(token,0xc0)).hex() for token in self.items) or '-'
        return str(result),bytes(self.mu.mem_read(NODE,size)).hex(),nodes,trace


def generate():
    """두 정밀도 원본 실행이 같을 때만 새 기대 원문을 저장한다."""
    oracle=Oracle();rows=[]
    # 매 입력은 두 실행에서 동일한 명시 raw/전역을 새로 공급한다.
    for row in inputs():
        first=oracle.run(row,0x27f);second=oracle.run(row,0x37f);assert first==second,row;rows.append([*row,*first])
    FIXTURE.write_text('# 연산 명시입력 반환 전체노드 전체생성항목 호출순서\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_factory_menu.py',ROOT/'tools/decomp_priestmenu.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE]
    r=dict(host=socket.gethostname(),date='2026-10-18',unit=175,rows=len(rows),counts=dict(collections.Counter(row[0] for row in rows)),normalReturns=oracle.returns,instructions=oracle.instructions,controls=[0x27f,0x37f],crtCpuInput=dict(sse2=1,mxcsr=0x1f80),precisionDifferences=0,nativeCalls=dict(oracle.hits),boundaries=dict(oracle.boundaries),sha256={path.relative_to(ROOT).as_posix():digest(path) for path in paths},limits=['43d790/43e120/43d690/43e1e0/43d850/43d9c0 전체와 실제 기본 생성자·ceil/ftol을 실행한다. 목록 순회/할당·idle/시계/기본부착/자식 항목 부착/통지의 가상부착·Push/부모·좌표·범위/전달·기본처리는 명시 경계다.','10.78만 관측. 게임 창/OS 호출/보호 파일 변경/새 디컴파일 없음. 항목 아이콘/전체 생산 패널의 실제 Client 연결·미션 완주 증거는 아니다.'])
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('생산 격자 원본 관측',len(rows),oracle.returns,oracle.instructions)


def verify():
    """기대 원문을 재생성하지 않고 입력·출처 SHA·정상 반환을 검사한다."""
    r=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    # 기존 출력의 모든 원문 출처를 확인한다.
    for path,wanted in r['sha256'].items():assert digest(ROOT/path)==wanted,path
    assert len(rows)==r['rows']==len(inputs()) and not r['precisionDifferences'] and r['normalReturns']==(len(rows)+sum(row[0] in ('Attach','Handle','Gone') for row in rows))*2
    # 입력 순서와 전체 출력 열은 그대로 보존한다.
    for row,case in zip(rows,inputs()):assert row[:2]==list(case) and len(row)==6
    print('생산 격자 원본/SHA/입력 감사',len(rows))


# 명시한 옵션에서만 새 관측 또는 기존 독립 근거 감사를 실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
