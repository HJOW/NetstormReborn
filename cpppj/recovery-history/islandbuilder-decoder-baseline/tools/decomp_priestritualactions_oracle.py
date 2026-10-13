#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 사제 의식 프레임/604 예약 wrapper를 두 보호 PE의 제한 x86으로 대조한다."""
import argparse
import collections
import csv
import itertools
import json
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle,ROOT,POOL,HEAP,VTABLE,bits,sha
from decomp_bridgedecay_oracle import TYPES,CAPACITY,FPU_CONTROLS,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 원본의 입력 타입/함수 주소이며 게임/OS를 호출하지 않는다.
SPEC={
 'originals':dict(begin=0x491ca0,follow=0x491cf0,set=0x4acee0,new=0x4e4391,shared=0x496f00),
 'originalCD':dict(begin=0x40cf10,follow=0x40cf60,set=0x4acbc0,new=0x4f1650,shared=0x48f1d0),
}
MEMORY=HEAP+0x6000 # 명시 예약 확보가 반환하는 격리 new0x28 메모리다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priestritualactions-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestritualactions-evidence.json'


def inputs():
    """실제 SID 범위·현재 타입 first-table·K..P 단계·확보 실패·frame 뒤 입력 변경을 교차한다."""
    cases=[]
    # 유효 번호에서 두 wrapper의 모든 실제 first-table 단계와 현재 타입을 대조한다.
    for index,row in enumerate(itertools.product((0,1),(5,CAPACITY-1),range(6),range(3),(133,158),(0,1),range(3))):
        cases.append((*row,(0,2,4,8)[index%4]))
    # 범위 밖은 frame/예약을 모두 건너뛰는 원본 자연 반환이다.
    cases.extend((mode,sid,stage,0,158,1,0,0) for mode,sid,stage in itertools.product((0,1),(0,CAPACITY,0xffffffff),(0,5)))
    return cases


class ActionOracle(FinderOracle):
    """기존 PE 로더만 재사용하고 세 효과/현재 정밀 wrapper 범위만 허용한다."""
    def __init__(self,edition):
        """같은 PC 정밀 내보내기의 실제 불연속 함수 범위를 고정한다."""
        super().__init__(edition);self.patch=edition=='originals';self.p=SPEC[edition];self.stubs={};self.allowed[:]=bytes(len(self.allowed));self.hits=collections.Counter();self.returns=0
        self.manifest=ROOT/'tools/ghidra/priestritualactions-functions.json';self.table=ROOT/f'extracted/priestritualactions/{edition}/functions.tsv';self.source=ROOT/f'extracted/priestritualactions/{edition}/creation.c';self.entries={}
        selected={int(e,16) for e in json.loads(self.manifest.read_text(encoding='utf-8'))['editions'][edition]}
        with self.table.open(encoding='utf-8') as stream:
            # 함수 외의 코드/OS 진입을 허용하지 않는다.
            for row in csv.DictReader(stream,delimiter='\t'):
                if int(row['entry'],16) not in selected:continue
                self.entries[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        self.spec=dict(self.spec,entries={'Begin':self.p['begin'],'Continue':self.p['follow']});self.boundaries={self.p['set']:'frame',self.p['new']:'reserve',self.p['shared']:'shared'}

    def read(self,address,width=4):
        """현재 격리 DWORD/필드를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """입력/명시 효과만 원본 폭대로 기록한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def on_instruction(self,mu,address,size,data):
        """현재 타입/프레임/분기는 원본이고 frame setter·new·공유 생성 요청만 기록한다."""
        if address in getattr(self,'entries',{}):self.hits[self.entries[address]]+=1
        boundary=getattr(self,'boundaries',{}).get(address)
        if not boundary:return super().on_instruction(mu,address,size,data)
        self.stub_calls[boundary]+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);a=[self.read(sp+4*i) for i in range(1,4)];this=mu.reg_read(UC_X86_REG_ECX);purge=0;result=0
        if boundary=='frame':
            assert this==self.slot(self.case[1]) and a[1]==0;purge=8;self.events.append('F:'+','.join(map(str,(self.case[1],a[0],a[1]))));self.put(this+self.o['frame'],a[0],self.o['frame_size'])
            if self.case[6]==1:self.put(this+10,133 if self.case[4]==158 else 158,1)
        elif boundary=='reserve':
            assert a[0]==40;self.events.append('N:40');result=MEMORY if self.case[5] else 0
            if self.case[6]==2:self.put(self.slot(self.case[1])+11,2,1)
        elif boundary=='shared':
            assert this==MEMORY and a==[0x25c,self.case[1],bits(0.01)];purge=12;result=MEMORY;self.events.append('S:'+','.join(map(str,a)))
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,case,control):
        """원본 타입 first-table 입력을 만들고 실제 wrapper의 모든 요청/현재 raw 결과를 읽는다."""
        self.case=case;mode,sid,stage,profile,kind,reserve,mutation,raw_state=case;data_sid=sid if 0<sid<CAPACITY else 5;self.mu.mem_write(self.slot(data_sid),bytes(self.stride));self.mu.mem_write(TYPES,bytes(0x40000))
        self.put(self.slot(data_sid),VTABLE);self.put(self.slot(data_sid)+10,kind,1);self.put(self.slot(data_sid)+11,raw_state,1)
        # 앞 A 프레임 둘과 각 K..P의1/2/3 프레임 구간은 합성 자산 입력이다. 선택 결과는 wrapper가 읽는다.
        for typ in (133,158):
            for index in range(6):self.put(TYPES+typ*self.type_stride+0x158+4*index,2+index*(profile+1))
        self.call('Continue' if mode else 'Begin',[sid,stage],control=control);self.returns+=1
        return [';'.join(self.events) or '-',bytes(self.mu.mem_read(self.slot(data_sid),self.stride)).hex()]


