#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fort 레지던스 owner 후처리·실제 시각 seed·AddRandom/local PRNG의 두 PE 정상 반환을 관찰한다."""
import argparse,collections,csv,itertools,json,struct
from pathlib import Path
from decomp_fortobjectload_oracle import FortOracle,DATA,ROOT,sha,TYPES,POOL,STOP,STACK
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import *

# 고유 fixture/근거와 격리 메모리의 목록·Player·local seed 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/fortfinishowner-x86.tsv'
REPORT=ROOT/'cpppj/recovery-fortfinishowner-evidence.json'
LIST=DATA+0x37000
PLAYERS=DATA+0x38000
LOCAL=DATA+0x36f00
KEYS=('owner','mode','fort','special','region','list','timeLow','timeHigh','rect','successAt','period','seed','type','quantity','force','mutation')
SPEC={
 'originals':dict(entry=0x40e1a0,seed=0x4bcab0,batch=0x40ded0,bounds=0x46f130,clock=0x460d70,place=0x40db90,diagnostic=0x4c2630,
    list=0x59a9d8,count=0x59a9e0,players=0x59534c,player_stride=0xb4,player_region=0x78,mode=0x594fa4,special=0x5c85a4,
    fort=0x594fb8,sentinel=0x50c038,type=0x5411bc,owner=34),
 'originalCD':dict(entry=0x42ca70,seed=0x4a5a10,batch=0x42c7a0,bounds=0x4bd8a0,clock=0x4011b0,place=0x42c3d0,diagnostic=0x48ca70,
    list=0x565ae0,count=0x565ae8,players=0x50f834,player_stride=0xac,player_region=0x70,mode=0x52e170,special=0x518904,
    fort=0x540a1c,sentinel=0x505eb0,type=0x51caa8,owner=32)}


def double_words(value):
    """double 시각 입력 비트다."""
    return struct.unpack('<II',struct.pack('<d',value))


def rectangles(profile):
    """유효·소수/영·음수 extent의 입력 사각형이다."""
    return [(32.0,48.0,96.0,112.0),(15.25,21.5,16.1,23.9),(-15.25,-3.5,4.5,9.5),
            (20.0,21.0,20.0,21.0),(20.0,21.0,19.0,19.0),(1.0,2.0,255.0,254.0)][profile]


def case(**changes):
    """기대값 없이 native 입력만 만든다."""
    low,high=double_words(2.5);c=dict(zip(KEYS,(1,0,0,0,3,0,low,high,0,1,1,0,89,1,0,0)));c.update(changes);return c


def nodes(profile,owner):
    """실제 residence 목록의 순서/타입/genus/state/owner 입력이다."""
    return [[],[(50,89,0,owner,0x40000000)],[(50,89,0,(owner+1)%9,0x40000000)],[(50,100,0,owner,0)],
            [(50,100,0,owner,0),(51,89,0,(owner+1)%9,0x40000000),(52,89,0,owner,0x40000000),(52,89,0,owner,0x40000000)],
            [(50,89,15,owner,0x40000000)]][profile]


def inputs():
    """초기 자연 반환/목록 판단·반복 생성·시각 seed와 하위 성공/1000회 경계를 각각 확인한다."""
    result=[]
    for owner,mode,fort,special,region,profile in itertools.product((0,1,2,8),(0,1),(0,1),(0,1),(3,127),range(6)):
        result.append(('F',case(owner=owner,mode=mode,fort=fort,special=special,region=region,list=profile)))
    for at,rect,mutation,time in itertools.product((2,5,1002),range(6),(0,1,2),(0.0,2.5)):
        low,high=double_words(time);result.append(('F',case(successAt=at,rect=rect,mutation=mutation,timeLow=low,timeHigh=high)))
    for at,period,quantity,rect,mutation in itertools.product((1,5,0xffffffff),(1,3),(0,1,3),range(6),(0,2)):
        result.append(('B',case(successAt=at,period=period,quantity=quantity,rect=rect,mutation=mutation,seed=0x12345678)))
    for typ in (0,0xffffffff):result.append(('B',case(type=typ)))
    for time in (0.0,0.001,0.1,1.0,2.5,123.456,-1.0,-30.25,1000000.125):
        low,high=double_words(time);result.append(('S',case(timeLow=low,timeHigh=high)))
    return result


