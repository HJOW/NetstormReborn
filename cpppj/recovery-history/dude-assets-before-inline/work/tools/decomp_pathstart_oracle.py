#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 이동 시작·flags0 Init·BFS·Prepare·사제 가이저 finder/이벤트 취소를 두 PE에서 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle, ROOT, POOL, HEAP, MAP, STACK_BASE, VTABLE, bits, sha
from decomp_bridgedecay_oracle import TYPES, SPOTS, CODES, FPU_CONTROLS, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP
from decomp_pathruntime_oracle import SPEC as RUNTIME, dbits
from decomp_pathsearch_oracle import SPEC as SEARCH

# Path·구 경로·BFS 작업 배열과 관찰 경계 주소는 에뮬레이터에서만 존재한다.
PATH, OLD, DISTANCE, QUEUE = 0x11400000, 0x11420000, 0x11440000, 0x11468000
ATTACH, DISPOSE, POP = HEAP+0xf000, HEAP+0xf010, HEAP+0xf020
FIXTURE = ROOT/'cpppj/tests/fixtures/pathstart-x86.tsv'
TYPE_FIXTURE = ROOT/'cpppj/tests/fixtures/pathprocesstype-x86.tsv' # 각 타입/genus의 실제 종류 선택과 CD assert 관찰이다.
REPORT = ROOT/'cpppj/recovery-pathstart-evidence.json'
# 원본 mutable 전역·가상 사제 통지·상위 몸체와 명시 효과 경계다.
SPEC = {
 'originals':dict(start=0x489080,choose=0x488fe0,notify=0x427750,finish=0x493f20,lookup=0x4b1a90,fromform=0x4ae2d0,event=0x4afb40,frame=0x4acee0,clear=0x443800,
  snap=0x595180,retry=0x5951b0,dude=0x541170,teleport=0x541270,priest=0x5412d0,mana=0x5411fc,geyser=0x541240,empty=0x541344,prof=0x460d70),
 'originalCD':dict(start=0x47d150,choose=0x47d020,notify=0x4e6e70,finish=0x40c510,lookup=0x4eb310,fromform=0x4af4e0,event=0x4ac890,frame=0x4acbc0,clear=0x4d2450,
  snap=0x52da98,retry=None,dude=0x51ca5c,teleport=0x51cb5c,priest=0x51cbbc,mana=0x51cae8,geyser=0x51cb2c,empty=None,prof=0x4011b0),
}


def inputs():
    """정상/실패/기존 명령·반복 취소·소수 스냅·권한 입력을 교차한다. 결과는 계산하지 않는다."""
    return list(itertools.product((134,158),range(8),(0,1),(0,1),(0,1),(0,2)))


