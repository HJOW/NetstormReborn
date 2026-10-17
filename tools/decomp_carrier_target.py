#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 운반 대상 이동/일반 접근점/변 중간점과 현재 AI 조회를 독립 관측한다."""
import argparse,collections,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL,PLAYERS
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

DATA,OUT,INPUT=0x14500000,0x14500100,0x14500200 # 명시 사각형/결과/고정 호출 입력이다.
THUNKS=(STOP+0xa00,STOP+0xa80,STOP+0xb00,STOP+0xb80) # 입력마다 코드를 바꾸지 않는 네 호출자다.
ENTRIES=(0x41d400,0x41d530,0x41ce90,0x4894e0,0x427600,0x4276a0,0x428fd0,0x40f100,0x4ac200,0x49a840,0x4af170) # 전체 실제 몸체만 허용한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-target-x86.tsv' # 이전 fixture와 분리한 새 입력/두 정밀도 원문이다.
REPORT=ROOT/'cpppj/carrier-target.json' # 실제 함수/통제/현재 raw/정밀도 차이와 SHA다.
RECTANGLES=((20,21,23,24),(20.25,21.75,23.5,24.125),(0,0,255,255),(-3,-5,1,2),(16777216,20,16777220,24),(2**40,-2**40,2**40+2**18,-2**40+2**18)) # float 저장 시점/지도 밖 입력도 보정하지 않는다.
POINTS=((10,22),(30,22),(21.5,10),(21.5,30),(21.5,22.5),(20,21),(float('nan'),30),(30,float('nan'))) # 방향 동률과 순서 없는flags2 비교도 관측한다.


def cases():
    """기하/현재 AI/free/genus/재시도/발자국 전후 읽기를 교차한다."""
    base=[0,*map(bits,RECTANGLES[0]),*map(bits,POINTS[0]),1,0,1,0xf0000001,1,1,0,0,0x10000000,1,60,0,bits(-10000),bits(-10000),0];rows=[]
    # 네 정상 방향과 실제 진단 반환/flags4의 오른쪽 넓은 값을 입력한다.
    for rectangle,side,flags in itertools.product(RECTANGLES,(0,2,4,6,1,7,0xffffffff),(0,1,4,5,0xffffffff,0x7f800001)):
        r=base.copy();r[1:5]=map(bits,rectangle);r[7:9]=flags,side;rows.append(r)
    # 일반 접근점은 Graph 접근점과 달리flags1/2/4만 사용한다.
    for rectangle,point,flags,mutation in itertools.product(RECTANGLES,POINTS,range(8),(0,1)):
        r=base.copy();r[0]=1;r[1:5]=map(bits,rectangle);r[5:7]=map(bits,point);r[7]=flags;r[21]=mutation;rows.append(r)
    # 첫 접근점과 네 변의 재시도는 최초 사각형/명령 인자를 유지한다.
    for rectangle,point,success,mutation,replace in itertools.product(RECTANGLES[:2],POINTS[:4],range(6),range(3),(0,0xffffffff)):
        r=base.copy();r[0]=2;r[1:5]=map(bits,rectangle);r[5:7]=map(bits,point);r[9]=replace;r[11]=success;r[18]=mutation;rows.append(r)
    # AI 존재/중립 owner/free 대 dead/genus와 단일 시도의 구별이다.
    for op,owner,brain,state,genus,success,mutation in itertools.product((3,4),(0,1,8),(0,1,0xffffffff),(0,1,2,3),(0,0x10000000),(0,1,2),(0,1)):
        r=base.copy();r[0]=op;r[11:16]=success,owner,brain,state,genus;r[18]=mutation;rows.append(r)
    # 귀환은 후보 선택 뒤 boss를 재검사하지 않는다.
    for boss,target,success,mutation in itertools.product((0,1),(0,60),range(6),(0,1,3)):
        r=base.copy();r[0]=5;r[11]=success;r[16:19]=boss,target,mutation;rows.append(r)
    r=base.copy();r[0]=6;rows.append(r)
    return list(dict.fromkeys(tuple(r) for r in rows))


