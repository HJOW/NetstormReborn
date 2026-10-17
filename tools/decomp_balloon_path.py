#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""벌룬42 생성/이륙 경로/실제 Step과 고도 프레임을 보호 PE에서 독립 관측한다."""
import argparse,collections,itertools,json,socket,struct,zlib
import pefile
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,POOL,TYPES,digest
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX,UC_X86_REG_EAX

HEAP,SIZE=0x14500000,0x186f0 #기존 전체 경로와 동일한 실제 할당 크기다.
FIXTURE=ROOT/'cpppj/tests/fixtures/balloon-path-x86.tsv' #기존 비행/웜 원문을 덮어쓰지 않는다.
REPORT=ROOT/'cpppj/balloon-path.json' #정상 반환과 원본/관측기 SHA다.
ENTRIES=(0x48be70,0x48b2c0,0x48acd0,0x488d00,0x4882e0,0x41ce90,0x48ad60,0x48a330,0x41bfb0,0x425e20,0x488f40,0x488f80,0x41cd60,0x488f10,0x4e49c0,0x4ac550,0x49a840) #실제 선택 몸체만 허용한다.

def bits(value):
    """명시 float 입력을 원문 DWORD로 전달한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]

def cases():
    """정답 없이 할당/고도/현재 단계/시각/실제 경로 입력만 구성한다."""
    result=[]
    # 열은 op,초기count/현재단계,고도,종류,x/now float비트,y,상태,효과변형,경로변형이다.
    for fill,allocation in itertools.product((0,0xa5,0xff),(0,1)):result.append((0,fill,allocation,0,0,0,0,0,0))
    for count,height,point in itertools.product((0,1,3),(0,1,42,43,44,255),((10.,20.),(13.5,20.),(10.,24.75),(6.25,20.),(10.,15.5),(14.,24.),(6.,16.),(12.25,22.25))):result.append((1,count,height,111,bits(point[0]),bits(point[1]),0,0,0))
    for current,height,kind,now in itertools.product(range(5),(0,20,44,255),(111,141),(0.,.25,.5,.999,1.,1.5,2.999,3.,3.25,3.999,4.,4.5)):result.append((2,current,height,kind,bits(now),0,0,0,0))
    # 처음부터 종료/가상 Halt 중단이면 Step과 고도를 읽지 않는다.
    for state,current in itertools.product((1,2),range(5)):result.append((2,current,0,111,bits(.5),0,state,0,0))
    # 화염 뒤 현재 좌표 재독과 Unpop 뒤 원래 고도 BYTE 쓰기다.
    for mutation,current,now in itertools.product((1,2),(0,1,3),(0.,.5,3.5)):result.append((2,current,0,111,bits(now),0,0,mutation,0))
    # 극히 짧은 구간의 최소 float 저장과 같은 좌표의 고도 쓰기 생략이다.
    for current,height,now in itertools.product((1,2,3),(0,44),(1.,1.000005,1.00001)):result.append((2,current,height,111,bits(now),0,0,0,1))
    for current,now in itertools.product((0,1,2,3),(0.,.5,3.5)):result.append((2,current,0,111,bits(now),0,0,0,2))
    return list(dict.fromkeys(result))

class Oracle(BaseOracle):
    """실제 Step까지 실행하고 할당/Halt/도착/공간/음향 장치만 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE와 별도 raw 경로를 준비한다."""
        super().__init__();self.mu.mem_map(HEAP,0x19000);self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.normal=collections.Counter();self.pop=self.get(0x5040c0+0x90);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def invoke(self,entry,args=b'',this=0,purge=0,precision=0x27f):
        """모든 최상위 반환에서 GP/스택/canary/SEH/x87를 확인한다."""
        self.put(0,0x87654321);self.call(entry,this,args,purge,precision);assert self.get(0)==0x87654321;self.normal[f'{entry:08x}']+=1

    def instruction(self,mu,address,size,data):
        """PE 밖 큰 효과는 인자를 기록하고 명시 입력에 따라서만 자료를 바꾼다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);raw=POOL+50*50
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x4e0620:raise RuntimeError(f'벌룬 입력 {self.value} 원본 진단 {self.string(self.get(sp+4))} {self.string(self.get(sp+8))} line{self.get(sp+12):x}')
        if address==0x4e4391:
            assert self.get(sp+4)==SIZE;self.event('N',SIZE);self.ret(HEAP if self.allocation else 0);return
        if address==0x489ff0:self.event('H');self.ret(int(self.value[6]==2));return
        if address==0x48b340:self.event('A');self.put(HEAP+0x186c8,1);self.ret();return
        if address==0x4a9c30:self.event('S',self.get(sp+4),self.get(sp+8),self.get(sp+12));self.ret();return
        if address==0x4a9d70:
            assert self.string(self.get(sp+12))=='balloonFlame.wav';self.event('F',self.get(sp+4),self.get(sp+8),self.get(sp+16))
            if self.value[7]==1:self.put(raw+14,bits(30.));self.put(raw+18,bits(40.))
            self.ret();return
        if address==0x4afe50:
            assert mu.reg_read(UC_X86_REG_ECX)==raw;self.event('U',50,self.get(sp+4))
            if self.value[7]==2:self.put(raw+30,99,1)
            self.ret(0,4);return
        if address==self.pop:
            assert mu.reg_read(UC_X86_REG_ECX)==raw;self.event('P',self.get(sp+4),self.get(sp+8),self.get(sp+12));self.put(raw+14,self.get(sp+4));self.put(raw+18,self.get(sp+8));self.ret(0,12);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'벌룬 경로 범위밖 명령 {address:x}')

    def write(self,mu,access,address,size,value,data):
        """원문 몸체는 자기 경로/부모 고도/관찰전역/스택만 쓴다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((HEAP,HEAP+SIZE),(POOL+50*50,POOL+51*50),(STACK-0x7f000,STACK+0x1000),(0,4),(0x5951b8,0x5951c4))):return
        raise RuntimeError(f'벌룬 경로 범위밖 쓰기 {address:x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """읽기도 보호 이미지/명시 자료와 실제 스택 안에 제한한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0x400000,0x600000),(HEAP,HEAP+SIZE),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000),(0,4))):return
        raise RuntimeError(f'벌룬 경로 범위밖 읽기 {address:x}+{size}')

    def sample(self,value,precision):
        """출력 전체 경로 체크섬/부모50BYTE/관찰전역과 외부 효과를 읽는다."""
        self.value=value;op,a,height,kind,x,y,state,mutation,profile=value;self.events=[];self.allocation=bool(height) if op==0 else True
        self.mu.mem_write(HEAP,bytes([a if op==0 else 0])*SIZE);self.invoke(0x48be70,precision=precision)
        if op==0:return ';'.join(self.events),str(zlib.adler32(bytes(self.mu.mem_read(HEAP,SIZE)))),str(bool(self.mu.reg_read(UC_X86_REG_EAX))*1),'-','-'
        self.events=[];raw=bytearray(50);struct.pack_into('<I',raw,0,0x5040c0);struct.pack_into('<H',raw,6,7);raw[10]=kind;raw[30]=height;struct.pack_into('<ff',raw,14,10.,20.);self.mu.mem_write(POOL+50*50,bytes(raw));self.put(0x5c8464,POOL);self.put(0x5c847c,32768);self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(TYPES+kind*500+0x6c,123);self.put(HEAP+4,7);self.put(HEAP+8,50);self.pop=self.get(0x5040c0+0x90);self.put(0x5951b8,0,8);self.put(0x5951c0,0)
        if op==1:
            self.put(HEAP+0x10,a);self.put(HEAP+0x186c0,2);self.invoke(0x48acd0,struct.pack('<5I',50,x,y,0x12345678,0xabcdef01),HEAP,20,precision)
        else:
            points=[(10.,20.),(10.,16.),(14.,16.),(14.,20.)] if profile!=2 else [(10.,20.)]*4;times=[0.,1.,3.,4.] if profile!=1 else [0.,1.,1.00001,1.00002]
            self.put(HEAP+0x10,4)
            # 네 꼭짓점의 좌표/방향/시각은 명시 입력이며 관측값은 계산하지 않는다.
            for index,(point,time,direction) in enumerate(zip(points,times,(0,0,2,2))):
                self.mu.mem_write(HEAP+0x14+index*8,struct.pack('<2f',*point));self.mu.mem_write(HEAP+0x9c58+index*8,struct.pack('<d',time));self.put(HEAP+0x13898+index*4,direction)
            self.put(HEAP+0x186c0,a);self.put(HEAP+0x186c8,int(state==1));self.mu.mem_write(0x55b4d0,struct.pack('<d',struct.unpack('<f',struct.pack('<I',x))[0]));self.invoke(0x48ad60,this=HEAP,precision=precision)
        return ';'.join(self.events) or '-',str(zlib.adler32(bytes(self.mu.mem_read(HEAP,SIZE)))),bytes(self.mu.mem_read(POOL+50*50,50)).hex(),str(self.get(0x5951c0)),str(self.get(0x5951b8))

