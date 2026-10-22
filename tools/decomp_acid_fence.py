#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""애시드145의 후처리/주기/+X·+Y 공격/13영역 포획과 로컬 수신을 격리 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_arc_spire import Oracle as BaseOracle, ROOT, STACK, STOP, POOL, TYPES, VTABLE, IMAGE, HEAP, SPOTS, OUTPUT, WRAPPER, DAMAGE, bits, floating
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX,UC_X86_REG_EIP

ENTRIES=(0x48c7a0,0x48ea30,0x48c830,0x48ca70,0x48c300,0x40d890,0x4ac550,0x49a840,0x4ac200,0x41d770,0x41d7a0,0x4ab7c0,0x4e49c0,0x40ad30,0x4df230,0x48bfe0,0x4abaf0)  # 전체 제어/산술/필터/헤더/수신 몸체다.
DESTROY=STOP+0x540  # 실제 가상10의 명시 삭제 경계다.
IDS=(50,60,61,*range(70,114))  # 원문이 변경할 수 있는47개 슬롯을 전부 보존한다.
KINDS=(133,144,144,73,73,138,138,144,138,73,146,141,111)  # 13영역의 독립 후보 타입 입력이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/acid-fence-x86.tsv'  # 기존 자료와 분리한 최초 전체 관측이다.
REPORT=ROOT/'cpppj/acid-fence-20261023.json'  # 현재 PC/원문/정밀도/호출 경계의 근거다.