def generate(smoke=False):
    """각 고유 입력의 두 정밀도 결과를 확인하고 현재 구현/보호 PE/범위 SHA를 고정한다."""
    cases=inputs();cases=cases[::83] if smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE}
    # 두 판본을 동일 입력으로 독립 실행한다.
    for edition in SPEC:
        oracle=ActionOracle(edition)
        for index,case in enumerate(cases):
            try:
                result=oracle.run(case,FPU_CONTROLS[0]);assert oracle.run(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,','.join(map(str,case)),*result])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,entries=dict(oracle.hits),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions);files.update((oracle.manifest,oracle.table,oracle.source,ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')))
    FIXTURE.write_text('# edition mode,sid,stage,profile,type,reserve,mutation,state effects raw\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files.update(ROOT/name for name in ('cpppj/src/o/RawPriestRitualActions.h','cpppj/src/o/RawPriestRitualActions.cpp','cpppj/tests/PriestRitualActionsTests.cpp','cpppj/src/o/SquidFrame.h','cpppj/src/o/SquidFrame.cpp','cpppj/src/o/SquidProcess.h','cpppj/src/o/SquidProcess.cpp','cpppj/src/o/RiftType.h','cpppj/src/o/RiftType.cpp','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',total=len(rows),editions=editions,files={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(files) if p.exists()},game_runs=0,limits=['실제 wrapper/현재 타입 first-table 선택·SID 범위·frame 뒤 예약 순서는 실제 PE다.','frame setter·malloc·SharedRegular ctor는 명시 관찰 경계이며 C++ 소비자는 기존 SquidFrame/Host를 재사용한다.','K..P stage0..5 정상 타입 first-table 범위만 복원한다. 신전 ritual/좌표/음향/다음 의식 단계·실제 GameWorld/GUI는 후속이다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 의식 동작 native 고유 입력:',len(rows))


def verify():
    """현재 SHA·고유 입력/반환·실제 두 wrapper 진입과 세 명시 경계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert report['total']==len(rows)==2*len(inputs()) and report['game_runs']==0
    for name,value in report['files'].items():assert sha(ROOT/name)==value,name
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];assert [row[1] for row in selected]==[','.join(map(str,case)) for case in inputs()];assert item['returns']==2*len(selected) and item['asserts']==0
        assert item['entries'][f'{SPEC[edition]["follow"]:08x}']==2*sum(case[0]==1 for case in inputs())
        assert set(item['boundaries'])<={'frame','reserve','shared'}
    print('사제 의식 동작 SHA/실제wrapper/반환 감사:',len(rows))


# 명시 호출에서만 새 관찰을 생성하거나 현재 근거를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
