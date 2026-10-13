#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 보호 PE의 Path 생성자/시각/Step/프레임/도착을 격리 x86으로 실행한다. 게임/GUI 실행은 없다."""
import argparse
import collections
import csv
import json
import struct
from decomp_reward_oracle import RewardOracle,ROOT,ARENA,ARENA_SIZE,POOL,TYPES,VTABLE,SCRATCH,STACK,STOP,digest,bits
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 격리 Path 이미지와 출력/대체 가상 함수 주소다. 원본 파일의 주소와 섞지 않는다.
PATH=ARENA+0x20000
PARENT=5
SOUND=0x13000100
HALT,ARRIVAL,UNPOP,POP,REPOP=0x13000200,0x13000210,0x13000220,0x13000230,0x13000240
# 두 판본의 실제 생성자 및 경로 함수/배열 위치다.
SPEC={
 'originals':dict(ctor=(0x48be40,0x48bf60),prepare=0x488e20,step=0x48a330,run=(0x48a950,0x48bb40),arrive=(0x48b340,0x48bd90),halt=0x489ff0,
   size=0x186f0,capacity=5000,times=0x9c58,directions=0x13898,current=0x186c0,angle=0x186c4,finished=0x186c8,dead=0x186ec,vtables=(0x50e284,0x50e458),
   now=0x55b4d0,observe=(0x5951c0,0x5951b8),sound=0x4a9c30,malloc=0x4e4391,assertion=0x4e0620,typeptr=0x59ab20,clock=None,kill=0x41bf10,freeze=0x4287a0,boss=0x540bc4,capacityGlobal=0x5c847c,spots=0x5c7c44),
 'originalCD':dict(ctor=(0x47f310,0x47fcf0),prepare=0x402590,step=0x47e820,run=(0x47efb0,0x4805d0),arrive=(0x47e220,0x480940),halt=0x47e4d0,
   size=0x2f28,capacity=600,times=0x12d8,directions=0x2598,current=0x2f00,angle=0x2f04,finished=0x2f08,dead=0x2f24,vtables=(0x503d60,0x503eb0),
   now=0x5484a8,observe=(0x567438,0x567430),sound=0x438ae0,malloc=0x4f1650,assertion=0x490040,typeptr=0x51c960,clock=0x4f1560,kill=0x48f7b0,freeze=0x4e46b0,boss=0x540a2c,capacityGlobal=0x5395f4,spots=0x52fe48),
}
FIXTURE=ROOT/'cpppj/tests/fixtures/pathruntime-x86.tsv'
EVIDENCE=ROOT/'cpppj/pathruntime.json'
# 독립 코드/설계/검사와 정밀 내보내기 SHA를 함께 보존한다.
SOURCES=['tools/decomp_pathruntime_oracle.py','tools/decomp_reward_oracle.py','tools/ghidra/pathruntime-functions.json','tools/ghidra/pathruntimehelpers-functions.json','tools/ghidra/pathmath-functions.json',
 'cpppj/src/o/RawPathRuntime.h','cpppj/src/o/RawPathRuntime.cpp','cpppj/src/o/RawPathProcess.h','cpppj/src/o/RawPathProcess.cpp','cpppj/tests/RawPathRuntimeTests.cpp']


