#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""아이스147의 파괴와 성장형150의 전체 Pop/PostDestroy/주기를 보호PE에서 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import pefile
from decomp_arc_spire import Oracle as BaseOracle,ROOT,STACK,STOP,POOL,TYPES,VTABLE,IMAGE,HEAP,OUTPUT,WRAPPER,bits
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX

ENTRIES=(0x48ead0,0x48eb80,0x48ebe0,0x48edf0,0x4ac550,0x49a840,0x4ac220,0x496e70)  # 성장/재생성/WORD와 순수 현재TYPE/getter 전체다.
UNPOP,DESTROY,OWNER,POP=(STOP+0x600+index*0x20 for index in range(4))  # 서로 다른 가상 효과 경계다.
IDS=(50,60,61)  # 부모/표면/새객체 전체50바이트를 보존한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/ice-tower-x86.tsv'  # 기존 기대값을 건드리지 않는 최초 관측이다.
REPORT=ROOT/'cpppj/ice-tower-20261023.json'  # PC/원문/정밀도/명령/호출 경계의 근거다.


def cases():
    """각 관문/예약실패/현재프레임 감김과 하위 효과 뒤의 현재값을 교차한다."""
    result=[]
    for flags,extra,total,allocation in itertools.product((0,1,2,3,0x50),(0,1,8,9,32),(0,1,7,42),(0,1)):result.append(('P',(flags,extra,total,allocation,0)))  # extra/편집 관문을 임의로 추가하지 않는다.
    for event,count,extra,total,boss,frame in itertools.product((0,0x3a6,0x3a7),(0,0xffffffff),(0,32),(0,1,7),(0,1),(0,1,5,6,7,0x7fffffff,0xffffffff)):
        result.append(('R',(event,count,extra,total,boss,frame,bits(.75),0)))  # event밖은원래payload다.
    for flags,extra,kind,loading,battle,surface in itertools.product((0,0x200000,0x8000000,0x8200000,0x400048),(0,1,8,9,32),(147,78),(0,1),(0,1),(0,157,95)):
        result.append(('D',(flags,extra,kind,loading,battle,surface,0)))  # 섬정리와재성장관문을독립관측한다.
    for flags,extra,found in itertools.product((0,0x200000,0x8000000,0x8200000,0x400048),(0,1,8,9,32),(0,1)):result.append(('G',(flags,extra,found,0)))  # 성장형은공통Post만호출한다.
    for mutation in range(1,13):
        result.extend((('P',(1,0,7,1,mutation)),('R',(0x3a6,0,0,7,1,0,bits(.75),mutation)),('R',(0x3a6,0,0,7,1,5,bits(.75),mutation)),('D',(0,0,147,0,1,157,mutation)),('D',(0x8000000,0,147,0,1,95,mutation)),('G',(0x8000000,0,1,mutation))))  # 콜백이공급한상태뒤실제원문의재조회를관측한다.
    return list(dict.fromkeys(result))


