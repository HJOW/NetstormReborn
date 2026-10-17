#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE 전체429800의 명시 채취 계약에서 실제 헤더/송신/로컬 수신 순서를 관측한다."""
import argparse,collections,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-mining-x86.tsv' # 기존 수량 몸체2312행과 구별하는 새 통지 원문이다.
REPORT=ROOT/'cpppj/carrier-mining.json' # 전체 도착 실행의 명시 계약과 새 비교 범위를 기록한다.
ENTRIES=(0x429800,0x4ac200,0x49a840,0x4ac1e0,0x40ac20,0x4df230) # 헤더 생성과 TYPE getter는 실제 명령이다.


def cases():
    """헤더 BYTE/WORD의 경계와 지면 WORD 및 송신 효과를 교차한다."""
    return list(itertools.product((0,1,0x80,0xff),(0,0x5b,0xff),(0,1,3,4,6,0xffff),(60,256,1023),range(4)))


class Oracle(BaseOracle):
    """boss1/AI1/너겟없음/유효간헐천 계약의 전체 도착을 실행한다. 큰 조회와 효과는 명시 경계다."""
    def __init__(self):
        """현재 풀/TYPE와 실제 상위·헤더의 실행 범위만 확보한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.created=0
        self.code_high={a:hi for lo,hi in self.allowed for a in range(lo,hi)} # 각 실제 명령의 허용 끝 주소다.
        self.boundaries={0x4ac9f0,0x4b2070,0x4263a0,0x4b21d0,0x426900,0x427ce0,0x462c80,0x40f100,0x4afa70,0x4e2e30,0x4266c0,0x4202f0,0x4b1810} # 실제 큰 검색/내용물 생성/전송/로컬 수신 경계다.
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def instruction(self,mu,address,size,data):
        """전체 도착의 분기·헤더 작성·지면 기록·송신 전후 순서는 원본이 결정한다."""
        self.instructions+=1
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address not in self.boundaries:
            if address+size<=self.code_high.get(address,0):return
            raise RuntimeError(f'채취 통지 범위 밖 실행 {address:08x}')
        sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<6I',mu.mem_read(sp+4,24));this=mu.reg_read(UC_X86_REG_ECX);c=self.case
        if address==0x4ac9f0:self.ret(0,12);return
        if address==0x4b2070:self.ret(c[3] if a[2]==0x10000000 else 0);return
        if address in (0x4263a0,0x4b21d0,0x427ce0,0x462c80):self.ret();return
        if address==0x426900:mu.mem_write(a[0],bytes(8));self.ret(a[0],12);return
        if address==0x40f100:self.ret(1);return
        if address==0x4afa70:
            assert (this-POOL)//50==50 and a[:5]==(154,0,32,0,0);self.created+=1;self.ret(0,20);return
        if address==0x4e2e30:
            assert self.created==self.beforeCreated+1;self.packet=a[0];self.events.append('S:'+bytes(mu.mem_read(a[0],6)).hex())
            if c[4]==1:self.put(0x52ee32,0x23,1);self.put(0x52ee33,0x81,1);self.put(0x52ee34,7,2)
            if c[4]==2:self.put(a[0]+4,61,2)
            if c[4]==3:self.put(POOL+c[3]*50+11,1,1)
            self.ret();return
        if address==0x4266c0:
            assert a[:2]==(0,0) and a[2]==self.packet;self.events.append('L:'+bytes(mu.mem_read(a[2],6)).hex());self.ret();return
        if address in (0x4202f0,0x4b1810):
            mu.mem_write(this+52,bytes(4));self.ret(this,12 if address==0x4202f0 else 0);return
        raise RuntimeError(f'채취 통지 미처리 경계 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """직접 원본 쓰기는 stack에 한정한다. 경계의 명시 입력 효과는 별도다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000:return
        raise RuntimeError(f'채취 통지 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호 코드/격리 raw·TYPE/stack만 읽을 수 있다."""
        spans=((0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'채취 통지 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """같은 현재 입력에서 원본의 선택된 통지 효과와 현재 헤더/지면 상태를 기록한다."""
        self.case=c;self.events=[];self.packet=0;self.beforeCreated=self.created;self.mu.mem_write(POOL,bytes(0x10000));self.put(POOL+50*50+10,133,1);self.put(POOL+50*50+34,1,1);self.put(POOL+c[3]*50+12,1000,2)
        self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(TYPES+133*500+0xec,0x10000);self.put(TYPES+133*500+0xe8,0);self.put(0x540bc4,1);self.put(0x5e4794,0);self.put(0x5412c0,154);self.put(0x5412d0,158);self.put(0x5412ec,165);self.put(0x5411a8,84);self.put(0,0x87654321)
        self.put(0x52ee33,c[0],1);self.put(0x52ee32,c[1],1);self.put(0x52ee34,c[2],2)
        PreviousOracle.invoke(self,0x429800,struct.pack('<2I',0,0),precision,False);self.returns+=1
        assert self.get(0)==0x87654321 and self.mu.reg_read(UC_X86_REG_EAX)==1000 and len(self.events)==2 and self.created==self.beforeCreated+1
        current=(self.get(0x52ee33)&255,self.get(0x52ee32)&255,self.get(0x52ee34)&65535,self.get(POOL+c[3]*50+11)&255)
        return ';'.join(self.events),bytes(self.mu.mem_read(self.packet,6)).hex(),','.join(map(str,current))


def main():
    """독립 원문을 새로 관측하거나 기존 원문/도구의 SHA만 읽기 전용으로 검사한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--verify',action='store_true');args=p.parse_args()
    if args.verify:
        r=json.loads(REPORT.read_text('utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text('utf-8').splitlines() if s and not s.startswith('#')]
        # 기대값 재계산이나 원문 덮어쓰기는 하지 않는다.
        for path,want in r['sha256'].items():assert digest(ROOT/path)==want,path
        assert [x[0] for x in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)*2==r['normalReturns'];assert all(x[1:4]==x[4:7] for x in rows);print('채취 통지 원문 감사:',len(rows),'입력/',r['normalReturns'],'정상반환');return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # C++는 기대값 생성에 사용하지 않고 두 실제 정밀도를 별도로 실행한다.
    for i,c in enumerate(cases()):
        out=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if out[0]!=out[1]:different.append(i)
        rows.append((','.join(map(str,c)),*out[0],*out[1]))
    FIXTURE.write_text('# input effects53 packet53 current53 effects64 packet64 current64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_mining.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-mining-x86.tsv')
    r=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),createdNuggetBoundary=sum(o.created for o in oracles),precisionDifferentInputs=different,boundaries='전체429800 및 실제40ac20/4df230/TYPE getter를 실행했다. boss1/AI1/owner1/빈내용물/유효간헐천/Graph같음 계약이며 다른 현재 조회/내용물생성/송신/로컬수량수신은 명시 큰 경계다. 새C++비교범위는 그 안의6바이트통지작성·송신→로컬수신·현재헤더/지면상태다. 수량/고갈본문은 기존geysermined2312행 별도이고 전체429800 모든분기/실제게임창/완주 검증은 아니다. GP/retN/SEH/x87/명령·읽기·쓰기범위를검사했다.',sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:r[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','createdNuggetBoundary','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
