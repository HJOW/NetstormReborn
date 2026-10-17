#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본10.78 커서 잠금/해제/회전의 현재값 재조회와 효과 순서를 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, PROCESS, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ESP

ENTRIES=(0x446a10,0x446a40,0x446a70)  # 실제 Lock/Unlock/Rotate 전체 진입점이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/cursor-actions-x86.tsv'  # 기존 수명/위치 fixture를 변경하지 않는 새 입력이다.
REPORT=ROOT/'cpppj/cursor-actions.json'  # 새 원본 입력과 실행기/제품의 고정 SHA다.
GLOBALS=(0x5325c8,0x55a2a4,0x55a27c,0x55a284,0x55a1d8,0x55a274)  # 깊이/방향/Gump/TYPE/첫SID/현재 프로세스다.


def cases():
    """DWORD 감김과 각 외부 효과 뒤 변경을 구분하는 명시 입력을 만든다."""
    # op,lock,arg,direction,gump,group,type,special,activate,create,sound,remove,select,rangeResult다.
    base=[2,0,2,0,0,0,100,140,0,0,0,0,0,1];rows=[]
    # 잠금은 nonzero 인자의 의미를 bool로 좁히지 않고 현재 깊이 감김을 확인한다.
    for lock,argument,activate in itertools.product((0,1,0xffffffff),(0,1,0xffffffff),range(4)):
        row=base.copy();row[:3]=0,lock,argument;row[8]=activate;rows.append(row)
    # 해제는0에서 감소하는 값과 Create/커서 선택 뒤 현재 전역 변경도 읽는다.
    for lock,gump,create,select in itertools.product((0,1,2,0xffffffff),(0,7),range(4),(0,1)):
        row=base.copy();row[0:2]=1,lock;row[4]=gump;row[9]=create;row[12]=select;rows.append(row)
    # 전체 방향 DWORD와 signed 마스크 경계를 정상/중첩/감김 깊이에서 관측한다.
    for lock,step,direction in itertools.product((0,1,0xffffffff),(0,1,2,0xfffffffe,0xffffffff,0x7fffffff,0x80000000),(0,1,7,8,0xfffffff9,0x7fffffff,0x80000000)):
        row=base.copy();row[1:4]=lock,step,direction;rows.append(row)
    # group1 또는 특별 TYPE만 사정거리를 재생성하며 반환0도 그대로 보존한다.
    for group,selected,special,result in itertools.product((0,1,3,0xffffffff),(100,140),(100,140),(0,1)):
        row=base.copy();row[5:8]=group,selected,special;row[13]=result;rows.append(row)
    # 다른 경계는 한 번에 하나만 바꾸어 재조회 위치를 구분한다.
    for slot,count in ((8,4),(9,4),(10,3),(11,3),(12,2)):
        # 선택한 경계의 각 변경은 별도 입력 한 행이다.
        for mutation in range(1,count):
            row=base.copy();row[slot]=mutation;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """세 실제 부모 몸체만 실행하고 표시/음향/기존 하위 몸체는 명시 효과로 둔다."""
    def __init__(self):
        """같은 보호 PE와 기존 격리 메모리/반환 검사를 재사용한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter()

    def instruction(self,mu,address,size,data):
        """선택한 효과가 바꾼 저장소는 원본의 다음 접근에서만 읽히도록 적용한다."""
        self.instructions+=1;esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<6I',mu.mem_read(esp+4,24))
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x445aa0:
            self.event('A',args[0]);mode=self.case[8]
            if mode==1:self.put(0x5325c8,3)
            if mode==2:self.put(0x55a2a4,0xfffffff9)
            if mode==3:self.put(0x55a280,TYPES+101*500);self.put(0x55a284,self.case[7])
            self.ret();return
        if address==0x445b30:
            self.event('B');mode=self.case[9]
            if mode==1:self.put(0x55a27c,9)
            if mode==2:self.put(0x5325c8,5)
            if mode==3:self.put(0x55a1d8,101);self.put(TYPES+100*500+0x9c,1)
            self.ret();return
        if address==0x4a4410:
            self.event('K',args[0])
            if self.case[12]:self.put(0x55a280,TYPES+101*500)
            self.ret();return
        if address==0x4a9cb0:
            self.event('S',self.string(args[0]),*args[1:]);mode=self.case[10]
            if mode==1:self.put(0x5325c8,1)
            if mode==2:self.put(0x55a280,TYPES+101*500)
            self.ret();return
        if address==0x4b1a90:
            self.event('D',*args[:4]);mode=self.case[11]
            if mode==1:self.put(0x55a1d8,102)
            if mode==2:self.put(0x55a280,TYPES+100*500);self.put(TYPES+100*500+0x9c,2)
            self.ret();return
        if address==0x474900:
            self.event('G',*args[:2]);self.ret(PROCESS if self.case[13] else 0);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'커서 동작 범위 밖 실행 {address:08x}')

    def sample(self,case,precision):
        """수식 기대값 없이 실제 반환 후 전역과 명시 효과 순서를 기록한다."""
        self.case=case;self.events=[];self.mu.mem_write(0x55a198,bytes(0x55a440-0x55a198))
        # 두 TYPE 논리 포인터는 번호로 정규화하며 group은 실제+9c에서 읽힌다.
        for type in (100,101):self.put(TYPES+type*500+0x9c,case[5] if type==100 else 1)
        # 여섯 원본 전역은 각 독립 입력에서만 초기화한다.
        for address,value in zip(GLOBALS,(case[1],case[3],case[4],case[6],100,PROCESS+16)):self.put(address,value)
        self.put(0x55a280,TYPES+100*500);self.put(0x541288,case[7]);self.put(0x54113c,51)
        args=b'' if case[0]==1 else struct.pack('<I',case[2]);self.call(ENTRIES[case[0]],0,args,0,precision);self.returns+=1
        values=[self.get(address) for address in GLOBALS];values[-1]=1 if values[-1]==PROCESS else 2 if values[-1]==PROCESS+16 else values[-1]
        values.extend(((self.get(0x55a280)-TYPES)//500,self.get(TYPES+100*500+0x9c),self.get(TYPES+101*500+0x9c)))
        return ';'.join(self.events) or '-',','.join(map(str,values))


def main():
    """새 관측을 생성하거나 기존 fixture의 원본 입력/정밀도/해시를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        # 고정 입력의 순서와 전체 파일 SHA를 함께 검사한다.
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert [row[0] for row in rows]==[','.join(map(str,case)) for case in cases()]
        assert len(rows)==report['uniqueInputs'] and report['normalReturns']==2*len(rows) and not report['precisionDifferentInputs']
        assert report['controls']==[0x27f,0x37f] and set(report['nativeCalls'])=={f'{value:08x}' for value in ENTRIES}
        assert sum(report['nativeCalls'].values())==report['normalReturns']
        # 모든 행의 출력 폭을 확인하되 출력 정수/분기를 다시 계산하지 않는다.
        for row in rows:assert len(row)==3 and len(row[2].split(','))==9
        print('커서 잠금/회전 근거 감사:',len(rows),'입력/',report['normalReturns'],'정상 반환');return
    oracles=[Oracle(),Oracle()];rows=[]
    # 두 정밀도의 실제 출력끼리 먼저 비교하고 호스트 산술을 기대값으로 사용하지 않는다.
    for case in cases():
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    FIXTURE.write_text('# input events globals\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=['originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_previews.py','tools/decomp_cursor_position.py','tools/decomp_cursor_actions.py',FIXTURE.relative_to(ROOT).as_posix()]
    report=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',date='2026-10-17',unit=136,controls=[0x27f,0x37f],entries=[f'{value:08x}' for value in ENTRIES],uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),instructionObservations=sum(oracle.instructions for oracle in oracles),precisionDifferentInputs=[],nativeCalls=dict(sum((oracle.calls for oracle in oracles),collections.Counter())),sha256={path:digest(ROOT/path) for path in paths},limitations=['실제 세 부모 몸체와 보존GP/스택/canary/x87 균형만 새 실행; Activate/Create/커서/음향/ContainedLookup/Range는 명시 효과다.','경계 뒤 현재 잠금·방향·Gump·TYPE포인터·첫SID 재조회와 signed 마스크 꼬리를 기록한다.','10.78 전용이며 CD 비교 주소만 문서화한다. 실제 Client 전체 입력·생산 GUI·사용자 자연 플레이는 별도다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('커서 잠금/회전:',len(rows),'입력/',report['normalReturns'],'정상 반환/정밀도차이0')


if __name__=='__main__':main()  # 명시 실행에서만 새 CPU 관측을 수행한다.
