#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 생산 내용물 항목의 부착·클릭·장식·비용 판정을 독립 x86으로 관측한다."""
import argparse
import collections
import json
import socket
import struct
from pathlib import Path
from decomp_inventory_image import Oracle as ImageOracle, ENTRIES as IMAGE_ENTRIES, ROOT, NODE, TEXT, TYPES, fbits, digest
from decomp_priestmenu import POOL
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

EXTRA=(0x43ce40,0x43d4c0,0x43cdc0,0x4af010,0x49aa30,0x4bb170,0x4756a0) # 실제 전체 항목/비용·검색·텍스트 생성 몸체다.
FIXTURE=ROOT/'cpppj/tests/fixtures/inventory-item-x86.tsv' # 새 항목 C++ 전에 동결할 원문이다.
REPORT=ROOT/'cpppj/inventory-item.json' # 실제 명령 범위와 입력 출처다.
BOUNDS=((0,0,64,96),(-10,-7,10,30),(1000,1000,0,0)) # 명시한 측정 경계 입력이며 배치 결과가 아니다.
CELLS=((),((7,0,0),),((7,-0.5,2.25),(8,1.5,-1))) # 명시한 decoder 경계의 프레임/좌표 입력이다.


def inputs():
    """목록/원본 플래그·배율·측정·입력 관문을 출력 계산 없이 교차한다."""
    rows=[]
    # 직접·선택·비활성·비용 불필요와 실제 수량 글자의 조건이다.
    for flags in (0,8,0x108,0x418):
        for scale in (0,fbits(0.63),fbits(2)):
            for profile in (0,1,2):
                rows.append(('Attach',f'128,{flags},3,3,-1,{fbits(40)},{fbits(250)},{scale},1,2,{profile},{fbits(799)},0,0,1'))
    # 패턴 없음/강제 프레임/빈 종류/품질signed코드/수량0과byte255를 관측한다.
    for kind,flags,quality,override,stock,cells in ((0,8,3,-1,1,1),(128,8,0,7,0,1),(128,8,255,7,1,0),(82,64,1,-1,0,2),(82,64,3,-1,0,2),(82,64,0,-1,0,2)):
        rows.append(('Attach',f'{kind},{flags},9,{quality},{override},{fbits(36)},{fbits(36)},0,{stock},{cells},1,{fbits(800)},0,0,1'))
    rows.append(('Attach',f'128,8,3,3,-1,{fbits(40)},{fbits(250)},0,1,1,0,{fbits(800)},0,1,0'))
    # 사건mask·상위signbit/열린지식·정지관문/실제자식hit·클릭조건·forward결과를 분리한다.
    for event in (0x10000,0x20000,0x80010000,0x82000107,0x02000107,0x02000003):
        for opened,gate,require,children,denied,forward,last in ((0,0,1,0,0,0,128),(1,0,1,1,0,0,128),(0,1,1,1,0,0,128),(0,0,0,0,0,17,128),(0,0,0,2,1,0,128),(0,0,1,3,0,0,128),(0,0,1,1,0,17,129)):
            rows.append(('Handle',f'{event},{opened},{gate},{require},{children},{denied},{forward},{last},0'))
    rows.append(('Handle','131072,0,0,1,1,0,0,128,1'))
    # 기본 크기·단정도 절삭과 장식0의 조기 반환이다.
    for flag in (0,1,255):
        for width,height in ((40,250),(19.75,-3.5)):
            rows.append(('Draw',f'{flag},{fbits(width)},{fbits(height)}'))
    # 비용은 원래encoded표를 공급한다. NaN SP의unordered도 독립 관측한다.
    for cost in (-1,0,800,16777217):
        for money in (fbits(-1),fbits(0),fbits(799),fbits(800),fbits(float('nan'))):
            for editor in (0,1):rows.append(('Afford',f'{cost},{money},{editor}'))
    return rows


