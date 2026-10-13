#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 PathProcess cleanup·귀환 장소/주변 타입·contained/Graph 조회를 두 PE에서 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle,ROOT,POOL,HEAP,MAP,VTABLE,bits,sha
from decomp_bridgedecay_oracle import TYPES,FPU_CONTROLS,STACK_BASE,CAPACITY,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 경로/예약물과 관찰 가상 주소는 격리된 메모리다.
PATH=0x11400000
MEMORY=HEAP+0x6000
UNPOP,REPOP,DESTROY=HEAP+0xf000,HEAP+0xf010,HEAP+0xf020
FIXTURE=ROOT/'cpppj/tests/fixtures/pathcleanup-x86.tsv'
REPORT=ROOT/'cpppj/recovery-pathcleanup-evidence.json'
SPEC={
 'originals':dict(main=0x48aa00,returnPlace=0x4263a0,nearby=0x4b21d0,helper=0x186d4,size=0x186f0,new=0x4e4391,shared=0x496f00,ritual=0x447e40,refund=0x442a40,boss=0x540bc4,checking=0x5e4794,priest=0x5412d0,dais=0x5412ec,outpost=0x541250,nugget=0x5412c0,event=0x5326c4,contained=0x541088,graphReady=0x540414,graphInvalid=0x540410),
 'originalCD':dict(main=0x47f0a0,returnPlace=0x4e3d70,nearby=0x4eb7b0,helper=0x2f14,size=0x2f28,new=0x4f1650,shared=0x48f1d0,ritual=0x481fc0,refund=0x4d1bc0,boss=0x540a2c,checking=None,priest=0x51cbbc,dais=0x51cbd8,outpost=0x51cb3c,nugget=0x51cbac,event=0x520fe0,contained=0x51c978),
}
# 입력 좌표만 구성한다. 실제 범위/반환/효과는 원본 몸체가 결정한다.
COORDINATES=((40.75,41.25),(40.00004959106445,41.00004959106445),(40.000003814697266,41.000003814697266))


