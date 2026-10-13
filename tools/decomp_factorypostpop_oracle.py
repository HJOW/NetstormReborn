#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Factory/아웃포스트·상태 getter·프레임 구간·일반 finder·고유 목록을 두 PE에서 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle, ROOT, POOL, HEAP, MAP, STACK_BASE, VTABLE, bits, sha
from decomp_bridgedecay_oracle import TYPES, FPU_CONTROLS, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 실제 생성자가 쓰는 vtable 시작과 전용 몸체·전역 주소다.
SPEC = {
    'originals': dict(tables=(0x509148,0x509210), entries=(0x4538f0,0x4557c0), adjust=0x453780,
        count=0x4ad0f0, allocate=0x4e4391, regular=0x496e80, damageable=0x44bf80, frame=0x4acee0,
        claim=0x455380, warning=0x4c2630, destroy=0x4af780, lists=(0x55a70c,0x59a9d8)),
    'originalCD': dict(tables=(0x503298,0x503358), entries=(0x47a4b0,0x47b680), adjust=0x47a1f0,
        count=0x4ace00, allocate=0x4f1650, regular=0x48efd0, damageable=0x4621c0, frame=0x4acbc0,
        claim=0x47b3c0, warning=0x48ca70, destroy=0x4ab7e0, lists=(0x5670e0,0x565ae0)),
}
# 별도 fixture·근거·목록 저장소이며 기존 독립 finder/자산 관찰을 변경하지 않는다.
FIXTURE = ROOT/'cpppj/tests/fixtures/factorypostpop-x86.tsv'
REPORT = ROOT/'cpppj/factorypostpop.json'
LISTS, ALLOCATION = HEAP+0x9000, HEAP+0xa000
KEYS = ('kind','type','flags','extra','boss','count','frame','reserve','list_count','present','mutation')


def inputs():
    """분기·프레임 경계·할당 실패·목록/콜백 변경 입력을 만들며 기대값을 계산하지 않는다."""
    result=[]
    # 최초/재Pop과 abstract/buried·권한을 두 실제 가상 표에 교차한다.
    for kind,number,flags,extra,boss in itertools.product((0,1),(85,87,86,126),(0,1,2,9),(0,1,8,9),(0,1)):
        result.append((kind,number,flags,extra,boss,0,7,7,0,0,0))
    # 현재 상태별 빈 구간·양끝·signed frame와 CD BYTE 차이를 보존한다.
    for kind,number,count,frame in itertools.product((0,1),(85,87,126),(0,1,2,7),(0,1,2,4,5,7,255,0xffffffff)):
        result.append((kind,number,1,0,0,count,frame,7,0,0,0))
    # 각 예약의 독립 실패와 목록의 중복/용량 포화를 공급한다.
    for kind,reserve,size,present in itertools.product((0,1),(0,1,2,4),(0,2,4),(0,1)):
        result.append((kind,85,1,0,1,1,7,reserve,size,present,0))
    # 외부 효과 후 현재 type/state 및 다음 finder 후보의 변경을 실제 명령이 다시 읽어야 한다.
    for kind,mutation in itertools.product((0,1),(1,2,3,4)):
        result.append((kind,85,1,0,1,0,7,7,0,0,mutation))
    return list(dict.fromkeys(result))