class Oracle(ImageOracle):
    """실제 항목·그림·TextGump 생성은 실행하고 목록/측정·GUI 효과는 명시한다."""
    def __init__(self):
        """기존 실제 SHP Gump에 항목/비용/텍스트의 전체 함수 구간을 더한다."""
        super().__init__()
        for entry in EXTRA:
            for part in self.facts[entry]['ranges'].split(';'):
                low,high=(int(word,16) for word in part.split('-'));self.allowed.append((low,high+1))

    def cell(self,pointer):
        """명시한 현재 decoder 입력 한 칸만 실제 필드 위치에 쓴다."""
        cells=CELLS[self.cells];valid=self.cellIndex<len(cells);self.put(pointer+20,int(valid))
        if valid:
            frame,x,y=cells[self.cellIndex];self.put(pointer+16,frame);self.put(pointer+32,fbits(x));self.put(pointer+36,fbits(y))

    def instruction(self,mu,address,size,data):
        """항목 계산·이미지 생성·비용·검색은 원본 그대로 실행한다."""
        self.recent.append(hex(address));args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,9);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address in EXTRA or address in IMAGE_ENTRIES:self.hits[hex(address)]+=1
        if address==0x419cd0 and self.op=='Handle':self.event('ImageHit',ecx,*args[:2]);self.ret(int(ecx==TEXT+0x3200 or self.children!=3),8);return
        if address in (0x466750,0x419850,0x464740):
            if address==0x466750:self.event('Attach',ecx,*args[:4]);self.ret(purge=16)
            elif address==0x419850:self.event('Header',(ecx-TYPES)//500,*args[:2]);self.ret(TEXT+0xe000,8)
            else:self.event('Position');self.put(args[0],17);self.put(args[0]+4,-9);self.ret(args[0],4)
            return
        if any(low<=address and address+size<=high for low,high in self.allowed):self.instructions+=1;return
        self.boundaries[hex(address)]+=1
        if address==0x464c60:self.event('Clear');self.ret()
        elif address==0x4662b0:self.event('Cleanup');self.ret()
        elif address==0x4ac140:self.event('Robust',*args[:2]);self.ret(self.robust)
        elif address==0x4e0620:self.event('Assert',self.string(args[0]).hex());self.ret()
        elif address==0x43cbd0:
            self.event('Measure',*args[1:4],args[6]);mu.mem_write(args[0],struct.pack('<4f',*BOUNDS[self.bounds]));self.put(args[4],fbits(16));self.put(args[5],fbits(11));self.ret()
        elif address==0x425c20:self.event('Walk',*args[:6]);self.cellIndex=0;self.cell(ecx);self.ret(ecx,24)
        elif address==0x425860:self.event('Advance');self.cellIndex+=1;self.cell(ecx);self.ret()
        elif address==0x4e4391:
            token=TEXT+0x4000+len(self.born)*0x100;self.born.append((token,args[0]));self.event('New',args[0],token);self.ret(token)
        elif address==0x40ec50:self.event('Money',args[0]);self.put(args[1],self.money);self.ret()
        elif address==0x4a89c0:
            assert self.string(args[1])==b'~w~E%d';mu.mem_write(args[0],('~w~E'+str(args[2])).encode()+b'\0');self.ret(args[0])
        elif address==0x4bc800:self.event('Text',ecx,self.string(args[0]).hex());self.ret(purge=4)
        elif address==0x4bb310:self.event('Fit',ecx,*args[:2]);self.ret(purge=8)
        elif address==0x4650a0:self.event('Local',*args[1:4]);self.put(args[0],fbits(7.5));self.put(args[0]+4,fbits(-3.25));self.ret(purge=16)
        elif address==0x445330:
            value=self.opened if self.knowledgeCalls==0 else self.secondOpen;self.knowledgeCalls+=1;self.event('Knowledge',value);self.ret(value)
        elif address==0x4c2790:self.event('Denied',*args[:2]);self.ret(self.denied)
        elif address==0x464e90:self.event('Forward',*args[:3]);self.ret(self.forward,12)
        elif address==0x4cf960:self.event('Tell',self.string(args[0]).hex());self.ret()
        elif address==0x43d330:self.event('About',*args[:2]);self.ret(0x80000009,8)
        elif address==0x465690:self.event('Background',*args[:7]);self.ret()
        else:raise RuntimeError('생산 항목 미검토 명령 '+hex(address))

    def run(self,row,control):
        """명시 raw/TYPE/자식·측정 입력만 공급하고 전체 정상 결과를 읽는다."""
        self.op,text=row;v=list(map(int,text.split(',')));self.prepare([85,1,1,128,0,3,0,800,0,'-','-','-','-',0]);self.mu.mem_write(TEXT+0x1000,b'\xa5'*0xb000);self.put(0,0xffffffff);self.mu.mem_write(0x563e38,bytes(4000));self.put(0x565dc8,0);self.put(0x540418,23);self.events=[];self.born=[];self.money=fbits(800);self.robust=1;self.opened=0;self.knowledgeCalls=0;self.secondOpen=0
        self.put(NODE,0x5068f4);self.put(NODE+4,9);self.put(NODE+24,0);self.put(NODE+160,128);self.put(NODE+44,41);self.put(POOL+128*50+18,128,2);self.put(0x59ab20,TYPES);self.put(0x540c70,1);self.put(0x5c85a4,0);self.put(0x55b4b0,0);self.put(0x557d6c,0);self.put(0x5ca920,96);self.put(0x5ca924,2)
        # 현재 헤더와 TYPE/프레임 코드의 실제 입력이다.
        for kind in (0,82,128):
            self.put(TYPES+kind*500+0xdc,0x16000000);self.put(TYPES+kind*500+0x1dc,2);self.put(TYPES+kind*500+0x1e0,3);self.put(TYPES+kind*500+0x124,TEXT+0xd000);self.put(TYPES+kind*500+0x114,16);self.put(0x59ab50+kind*4,800+kind*23)
        self.mu.mem_write(TEXT+0xd000,b'AP\0\0'*16);self.mu.mem_write(TEXT+0xd000+7*4,b'AP\0\0');self.mu.mem_write(TEXT+0xd000+8*4,b'BP\0\0');self.mu.mem_write(TEXT+0xd000+9*4,b'AP\0\x20');self.mu.mem_write(TEXT+0xd000+10*4,b'BP\0\x20');self.mu.mem_write(TEXT+0xd000+11*4,b'AP\0\x40');self.mu.mem_write(TEXT+0xd000+12*4,b'BP\0\x40')
        for off,value in ((24,40),(26,60),(28,12),(30,-7)):self.put(TEXT+0xe000+off,value,2)
        if self.op=='Attach':
            kind,flags,quantity,quality,override,width,height,scale,stock,self.cells,self.bounds,self.money,editor,dead,self.robust=v;self.put(POOL+128*50+18,kind,2);self.put(POOL+128*50+20,flags,2);self.put(POOL+128*50+12,quantity,2);self.put(POOL+128*50+30,quality,1);self.put(TYPES+kind*500+0x11c,override);self.put(NODE+68,width);self.put(NODE+72,height);self.put(NODE+188,scale);self.put(NODE+4,9|(0x10000 if dead else 0));self.put(0x5318f4,stock);self.put(0x5c85a4,editor);self.put(0x59a92c,fbits(16));self.put(0x59a930,fbits(11));self.put(0x5411a0,82)
            _,trace=self.invoke(0x43ce40,(fbits(5),fbits(6),0,7),NODE,16,control);result='-'
        elif self.op=='Handle':
            event,self.opened,gate,require,self.children,self.denied,self.forward,last,self.secondOpen=v;self.put(0x55b4b0,gate);self.put(NODE+180,require);self.put(0x557d6c,last)
            if self.children:
                tokens=(TEXT+0x3000,TEXT+0x3100,TEXT+0x3200) if self.children==3 else (TEXT+0x3000,);self.put(NODE+24,tokens[0])
                for index,token in enumerate(tokens):self.put(token+12,tokens[index+1] if index+1<len(tokens) else 0);self.put(token+32,3 if self.children!=2 and index!=0 or self.children==1 else 45)
            result,trace=self.invoke(0x43d4c0,(event,17,0xfffffff7),NODE,12,control);result=str(result)
        elif self.op=='Draw':
            flag,width,height=v;self.put(NODE+177,flag,1);self.put(NODE+60,width);self.put(NODE+64,height);_,trace=self.invoke(0x43cdc0,(),NODE,0,control);result='-'
        else:
            cost,self.money,editor=v;self.put(0x59ab50+128*4,cost+128*23);self.put(0x5c85a4,editor);result,trace=self.invoke(0x4af010,(),POOL+128*50,0,control);result=str(result)
        assert self.words(0,1)==(0xffffffff,)
        nodes=';'.join(str(token)+'='+bytes(self.mu.mem_read(token,size)).hex() for token,size in self.born) or '-'
        return result,bytes(self.mu.mem_read(NODE,0xc0)).hex(),str(self.words(0x557d6c,1)[0]),nodes,trace


def generate():
    """두 실제 원본 정밀도 결과가 일치할 때만 독립 기대 원문을 저장한다."""
    oracle=Oracle();rows=[]
    for row in inputs():
        first=oracle.run(row,0x27f);second=oracle.run(row,0x37f);assert first==second,row;rows.append([*row,*first])
    FIXTURE.write_text('# 연산 명시입력 반환 전체항목 lastType 전체생성노드 호출순서\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_inventory_image.py',ROOT/'tools/decomp_factory_menu.py',ROOT/'tools/decomp_priestmenu.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE]
    r=dict(host=socket.gethostname(),date='2026-10-18',unit=177,rows=len(rows),counts=dict(collections.Counter(row[0] for row in rows)),normalReturns=oracle.returns,instructions=oracle.instructions,controls=[0x27f,0x37f],precisionDifferences=0,nativeCalls=dict(oracle.hits),boundaries=dict(oracle.boundaries),sha256={path.relative_to(ROOT).as_posix():digest(path) for path in paths},limits=['43ce40/43d4c0/43cdc0/4af010과 실제 이미지·기본·TextGump 생성/프레임 검색·TYPE/CRT를 실행한다. 측정·decoder 입력/부착·수명·검사·SP/문자열 작성·맞춤/지식·정책·전달·적중·About/장식은 명시 경계다.','게임/OS/창 실행·보호 파일 변경/새 디컴파일 없음. 실제 Client 생산 패널과 자연 플레이 완주의 증거는 아니다.'])
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('생산 항목 원본 관측',len(rows),oracle.returns,oracle.instructions)


def verify():
    """기대 출력을 재생성하지 않고 원문/입력/정상 반환을 확인한다."""
    r=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    for path,wanted in r['sha256'].items():assert digest(ROOT/path)==wanted,path
    assert len(rows)==r['rows']==len(inputs()) and r['normalReturns']==len(rows)*2 and not r['precisionDifferences']
    for row,case in zip(rows,inputs()):assert row[:2]==list(case) and len(row)==7
    print('생산 항목 원본/SHA/입력 감사',len(rows))


# 새 관측 또는 기존 근거 감사를 명시 옵션으로 선택한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