def cases():
    """예약/감김·범위·전체13영역/포화/필터와 효과 뒤 재조회를 교차한다."""
    rows=[]
    for flags,extra,allocation in itertools.product((0,1,2,3,0x50),(0,1,8,9,32),(0,1)):rows.append(('P',(flags,extra,allocation)))  # 최초 예약과 실패다.
    for event,count,extra,total in itertools.product((0,1,2),(0,1,5,6,7,0x7fffffff,0xffffffff),(0,32),(1,7)):rows.append(('R',(event,count,extra,total,bits(.3))))  # 공통Regular에위임하지않는다.
    for radius,mask,mode in itertools.product((0.,1.,4.,6.75,12.),(0,1,2,3),(0,4)):rows.append(('A',(bits(radius),bits(10),bits(10),mask,mode,1,0,0,0,3,4)))  # 첫동맹/끝제외/양축이다.
    for x,y in ((10.25,10.),(10.,10.75),(253.5,253.5)):rows.append(('A',(bits(12),bits(x),bits(y),3,4,1,0,0,0,3,4)))  # 정수이웃과254clamp다.
    base=(1,1,4,1,0,0,0,bits(10),bits(10),bits(14),bits(15),3,4,0x50100,133,0,0,2,1)
    for count,orientation,mode,mutation in itertools.product((0,1,2),(0,1),(0,1,2,3,4,5),range(9)):
        row=list(base);row[:3]=count,orientation,mode;row[6]=mutation;rows.append(('S',tuple(row)))  # 중복 순서/포화와 현재 효과를 비교한다.
    for kind,genus in itertools.product((133,73,144,138,146,141,111),(0,0x100,0x10000,0x40000,0x50100,0x100000,0x20000,0x200,0x100200,0x210000)):
        for orientation in (0,1):  # 특정TYPE와genus조건을별도로관측한다.
            row=list(base);row[1:3]=orientation,6;row[13:15]=genus,kind;rows.append(('S',tuple(row)))
    for extra,flags,owner,boss,editor,friendly in itertools.product((0,1,2,8,32),(0,1,2,3,4),(1,2),(0,1),(0,1),(0,1)):
        row=list(base);row[2:6]=6,boss,editor,friendly;row[15:18]=flags,extra,owner;rows.append(('S',tuple(row)))  # 후보수집과직전생존필터를구분한다.
    for x,y,bx,by,fx,fy,orientation in itertools.product((10.000005,10.00002,10.00005,10.5,-.25),(10.00002,), (14.000005,), (15.00002,), (1,3),(1,4),(0,1)):
        row=list(base);row[2]=0;row[7:13]=bits(x),bits(y),bits(bx),bits(by),fx,fy;row[1]=orientation;rows.append(('S',tuple(row)))  # float저장과502310경계다.
    for radius,mask,mode in itertools.product((0.,1.,4.,6.75,12.),(0,1,2,3),(0,4)):rows.append(('B',(bits(radius),bits(10),bits(10),mask,mode,1,0,0,0,3,4)))  # 정확히정렬된두이웃의추가전체공격이다.
    for x,y in ((10.25,10.),(10.,10.75),(253.5,253.5)):rows.append(('B',(bits(12),bits(x),bits(y),3,4,1,0,0,0,3,4)))  # 정수이웃과소수부모를별도로비교한다.
    for orientation in (0,1):
        row=list(base);row[1]=orientation;row[6]=9;rows.append(('C',tuple(row)))  # finder가비행체TYPE전역을0으로바꾼뒤현재값을검사한다.
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """실제13영역·후보필터·패킷·수신을 실행하고 큰 세계/장치 효과만 분리한다."""
    def __init__(self):
        """같은 보호 이미지/분리 힙/지도와 새 함수 범위를 설치한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES)
        code=b''.join(b'\xff\x35'+struct.pack('<I',OUTPUT+0x20+index*4) for index in (2,1,0));at=WRAPPER+len(code);code+=b'\xe8'+struct.pack('<i',0x48ea30-at-5)+b'\xd9\x1d'+struct.pack('<I',OUTPUT)+b'\xc3';self.mu.mem_write(WRAPPER,code)

    def instruction(self,mu,address,size,data):
        """정상반환/부수효과를 기록하며 필터/산술/중복·생존 재조회는 대체하지 않는다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);a=[self.get(sp+4+index*4) for index in range(4)]
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x4e4391:
            if a[0]==40:
                self.event('N',40)
                if self.mutation==8:self.put(0x540bc4,0)
            result=HEAP+self.allocated if a[0]!=40 or self.allocation else 0;self.allocated+=(a[0]+15)&~15;self.ret(result);return
        if address==0x496e80:self.event('R',a[0],a[1],a[2]);self.ret(ecx,12);return
        if address==0x44bf80:self.event('D',a[0]);self.ret(0,4);return
        if address==0x4acee0:self.event('B',a[0],a[1]);self.put(ecx+36,a[0]);self.ret(0,8);return
        if address==0x4b1fa0:
            self.event('F',*a[:3]);sid=60 if self.mask&1 and floating(a[0])==int(self.x)+4 and floating(a[1])==int(self.y) else 61 if self.mask&2 and floating(a[0])==int(self.x) and floating(a[1])==int(self.y)+5 else 0;self.ret(sid);return
        if address==0x4b16d0:
            phase=self.scans%13;self.scans+=1;self.event('I',*a);self.index=0;self.victims=() if self.mode==0 else (70,) if self.mode in (1,6) else (50,60,61,70,71,72,73,74,75) if self.mode==2 else tuple(range(70,114)) if self.mode==3 else (70+phase,) if self.mode==4 else (70,70,70,71,72)
            self.put(ecx+0x34,self.victims[0] if self.victims else 0)
            if self.mutation==4 and self.scans==1:self.put(POOL+50*50+14,bits(11.25));self.put(POOL+50*50+18,bits(12.5))
            if self.mutation==5 and self.scans==13:self.put(POOL+70*50+10,158,1)
            if self.mutation==9 and self.scans==1:self.put(0x541298,0)
            self.ret(ecx,16);return
        if address==0x4b1810:self.index+=1;self.put(ecx+0x34,self.victims[self.index] if self.index<len(self.victims) else 0);self.ret();return
        if address==0x4e2e30:
            packet=a[0];self.events.append('S:'+bytes(mu.mem_read(packet,11)).hex());victim=self.get(packet+4)&65535
            if self.mutation==1:self.put(POOL+victim*50+11,1,1)
            if self.mutation==2:self.put(POOL+50*50+34,3,1)
            if self.mutation==7:self.put(packet+4,71,2)
            self.ret();return
        if address==0x40da40:self.ret(1);return
        if address==0x4a9d70:self.events.append('W:'+':'.join(map(str,a[:2]))+':'+self.string(a[2])+':'+str(a[3]));self.ret();return
        if address==0x450510:self.event('X',*a);self.ret(1);return
        if address==DESTROY:
            sid=(ecx-POOL)//50;self.event('K',sid,a[0]);self.put(ecx+11,(self.get(ecx+11)&255)|2,1)
            if self.mutation==3:self.put(POOL+50*50+14,bits(11.5));self.put(POOL+50*50+18,bits(12.25))
            self.ret(0,4);return
        if address==DAMAGE:
            sid=(ecx-POOL)//50;self.event('H',sid,*a[:3])
            if self.mutation==6:self.put(ecx+11,2,1);self.put(POOL+50*50+34,3,1)
            self.ret(0,12);return
        if address==0x4e4354:self.ret();return
        if address==0x4e0620:self.event('!',a[2]);self.ret();return
        if WRAPPER<=address<WRAPPER+40:return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'애시드 미검토 실행 {address:x}')

    def write(self,mu,access,address,size,value,data):
        """원본의 동일 풀 주소 재저장 외에는 기존 분리 쓰기 가드를 유지한다."""
        if address==0x5c8464 and size==4 and value==POOL:return
        super().write(mu,access,address,size,value,data)

    def sample(self,case,precision):
        """현재 독립 입력을 설치하고47슬롯/모든 외부 효과/반환을 동결한다."""
        self.kind,v=case;self.events=[];self.allocated=0;self.scans=0;self.index=0;self.x=self.y=10.;bx,by=14.,15.;radius=6.;self.mask=3;self.mode=0;boss=1;editor=friendly=self.mutation=0;self.allocation=1;fx,fy=3,4;total=7
        if self.kind=='P':self.allocation=v[2]
        elif self.kind in ('A','B'):radius=floating(v[0]);self.x,self.y=map(floating,v[1:3]);self.mask,self.mode,boss,editor,friendly,self.mutation,fx,fy=v[3:]
        elif self.kind in ('S','C'):self.mode,boss,editor,friendly,self.mutation=v[2:7];self.x,self.y,bx,by=map(floating,v[7:11]);fx,fy=v[11:13];self.allocation=v[18]
        self.mu.mem_write(SPOTS,bytes([1])*65536);self.mu.mem_write(HEAP,b'\xa5'*65536);self.mu.mem_write(IMAGE,bytes(4096));self.mu.mem_write(TYPES,bytes(0x20000))
        for typ,genus in ((133,0x50100),(158,0x210000),(73,0x100000),(144,0x100000),(138,0x100000),(146,0x20000),(141,0x20000),(111,0x20000)):self.put(TYPES+typ*500+0xec,genus)  # 각군을독립메타입력으로공급한다.
        for sid in IDS:  # 부모/두폴/후보44개의원래50바이트다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=145 if sid<70 else 133;raw[34]=1 if sid<70 else 2;struct.pack_into('<2f',raw,14,bx if sid==60 else self.x,by if sid==60 else self.y+5 if sid==61 else self.y);self.mu.mem_write(POOL+sid*50,bytes(raw))
        if self.kind=='B':
            self.put(POOL+60*50+14,bits(int(self.x)+4));self.put(POOL+60*50+18,bits(int(self.y)));self.put(POOL+61*50+14,bits(int(self.x)));self.put(POOL+61*50+18,bits(int(self.y)+5))  # 기존1080행은바꾸지않고정렬입력만추가한다.
        if self.mode in (2,5):self.put(POOL+71*50+10,158,1);self.put(POOL+72*50+10,138,1);self.put(POOL+73*50+34,1,1);self.put(POOL+74*50+40,1,1);self.put(POOL+75*50+10,90,1);self.put(TYPES+90*500+0xec,0x200)
        if self.mode==4:
            for phase,typ in enumerate(KINDS):self.put(POOL+(70+phase)*50+10,typ,1)  # 각영역은독립적인기지정TYPE를반환한다.
        if self.kind=='S' and self.mode==6:self.put(TYPES+v[14]*500+0xec,v[13]);self.put(POOL+70*50+10,v[14],1);self.put(POOL+70*50+11,v[15],1);self.put(POOL+70*50+40,v[16],1);self.put(POOL+70*50+34,v[17],1)
        self.mu.mem_write(0x595200,bytes(81*4))
        for a,b in ((1,1),(2,2),(3,3),(1,3),(3,1)):self.put(0x595200+(a*9+b)*4,1)  # 역방향동맹도독립값이다.
        self.put(VTABLE+0x98,DAMAGE);self.put(VTABLE+0x10,DESTROY);self.put(VTABLE+0xc8,0x48c830)
        for address,value in ((0x5c8464,POOL),(0x5c847c,128),(0x59ab20,TYPES),(0x541348,188),(0x5c7c44,SPOTS),(0x5c85a4,editor),(0x540bc4,boss),(0x55a490,friendly),(0x5412e8,164),(0x5412b0,150),(0x541298,144),(0x54117c,73),(0x541280,138),(0x5412a0,146),(0x54128c,141),(0x541214,111),(0,0)):self.put(address,value)  # 모든현재전역은명시입력이다.
        self.put(TYPES+145*500+0x44,bits(radius));self.put(TYPES+145*500+0x114,total);self.put(TYPES+145*500+0x1d4,fx);self.put(TYPES+145*500+0x1d8,fy);self.put(0x52ee42,0x61,1);self.put(0x52ee43,1,1);self.put(0x52ee44,11,2)
        receiver=POOL+2500;args=b'';purge=0
        if self.kind=='P':self.put(receiver+40,v[1],1);entry=0x48c7a0;args=struct.pack('<I',v[0]);purge=4
        elif self.kind=='R':
            self.put(receiver+40,v[2],1);self.put(TYPES+145*500+0x114,v[3]);entry=WRAPPER
            for index,value in enumerate((v[0],v[1],v[4])):self.put(OUTPUT+0x20+index*4,value)  # 고정코드가현재실행인수를읽는다.
        elif self.kind in ('A','B'):entry=0x48c830
        else:self.put(IMAGE,HEAP);self.put(IMAGE+4,20);self.put(IMAGE+8,v[0]);self.put(HEAP,60);self.put(HEAP+4,61);self.allocated=0x100;entry=0x48ca70;args=struct.pack('<II',IMAGE,v[1]&0xffffffff);purge=8
        self.call(entry,receiver,args,purge,precision);self.returns+=1
        return ';'.join(self.events) or '-', ''.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in IDS),str(self.get(OUTPUT) if self.kind=='R' else 0)