class StartOracle(FinderOracle):
    """기존 실제 네 단계 finder에 Path의 고유 몸체만 추가하고 외부 효과 경계는 명시한다."""
    def __init__(self,edition):
        """PE·원본 함수 범위·격리 배열을 준비한다. 파일의 원본 코드를 수정하지 않는다."""
        super().__init__(edition);self.patch=edition=='originals';self.p=SPEC[edition];self.r=RUNTIME[edition];self.q=SEARCH[edition]
        self.stubs={};self.entries={};self.returns=0;self.mu.mem_map(PATH,0x80000)
        # 정밀 내보낸 불연속 범위를 합치고 실제 진입점마다 호출을 기록한다.
        for name in ('pathstart','pathstarthelpers','pathruntime','pathruntimehelpers','pathmath','pathsearch'):
            manifest=ROOT/f'tools/ghidra/{name}-functions.json';self.body_paths.append(manifest)
            paths=[ROOT/f'extracted/{name}/{edition}/{leaf}' for leaf in ('creation.c','functions.tsv')]
            selected={int(value,16) for value in json.loads(manifest.read_text(encoding='utf-8'))['editions'][edition]}
            if not paths[1].exists():paths=[ROOT/f'extracted/refined/{edition}/{leaf}' for leaf in ('raw.c','functions.tsv')]
            self.body_paths.extend(paths)
            with paths[1].open(encoding='utf-8') as stream:
                # 전체 PE 허용 없이 함수의 body 범위만 추가한다.
                for row in csv.DictReader(stream,delimiter='\t'):
                    if int(row['entry'],16) not in selected:continue
                    self.entries[int(row['entry'],16)]=f"{int(row['entry'],16):08x}"
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        # 실패 발자국 사각형의 최소/최대 확장은 최신 전체 정밀 결과의 두 작은 실제 몸체를 읽는다.
        if self.patch:
            table=ROOT/f'extracted/refined/{edition}/functions.tsv';self.body_paths.extend((table,ROOT/f'extracted/refined/{edition}/raw.c'))
            with table.open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream,delimiter='\t'):
                    if int(row['entry'],16) not in (0x40e800,0x40e880,0x40e8f0,0x41d0d0):continue
                    self.entries[int(row['entry'],16)]=f"{int(row['entry'],16):08x}"
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        self.boundaries={self.r['malloc']:'allocate',self.r['kill']:'kill',self.p['lookup']:'oldLookup',self.p['fromform']:'oldProcess',self.p['event']:'eventLookup',self.p['frame']:'frame',self.p['clear']:'clear',self.p['prof']:'profilingClock',ATTACH:'attach',DISPOSE:'dispose',POP:'pop'}

    def read(self,address,width=4):
        """관찰한 필드를 little endian 정수로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """입력과 명시 효과 필드만 지정 폭으로 쓴다."""
        self.mu.mem_write(address,(value&((1<<(8*width))-1)).to_bytes(width,'little'))

    def on_write(self,mu,access,address,size,value,data):
        """실제 body의 쓰기를 Path·작업 배열·스택·해당 전역으로 제한한다."""
        if PATH<=address and address+size<=PATH+0x80000:return
        if self.q['mask']<=address and address+size<=self.q['mask']+32:return
        if self.p['retry'] and self.p['retry']<=address and address+size<=self.p['retry']+4:return
        return super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """몸체는 실제 실행하며 힙/Host/삭제/Frame/명시 시계의 인자와 현재 상태만 관찰한다."""
        if address==self.spec['assert_report']:
            sp=mu.reg_read(UC_X86_REG_ESP);self.assertions+=1
            raise RuntimeError(f'원본 assert 도달: {self.edition} {self.read(sp+4):08x}/{self.read(sp+8):08x}/{self.read(sp+12)}')
        if address in self.entries:self.native_calls[self.entries[address]]+=1
        boundary=getattr(self,'boundaries',{}).get(address)
        if not boundary:return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);ret=self.read(sp);args=[self.read(sp+4*i) for i in range(1,5)];purge=0;result=0
        self.stub_calls[boundary]+=1
        if boundary=='allocate':
            assert args[0]==self.r['size'];self.events.append('C');result=PATH
        elif boundary=='oldLookup':result=77 if self.legacy else 0
        elif boundary=='oldProcess':result=OLD
        elif boundary=='kill':
            assert args[0]==0;purge=4
            if this==OLD:
                self.events.append('K');self.legacy=False
                # 기존 Path 삭제의 공간 접두는 Unpop→Repop이다. 그 결과 부모는 현재 위치에서 live 상태다.
                raw=self.slot(5);self.put(raw+11,self.read(raw+11,1)&~4,1)
            else:
                event=self.event_objects.pop(this);self.pending[event]-=1;self.events.append('E:'+str(event))
        elif boundary=='eventLookup':
            purge=4;event=args[0]
            if self.pending.get(event,0):result=HEAP+0xb000+event;self.event_objects[result]=event
        elif boundary=='frame':
            assert args[1]==0;purge=8;self.events.append('M:'+str(args[0]));self.put(this+self.o['frame'],args[0],self.o['frame_size'])
        elif boundary=='clear':
            assert args[2]==self.parent_type;self.events.append('B:'+','.join(map(str,args)))
        elif boundary=='profilingClock':self.mu.mem_write(args[0],bytes(8));result=args[0]
        elif boundary=='attach':
            assert args[:2]==[5,0];purge=12;self.events.append('A:'+str(args[2]));self.put(PATH+4,777);self.put(PATH+8,5);result=PATH
        elif boundary=='dispose':assert args[0]==1;purge=4;self.events.append('D')
        elif boundary=='pop':
            purge=12;self.events.append('P:'+','.join(map(str,args[:3])));self.put(this+14,args[0]);self.put(this+18,args[1]);self.put(this+11,self.read(this+11,1)&~4,1)
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,case,control=FPU_CONTROLS[0]):
        """실제 ctor→Init→BFS→명령 필드→Prepare→사제 finder/취소의 결과를 읽는다."""
        typ,profile,boss,legacy,replace,pending=case;self.parent_type=typ;self.legacy=bool(legacy);self.pending={604:pending,605:pending};self.event_objects={};self.events=[]
        self.mu.mem_write(PATH,bytes(0x80000));self.mu.mem_write(POOL,bytes(100*self.stride));self.mu.mem_write(TYPES,bytes(256*self.type_stride));self.mu.mem_write(SPOTS,bytes(65536))
        objects=[];sid=6
        # 같은 표면 Graph3와 프레임AA를 실제 BFS가 읽는다.
        for y in range(9,15):
            for x in range(9,15):
                objects.append((sid,83,0,0,bits(x),bits(y),1,1,0,0));sid+=1
        objects.extend([(50,122,0,0,bits(13.75),bits(13.25),1,1,0,1),(51,122,0,8,bits(13.75),bits(13.25),1,1,0,1),(52,122,0,0,bits(13.5),bits(13.5),1,1,0,1)])
        if profile==6:objects.append((53,150,0,0,bits(10.75),bits(10.25),1,1,0,1))
        self.setup(objects)
        for sid in range(6,42):self.put(self.slot(sid)+self.o['graph'],3,1)
        self.put(TYPES+83*self.type_stride+0xe8,0x800);self.put(TYPES+83*self.type_stride+0xec,2);self.put(TYPES+83*self.type_stride+0x114,1);self.put(TYPES+83*self.type_stride+0x124,CODES);self.mu.mem_write(CODES,b'AA\x01\x00')
        for y in range(9,15):
            for x in range(9,15):self.put(SPOTS+y*256+x,6,1)
        raw=self.slot(5);self.put(raw,VTABLE);self.put(raw+10,typ,1);self.put(raw+11,4 if profile==4 else 0,1);self.put(raw+14,bits(9.2));self.put(raw+18,bits(9.3));self.put(raw+(34 if self.patch else 32),2,1)
        self.put(TYPES+typ*self.type_stride+self.foot_offset,1);self.put(TYPES+typ*self.type_stride+self.foot_offset+4,1)
        self.put(TYPES+typ*self.type_stride+0xe0,bits(0.7));self.put(TYPES+typ*self.type_stride+0xe4,bits(90));self.put(TYPES+typ*self.type_stride+0x118,7)
        self.put(self.r['boss'],boss);self.put(self.r['now'],dbits(123.125),8);self.put(self.p['snap'],profile==1);self.put(self.p['geyser'],122)
        if self.p['retry']:self.put(self.p['retry'],profile==7)
        if profile==6:self.put(TYPES+150*self.type_stride+0xec,0x40000)
        self.put(self.q['work'],DISTANCE);self.put(self.q['queue'],QUEUE);self.put(self.q['enabled'],1);self.put(self.q['avoid'],1);self.put(self.q['graphs'],1)
        if self.q['policy']:self.put(self.q['policy'],6)
        self.put(self.q['graphbase'],HEAP)
        for i,v in enumerate((0,5,0,20,0,80,0,65)):self.put(self.q['mask']+i*4,v)
        for i,v in enumerate((0,2,4,6,1,3,5,7)):self.put(self.q['order']+i*4,v)
        dest=(13.2,13.3)
        if profile in (2,6,7):
            self.put(MAP+(10*256+10)*2,0,2);self.put(MAP+(9*256+9)*2,0,2)
        if profile==3:dest=(-1.25,400.5)
        if profile==5:dest=(9.2,9.3)
        self.put(OLD+(0x186cc if self.patch else 0x2f0c),0x87654321);self.put(OLD+(0x186d0 if self.patch else 0x2f10),0xfedcba98)
        for i,ctor in enumerate(self.r['ctor']):
            self.put(TYPES+(23 if i==0 else 54)*self.type_stride+(0x1d0 if self.patch else 0x1b0),ctor)
            table=self.r['vtables'][i];self.put(table,DISPOSE);self.put(table+0x2c,ATTACH)
        self.put(VTABLE+0x90,POP);self.put(VTABLE+0x44,self.p['notify'] if typ==158 else (0x4437f0 if self.patch else 0x401c30));self.put(VTABLE+0x40,self.p['finish'] if typ==158 else (0x480260 if self.patch else 0x4ac480))
        self.spec=dict(self.spec,entries=dict(self.spec['entries'],Start=self.p['start']));self.call('Start',[5,bits(dest[0]),bits(dest[1]),replace,0x12345678,0x87654321,0xa5fedcba,0x13579bdf],control=control);self.returns+=1
        result=self.mu.reg_read(UC_X86_REG_EAX);count=self.read(PATH+0x10);points=[]
        for i in range(count):points.append(','.join(map(str,(self.read(PATH+0x14+i*8),self.read(PATH+0x18+i*8),self.read(PATH+self.r['directions']+i*4),self.read(PATH+self.r['times']+i*8,8)))))
        fields=[self.read(PATH+off) for off in ((0x186cc,0x186d0,0x186d4,0x186e4,0x186e8,0x186dc,0x186e0) if self.patch else (0x2f0c,0x2f10,0x2f14,0x2f1c,0x2f20))]
        remaining=','.join(map(str,(self.pending[604],self.pending[605],self.read(self.p['retry']) if self.p['retry'] else int(profile==7))))
        return [int(bool(result)), ';'.join(self.events) or '-',','.join(map(str,fields)), ';'.join(points) or '-', self.read(raw+self.o['frame'],self.o['frame_size']),remaining]

    def choose(self,type_,genus):
        """선택 함수의 모든 타입 입력에서 반환 또는 원본 assert를 직접 관찰한다."""
        self.put(TYPES+type_*self.type_stride+0xec,genus);self.spec=dict(self.spec,entries=dict(self.spec['entries'],Choose=self.p['choose']))
        try:self.call('Choose',[type_]);return self.mu.reg_read(UC_X86_REG_EAX),0
        except RuntimeError as error:
            if not str(error).startswith('원본 assert 도달:'):raise
            return 0,1


def generate(smoke=False):
    """고유 입력과 원본53비트 결과를 기록하고64비트 차이를 별도 보존한다."""
    cases=inputs();cases=cases[::37] if smoke else cases;rows=[];type_rows=[];editions={};files={Path(__file__),FIXTURE,TYPE_FIXTURE};differences=[]
    for edition in SPEC:
        oracle=StartOracle(edition)
        for index,case in enumerate(cases):
            try:
                result=oracle.run(case);again=oracle.run(case,FPU_CONTROLS[1])
                if result!=again:differences.append(dict(edition=edition,input=case,result53=result,result64=again))
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,','.join(map(str,case)),*result])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,assertions=oracle.assertions,entries=dict(oracle.native_calls),boundaries=dict(oracle.stub_calls))
        # 알려지지 않은 CD 타입의 assert는 Start 정상 입력과 구분하여 종류 선택 자체의 근거로 남긴다.
        for type_,genus in itertools.product(range(188 if oracle.patch else 171),(0,0x20000)):
            type_rows.append([edition,type_,genus,*oracle.choose(type_,genus)])
        editions[edition]['type_assertions']=oracle.assertions;files.update(oracle.body_paths);files.add(ROOT/oracle.spec['binary'])
    FIXTURE.write_text('# edition type,profile,boss,legacy,replace,pending success events command_fields points frame pending604,pending605,retry\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    TYPE_FIXTURE.write_text('# edition type genus result assertion\n'+'\n'.join('\t'.join(map(str,row)) for row in type_rows)+'\n',encoding='utf-8',newline='\n')
    files.update(ROOT/name for name in ('cpppj/src/o/RawPathStart.h','cpppj/src/o/RawPathStart.cpp','cpppj/src/o/RawPathRuntime.h','cpppj/src/o/RawPathRuntime.cpp','cpppj/src/client/RawPriestWorldBindings.h','cpppj/src/client/RawPriestWorldBindings.cpp','cpppj/tests/RawPathStartTests.cpp','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    REPORT.write_text(json.dumps(dict(total=len(rows),new_unique_cases=len(rows),type_total=len(type_rows),regression_type_cases=len(type_rows),host='HJOW-Athlon',editions=editions,files={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(files) if p.exists()},precision_differences=differences,os_calls=0,limits=['실제 상위 Start/종류선택/ctor/flags0 Init/BFS/Prepare/사제 가이저 finder/취소 루프를 실행한다. 종류선택718개는 이전2872개에 포함되는 회귀이며 신규 합산하지 않는다.','힙·구 경로 조회/삭제·이벤트 조회/삭제·Host 부착·프레임 갱신·profiling 시계는 명시 관찰 경계다.','실제 운반/의식·삭제 cleanup·캠페인 완주/GUI 검증으로 세지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('PathStart 실제 입력:',len(rows),'정밀도 차이:',len(differences))


def verify():
    """현재 SHA·고유 입력·실제 정상 반환과 assert0을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    for name,expected in report['files'].items():assert sha(ROOT/name)==expected,name
    assert len(rows)==report['total']==2*len(inputs()) and report['os_calls']==0
    for edition,item in report['editions'].items():
        chosen=[r for r in rows if r[0]==edition];assert [r[1] for r in chosen]==[','.join(map(str,c)) for c in inputs()]
        assert item['returns']==2*len(chosen) and item['assertions']==0 and not any(k in item['boundaries'] for k in ('begin','next','search','prepare','init'))
    print('PathStart 실제 입력/SHA/반환 감사:',len(rows))


# 명시 실행으로만 원본 관찰을 생성하거나 저장 근거를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