def main():
    """독립 원문을 생성하거나 기대값 재계산 없이 저장 근거를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[v.split('\t') for v in FIXTURE.read_text('utf-8').splitlines() if v and not v.startswith('#')]
        for p,h in report['sha256'].items():assert digest(ROOT/p)==h,p
        assert len(rows)==report['uniqueInputs'] and [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and all(r[1:6]==r[6:11] for r in rows);print('벌룬42 원문 감사',len(rows));return
    oracle=Oracle();rows=[];different=[]
    # 기대 출력은 두 정밀도의 실제 PE만 실행해서 읽는다.
    for index,value in enumerate(cases()):
        result=[oracle.sample(value,p) for p in (0x27f,0x37f)]
        if result[0]!=result[1]:different.append(index)
        rows.append((','.join(map(str,value)),*result[0],*result[1]))
    FIXTURE.write_text('# input effects53 adler53 raw53 flags53 stamp53 effects64 adler64 raw64 flags64 stamp64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image();tables={hex(v):[hex(struct.unpack_from('<I',image,v-0x400000+o)[0]) for o in range(0,52,4)] for v in (0x50e2b8,0x50e284,0x50e2ec)}
    paths=('originals/Netstorm.exe','tools/decomp_balloon_path.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/balloon-path-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=dict(oracle.normal),instructions=oracle.instructions,nativeCalls=dict(oracle.calls),precisionDifferentInputs=different,vtables=tables,boundaries='48be70/48b2c0/48acd0/488d00/48ad60와 실제 Step/좌표·각도·TYPE/CRT 하위를 실행했다. new/Halt/큰도착/가상공간/음향은 명시경계다. 전체100080BYTE/부모50BYTE/관찰전역/효과를 대조한다. GP/retN/canary/SEH/x87/코드·읽기·쓰기 범위를 검사한다. 원본게임·OS·창/전체42 이동 검증이 아니다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','precisionDifferentInputs')},ensure_ascii=False))

if __name__=='__main__':main()