def dbits(value):
    """double 입력/관찰을 QWORD 비트로 보존한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def fnv(data):
    """원본 생성자가 남긴 전체 byte 이미지의 독립 체크섬이다."""
    value=2166136261
    # 초기화하지 않은 byte도 포함해 원본의 보존 범위를 대조한다.
    for byte in data:value=((value^byte)*16777619)&0xffffffff
    return value


class PathOracle(RewardOracle):
    """PE 로더만 재사용하며 실행/쓰기 목록은 Path의 고유 정밀 몸체로 다시 지정한다."""
    def __init__(self,edition):
        """알려진 실제 body와 외부 가상 효과/진단 시계 경계만 허용한다."""
        super().__init__(edition);self.m=SPEC[edition];self.allowed=[];self.entry_names={};self.mu.mem_map(0,0x1000)
        self.mu.mem_map(ARENA+0x40000,0x20000)
        # 두 고유 export의 모든 함수 범위를 읽되 실제 호출된 것만 근거에 센다.
        for name in ('pathruntime','pathruntimehelpers','pathmath'):
            with (ROOT/f'extracted/{name}/{edition}/functions.tsv').open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream,delimiter='\t'):
                    self.entry_names[int(row['entry'],16)]=row['entry']
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.external={self.m['malloc'],self.m['sound'],self.m['assertion'],self.m['kill'],self.m['freeze'],HALT,ARRIVAL,UNPOP,POP,REPOP}
        if self.m['clock']:self.external.add(self.m['clock'])
        self.writes=[(ARENA,ARENA+0x60000),(STACK-0xf000,STACK+0x1000),(0,4),(self.m['now'],self.m['now']+8),
                     (self.m['observe'][0],self.m['observe'][0]+4),(self.m['observe'][1],self.m['observe'][1]+8)]
        self.events=[];self.return_code=1002;self.halt_code=10;self.deleted=False;self.frozen=False

    def on_instruction(self,mu,address,size,data):
        """실제 계산은 원본이 수행하며 가상 공간/할당/소멸/오디오 요청을 입력 경계에서 기록한다."""
        if address not in self.external:return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret=self.u32(sp);result=0;pop=0
        if address==self.m['malloc']:
            assert self.u32(sp+4)==self.m['size'];result=PATH
        elif address==self.m['assertion']:
            raise RuntimeError(f'원본 assert {self.binary}: {self.u32(sp+4):08x}')
        elif address==self.m['sound']:
            self.events.append(('sound',self.u32(sp+4),self.u32(sp+8),self.u32(sp+12)))
        elif address==self.m['clock']:result=0x12345678
        elif address==self.m['freeze']:result=int(self.frozen)
        elif address==HALT:result=self.halt_code;self.events.append(('halt',result))
        elif address==ARRIVAL:
            pop=8;result=self.return_code;self.events.append(('arrival',self.u32(sp+4),self.u32(sp+8),result))
            if result==1001:self.deleted=True;self.mark_dead()
        elif address==self.m['kill']:
            pop=4;self.deleted=True;self.mark_dead();self.events.append(('kill',self.u32(sp+4)))
        elif address==UNPOP:pop=4;self.events.append(('unpop',self.u32(sp+4)))
        elif address==REPOP:pop=4;self.events.append(('repop',self.u32(sp+4)))
        elif address==POP:
            pop=12;x,y,flags=(self.u32(sp+v) for v in (4,8,12));self.events.append(('pop',x,y,flags))
            raw=POOL+PARENT*self.s['stride'];self.put(raw+14,x);self.put(raw+18,y)
        self.stub_hits[hex(address)]+=1;mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+pop);mu.reg_write(UC_X86_REG_EIP,ret)

    def mark_dead(self):
        """원본 destructor가 쓰는 현재 stack deadFlag를 실제 주소에 반영한다."""
        pointer=self.u32(PATH+self.m['dead'])
        if pointer:self.put(pointer,1)

    def invoke(self,entry,args=(),this=0):
        """ret N의 실제 thiscall을 실행하고 스택 종료를 cdecl 검사에 맞춰 관찰한다."""
        # 기존 호출 도구는 cdecl만 검사하므로 각 thiscall의 실제 ret 폭에 맞춘 작은 종료 tramp를 쓴다.
        original=self.mu.mem_read(STOP,8)
        amount=len(args)*4
        self.mu.mem_write(STOP,b'\x83\xec'+bytes([amount])+b'\x90')
        # ret N을 반환한 뒤 STOP에서 ESP를 읽을 수 있도록 자체 실행한다.
        import unicorn.x86_const as reg
        for register in (reg.UC_X86_REG_EAX,reg.UC_X86_REG_EBX,reg.UC_X86_REG_ECX,reg.UC_X86_REG_EDX,reg.UC_X86_REG_ESI,reg.UC_X86_REG_EDI,reg.UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(reg.UC_X86_REG_ECX,this);self.mu.reg_write(reg.UC_X86_REG_ESP,STACK);self.mu.reg_write(reg.UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(reg.UC_X86_REG_FPCW,0x027f);self.mu.reg_write(reg.UC_X86_REG_FPSW,0)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.mu.emu_start(entry,STOP,timeout=30000000,count=2000000)
        assert self.mu.reg_read(reg.UC_X86_REG_EIP)==STOP
        expected=STACK+4+(amount if this else 0)
        assert self.mu.reg_read(reg.UC_X86_REG_ESP)==expected,(self.binary,hex(entry),hex(self.mu.reg_read(reg.UC_X86_REG_ESP)),hex(expected))
        assert self.mu.reg_read(reg.UC_X86_REG_FPSW)&0x3800==0,(self.binary,hex(entry),'x87')
        self.mu.mem_write(STOP,bytes(original));return self.mu.reg_read(reg.UC_X86_REG_EAX)

    def reset(self,priest=0,fill=0):
        """실제 생성자 및 raw 부모/타입/표면에 명시 합성 입력을 준비한다."""
        self.mu.mem_write(ARENA,bytes(0x60000));self.mu.mem_write(PATH,bytes([fill])*self.m['size']);self.events=[];self.deleted=False
        self.invoke(self.m['ctor'][priest]);self.put(PATH+4,7);self.put(PATH+8,PARENT)
        self.put(self.s['pool_ptr'],POOL);self.put(self.m['capacityGlobal'],100)
        self.put(self.m['typeptr'],TYPES);self.put(self.m['boss'],1);self.put(self.m['spots'],ARENA+0x40000)
        self.mu.mem_write(ARENA+0x40000,bytes([6])*65536)
        raw=POOL+PARENT*self.s['stride'];self.put(raw,VTABLE);self.put(raw+6,7,2);self.put(raw+10,158 if priest else 134,1)
        self.put(raw+14,bits(10));self.put(raw+18,bits(20))
        form=POOL+7*self.s['stride'];self.put(form+10,54 if priest else 23,1)
        type_=TYPES+(158 if priest else 134)*self.s['type_stride'];self.put(type_+0x6c,321)
        # 가상 표는 실제 원본 Path 표를 복사하고 부모의 외부 효과만 격리 경계로 치환한다.
        self.put(VTABLE+0x38,HALT);self.put(VTABLE+0x3c,ARRIVAL);self.put(VTABLE+0x48,UNPOP);self.put(VTABLE+0x90,POP);self.put(VTABLE+0x4c,REPOP)
        self.put(self.m['now'],dbits(0),8)
        if self.binary=='originalCD':self.put(0x546778,0)
        self.events=[]

    def path(self,points,directions,times):
        """원본 경로 배열 입력을 계산 없이 쓴다."""
        self.put(PATH+0x10,len(points))
        # 방향/꼭짓점/시각 배열을 원본 레이아웃으로 배치한다.
        for index,((x,y),direction,time) in enumerate(zip(points,directions,times)):
            self.put(PATH+0x14+index*8,bits(x));self.put(PATH+0x18+index*8,bits(y));self.put(PATH+self.m['directions']+index*4,direction);self.put(PATH+self.m['times']+index*8,dbits(time),8)

    def animation(self):
        """현재 타입의 실제 SHP/글자 표 입력을 준비한다. 프레임 결정은 원본 RunFrame이 수행한다."""
        raw=POOL+PARENT*self.s['stride'];type_=TYPES+self.u32(raw+10)%256*self.s['type_stride'];shape=ARENA+0x18000;codes=ARENA+0x19000
        self.put(type_+0xdc,shape);self.put(type_+0x114,16);self.put(type_+0x124,codes)
        # 각 방향에 두 프레임이 있으며 실제 cellHotspot은 0.3,0.2다.
        for index in range(16):
            self.put(codes+index*4,ord('A')+index//2,1);self.put(codes+index*4+2,index%2+1,1)
            header=shape+0x100+index*0x40;self.put(shape+8+index*8,header-shape+0x24);self.put(header+0x10,bits(0.3));self.put(header+0x14,bits(0.2))
        # 글자 구간 표는 원본 type 생성 결과를 입력으로만 제공한다.
        for direction in range(8):self.put(type_+0x130+direction*4,direction*2);self.put(type_+0x170+direction*4,2)

    def observed(self):
        """프레임/도착의 실제 필드와 요청 순서를 관찰한다. 기대 동작을 다시 계산하지 않는다."""
        raw=POOL+PARENT*self.s['stride'];frame=self.u32(raw+36) if self.patch else self.mu.mem_read(raw+34,1)[0]
        fields=[self.u32(raw+14),self.u32(raw+18),frame,self.u32(PATH+self.m['current']),self.u32(PATH+self.m['angle']),self.u32(PATH+self.m['finished']),int(self.deleted)]
        return ','.join(map(str,fields)),';'.join(','.join(map(str,event)) for event in self.events) or '-'


def generate():
    """기대값을 구현으로 만들지 않고 원본 코드의 결과 bit·호출 순서만 fixture에 기록한다."""
    rows=[];counts=collections.Counter();hits=collections.Counter();stubs=collections.Counter()
    for edition in SPEC:
        oracle=PathOracle(edition);m=oracle.m
        # 두 실제 ctor와 heap 보존 입력을 교차한다.
        for priest in (0,1):
            for fill in (0,0xa5):
                oracle.mu.mem_write(PATH,bytes([fill])*m['size']);oracle.invoke(m['ctor'][priest])
                rows.append(['ctor',edition,priest,fill,fnv(oracle.mu.mem_read(PATH,m['size']))]);counts['ctor']+=1
        # 회전/긴 거리/경계 startTime 및 float reciprocal 저장 차이를 교차한다.
        for speed in (0.7,1.0,1.8,13.3):
            for turning in (17.5,90.0):
                for start in (-2.000001,-2.0,-1.0):
                    oracle.reset();points=[(10,20),(13.1,24.7),(15.2,19.8),(23.9,27.6)];directions=[7,0,4,6];oracle.path(points,directions,[0]*4)
                    lo,hi=struct.unpack('<2I',struct.pack('<d',start));oracle.invoke(m['prepare'],[lo,hi,bits(speed),bits(turning),7],PATH+0x10)
                    result=[struct.unpack('<Q',oracle.mu.mem_read(PATH+m['times']+i*8,8))[0] for i in range(4)]
                    rows.append(['prepare',edition,dbits(0),dbits(start),bits(speed),bits(turning),','.join(map(str,result))]);counts['prepare']+=1
        # 시간별 호출 연쇄로 꼭짓점 하나씩만 진행하는 실제 Step과 짝수 발소리를 관찰한다.
        for profile in range(8):
            oracle.reset();directions=[profile,(profile+1)%8,(profile+5)%8,profile];points=[(10,20),(13.1,24.7),(15.2,19.8),(23.9,27.6)]
            oracle.path(points,directions,[0,1,2,3])
            for now in (-0.1,0,0.25,0.5,1,1.5,2,2.5,3,10):
                oracle.put(m['now'],dbits(now),8);before=oracle.u32(PATH+m['current']);old_angle=oracle.u32(PATH+m['angle']);oracle.events=[]
                result=oracle.invoke(m['step'],[SCRATCH,SCRATCH+8],PATH)
                values=[oracle.u32(SCRATCH),oracle.u32(SCRATCH+4),oracle.u32(SCRATCH+8),oracle.u32(PATH+m['current']),oracle.u32(PATH+m['angle']),result,len(oracle.events)]
                rows.append(['step',edition,profile,dbits(now),before,old_angle,','.join(map(str,values))]);counts['step']+=1
        # 실제 RunFrame은 원본 Step/Halt/프레임 몸체를 호출한다. 공간/부모 가상 효과는 위 명시 경계다.
        for priest in (0,1):
            for current in (0,1,2):
                for now in (0.0,0.5,1.0,2.0,3.0):
                    for angle in (0.0,23.5):
                        oracle.reset(priest);oracle.animation();oracle.path([(10,20),(15,20),(17,22)],[0,0,2],[0,1,3])
                        oracle.put(m['current']+PATH,current);oracle.put(PATH+m['angle'],bits(angle));oracle.put(m['now'],dbits(now),8)
                        oracle.return_code=1002;oracle.halt_code=10;oracle.frozen=False
                        oracle.invoke(m['run'][priest],this=PATH);fields,events=oracle.observed()
                        rows.append(['frame',edition,priest,current,dbits(now),bits(angle),fields,events]);counts['frame']+=1
        # 1000 삭제/1001 callback 자기 삭제/1002 재시도와 Halt11의 실제 분배를 대조한다.
        for priest in (0,1):
            for result in (1000,1001,1002):
                for halt in (10,11):
                    oracle.reset(priest);oracle.animation();oracle.path([(10,20),(15,20),(17,22)],[0,0,2],[0,1,3])
                    oracle.put(PATH+m['current'],3);oracle.put(m['now'],dbits(3),8);oracle.return_code=result;oracle.halt_code=halt;oracle.frozen=False
                    oracle.invoke(m['arrive'][priest],this=PATH);fields,events=oracle.observed()
                    rows.append(['arrival',edition,priest,result,halt,fields,events]);counts['arrival']+=1
        hits.update(oracle.hits);stubs.update(oracle.stub_hits)
        print(f'{edition} ctor/timing/Step/Frame/도착 원본 실행 완료',flush=True)
    FIXTURE.write_text('# kind edition input... observed\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files=list(SOURCES)
    for edition in SPEC:
        files.append(f'{edition}/{"Netstorm.exe" if edition=="originals" else "NETSTORM.EXE"}')
        for name in ('pathruntime','pathruntimehelpers','pathmath'):files.extend([f'extracted/{name}/{edition}/creation.c',f'extracted/{name}/{edition}/functions.tsv'])
    report=dict(host='VM-W11-CODEX',date='2026-10-13',cases=len(rows),kinds=dict(counts),entries=dict(hits),external_calls=dict(stubs),fixture_sha256=digest(FIXTURE),files={path:digest(ROOT/path) for path in files},
        limits=['heap 할당/음향/진단 시계·부모 Halt/Arrival·공간 요청·삭제·Regular freeze 조회만 명시 대체한다. 계산/분배/CRT fmod는 실제 기계어다.',
                'BFS/transport 재탐색 및 수명 정리 하위 몸체는 이 근거의 검증 범위에 포함하지 않는다.'])
    EVIDENCE.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """보호 PE/정밀 함수/새 구현/입력 개수와 fixture SHA를 감사한다."""
    report=json.loads(EVIDENCE.read_text('utf-8'));assert digest(FIXTURE)==report['fixture_sha256']
    for path,expected in report['files'].items():assert digest(ROOT/path)==expected,path
    rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['cases']==360;assert collections.Counter(row[0] for row in rows)==report['kinds']==dict(ctor=8,prepare=48,step=160,frame=120,arrival=24)
    # 판본별 180개와 입력의 유일성을 확인한다. 반복 원본 실행은 새 입력으로 세지 않는다.
    assert collections.Counter(row[1] for row in rows)==dict(originals=180,originalCD=180)
    assert len(set(tuple(row[:-1]) for row in rows))==360
    print(f'Path native {len(rows)}개 SHA/입력 감사 통과')


def main():
    """생성 또는 기존 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()


if __name__=='__main__':main()