def main():
    """새 입력을 두 정밀도로 동결하거나 원문/제품 SHA만 검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');parser.add_argument('--extend-axes',action='store_true');args=parser.parse_args()
    if args.verify:
        r=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**r['sha256'],**r.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 동일한검증바이트만확인한다.
        rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==r['uniqueInputs'] and len({tuple(row[:2]) for row in rows})==len(rows);print('애시드13영역 원문 SHA:',len(rows));return
    inputs=cases();inputs=[inputs[0],('R',(0,0,0,7,bits(.3))),('S',(1,1,4,1,0,0,0,bits(10),bits(10),bits(14),bits(15),3,4,0x50100,133,0,0,2,1))] if args.smoke else inputs
    previous=None;rows=[]
    if args.extend_axes:
        previous=json.loads(REPORT.read_text(encoding='utf-8'));assert digest(FIXTURE)==previous['sha256'][FIXTURE.relative_to(ROOT).as_posix()]
        rows=[tuple(line.split('\t')) for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==previous['uniqueInputs'];seen={(row[0],row[1]) for row in rows};inputs=[case for case in inputs if (case[0],','.join(map(str,case[1]))) not in seen]  # 먼저동결한행은그대로두고새입력만이어붙인다.
    oracles=(Oracle(),Oracle())
    for case in inputs:  # 정밀도 반복은 고유 입력 수와 구분한다.
        first,second=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))];assert first==second,case;rows.append((case[0],','.join(map(str,case[1])),*first))
    if args.smoke:print('애시드 전체/13영역/수신 smoke 정상반환');return
    FIXTURE.write_text('# kind input effects raws47 result\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_acid_fence.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),nativeInstructions=sum(o.instructions for o in oracles),precisionDifferentInputs=[],nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),sha256={p:digest(ROOT/p) for p in paths},limitations=['48c7a0/48ea30/48c830/48ca70/48c300과13영역의전체산술/후보필터/packet/48bfe0수신을실행한다.','큰finder후보/heap/Regular생성/프레임표시/현재localOwner조회/세계음향/분해생성/삭제/가상피해는명시경계다. 기존fixture는바꾸지않는다.'])
    if previous:
        facts['normalReturns']+=previous['normalReturns'];facts['nativeInstructions']+=previous['nativeInstructions'];facts['nativeCalls']=dict(collections.Counter(facts['nativeCalls'])+collections.Counter(previous['nativeCalls']))  # 실제추가관측수만합한다.
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('애시드13영역:',len(rows),'입력/두 정밀도 차이0')


if __name__=='__main__':main()