class FinishOracle(FortOracle):
    """같은 PE 로더만 재사용하며 이번 작은 entry/seed/batch·local PRNG 실제 범위로 제한한다."""
    def __init__(self,edition):
        """고유 실제 export와 현재 상태의 명시 지역/시각/배치 경계를 준비한다."""
        super().__init__(edition);self.f=SPEC[edition];self.exports=[ROOT/f'extracted/fortfinishowner/{edition}/{n}' for n in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries={};self.native=collections.Counter();self.boundaries=collections.Counter()
        with self.exports[1].open(encoding='utf-8') as handle:
            for row in csv.DictReader(handle,delimiter='\t'):
                self.entries[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))

    def write(self,mu,access,address,size,value,data):
        """몸체의 계산 결과는 스택/local seed에만 쓴다. 전역/원본 pool은 읽기 전용이다."""
        if (0x30000000<=address and address+size<=0x30040000) or (LOCAL<=address and address+size<=LOCAL+4):return
        raise RuntimeError(f'finishOwner 미허용 쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """시각/지역/실제 배치/잘못된 타입 진단만 대체하고 전체 반복/난수/반환은 실제 명령이다."""
        f=self.f;c=self.c
        if address==f['clock']:
            output,=self.args(1);self.rounds+=1;value=self.time+(self.rounds-1)*0.125
            self.event('T',*double_words(value));mu.mem_write(output,struct.pack('<d',value));self.ret(output);return
        if address==f['bounds']:
            output,region=self.args(2);rect=rectangles(c['rect']);self.event('R',region,*map(bits,rect));mu.mem_write(output,struct.pack('<4f',*rect))
            if c['mutation']==1:self.put(f['type'],90)
            self.ret(0,8);return
        if address==f['place']:
            random,typ,x,y,region,mode,force=self.args(7);self.attempts+=1;self.seed_address=random
            self.event('P',typ,x,y,region,mode,force)
            if c['mutation']==2 and self.attempts==1:self.put(random,0x98765432)
            success=self.attempts>=c['successAt'] and (self.attempts-c['successAt'])%c['period']==0
            if success and self.mode=='F' and not self.added:
                self.added=True;sid=80;raw=bytearray(self.s['stride']);raw[10]=typ;raw[f['owner']]=c['owner'];mu.mem_write(self.slot(sid),bytes(raw))
                count=self.read(f['count']);self.put(LIST+count*4,sid);self.put(f['count'],count+1)
            self.ret(7 if success else 0);return
        if address==f['diagnostic']:
            self.event('D',*self.args(1));self.ret();return
        if address==f['seed']:self.seed_address=mu.reg_read(UC_X86_REG_ECX)
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'finishOwner 미허용 실행 {self.edition} {address:08x}')
        if address in self.entries:self.native[self.entries[address]]+=1

    def run(self,mode,c,control):
        """현재 residence 입력에서 실제 세 진입의 정상 반환·보존 레지스터/FPU/전체 효과/local seed를 읽는다."""
        mu=self.mu;s=self.s;f=self.f;self.mode,self.c=mode,c;self.events=[];self.rounds=self.attempts=0;self.added=False;self.seed_address=None
        self.time=struct.unpack('<d',struct.pack('<II',c['timeLow'],c['timeHigh']))[0]
        mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(POOL,bytes(0x200000));mu.mem_write(DATA,bytes(0x40000))
        self.put(s['types'],TYPES);self.put(s['pool'],POOL);self.put(f['players'],PLAYERS);self.put(f['list'],LIST);self.put(f['sentinel'],127);self.put(f['type'],c['type'])
        for key in ('mode','fort','special'):self.put(f[key],c[key])
        for owner in range(9):self.put(PLAYERS+owner*f['player_stride']+f['player_region'],c['region'])
        self.put(TYPES+89*s['type_stride']+0xec,0x40000000);self.put(TYPES+90*s['type_stride']+0xec,0x40000000)
        records=nodes(c['list'],c['owner']);self.put(f['count'],len(records))
        for index,(sid,typ,state,owner,genus) in enumerate(records):
            self.put(LIST+index*4,sid);self.put(self.slot(sid)+10,typ,1);self.put(self.slot(sid)+11,state,1);self.put(self.slot(sid)+f['owner'],owner,1);self.put(TYPES+typ*s['type_stride']+0xec,genus)
        ecx=0;entry=f['entry'];args=(c['owner'],)
        if mode=='B':
            rect=rectangles(c['rect']);self.put(LOCAL,c['seed']);entry=f['batch'];args=(LOCAL,*map(bits,rect),c['region'],c['quantity'],c['type'],c['force']);self.seed_address=LOCAL
        if mode=='S':entry=f['seed'];ecx=LOCAL;args=(1,)
        for reg,value in ((UC_X86_REG_EBX,0x11223344),(UC_X86_REG_ESI,0x22334455),(UC_X86_REG_EDI,0x33445566),(UC_X86_REG_EBP,0x44556677),
                (UC_X86_REG_ECX,ecx),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0)):mu.reg_write(reg,value)
        mu.mem_write(STACK,struct.pack(f'<{len(args)+1}I',STOP,*args));mu.emu_start(entry,STOP,count=3000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+(8 if mode=='S' else 4):raise RuntimeError('finishOwner 정상 반환 규약 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control:raise RuntimeError('finishOwner x87 규약 오류')
        for reg,value in ((UC_X86_REG_EBX,0x11223344),(UC_X86_REG_ESI,0x22334455),(UC_X86_REG_EDI,0x33445566),(UC_X86_REG_EBP,0x44556677)):
            if mu.reg_read(reg)!=value:raise RuntimeError('finishOwner 보존 레지스터 오류')
        items=[self.read(LIST+i*4) for i in range(self.read(f['count']))];listing=','.join(map(str,items)) or '-'
        result=0 if mode=='F' else mu.reg_read(UC_X86_REG_EAX) if mode=='B' else self.read(LOCAL)
        seed=self.read(self.seed_address) if self.seed_address is not None else 0
        return ';'.join(self.events) or '-',str(result),str(seed),listing,str(self.read(f['type']))


def generate():
    """각 고유 입력을 두 정밀도에서 정상 반환하여 같은 native 결과만 저장한다."""
    rows=[];reports={};files={Path(__file__),FIXTURE,ROOT/'tools/ghidra/fortfinishowner-functions.json',ROOT/'cpppj/src/o/RawFortFinishOwner.h',ROOT/'cpppj/src/o/RawFortFinishOwner.cpp',ROOT/'tools/decomp_fortobjectload_oracle.py'}
    for edition in SPEC:
        oracle=FinishOracle(edition)
        for mode,c in inputs():
            output=[oracle.run(mode,c,control) for control in CONTROLS];assert output[0]==output[1],(edition,mode,c)
            rows.append('\t'.join((edition,mode,','.join(str(c[k]) for k in KEYS),*output[0])))
        reports[edition]=dict(inputs=len(inputs()),native=dict(oracle.native),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
        print(f'{edition} finishOwner/seed/AddRandom {len(inputs())}개 정상 반환',flush=True)
    FIXTURE.write_text('# 판본 모드 입력 effects result localSeed residences currentType\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='VM-W11-CODEX',date='2026-10-13',total=len(rows),editions=reports,
        files={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(files)},limits=['실제 owner entry·현재 residence 전체 순회·시각 seed·AddRandom/local PRNG/1000회 경계 실행',
        '실제 지역 경계/현재 wall time/개별 생성·Pop·postPop 전체와 진단 출력은 필수 명시 경계','게임/GUI/실제 캠페인 완주 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """현재 source/PE/export/fixture SHA와 세 실제 진입·저장 input 수를 감사한다."""
    r=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in r['files'].items():assert sha(ROOT/name)==value,name
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==r['total']==2*len(inputs());assert all(len(row)==8 for row in rows)
    for edition,f in SPEC.items():assert all(r['editions'][edition]['native'][f'{f[key]:08x}']>0 for key in ('entry','seed','batch'))
    print(f'Fort finishOwner/시각 seed/반복 배치 SHA·입력 {len(rows)}개 감사 통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');o=parser.parse_args();verify() if o.verify else generate()
