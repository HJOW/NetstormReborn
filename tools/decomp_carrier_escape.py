#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE429010/427d30/Prepare/ftol과 원본 방향표의 실제 회피 결과를 독립 관측한다."""
import argparse,collections,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP
MAP,OLD,NEW=0x14600000,0x14900000,0x14b00000 # 현재spot/기존Runtime/새Runtime의격리입력이다.
ENTRIES=(0x429010,0x427d30,0x4ac200,0x4ac1e0,0x4ac550,0x49a840,0x488fb0,0x488e20,0x41cd60,0x41cfc0,0x425e20,0x40e800,0x4e49c0,0x4e4a40,0x422230,0x41d7a0) # 실제 상위/순수 getter/좌표/경로시각/stack probe 몸체다.
FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-escape-x86.tsv' # 이전 후보/회피 경계 자료와 구별되는 전체 회피 관측이다.
REPORT=ROOT/'cpppj/carrier-escape.json' # 실제 정상반환/효과/전체raw/깊이/정밀도/보호SHA다.
SCENES=((),(60,),(60,61),(61,60)) # 같은bucket에 놓인 실제 체인의 명시 순서다.


def qbits(value):
    """현재 시각 입력을 raw64로 기록한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def route(mode,which,mask):
    """BFS 경계가 공급할 명시 배열이다. 예상 선택이나 시각은 계산하지 않는다."""
    if not mask&(1<<(which-60)):return [],[]
    length=((2.0,4.0),(2.0,2.0),(4.0,2.0),(0.125,0.25))[mode][which-60]
    return [(0.0,0.0),(length,0.0)],[0,0]


def cases():
    """깊이/거리/타입/네방향/실패와 주변후보·효과뒤변경을 별도로 교차한다."""
    # distance/event/depth/path/check/genus/type/spot/startSuccess/random/scene/surfaceMask/blockerMask/bfsMask/route/mutation/x/y/speed/now다.
    base=[bits(1),0,0,1,0,0x10000,133,0,1,0,0,3,0,3,0,0,bits(20.75),bits(21.5),bits(2),qbits(100)];rows=[]
    # 실제 네 방향/비행단위거리/깊이한도/첫난수방향4/실패후다음시도다.
    for depth,distance,genus,event,blocked,success,random in itertools.product((0,2,3),(0,1,2),(0x10000,0x20000),(0,2,6,0xffffffff),(0,1),(0,1,4),(0,3999,4999)):
        c=base.copy();c[0:3]=bits(distance),event,depth;c[5]=genus;c[7:10]=blocked,success,random;rows.append(c)
    # 표면조건/다른carrier/빈경로 조합과 finder의 빈목록/역순이다.
    for scene,surface,blocker,bfs in itertools.product(range(4),range(4),range(4),range(4)):
        c=base.copy();c[1]=0x1da1a;c[9:14]=4999,scene,surface,blocker,bfs;rows.append(c)
    # 엄격한동률/현재타입cardinal/시각/좌표/메타데이터변경은 원문 준비시각까지 실제 실행한다.
    for typ,mode,scene,mutation in itertools.product((133,137,158),range(4),(2,3),(0,4,5,6,8)):
        c=base.copy();c[1]=0x1da1a;c[6]=typ;c[10]=scene;c[14:16]=mode,mutation;rows.append(c)
    # 진단은 최초깊이로조건을고르고 최종반환은현재같은Runtime에서다시읽는다.
    for depth,path,check,mutation in itertools.product((0xffffffff,3,4,0x7fffffff),(0,1),(0,1),(0,1,7,9)):
        c=base.copy();c[2:5]=depth,path,check;c[15]=mutation;rows.append(c)
    # 현재부모위치캡처와 두거의올림상수, 선형spot주소의음수x감김을검사한다.
    for x,distance,event,mutation in itertools.product((bits(-1.25),bits(-0.25),bits(20.99999),bits(20.9999)),(bits(0),bits(.125),bits(.9999)),(0,0xffffffff),(0,2,3)):
        c=base.copy();c[0]=distance;c[1]=event;c[15:17]=mutation,x;rows.append(c)
    return list(dict.fromkeys(tuple(c) for c in rows))


class Oracle(BaseOracle):
    """큰탐색/시작만명시경계로두고분기와시간은원본을실행한다."""
    def __init__(self):
        """실제 코드 범위와 고정 크기 원본 입력만 준비한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.deletions=0
        # 기존/새경로는 서로 다른 물리 저장소다. 기존삭제 뒤 실제 명령의 접근은 금지한다.
        for a,n in ((POOL,0x10000),(MAP,0x10000),(OLD,0x19000),(NEW,0x19000)):self.mu.mem_map(a,n)
        self.code_high={a:hi for lo,hi in self.allowed for a in range(lo,hi)};self.boundaries={0x427ce0,0x4558f0,0x489080,0x4b16d0,0x4b1810,0x4b2070,0x488af0,0x4e0620}
        self.originalDirections=struct.unpack('<17i',self.mu.mem_read(0x52f83c,68));self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def candidate(self):
        """명시 finder의 현재후보이며 그 뒤 선택은 원본의 실제조건이 결정한다."""
        return self.candidates[self.index] if self.index<len(self.candidates) else 0

    def instruction(self,mu,address,size,data):
        """원본 루프/좌표/깊이/후보시간을 대체하지 않고 실제 큰 경계 인자만 기록한다."""
        self.instructions+=1
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address not in self.boundaries:
            if address+size<=self.code_high.get(address,0):return
            raise RuntimeError(f'회피 범위 밖 실행 {address:08x}')
        sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<8I',mu.mem_read(sp+4,32));this=mu.reg_read(UC_X86_REG_ECX);c=self.case;slot=POOL+50*50;m=c[15]
        if address==0x427ce0:
            self.event('P',50)
            if m==1:self.put(slot+14,bits(25.25));self.put(slot+18,bits(26.25));self.put(OLD+0x186d8,1)
            self.ret(OLD if c[3] else 0);return
        if address==0x4558f0:
            self.event('R',*a[:2]);assert a[:2]==(0,5000)
            if m==2:self.put(slot+14,bits(1));self.put(slot+18,bits(2))
            self.ret(c[9]);return
        if address==0x489080:
            self.starts+=1;assert a[0]==50 and a[3:8]==(0,0,0,0,0);self.event('S',*a[:3])
            if m==3:self.put(slot+14,bits(42.25));self.put(slot+18,bits(43.5));self.put(0x5e4794,0)
            if c[3] and not self.deleted:self.deleted=True;self.deletions+=1;mu.mem_write(OLD,b'\xdd'*0x186f0)
            self.ret(NEW if c[8] and self.starts==c[8] else 0);return
        if address==0x4b16d0:
            self.event('F',*a[:4]);self.iterator=this;self.index=0;self.put(this+52,self.candidate());self.ret(0,16);return
        if address==0x4b1810:self.index+=1;self.put(this+52,self.candidate());self.ret();return
        if address==0x4b2070:
            assert a[2]==0x10000;self.event('O',*a[:2]);which=self.candidate();self.active=which
            if m==4:self.put(slot+10,137,1);self.put(POOL+which*50+18,bits(23.5))
            self.ret(70 if c[12]&(1<<(which-60)) else 0);return
        if address==0x488af0:
            output=a[0];self.event('B',*a[1:6]);assert self.get(output)==0 and bytes(mu.mem_read(output+0x9c48,40000))==bytes(40000)
            points,directions=route(c[14],self.active,c[13]);self.put(output,len(points))
            # 실제 경로 시각은 원본Prepare가 계산한다. 외부입력은 점/방향뿐이다.
            for i,(point,direction) in enumerate(zip(points,directions)):mu.mem_write(output+4+i*8,struct.pack('<2f',*point));self.put(output+0x13888+i*4,direction)
            if m==5:self.put(0x55b4d0,qbits(1000.25),8);self.put(slot+10,135,1)
            if m in (6,8):self.put(POOL+self.active*50+14,bits(23) if m==6 else bits(0));self.put(POOL+self.active*50+18,bits(23))
            self.ret();return
        if address==0x4e0620:
            self.event('A',a[2]);assert a[2] in (0x87b,0x87c)
            if m in (7,9):self.put(OLD+0x186d8,0)
            if m==7:self.put(0x5e4794,0)
            self.ret();return

    def write(self,mu,access,address,size,value,data):
        """실제 직접 쓰기는 원본stack과 새 경로 깊이에만 허용한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==NEW+0x186d8 and size==4:return
        raise RuntimeError(f'회피 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """호출자가 삭제한 기존Runtime을 원본도 다시 읽으면 실패한다."""
        if OLD<=address and address+size<=OLD+0x186f0 and self.deleted:raise RuntimeError('회피 시작 뒤 기존Runtime 접근')
        spans=((0x400000,0x600000),(POOL,POOL+0x10000),(MAP,MAP+0x10000),(OLD,OLD+0x19000),(NEW,NEW+0x19000),(TYPES,TYPES+0x20000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'회피 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """새 물리입력으로 정상반환/깊이/전역/전체raw와 효과를 기록한다."""
        self.case=c;self.events=[];self.starts=0;self.deleted=False;self.candidates=SCENES[c[10]];self.index=0;self.active=0;self.mu.mem_write(POOL,b'\xa5'*0x10000);self.mu.mem_write(MAP,bytes((16 if c[7] else 0,))*65536);self.put(OLD+0x186d8,c[2]);self.put(NEW+0x186d8,12345)
        # 부모와 후보의 같은 실제 raw 체인을 입력한다. finder 결과는 별도 명시 경계다.
        for sid,typ,x,y in ((50,c[6],c[16],c[17]),(60,70,bits(22.25),bits(22.5)),(61,71,bits(22.75),bits(22.5))):
            p=POOL+sid*50;self.put(p+10,typ,1);self.put(p+11,0,1);self.put(p+40,0,1);self.put(p+33,1,1);self.put(p+14,x);self.put(p+18,y);self.put(p+4,0,2)
        # 두 후보는 같은bucket의명시체인이다. 꼬리는0으로끝낸다.
        for i,sid in enumerate(self.candidates):self.put(POOL+sid*50+4,self.candidates[i+1] if i+1<len(self.candidates) else 0,2)
        for typ in (133,137,135,158):self.put(TYPES+typ*500+0xec,c[5]);self.put(TYPES+typ*500+0xe0,bits(3.25) if typ==135 else c[18]);self.put(TYPES+typ*500+0xe4,bits(90))
        for i,typ in enumerate((70,71)):self.put(TYPES+typ*500+0xe8,0x800 if c[11]&(1<<i) else 0)
        for a,v in ((0x5c8464,POOL),(0x59ab20,TYPES),(0x5c7c44,MAP),(0x5e4794,c[4]),(0x531928,256),(0x54127c,137),(0x5412c8,156),(0x541274,135),(0x541344,187)):self.put(a,v)
        self.put(0x55b4d0,c[19],8);self.put(0,0x87654321);PreviousOracle.invoke(self,0x429010,struct.pack('<2I',c[0],c[1]),precision,False);self.returns+=1;assert self.get(0)==0x87654321
        state=(self.get(NEW+0x186d8),int(self.deleted),self.get(0x5e4794),struct.unpack('<Q',self.mu.mem_read(0x55b4d0,8))[0])
        return str(self.mu.reg_read(UC_X86_REG_EAX)),';'.join(self.events) or '-',','.join(map(str,state)),','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,60,61))


def main():
    """독립PE관측이나기존자료의읽기전용SHA감사를선택한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--verify',action='store_true');args=p.parse_args()
    if args.verify:
        r=json.loads(REPORT.read_text('utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text('utf-8').splitlines() if s and not s.startswith('#')]
        # 검사는 기대값을 다시 생성하지 않는다.
        for path,want in r['sha256'].items():assert digest(ROOT/path)==want,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)*2==r['normalReturns'];assert all(x[1:5]==x[5:9] for x in rows);print('회피 원본 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 현재 C++ 구현은 원본 기대값 생성에 사용하지 않는다.
    for i,c in enumerate(cases()):
        out=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if out[0]!=out[1]:different.append(i)
        rows.append((','.join(map(str,c)),*out[0],*out[1]))
        if i%128==0:print('원본 회피 입력',i,'누적명령',sum(o.instructions for o in oracles),flush=True)
    FIXTURE.write_text('# input result53 effects53 state53 raws53 result64 effects64 state64 raws64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_escape.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','tools/decomp_nugget_regular.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-escape-x86.tsv')
    r=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),oldRuntimeDeletions=sum(o.deletions for o in oracles),originalDirections=list(oracles[0].originalDirections),precisionDifferentInputs=different,boundaries='전체429010/427d30/실제TYPE·cardinal·좌표·ftol·Prepare·경로시각·stack probe를실행했다. 현재경로조회/난수반환/일반finder결과/BFS입력배열/새이동시작/복귀진단은명시경계다. 새경로깊이/기존Runtime삭제후무접근/효과순서·인자/전체부모·후보raw/현재checking·시각과GP/retN/SEH/canary/x87/명령·읽기·쓰기범위를검사했다. BFS계산·실제게임창·자연완주는아니다.',sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:r[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','oldRuntimeDeletions','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
