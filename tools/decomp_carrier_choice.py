#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 실제 골렘 후보 수집/평가/기하/정렬/Prepare 전체를 독립 관측한다."""
import argparse,collections,itertools,json,socket,struct,zlib
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL,PLAYERS
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

INPUT,LIST,RETURNS,GEYSERS=0x14500000,0x14500100,0x14600000,0x14601000 # 독립 입력/descriptor/현재 두 원래 목록이다.
BUFFER,ALIAS,THUNK=0x14700000,0x051eb854,STOP+0xc00 # 임시 후보 배열 두 주소와 고정 thiscall 호출자다.
METRIC_ALIAS=0x80000004 # 주소 감김이 방금 기록한 후보 거리 WORD와 겹치는 추가 배열 주소다.
ENTRIES=(0x428ea0,0x429420,0x4280a0,0x428020,0x4e6010,0x4e5fa0,0x4e4a40,0x4e49c0,0x41d530,0x41d400,0x41ce90,0x41cfc0,0x41cd60,0x488e20,0x488fb0,0x4ac550,0x4ac1e0,0x4ac200,0x49a840,0x40f100) # 실제 코드로만 실행할 본문이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-choice-x86.tsv' # 새 독립 원문이며 기존 정렬/기하 자료는 수정하지 않는다.
REPORT=ROOT/'cpppj/carrier-choice.json' # 실제 호출/경계/정밀도/보호 SHA다.


def cases():
    """AI/비행/경로 실패/시각/동률/현재 목록 변경과500개 제한을 교차한다."""
    base=[0,0,4,8,1,0,0,bits(0.5),bits(90),1,1,0,60,7,1,BUFFER,0x461c4000,0,50];rows=[]
    # op/listPattern/count/capacity/owner/brain/genus/speed/turn/probe/check/mutation/home/validMask/pathPattern/buffer/baseline/geometry/mover다.
    for owner,brain,genus,probe,path,pattern in itertools.product((0,1,8),(0,1),(0,0x20000),(0,1),(0,1,2,3,4,5),(0,2)):
        c=base.copy();c[4:7]=owner,brain,genus;c[9]=probe;c[14]=path;c[1]=pattern;rows.append(c)
    # 발자국 전 좌표/속도 캡처와 탐색 직전 현재 타입·시각/설정 재조회를 구별한다.
    for mutation,speed,geometry,buffer in itertools.product((0,1,2,3,4,7,8,9),(bits(0.5),bits(3.25)),(0,1),(BUFFER,ALIAS)):
        c=base.copy();c[11]=mutation;c[7]=speed;c[17]=geometry;c[15]=buffer;c[1]=5 if mutation==4 else 1 if mutation==8 else 0;rows.append(c)
    # 빈 목록/구멍/용량 부족/표면 진단/무효 mover의 원래 두 순회다.
    for pattern,count,capacity,mover in itertools.product((0,1,2,3,4,5),(0,1,2,8),(2,8),(0,50)):
        if count>capacity:continue
        c=base.copy();c[1:4]=pattern,count,capacity;c[18]=mover;c[11]=8 if pattern==1 else 0;rows.append(c)
    # caller 수집은 같은 실제 평가기를 실행하며 후보 검증과 Graph만 명시 경계다.
    for op,pattern,count,home,mask,mutation in itertools.product((1,2),(0,1,2),(0,4,501),(0,60),(0,3,7),(0,5,6)):
        if mutation==6 and op!=2:continue
        c=base.copy();c[0:4]=op,pattern,count,512;c[9]=0;c[12:14]=home,mask;c[11]=mutation;rows.append(c)
    # 원래 signed source count가 음수면 수집하지 않는다. 임시 평가 목록 자체의 손상은 정상 반환으로 다루지 않는다.
    for op in (1,2):
        c=base.copy();c[0]=op;c[2]=0xffffffff;c[3]=8;c[9]=0;rows.append(c)
    # 귀환점이 부모와 다른 실제 간헐천 평가에서도 BFS 시작/끝 인자를 대조한다.
    for buffer,path,mutation in itertools.product((BUFFER,ALIAS),(1,2),(0,1)):
        c=base.copy();c[0]=2;c[15]=buffer;c[14]=path;c[11]=mutation;rows.append(c)
    # 정렬 비교의 주소가 현재 평가가 기록한 raw 거리 WORD를 직접 읽는 조합이다.
    for path,mutation in itertools.product((0,1,2),(0,1)):
        c=base.copy();c[2:4]=16,16;c[15]=METRIC_ALIAS;c[14]=path;c[11]=mutation;rows.append(c)
    return list(dict.fromkeys(tuple(c) for c in rows))