class Oracle(BaseOracle):
    """실제 기하/상위/Player AI/TYPE getter를 실행하고 큰 시작·선택 효과만 기록한다."""
    def __init__(self):
        """고정 호출자를 한 번 설치하고 독립 부모/Player/결과 페이지를 추가한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.code_spans=[]
        # 기존 격리 입력 영역과 겹치지 않는다.
        for a,n in ((POOL,0x10000),(PLAYERS,0x1000),(DATA,0x1000)):self.mu.mem_map(a,n)
        self.install(THUNKS[0],0x41d400,[('mem',INPUT+7*4),('mem',INPUT+8*4),('imm',OUT)],DATA,0)
        self.install(THUNKS[1],0x41d530,[('mem',INPUT+7*4),('mem',INPUT+6*4),('mem',INPUT+5*4),('imm',OUT)],DATA,0)
        self.install(THUNKS[2],0x4894e0,[('mem',INPUT+10*4),('mem',INPUT+9*4),('imm',60),('imm',50)],None,16)
        self.install(THUNKS[3],0x4af170,[('imm',0x1234)],None,4);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def install(self,address,entry,args,this,cleanup):
        """cdecl/thiscall 실제 스택 규약을 고정 기계어 호출자로 검사한다."""
        code=b'' if this is None else b'\xb9'+struct.pack('<I',this)
        # 역순 인자만 실제 스택에 올린다.
        for kind,value in args:code+=(b'\xff\x35' if kind=='mem' else b'\x68')+struct.pack('<I',value)
        code+=b'\xe8'+struct.pack('<i',entry-(address+len(code)+5))
        if cleanup:code+=b'\x83\xc4'+bytes((cleanup,))
        code+=b'\xc3';self.mu.mem_write(address,code);self.code_spans.append((address,address+len(code)))

    def instruction(self,mu,address,size,data):
        """발자국/Start/귀환 선택/진단 외의 계산은 원본 명령으로만 수행한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<8I',mu.mem_read(sp+4,32));this=mu.reg_read(UC_X86_REG_ECX);sid=(this-POOL)//50;c=self.case
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x4ade70:
            self.event('R',sid,a[1]);mu.mem_write(a[0],struct.pack('<4I',*c[1:5]))
            if c[18]==1:self.put(POOL+50*50+14,bits(42.25));self.put(POOL+50*50+18,bits(43.5))
            self.ret(a[0],8);return
        if address==0x489080:
            self.event('S',*a);self.attempt+=1
            if c[18]==2:self.put(POOL+50*50+14,bits(100));self.put(POOL+50*50+18,bits(101));self.put(POOL+60*50+14,bits(200));self.put(POOL+60*50+18,bits(201))
            self.ret(0x14600000+self.attempt*40 if c[11]==self.attempt else 0);return
        if address==0x428ea0:
            self.event('C',sid)
            if c[18]==3:self.put(0x540bc4,0);self.put(POOL+50*50+14,bits(21.75));self.put(POOL+50*50+18,bits(8.5))
            self.ret(c[17]);return
        if address==0x4e0620:
            self.event('!',a[2]);assert self.string(a[0])=='false' and self.string(a[1])=='\\Ns\\O\\Boardcoord.cpp' and a[2] in (0x1f9,0x262)
            if c[21]:self.put(0x545370,bits(42.125));self.put(0x545374,0x7f800001)
            self.ret();return
        if address in (0x4abb10,0x4952d0):self.event('I',address,*a[:1 if address==0x4abb10 else 0]);self.ret();return
        if any(lo<=address and address+size<=hi for lo,hi in self.code_spans):return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'운반 대상 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """원본 스택/SEH/두 출력점과 월드 초기화의 두 DWORD만 허용한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==0 and size==4 or OUT<=address and address+size<=OUT+8 or address in (0x545370,0x545374) and size==4:return
        raise RuntimeError(f'운반 대상 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """현재 raw/Player/TYPE/기하 입력과 보호 이미지 밖의 읽기를 거부한다."""
        spans=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(PLAYERS,PLAYERS+0x1000),(TYPES,TYPES+0x20000),(DATA,DATA+0x1000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'운반 대상 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """예상 접근점/시도 순서/반환을 계산하지 않고 명시 입력과 실제 결과만 저장한다."""
        self.case=c;self.events=[];self.attempt=0;self.mu.mem_write(DATA,struct.pack('<4I',*c[1:5]));self.mu.mem_write(OUT,b'\xa5'*8);self.mu.mem_write(INPUT,struct.pack('<22I',*c));self.mu.mem_write(PLAYERS,bytes(9*0xb4));self.put(PLAYERS+c[12]*0xb4+0xb0,c[13]);self.put(0x59534c,PLAYERS);self.put(0x59ab20,TYPES);self.put(0x5c8464,POOL);self.put(0x540bc4,c[16]);self.put(0x545370,c[19]);self.put(0x545374,c[20]);self.put(TYPES+122*500+0xec,c[15]);self.put(0,0x87654321)
        # 원래 부모/대상 전체 raw 입력이며 발자국 getter는 위의 명시 사각형 경계다.
        for sid,typ in ((50,133),(60,122)):
            raw=bytearray(b'\xa5'*50);raw[10]=typ;raw[11]=c[14] if sid==60 else 0;raw[34]=c[12] if sid==50 else 2;struct.pack_into('<II',raw,14,*c[5:7]);self.mu.mem_write(POOL+sid*50,bytes(raw))
        entries=(THUNKS[0],THUNKS[1],THUNKS[2],0x4276a0,0x427600,0x428fd0,THUNKS[3]);args=struct.pack('<I',60) if c[0] in (3,4) else b''
        PreviousOracle.invoke(self,entries[c[0]],args,precision,False);self.returns+=1;assert self.get(0)==0x87654321
        if c[0] in (0,1):assert self.mu.reg_read(UC_X86_REG_EAX)==OUT
        result=0 if c[0] in (0,1,6) else self.mu.reg_read(UC_X86_REG_EAX)
        state=(result,self.get(OUT),self.get(OUT+4),self.get(0x545370),self.get(0x545374),self.get(0x540bc4),self.attempt)
        return ';'.join(self.events) or '-',','.join(map(str,state)),','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,60))