class Oracle(BaseOracle):
    """전체원문몸체를실행하고큰세계의생성/공간/표시효과만분리한다."""
    def __init__(self):
        """보호 이미지와 분리 힙/풀을 빌리고 float반환 wrapper만 설치한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES)
        code=b''.join(b'\xff\x35'+struct.pack('<I',OUTPUT+0x20+index*4) for index in (2,1,0));at=WRAPPER+len(code);code+=b'\xe8'+struct.pack('<i',0x48ebe0-at-5)+b'\xd9\x1d'+struct.pack('<I',OUTPUT)+b'\xc3';self.mu.mem_write(WRAPPER,code)

    def position(self,x,y):
        """경계가 공급하는 현재 부모 좌표 변경만 적용한다."""
        self.put(POOL+50*50+14,bits(x));self.put(POOL+50*50+18,bits(y))

    def instruction(self,mu,address,size,data):
        """실제 프레임/관문/순서/재조회를 실행하고 외부 요청 인수만 기록한다."""
        self.instructions+=1
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        boundaries=(0x4e4391,0x496e80,0x4b0d30,0x44b9e0,0x4b0840,0x40eaf0,0x4b2070,0x4af530,0x4214a0,0x4acee0,0x46e6c0,UNPOP,DESTROY,OWNER,POP)
        if address not in boundaries:
            if WRAPPER<=address<WRAPPER+40:return
            if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'아이스 미검토 명령 {address:x}')
            return
        sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);a=[self.get(sp+4+index*4) for index in range(4)];sid=(ecx-POOL)//50
        if address==0x4e4391:
            self.event('N',a[0])
            if self.mutation==1:self.put(POOL+50*50+10,151,1)
            self.ret(HEAP if self.allocation else 0);return
        if address==0x496e80:
            self.event('R',*a[:3])
            if self.mutation==10:self.position(61,62)
            self.ret(ecx,12);return
        if address==0x4b0d30:self.event('P',a[0]);self.ret(0,4);return
        if address==0x44b9e0:self.event('D',a[0]);self.ret(0,4);return
        if address==0x4b0840:self.event('B',a[0]);self.ret(0,4);return
        if address==0x46e6c0:self.event('J',*a);self.ret();return
        if address==0x40eaf0:
            self.event('Q',*a[:2])
            if self.mutation==6:self.put(0x540bc0,0);self.position(30.5,31.25);self.put(POOL+50*50+34,4,1)
            if self.mutation==8:self.put(0x5412cc,95)
            self.ret(60 if self.surface else 0);return
        if address==0x4b2070:
            self.event('G',*a[:3])
            if self.mutation==11:self.position(50,51);self.put(POOL+50*50+34,4,1)
            self.ret(60 if self.found else 0);return
        if address==0x4af530:
            self.event('C',*a[:2]);raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE+0x100);raw[10]=82 if self.mutation==7 else a[0];raw[11]=4;raw[35]=0x99;struct.pack_into('<H',raw,12,0x7788);struct.pack_into('<I',raw,36,4);self.mu.mem_write(POOL+61*50,bytes(raw))
            if self.mutation==2:self.put(POOL+50*50+34,3,1);self.position(11.5,12.25);self.put(POOL+50*50+12,0x3344,2);self.put(POOL+50*50+35,77,1)
            if self.mutation==12:self.put(POOL+50*50+40,32,1)
            self.ret(POOL+61*50);return
        if address==UNPOP:
            self.event('U',sid,a[0]);self.put(ecx+11,(self.get(ecx+11)&255)|4,1)
            if self.mutation==5:self.put(0x540bc0,0)
            self.ret(0,4);return
        if address==DESTROY:
            self.event('K',sid,a[0]);self.put(ecx+11,(self.get(ecx+11)&255)|3,1)
            if self.mutation==4:self.position(90,91)
            self.ret(0,4);return
        if address==OWNER:
            self.event('O',sid,a[0]);self.put(ecx+34,a[0],1)
            if self.mutation==3:self.position(13.5,14.25);self.put(POOL+50*50+35,78,1)
            self.ret(0,4);return
        if address==POP:self.event('V',sid,*a[:3]);self.put(ecx+14,a[0]);self.put(ecx+18,a[1]);self.put(ecx+11,(self.get(ecx+11)&255)&~4,1);self.ret(0,12);return
        if address==0x4214a0:
            self.event('T',a[0]);self.put(POOL+50*50+35,88,1);self.put(POOL+50*50+34,4,1);self.position(19.5,20.5);self.ret();return
        if address==0x4acee0:
            self.event('F',*a[:2]);self.put(ecx+36,a[0])
            if self.mutation==9:self.put(ecx+10,151,1)
            self.ret(0,8);return

    def sample(self,case,precision):
        """같은 세 raw 슬롯과 전역 입력을 설치하고 모든 효과/반환을 동결한다."""
        self.kind,v=case;self.events=[];self.mutation=v[-1];self.surface=0;self.found=0;self.allocation=1;kind=150;extra=0;total=7;loading=0;battle=boss=1;frame=4
        if self.kind=='P':flags,extra,total,self.allocation,_=v
        elif self.kind=='R':event,count,extra,total,boss,frame,payload,_=v
        elif self.kind=='D':flags,extra,kind,loading,battle,self.surface,_=v
        else:flags,extra,self.found,_=v
        self.mu.mem_write(IMAGE,bytes(4096));self.mu.mem_write(TYPES,bytes(0x20000))
        for sid in IDS:  # 생성자 경계의 초기값과부모의WORD/QA를분리한다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE+(0x100 if sid==61 else 0));raw[10]=kind if sid==50 else self.surface if sid==60 else 0;raw[11]=6 if sid==50 and self.kind in ('D','G') else 1 if sid==61 else 0;raw[34]=2 if sid!=61 else 0
            if sid==50:struct.pack_into('<H',raw,12,0xa55a);struct.pack_into('<2f',raw,14,20.25,21.5);raw[35]=0x44;raw[40]=extra;struct.pack_into('<I',raw,36,frame)
            self.mu.mem_write(POOL+sid*50,bytes(raw))
        for table in (VTABLE,VTABLE+0x100):
            for offset,value in ((0x48,UNPOP),(0x10,DESTROY),(0x74,OWNER),(0x90,POP)):self.put(table+offset,value)  # 반환SID를명시한실제가상효과경계다.
        for address,value in ((0x5c8464,POOL),(0x5c847c,128),(0x59ab20,TYPES),(0x541348,188),(0x5412a4,147),(0x5412b0,150),(0x5412cc,157),(0x5411a0,82),(0x540bc0,battle),(0x540bc4,boss),(0x5c89b8,loading),(0,0)):self.put(address,value)  # 원문의현재전역입력이다.
        self.put(TYPES+150*500+0x114,total);self.put(TYPES+151*500+0x114,9)
        receiver=POOL+50*50;purge=4
        if self.kind=='R':
            entry=WRAPPER;args=b'';purge=0
            for index,value in enumerate((event,count,payload)):self.put(OUTPUT+0x20+index*4,value)  # 고정코드가현재데이터인수를읽도록한다.
        else:entry={'P':0x48ead0,'D':0x48edf0,'G':0x48eb80}[self.kind];args=struct.pack('<I',flags)
        self.call(entry,receiver,args,purge,precision);self.returns+=1
        return ';'.join(self.events) or '-', ''.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in IDS),str(self.get(OUTPUT) if self.kind=='R' else 0)


def main():
    """새 관측만 동결하며 확인모드는 원문/제품 SHA를 검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**report['sha256'],**report.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 원본/제품 바이트가 바뀌면확인을실패시킨다.
        rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==report['uniqueInputs'] and len({tuple(row[:2]) for row in rows})==len(rows)  # 고유입력과최초동결행수를함께검사한다.
        print('아이스 원문 SHA:',report['uniqueInputs']);return
    inputs=cases();inputs=[('P',(1,0,7,1,0)),('D',(0,0,147,0,1,157,0)),('R',(0x3a6,0,0,7,1,5,bits(.75),0)),('G',(0x8000000,0,1,0))] if args.smoke else inputs
    oracles=(Oracle(),Oracle());rows=[]
    for case in inputs:  # 같은입력을두x87정밀도로실행해반환/효과/슬롯전체를대조한다.
        first,second=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))];assert first==second,case;rows.append((case[0],','.join(map(str,case[1])),*first))
    if args.smoke:print('아이스 네몸체 smoke 정상반환');return
    FIXTURE.write_text('# kind input effects raws3 result\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_ice_tower.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),nativeInstructions=sum(o.instructions for o in oracles),precisionDifferentInputs=[],nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),sha256={p:digest(ROOT/p) for p in paths},limitations=['48ead0/48eb80/48ebe0/48edf0과현재TYPE/WORD/일반Regular반환의전체몸체를실행한다.','큰생성자/가상Unpop·소유자·Pop·삭제/표면·genus조회/프레임표시/받침정리/공통PostPop·PostDestroy/Regular생성은명시경계다. 기존fixture는바꾸지않는다.'])
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));report['vtableEntries']={f'{base:08x}':{f'{off:02x}':f'{int.from_bytes(pe.get_data(base-0x400000+off,4),"little"):08x}' for off in (0x10,0x14,0x18,0x20,0x5c,0x78,0x98,0xa0,0xa4)} for base in (0x50e940,0x50ea08,0x50ead0,0x50e888)}  # 실제공유수명/캡처주소와피해차이를함께보존한다.
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('아이스:',len(rows),'입력/두정밀도차이0')


if __name__=='__main__':main()