def inputs():
    """helper/권한/사망/부모 종류/예약 실패·후속 callback의 현재값을 교차한다."""
    result=[]
    # 전체 profile에서 각 기본 권한/상태 조합을 독립 입력으로 만든다.
    for index,row in enumerate(itertools.product(range(18),(0,1),(0,1),(0,1),(0,1),(0,1))):
        result.append((*row,index%3,(index//3)%3))
    # 실제 도착/공유 예약 뒤 mutable 권한 조회가 필요한 복합 profile의 모든 callback/좌표를 검사한다.
    result.extend(itertools.product((14,16),(0,1),(0,),(1,),(0,1),(0,1),range(3),range(3)))
    return list(dict.fromkeys(result))


def scene(case):
    """같은 raw/hash 입력을 native와 C++에 공급할 명시 상태 표를 만든다."""
    profile,boss,dead,priest,reserve,checking,mutation,coordinate=case;x,y=COORDINATES[coordinate]
    helper=profile in (1,2,3,4,5,6,7,14,15,16);near=profile in (1,6,7,16)
    # sid,type,state,extra,x,y,footX,footY,height,hashLevel,genus,owner,graph의 입력이다.
    objects=[]
    if helper:objects.append((6,128,1 if profile==7 or profile==5 else 0,(0 if profile==3 else 1)|(8 if profile==6 else 0),x if near else 80.25,y if near else 81.25,1,1,0,1,0 if profile==4 else 0x404200,1,7))
    place=profile in (8,9,10,11,14,16,17)
    if place:objects.append((7,126 if profile in (8,14,17) else 85,2 if profile==10 else 0,1 if profile==11 else 0,x,y,1,1,0,1,0 if profile in (8,14,17) else 0x200,2 if profile==9 else 1,3 if profile==17 else 7))
    temple=profile in (12,13,14,15,16)
    if temple:objects.append((9,165,0,8 if profile in (13,15) else 0,x,y,1,1,0,1,0,1,7))
    # 현재 graph 비교는 실제0단계 surface 머리의 번호를 사용한다.
    objects.append((11,97,0,0,x,y,1,1,0,0,2,1,7))
    return dict(parent=158 if priest else 133,dead=dead,helper=6 if helper else 0,nugget=place,objects=objects,x=x,y=y)


def query_inputs():
    """CD main에서 사용하지 않는 귀환 조회도 두 판본 원본 몸체를 독립 호출한다."""
    return list(itertools.product((8,9,10,11,12,13,14,16,17),range(3),(0,1,2),(126,165)))


class CleanupOracle(FinderOracle):
    """PE 로더/실제 일반 finder만 재사용하고 cleanup 고유 범위와 효과를 지정한다."""
    def __init__(self,edition):
        """전체 PE 허용 없이 현재 정밀 body의 실제 불연속 범위만 실행한다."""
        super().__init__(edition);self.patch=edition=='originals';self.p=SPEC[edition];self.stubs={};self.allowed[:]=bytes(len(self.allowed));self.entry_hits=collections.Counter();self.returns=0;self.query_returns=0;self.mu.mem_map(PATH,0x20000)
        self.manifest=ROOT/'tools/ghidra/pathcleanup-functions.json';self.table=ROOT/f'extracted/pathcleanup/{edition}/functions.tsv';self.source=ROOT/f'extracted/pathcleanup/{edition}/creation.c'
        selected={int(e,16) for e in json.loads(self.manifest.read_text(encoding='utf-8'))['editions'][edition]};self.entries={}
        with self.table.open(encoding='utf-8') as stream:
            # 각 원본 body의 시작/끝 외부로 흐르면 부모 실행기의 허용 검사가 실패한다.
            for row in csv.DictReader(stream,delimiter='\t'):
                if int(row['entry'],16) not in selected:continue
                self.entries[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        self.spec=dict(self.spec,entries={'Cleanup':self.p['main'],'ReturnPlace':self.p['returnPlace'],'Nearby':self.p['nearby']})
        self.boundaries={self.p['new']:'reserve',self.p['shared']:'shared',self.p['ritual']:'ritual',self.p['refund']:'refund',UNPOP:'unpop',REPOP:'repop',DESTROY:'destroy'}

    def read(self,address,width=4):
        """명시 폭의 little endian 값을 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """입력/명시 callback만 지정 폭으로 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def on_write(self,mu,access,address,size,value,data):
        """기계어의 새 쓰기는 경로 helper/현재 가상 예약 저장소/스택/FS만 허용한다."""
        if PATH<=address and address+size<=PATH+0x20000:return
        return super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """실제 분기/조회는 실행하고 수명·예약·환불·의식 효과의 요청만 기록한다."""
        if address in getattr(self,'entries',{}):self.entry_hits[self.entries[address]]+=1
        if address==self.spec['assert_report']:
            self.assertions+=1;self.events.append('A');boundary='assert'
        else:boundary=getattr(self,'boundaries',{}).get(address)
        if not boundary:return super().on_instruction(mu,address,size,data)
        self.stub_calls[boundary]+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX);a=[self.read(sp+4*i) for i in range(1,5)];purge=0;result=0
        if boundary in ('unpop','repop'):
            assert this==self.slot(5) and a[0]==0;purge=4;self.prefix.append(('U' if boundary=='unpop' else 'P')+':5:0')
        elif boundary=='reserve':
            assert a[0]==40;self.events.append('N:40');result=MEMORY if self.case[4] else 0
        elif boundary=='shared':
            assert this==MEMORY and a[:3]==[0x1a2c,6,bits(1.0)];purge=12;result=MEMORY;self.events.append('S:'+','.join(map(str,a[:3])))
            if self.case[6]==1:self.put(self.p['boss'],1-self.case[1])
        elif boundary=='ritual':
            assert this==self.slot(9) and a[:2]==[5,0];purge=8;self.events.append('R:9,5,0')
            if self.case[6]==2:self.put(self.p['boss'],1-self.read(self.p['boss']))
        elif boundary=='refund':self.events.append('F:'+','.join(map(str,a[:2])))
        elif boundary=='destroy':
            sid=self.sid_of(this);assert sid in (6,8) and a[0]==0;purge=4;self.events.append('D:'+str(sid)+',0');self.put(this+11,self.read(this+11,1)|2,1)
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def prepare_case(self,case):
        """정상 월드의 동일 raw 슬롯·네 버킷·종속 체인·현재 전역을 공급한다."""
        self.case=case;data=scene(case);self.prefix=[];self.mu.mem_write(POOL,bytes(16*self.stride));self.mu.mem_write(TYPES,bytes(0x40000));self.mu.mem_write(PATH,bytes([0xaa])*self.p['size'])
        records=[(*item[:4],bits(item[4]),bits(item[5]),*item[6:10]) for item in data['objects']];self.setup(records)
        # 각 입력 타입의 genus/owner/graph는 원본 조회와 같은 현재 표에 둔다.
        for sid,typ,state,extra,x,y,fx,fy,height,level,genus,owner,graph in data['objects']:
            self.put(TYPES+typ*self.type_stride+0xec,genus);self.put(TYPES+typ*self.type_stride+0xe8,0x800 if typ==97 or case[0]==17 and sid==7 else 0);self.put(self.slot(sid)+(34 if self.patch else 32),owner,1);self.put(self.slot(sid)+self.o['graph'],graph,1)
        gx,gy=(41,42) if case[7]==0 else (40,41);self.put(MAP+(gy*256+gx)*2,11,2)
        self.put(self.slot(5),VTABLE);self.put(self.slot(5)+10,data['parent'],1);self.put(self.slot(5)+11,2 if data['dead'] else 0,1);self.put(self.slot(5)+14,bits(data['x']));self.put(self.slot(5)+18,bits(data['y']));self.put(self.slot(5)+(34 if self.patch else 32),1,1);self.put(self.slot(5)+6,10,2)
        self.put(self.slot(10)+10,54,1);self.put(self.slot(10)+4,8 if data['nugget'] else 0,2);self.put(self.slot(8),VTABLE);self.put(self.slot(8)+10,6,1);self.put(self.slot(8)+18,154,2);self.put(self.slot(8)+20,32,2)
        self.put(PATH+4,10);self.put(PATH+8,5);self.put(PATH+self.p['helper'],data['helper']);self.put(VTABLE+0x48,UNPOP);self.put(VTABLE+0x4c,REPOP);self.put(VTABLE+0x10,DESTROY)
        for name,value in [('boss',case[1]),('priest',158),('dais',165),('outpost',126),('nugget',154),('event',0x1a2c),('contained',6)]:self.put(self.p[name],value)
        if self.patch:
            self.put(self.p['checking'],case[5]);self.put(self.p['graphReady'],1);self.put(self.p['graphInvalid'],254)
        return data

    def run(self,case,control):
        """실제 전체 cleanup 반환에서 공간 접두와 이후 효과/쓰기를 구분해 관찰한다."""
        data=self.prepare_case(case);before=bytes(self.mu.mem_read(PATH,self.p['size']));self.call('Cleanup',args=[0],this=PATH,purge=4,control=control);self.returns+=1
        after=bytes(self.mu.mem_read(PATH,self.p['size']));offset=self.p['helper'];assert after[:offset]==before[:offset] and after[offset+4:]==before[offset+4:]
        return [';'.join(self.prefix) or '-',';'.join(self.events) or '-',self.read(PATH+offset),self.read(self.p['boss']),';'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in range(5,12))]

    def query(self,case,control):
        """반경/부류/현재상태 조회 결과만 원본에게서 읽고 슬롯 불변도 확인한다."""
        profile,coordinate,radius,kind=case;data=self.prepare_case((profile,1,0,1,1,0,0,coordinate));before=bytes(self.mu.mem_read(POOL+5*self.stride,7*self.stride))
        returned=self.call('ReturnPlace',[bits(data['x']),bits(data['y']),radius],control=control);nearby=self.call('Nearby',[bits(data['x']),bits(data['y']),2,kind],control=control);self.query_returns+=2
        assert bytes(self.mu.mem_read(POOL+5*self.stride,7*self.stride))==before
        return ['-','Q:'+str(returned)+','+str(nearby),data['helper'],1,';'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in range(5,12))]


def generate(smoke=False):
    """53/64비트 반복이 같은 고유 입력만 별도 fixture와 현재 소스 SHA로 보존한다."""
    cases=inputs();queries=query_inputs();cases=cases[::47] if smoke else cases;queries=queries[::19] if smoke else queries;rows=[];editions={};files={Path(__file__),FIXTURE}
    # 두 실제 PE의 같은 입력을 각각 독립 실행한다.
    for edition in SPEC:
        oracle=CleanupOracle(edition)
        for index,case in enumerate(cases):
            try:
                result=oracle.run(case,FPU_CONTROLS[0]);assert oracle.run(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,','.join(map(str,case)),*result])
        # 독립 helper 조회도 기대값을 계산하지 않고 실제 두 정밀도 반환을 읽는다.
        for index,case in enumerate(queries):
            try:
                result=oracle.query(case,FPU_CONTROLS[0]);assert oracle.query(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} query#{index} {case}: {error}') from error
            rows.append([edition,'Q,'+','.join(map(str,case)),*result])
        selected=[row for row in rows if row[0]==edition];assertion_cases=sum('A' in row[3].split(';') for row in selected)
        editions[edition]=dict(cases=len(cases)+len(queries),cleanup_cases=len(cases),query_cases=len(queries),normal_cases=len(cases)+len(queries)-assertion_cases,assertion_cases=assertion_cases,returns=oracle.returns,query_returns=oracle.query_returns,entries=dict(oracle.entry_hits),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions)
        files.update((oracle.manifest,oracle.table,oracle.source,ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')))
    FIXTURE.write_text('# edition profile,boss,dead,priest,reserve,checking,mutation,coordinate spatial afterEffects helper boss slots5..11\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files.update(ROOT/name for name in ('cpppj/src/o/RawPathCleanup.h','cpppj/src/o/RawPathCleanup.cpp','cpppj/tests/PathCleanupTests.cpp','cpppj/src/o/RawPathRuntime.h','cpppj/src/o/RawPathRuntime.cpp','cpppj/recovery-pathcleanup-runtime-lineage-evidence.json','cpppj/src/o/RawSquidFinder.h','cpppj/src/o/RawSquidFinder.cpp','cpppj/src/o/RawCarrierPreDestroy.h','cpppj/src/o/RawCarrierPreDestroy.cpp','cpppj/src/o/RawPlayerPlacementAnchor.h','cpppj/src/o/RawPlayerPlacementAnchor.cpp','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',total=len(rows),normal_total=sum(e['normal_cases'] for e in editions.values()),assertion_total=sum(e['assertion_cases'] for e in editions.values()),editions=editions,files={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(files) if p.exists()},game_runs=0,limits=['공간 접두는 원본 호출 요청을 별도 기록하고 기존 RawPathProcess가 재사용한다. 새 C++ 몸체는 그 뒤부터다.','malloc·SharedRegular ctor·ritual·Refund·가상삭제는 명시 관찰 경계다. 기존 Host/Refund/contained/Graph는 별도 실제 소비자로 재사용하며 신전 ritual 하위는 미완료다.','캠페인 실제 raw 월드/사제·골렘 CarrierArrival/GUI/완주 완료로 세지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Path cleanup native 고유 입력:',len(rows))


def verify():
    """현재 SHA/고유 입력·실제 cleanup 및 finder 반환·명시 효과 경계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['total']==2*(len(inputs())+len(query_inputs())) and report['game_runs']==0
    # 코드/보호 PE/최신 정밀 범위가 생성 당시의 출처와 같아야 한다.
    for name,value in report['files'].items():assert sha(ROOT/name)==value,name
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];assert [row[1] for row in selected]==[','.join(map(str,case)) for case in inputs()]+['Q,'+','.join(map(str,case)) for case in query_inputs()]
        assert item['returns']==2*item['cleanup_cases'] and item['query_returns']==4*item['query_cases'] and item['entries'][f'{SPEC[edition]["main"]:08x}']==item['returns']
        assert item['asserts']==2*item['assertion_cases'];assert item['normal_cases']+item['assertion_cases']==len(selected)
        assert set(item['boundaries'])<={'unpop','repop','reserve','shared','ritual','refund','destroy','assert'}
    print('Path cleanup SHA/실제몸체/반환 감사:',len(rows))


# 명시 호출에서만 새 native 관찰을 생성하거나 현재 기록을 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