class FactoryOracle(FinderOracle):
    """기존 실제 일반 finder에 새 몸체를 추가하고 프로세스·Damageable·Frame/소유권·삭제 효과만 관찰한다."""
    def __init__(self,edition):
        """격리 PE와 허용 몸체·목록 전역의 바이트 쓰기 범위를 연결한다."""
        super().__init__(edition)
        self.factory=SPEC[edition] # 실제 Factory 함수와 원본 목록 전역 주소다.
        self.body_paths.extend(ROOT/f'extracted/factorypostpop/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        self.factory_entries=set() # 추가 몸체의 실제 진입 집계에 사용한다.
        with self.body_paths[-1].open(encoding='utf-8') as stream:
            # Ghidra가 내보낸 불연속 범위만 기존 코드 허용 표에 추가한다.
            for row in csv.DictReader(stream,delimiter='\t'):
                self.factory_entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'))
                    self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        for table,entry in zip(self.factory['tables'],self.factory['entries'],strict=True):
            assert self.read(table+0x20)==entry and self.read(table+0x80)==self.factory['count']
        self.write_ranges=[(HEAP,HEAP+0x20000),(POOL,POOL+128*self.stride)]
        for head in self.factory['lists']:
            self.write_ranges.append((head,head+12))
        self.returns=0 # 실제 최상위 정상 반환 수이며 내부 호출 수와 구별한다.
        self.spec=dict(self.spec,entries=dict(self.spec['entries'],Factory=self.factory['entries'][0],Outpost=self.factory['entries'][1]))

    def read(self,address,width=4):
        """정렬되지 않은 raw 값을 지정한 폭대로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """입력이나 명시 효과 경계의 raw 값만 쓴다."""
        self.mu.mem_write(address,int(value).to_bytes(width,'little'))

    def on_write(self,mu,access,address,size,value,data):
        """이 몸체의 실제 쓰기는 스택/FS·고유 목록 count/배열로만 제한한다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if address==0 and size==4:return
        if HEAP<=address and address+size<=HEAP+0x10000:return
        if any(head+8<=address and address+size<=head+12 for head in self.factory['lists']):return
        raise RuntimeError(f'Factory 허용 밖 쓰기: {self.edition} {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """actual finder/getter/list/body는 대체하지 않고 필요한 외부 효과의 현재 인자를 관찰한다."""
        if address in self.factory_entries:
            self.native_calls[f'{address:08x}']+=1
        p=self.factory
        if address not in {p[key] for key in ('allocate','regular','damageable','frame','claim','warning','destroy')}:
            return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX)
        args=[self.read(sp+4+4*i) for i in range(4)];purge=0;value=0
        if address==p['allocate']:
            assert args[0]==40;self.events.append('A:40')
            value=ALLOCATION if self.case[7]&(1<<self.alloc) else 0;self.alloc+=1
        elif address==p['regular']:
            self.events.append(f'R:{args[1]}:{args[0]}:{args[2]}');purge=12
            if self.case[10]==2 and args[0]==2:self.put(self.slot(50)+10,87,1)
        elif address==p['damageable']:
            self.events.append('D:'+str(args[0]));purge=4
            if self.case[10]==1:self.put(self.slot(50)+11,7<<5,1)
        elif address==p['frame']:
            self.events.append(f'F:{args[0]}:{args[1]}');purge=8
            self.put(this+self.o['frame'],args[0]&((1<<(8*self.o['frame_size']))-1),self.o['frame_size'])
        elif address==p['claim']:
            self.events.append(f'C:{args[0]}:{args[1]}')
            if self.case[10]==3:self.put(self.slot(50)+10,87,1)
        elif address==p['warning']:
            self.events.append(f'W:{args[2]}:{args[3]}')
        else:
            sid=(this-POOL)//self.stride;self.events.append(f'X:{sid}:{args[0]}');purge=4
            self.put(this+11,self.read(this+11,1)|2,1)
            if self.case[10]==4:
                self.put(self.slot(61)+self.o['extra'],0,1)
                self.put(self.slot(61)+10,self.read(self.slot(50)+10,1),1)
        self.stub_calls[f'factory_{address:08x}']+=1
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_factory(self,case,control=FPU_CONTROLS[0]):
        """현재 같은 hash/풀 입력과 프레임 구간을 공급하고 원본 결과만 기록한다."""
        self.case=case # 기대값 판단 없이 외부 효과 입력을 결정하는 명시 격자다.
        self.alloc=0 # 각 new의 성공 비트 위치이며 case마다 초기화한다.
        self.events=[];kind,number,flags,extra,boss,count,frame,reserve,length,present,mutation=case
        objects=[(50,number,count<<5,extra,bits(50.75),bits(52.5),1,1,0,1),
                 (60,number,0,0,bits(50.25),bits(52.75),1,1,0,1),
                 (61,86 if number!=86 else 87,0,8,bits(50.5),bits(52.25),1,1,0,1)]
        self.setup(list(reversed(objects)))
        p=self.factory;self.put(self.g['authority'],boss)
        self.put(VTABLE+0x80,p['count']);self.put(VTABLE+0x10,p['destroy'])
        # 원본 배열 초기화 결과와 같은 A/B/H 글자 구간만 입력으로 제공한다.
        for typ in (85,86,87,126):
            start=TYPES+typ*self.type_stride
            self.put(start+0x9c,10)
            self.mu.mem_write(start+0x130,struct.pack('<16i',0,2,-1,-1,-1,-1,-1,5,*([-1]*8)))
            self.mu.mem_write(start+0x170,struct.pack('<16i',2,3,0,0,0,0,0,2,*([0]*8)))
        self.put(self.slot(50)+self.o['frame'],frame&((1<<(8*self.o['frame_size']))-1),self.o['frame_size'])
        for index,head in enumerate(p['lists']):
            values=[50 if present else 7,8,9,10];storage=LISTS+index*0x100
            self.mu.mem_write(storage,struct.pack('<4I',*values));self.mu.mem_write(head,struct.pack('<III',storage,4,length))
        self.call('Outpost' if kind else 'Factory',[flags],self.slot(50),4,control)
        self.returns+=1
        output=bytearray(self.mu.mem_read(self.slot(50),self.stride));struct.pack_into('<I',output,0,p['tables'][kind])
        lists='|'.join(str(self.read(head+8))+':'+','.join(str(self.read(LISTS+i*0x100+4*j)) for j in range(4)) for i,head in enumerate(p['lists']))
        other='|'.join(f'{self.read(self.slot(sid)+10,1)},{self.read(self.slot(sid)+11,1)},{self.read(self.slot(sid)+self.o["extra"],1)}' for sid in (60,61))
        return [';'.join(self.events) or '-',output.hex(),lists,other]


def generate(smoke=False):
    """두 판본의 고유 입력·실제 반환·근거 SHA와 외부 경계를 보존한다."""
    cases=inputs();cases=cases[::71] if smoke else cases;rows=[];editions={}
    files=[Path(__file__),FIXTURE,ROOT/'tools/ghidra/factorypostpop-functions.json',
           ROOT/'tools/decomp_rawfinder_oracle.py',ROOT/'tools/decomp_bridgeeffects_oracle.py',ROOT/'tools/decomp_bridgedecay_oracle.py',
           ROOT/'cpppj/src/o/RawFactoryPostPop.h',ROOT/'cpppj/src/o/RawFactoryPostPop.cpp',ROOT/'cpppj/tests/FactoryPostPopTests.cpp']
    for edition in SPEC:
        oracle=FactoryOracle(edition)
        for case in cases:
            output=oracle.run_factory(case);rows.append([edition,','.join(map(str,case)),*output])
            # 확장 정밀도는 같은 입력에서 차이가 없는지 별도 반복하며 고유 관찰에 더하지 않는다.
            assert oracle.run_factory(case,FPU_CONTROLS[1])==output
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,assertions=oracle.assertions,
                              entries=dict(oracle.native_calls),external=dict(oracle.stub_calls))
        files.extend(oracle.body_paths+[ROOT/oracle.spec['binary']])
    FIXTURE.write_text('# edition kind,type,flags,extra,boss,count,frame,reserve,list_count,present,mutation events raw lists other\n'+
                       '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='VM-W11-CODEX',editions=editions,os_calls=0,
        files={path.relative_to(ROOT).as_posix():sha(path) for path in dict.fromkeys(files)},limits=[
            '예약 할당/Regular·Damageable·SquidFrame 가상 효과·아웃포스트 소유권 재계산·경고/삭제는 명시 관찰 경계다.',
            '상태 getter·프레임 구간 판단·일반 finder/CRT·고유 목록·Factory/아웃포스트 body는 실제 명령이다.',
            'C++은 실제 일반 finder와 직접 Damageable→공통 장부를 조합하며 미션/GUI 완주를 입증하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'total':len(rows),'editions':{k:v['cases'] for k,v in editions.items()}},ensure_ascii=False))


def verify():
    """출처 SHA·고유 격자·실제 두 정밀도 반환·assert0을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert report['total']==len(rows)==len(inputs())*2 and report['os_calls']==0
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition]
        assert [row[1] for row in chosen]==[','.join(map(str,case)) for case in inputs()]
        assert item['returns']==len(chosen)*2 and item['assertions']==0
        assert not any(key in item['external'] for key in ('begin','next'))
    print(f'Factory/아웃포스트 {len(rows)}개 실제 finder·입력·SHA 감사 통과')


# 명시 실행에서만 별도 결과를 생성하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)