def source_values(c):
    """구멍/중복/역순을 포함한 원래 목록의 명시 DWORD 입력이다."""
    return [0 if c[1]==1 and i%3==0 else 60+i%2 if c[1]==2 else 67-i%8 if c[1]==3 else 60+i for i in range(512)]


def route(pattern,candidate):
    """BFS 경계의 명시 반환 배열이며 예상 선택/시각을 계산하지 않는다."""
    if pattern==0 or pattern==2 and candidate%2==0:return [],[]
    if pattern==4:return [(0.0,0.0)],[7]
    if pattern==2:return [(0,0),(30,40),(31,41)],[0,2,4]
    if pattern==3:return [(0,0),(3+candidate%3,4+candidate%5)],[0,2]
    if pattern==5:return [(0,0),(float('nan'),4)],[0,2]
    return [(0,0),(3,4)],[0,2]


class Oracle(BaseOracle):
    """키/정렬/기하/시각 계산을 원본에 맡기고 큰 Footprint/BFS/검증·메모리 경계만 공급한다."""
    def __init__(self):
        """보호 이미지와 전체 물리 풀/두 source 목록/두 임시 배열을 독립 설치한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.allocations=0;self.frees=0
        # 24,000개 PATCH 슬롯과 canary 공간을 실제로 확보한다.
        for address,size in ((POOL,0x126000),(PLAYERS,0x1000),(INPUT,0x1000),(RETURNS,0x2000),(BUFFER-0x1000,0x3000),(0x051eb000,0x3000),(0x7ffff000,0x3000)):self.mu.mem_map(address,size)
        code=b'\xb9'+struct.pack('<I',LIST)+b'\xff\x35'+struct.pack('<I',INPUT+18*4)+b'\x6a\x32';code+=b'\xe8'+struct.pack('<i',0x4280a0-(THUNK+len(code)+5))+b'\xc3';self.mu.mem_write(THUNK,code);self.allowed.append((THUNK,THUNK+len(code)))
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.initial_pool=bytes([0x2a])*(24000*50)
        self.entries=frozenset(ENTRIES);self.boundaries=frozenset((0x4ade70,0x488af0,0x4274b0,0x427580,0x462bc0,0x4e4391,0x4e4354,0x4e0620)) # 매 명령에서 인자를 읽지 않고 실제 경계 진입에서만 읽는다.
        self.code_high={a:high for low,high in self.allowed for a in range(low,high)} # 기존 코드 범위 검사를 같은 끝 주소 표로 가속한다.
        spans=((0,4),(0x400000,0x600000),(POOL,POOL+24000*50),(PLAYERS,PLAYERS+0x1000),(TYPES,TYPES+0x20000),(INPUT,INPUT+0x1000),(RETURNS,RETURNS+0x2000),(BUFFER,BUFFER+2000),(ALIAS,ALIAS+2000),(METRIC_ALIAS,METRIC_ALIAS+2000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        self.read_pages={page:(low,high) for low,high in spans for page in range(low>>12,((high-1)>>12)+1)} # 실제 읽기 허용 범위는 바꾸지 않는 페이지별 경계다.

    def point(self,sid):
        """현재 raw 두 float만 읽어 Footprint 경계 입력으로 사용한다."""
        return struct.unpack('<2f',self.mu.mem_read(POOL+sid*50+14,8))

    def instruction(self,mu,address,size,data):
        """효과 인자만 기록하며 원본 본문의 분기/거리/정렬/Prepare는 대체하지 않는다."""
        self.instructions+=1
        if address in self.entries:self.calls[f'{address:08x}']+=1
        if address==0x4280a0:self.active_list=mu.reg_read(UC_X86_REG_ECX)
        if address not in self.boundaries:
            if address+size<=self.code_high.get(address,0):return
            raise RuntimeError(f'운반 후보 범위 밖 실행 {address:08x}')
        sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<6I',mu.mem_read(sp+4,24));sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;c=self.case
        if address==0x4ade70:
            self.event('R',sid,args[1]);self.footprints+=1
            if sid!=50:self.candidate=sid
            x,y=self.point(sid);mu.mem_write(args[0],struct.pack('<4f',x-0.5,y-0.5,x+0.5,y+0.5))
            if self.footprints==1:
                if c[11]==1:
                    self.put(POOL+50*50+14,bits(42.25));self.put(POOL+50*50+18,bits(43.5));self.put(POOL+60*50+14,bits(50.25));self.put(POOL+60*50+18,bits(51.5))
                if c[11]==2:self.put(POOL+50*50+10,137,1)
                if c[11]==7:self.put(self.active_list+8,2)
            self.ret(args[0],8);return
        if address==0x488af0:
            output=args[0];self.searches+=1;self.event('B',*args[1:]);assert self.get(output)==0 and bytes(mu.mem_read(output+0x9c48,40000))==bytes(40000)
            points,directions=route(c[14],self.candidate);self.put(output,len(points))
            # 입력 경로의 좌표/방향만 쓰며 실제 Prepare가 시각을 계산한다.
            for i,(point,direction) in enumerate(zip(points,directions)):
                mu.mem_write(output+4+i*8,struct.pack('<2f',*point));self.put(output+0x13888+i*4,direction)
            if self.searches==1 and c[11]==3:mu.mem_write(0x55b4d0,struct.pack('<d',13.75));self.put(0x5316ac,0);self.put(0x540bf8,0x7fa12345)
            if self.searches==1 and c[11]==9:self.put(POOL+50*50+10,156,1)
            self.ret();return
        if address in (0x4274b0,0x427580):
            is_return=address==0x4274b0;self.event('V' if is_return else 'G',sid,args[0]);self.validations+=1
            if self.validations==1 and c[11]==5:self.put(0x55a714,2);self.put(PLAYERS+c[4]*0xb4+0x7c,61)
            if not is_return and c[11]==6:self.put(0x5950e4,2)
            self.ret(int(bool(c[13]&(1<<(args[0]%3)))),4);return
        if address==0x462bc0:self.event('Q',args[0]);self.ret(7);return
        if address==0x4e4391:
            assert args[0]==2000;self.allocations+=1;mu.mem_write(c[15],b'\xa5'*2000);self.ret(c[15]);return
        if address==0x4e4354:assert args[0]==c[15];self.frees+=1;self.ret();return
        if address==0x4e0620:
            line=args[2];self.event('!',line);assert line in (0xace,0xb03) and self.string(args[1])=='\\Ns\\O\\Carrier.cpp'
            if c[11]==4:self.put(TYPES+121*500+0xe8,0)
            if c[11] in (4,8):self.put(0x540bf8,0x7fa12345)
            self.ret();return
        raise RuntimeError(f'운반 후보 미처리 경계 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제 본문은 stack/SEH/후보 배열/거리 WORD/임시 metric만 쓸 수 있다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==0 and size==4:return
        if self.case[15]<=address and address+size<=self.case[15]+2000:return
        if address==0x540bfc and size==4:return
        if POOL<=address and address+size<=POOL+24000*50 and (address-POOL)%50==8 and size==2:return
        raise RuntimeError(f'운반 후보 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """실제 PE/현재 raw·Player·TYPE/명시 목록/임시 배열/stack 외 읽기를 거부한다."""
        low,high=self.read_pages.get(address>>12,(0,0))
        if not low<=address or address+size>high:raise RuntimeError(f'운반 후보 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """명시 초기 상태와 실제 반환/효과/현재 전역/전체 raw Adler를 기록한다."""
        self.case=c;self.events=[];self.footprints=0;self.searches=0;self.validations=0;self.candidate=60;self.active_list=LIST;allocations=self.allocations;frees=self.frees
        self.mu.mem_write(INPUT,struct.pack('<19I',*c));self.mu.mem_write(POOL,self.initial_pool);self.mu.mem_write(PLAYERS,bytes(9*0xb4));self.put(PLAYERS+c[4]*0xb4+0xb0,c[5]);self.put(PLAYERS+c[4]*0xb4+0x7c,c[12]);self.put(0x59534c,PLAYERS);self.put(0x59ab20,TYPES);self.put(0x5c8464,POOL);self.put(0x5c847c,24000);self.put(0x5316ac,c[9]);self.put(0x5e4794,c[10]);self.put(0x540bf8,c[16]);self.put(0x540bfc,0x12345678);self.put(0,0x87654321);self.mu.mem_write(0x55b4d0,struct.pack('<d',10.5));self.mu.mem_write(TYPES,bytes(188*500))
        # 실제 TYPE/현재 getter와 원본 부모·후보의 물리 입력만 제공한다.
        for address,value in ((0x54127c,137),(0x5412c8,156),(0x541274,135),(0x541344,187)):self.put(address,value)
        for kind,speed,turn,genus in ((133,c[7],c[8],c[6]),(137,bits(2),bits(180),0),(156,bits(4),bits(45),0)):
            self.put(TYPES+kind*500+0xe0,speed);self.put(TYPES+kind*500+0xe4,turn);self.put(TYPES+kind*500+0xec,genus)
        for sid in [0,50,*range(60,572)]:
            raw=bytearray(b'\x2a'*50);raw[10]=133 if sid in (0,50) else 121 if sid==60 and c[1]==5 else 122;raw[11]=3 if c[1]==2 else 0;raw[34]=c[4] if sid in (0,50) else 2
            point=(20.25,21.5) if sid in (0,50) else (25+(sid-60)%8*3,20+(sid-60)//8*3)
            if c[17]:point=(point[0]*65536,point[1]*65536)
            struct.pack_into('<2f',raw,14,*point);struct.pack_into('<H',raw,8,(sid*31+111)&65535);self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(TYPES+121*500+0xe8,0x800);values=source_values(c);packed=struct.pack('<512I',*values);self.mu.mem_write(RETURNS,packed);self.mu.mem_write(GEYSERS,packed);self.put(0x55a70c,RETURNS);self.put(0x55a710,c[3]);self.put(0x55a714,c[2]);self.put(0x5950dc,GEYSERS);self.put(0x5950e0,c[3]);self.put(0x5950e4,c[2]);self.mu.mem_write(c[15]-16,b'\xa5'*16+packed[:2000]+b'\x5a'*16);self.mu.mem_write(LIST,struct.pack('<3I',c[15],c[3],c[2]))
        PreviousOracle.invoke(self,(THUNK,0x428ea0,0x429420)[c[0]],b'',precision,False);self.returns+=1
        assert self.get(0)==0x87654321 and self.allocations-allocations==self.frees-frees
        assert bytes(self.mu.mem_read(c[15]-16,16))==b'\xa5'*16 and bytes(self.mu.mem_read(c[15]+2000,16))==b'\x5a'*16
        state=(self.mu.reg_read(UC_X86_REG_EAX),self.get(0x540bfc),self.get(0x540bf8),self.get(0x5316ac),self.get(0x55a714),self.get(0x5950e4),self.get(PLAYERS+c[4]*0xb4+0x7c),self.footprints,self.searches,self.validations,zlib.adler32(bytes(self.mu.mem_read(POOL,24000*50)))&0xffffffff)
        return ';'.join(self.events) or '-',','.join(map(str,state)),bytes(self.mu.mem_read(c[15],c[2]*4)).hex() if c[0]==0 and c[2] else '-'


def main():
    """독립 관측과 읽기 전용 감사를 분리하고 현재 원문 입력/보호 SHA를 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text('utf-8').splitlines() if s and not s.startswith('#')]
        # 감사 모드는 원래 기대값을 덮어쓰지 않는다.
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)*2==report['normalReturns'];assert [i for i,r in enumerate(rows) if r[1:4]!=r[4:7]]==report['precisionDifferentInputs'];print('운반 후보 원본 감사:',len(rows),'입력/',report['normalReturns'],'정상 반환');return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # C++를 기대값 생성에 사용하지 않고 두 x87 제어어에서 실제 본문을 실행한다.
    for index,c in enumerate(cases()):
        outputs=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if outputs[0]!=outputs[1]:different.append(index)
        rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input effects53 state53 array53 effects64 state64 array64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_choice.py','tools/decomp_carrier_candidate_sort.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_nugget_regular.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-choice-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),allocations=sum(o.allocations for o in oracles),frees=sum(o.frees for o in oracles),precisionDifferentInputs=different,boundaries='428ea0/429420/4280a0/428020/실제 CRT정렬/41d530·400·ce90·cfc0·cd60/488e20/488fb0/TYPE·AI getter/ftol/stack probe 전체. Footprint/BFS/후보유효성/Graph/진단/2000바이트확보·해제는명시경계다. 호스트재생의미관측임시꼬리초기화/OOM은검증범위밖이다. GP/retN/SEH/canary/x87/명령·읽기·쓰기범위/전체raw Adler와효과순서를검사한다. 실제 게임창·자연완주검증은아니다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','allocations','frees','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