def main():
    """두 x87 정밀도의 독립 원문을 기록하거나 현재 전체 SHA/입력 순서를 감사한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--verify',action='store_true');args=p.parse_args()
    if args.verify:
        r=json.loads(REPORT.read_text('utf-8'));rows=[l.split('\t') for l in FIXTURE.read_text('utf-8').splitlines() if l and not l.startswith('#')]
        # 검증 모드는 새 관측으로 원문을 덮어쓰지 않는다.
        for path,want in r['sha256'].items():assert digest(ROOT/path)==want,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)*2==r['normalReturns'];assert [i for i,row in enumerate(rows) if row[1:4]!=row[4:7]]==r['precisionDifferentInputs'];print('운반 대상 원본 감사:',len(rows),'입력/',r['normalReturns'],'정상 반환');return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 각 입력을 보호 PE에서 독립 실행하며 C++ 결과를 기대값으로 사용하지 않는다.
    for index,c in enumerate(cases()):
        outputs=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if outputs[0]!=outputs[1]:different.append(index)
        rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input effects53 state53 raws53 effects64 state64 raws64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_target.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-target-x86.tsv')
    r=dict(host=socket.gethostname(),date='2026-10-17',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=different,boundaries='41d400/41d530/41ce90/4894e0/427600/4276a0/428fd0/40f100/4ac200/49a840/4af170 전체. 발자국4ade70/경로시작489080/귀환선택428ea0/진단 및초기화의4abb10·4952d0은명시경계다. GP/retN/SEH/canary/x87/명령·읽기·쓰기범위를검사했다. 큰후보선택/자연운반·채취·완주/게임창실행검증이아니다.',sha256={s:digest(ROOT/s) for s in paths})
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:r[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
