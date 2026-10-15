#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""튜토리얼2 전체 기계어와 기존 실제 gate/제작 getter를 GUI 없이 독립 실행한다."""
import argparse
import collections
import csv
import json
import math
import struct
from pathlib import Path
import decomp_missionframe_oracle as common
from decomp_missionframe_oracle import bits, digest, base_case, state_text
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 원본 기존 module의 주소와 분리된 합성 raw TYPE/선택 객체다.
ROOT=common.ROOT
ENTRY=0x4c3bb0
POOL,TYPES=0x12000000,0x13000000
FIXTURE=ROOT/'cpppj/tests/fixtures/tutorial-two-x86.tsv'
REPORT=ROOT/'cpppj/tutorial-two.json'
FOLDER=ROOT/'extracted/tutorial-two'
# 실제 옵션 표는8바이트를 관찰하여 BYTE2/4 밖 보존을 확인한다.
OPTIONS=0x52f7e0


def case(stage):
    """기대 결과 없이 튜토리얼 입력과 실제 공유 미션 상태를 준비한다."""
    value=base_case();value['state'].update(stage=ord(stage),timer=bits(0.0))
    value.update(temples=0,workshops=0,archers=0,selected=0,production=0,tell=0,options='030405060708090a',deny=7,allowed=9)
    return value


def cases():
    """A~H 조건/시각 포함경계/정지/잠금/선택/생산/재진입 입력을 생성한다."""
    result=[]
    # 신전/워크샾의 signed 제작수와 Advance 거부 뒤에도 남는 부수효과다.
    for stage in ('A','B'):
        for count in (-1,0,1,0x7fffffff):
            for depth in (0,2):
                for lock in (-1,0,1):
                    value=case(stage);value.update(depth=depth);value['temples' if stage=='A' else 'workshops']=count
                    value['state']['lock']=lock;result.append(value)
    # C는 현재 선택을 매번 읽으며 예약중 선택 해제 시 timer를 보존한다.
    for selected in (0,1,2):
        for timer in (0.0,math.nextafter(10.0,-math.inf),10.0,math.nextafter(10.0,math.inf)):
            for production in (0,1):
                for tell in (0,1):
                    for depth in (0,2):
                        value=case('C');value.update(selected=selected,production=production,tell=tell,depth=depth)
                        value['state']['timer']=bits(timer);result.append(value)
    # D/E/G는같은두번째로컬제작표의signed DWORD 경계를읽는다.
    for stage in ('D','E','G'):
        for count in (-1,0,1,2,3,4,0x80000000,0x7fffffff):
            for depth in (0,2):
                for lock in (0,1):
                    value=case(stage);value.update(archers=count,depth=depth);value['state']['lock']=lock
                    if stage=='G':value['state']['timer']=bits(17.0)
                    result.append(value)
    # F는예약후선택을재검사하지않고같은시각에도진행한다.
    for selected in (0,1,2):
        for timer in (0.0,math.nextafter(10.0,-math.inf),10.0,math.nextafter(10.0,math.inf)):
            for depth in (0,2):
                for lock in (0,1):
                    value=case('F');value.update(selected=selected,depth=depth);value['state'].update(timer=bits(timer),lock=lock);result.append(value)
    # H의회수사건은상위modifier를제거하며예약후다른사건에서도타이머만판정한다.
    for event in (0x1230000,0xf1230000,0x1240000):
        for timer in (0.0,math.nextafter(10.0,-math.inf),10.0,math.nextafter(10.0,math.inf)):
            for depth in (0,2):
                for lock in (0,1):
                    value=case('H');value.update(event=event,depth=depth);value['state'].update(timer=bits(timer),lock=lock);result.append(value)
    # 성공I/범위밖은 조건 처리를 실행하지 않는 원본switch의 기본분기다.
    for stage in ('I','@','J'):result.append(case(stage))
    # double 현재 시각의 단정도 축소를 검출하는 실제2/4초예약이다.
    for stage in ('C','F','H'):
        value=case(stage);value.update(now=bits(16777216.25),selected=2,event=0x1230000);result.append(value)
    # 명시 외부 효과가 현재 장부/선택/타이머/정지를 바꾸는 재진입 입력이다.
    for mutation in (1,2,3,4):
        value=case('A' if mutation==1 else 'C');value.update(mutation=mutation,selected=2,tell=1)
        if mutation!=1:value['state']['timer']=bits(10.0)
        result.append(value)
    unique={}
    # 같은 명시 입력은 새 원본 관찰 수에 중복 합산하지 않는다.
    for value in result:unique.setdefault(json.dumps(value,sort_keys=True),value)
    return list(unique.values())


def columns(value):
    """공통 미션10입력과 현재 제작/옵션/선택/생산 입력을 순서대로 기록한다."""
    return [*common.input_columns(value),value['temples']&0xffffffff,value['workshops']&0xffffffff,value['archers']&0xffffffff,
        value['selected'],value['production'],value['tell'],value['options'],value['deny'],value['allowed']]


class Oracle(common.Oracle):
    """기존 gate 실행기를 재사용하며 실제 Tutorial2·제작·선택/genusgetter를 추가한다."""
    def __init__(self):
        """원본 함수 범위와 서로 다른 raw 입력 영역을 보존한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.mu.mem_map(TYPES,0x20000)
        self.extra=[(ENTRY,0x4c3ed1),(0x4d5430,0x4d5436),(0x4ac200,0x4ac21a)]
        self.exports=ROOT/'extracted/tutorial-one/originals/functions.tsv'
        with self.exports.open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp,delimiter='\t'):
                if row['entry'] not in ('004c23e0','004c24c0','004c2500','0049a840'):continue
                for part in row['ranges'].split(';'):
                    low,high=(int(field,16) for field in part.split('-'));self.extra.append((low,high+1))
        self.calls=collections.Counter();self.checking=False

    def mutate(self,kind):
        """명시 외부 하위 첫 효과만 현재 공유 입력을 바꾼다."""
        if kind in self.changed:return
        self.changed.add(kind);mutation=self.case['mutation']
        if mutation==1 and kind=='R':self.put(0x5c98d0+123*4,1)
        elif mutation==2 and kind=='Tell':self.case['production']=1
        elif mutation==3 and kind=='Z':self.put(common.MISSION+0x98,bits(99.0),8)
        elif mutation==4 and kind=='K':self.depth=3;self.put(common.MISSION+0xb8,7)

    def instruction(self,mu,address,size,data):
        """새 실제 몸체는 그대로 실행하고 UI/범위/생산만 명시 ABI 경계로 관찰한다."""
        entries={ENTRY:'Two',0x4c33f0:'Advance',0x4c2a40:'Pause',0x4c23e0:'Allow',0x4c2500:'GenusBuilt',0x4c24c0:'Built',0x4d5430:'Selected',0x4ac200:'Genus'}
        if address in entries:self.calls[entries[address]]+=1
        if address==ENTRY:self.put(common.MISSION,0x20000200);self.put(0x20000228,0x4c2a40)
        if address==0x4d5430:self.trace.append('S')
        boundaries={0x4b4860:'R',0x44f970:'C',0x4d5c90:'Z',0x4d5c60:'X',0x482de0:'Tell',0x466710:'K'}
        kind=boundaries.get(address)
        if kind:
            self.boundaries[kind]+=1;esp=mu.reg_read(UC_X86_REG_ESP);result=0;cleanup=0
            if kind=='C':
                if self.get(esp+4)!=71:raise RuntimeError('현재 생산 조회 타입 불일치')
                self.trace.append('C71');result=self.case['production']
            else:self.trace.append(kind)
            self.mutate(kind)
            if kind=='Tell':
                name=bytes(mu.mem_read(self.get(esp+4),32)).split(b'\0')[0]
                if name!=b'NotVortex' or mu.reg_read(UC_X86_REG_ECX)!=common.MISSION:raise RuntimeError('실제 NotVortex 인자 오류')
                result=self.case['tell'];cleanup=4
            if kind=='Z' and any(self.get(esp+offset) for offset in (4,8,12)):raise RuntimeError('선택 초기화 실제0/0/0 오류')
            if kind in ('Z','X'):self.put(0x5caea0,0)
            ret=self.get(esp);mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);mu.reg_write(UC_X86_REG_EIP,ret);return
        if any(low<=address and address+size<=high for low,high in self.extra):return
        super().instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """실제 옵션 두BYTE/회수금지/허용표 밖의 추가 쓰기는 허용하지 않는다."""
        if address in (OPTIONS+2,OPTIONS+4) and size==1:return
        if address==0x595078 and size==4:return
        if address==0x5ca0d0+85*4 and size==4:return
        super().write(mu,access,address,size,value,data)

    def run(self,value):
        """현재 입력을 쓴 뒤 기존 실제 gate의 정상 반환/스택/레지스터/x87 검사로 실행한다."""
        self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x541348,188)
        self.mu.mem_write(POOL,bytes(0x10000));self.mu.mem_write(TYPES,bytes(0x20000))
        # 선택1은무관한타입,선택2는신전이며실제TYPE+ec를읽는다.
        self.put(POOL+50+10,71,1);self.put(POOL+100+10,123,1)
        for typ,genus in ((71,0x20),(85,0x4000),(123,0x200)):self.put(TYPES+typ*500+0xec,genus)
        self.mu.mem_write(0x5c98d0,bytes(1024))
        for typ,key in ((71,'archers'),(85,'workshops'),(123,'temples')):self.put(0x5c98d0+typ*4,value[key])
        self.put(0x541174,71);self.put(0x5411ac,85);self.put(0x54286c,0x1230000);self.put(0x595078,value['deny'])
        self.put(0x5ca0d0+85*4,value['allowed']);self.put(0x5caea0,value['selected']);self.mu.mem_write(OPTIONS,bytes.fromhex(value['options']))
        # 공통입력초기화 뒤 첫 실제명령에서 합성객체의 가상표를 설치한다.
        common.ENTRIES['Frame']=ENTRY
        result=super().run(value)
        return [*result,bytes(self.mu.mem_read(OPTIONS,8)).hex(),self.get(0x595078),self.get(0x5ca0d0+85*4),self.get(0x5caea0)]


def generate():
    """원본 독립 기대값과 해시/입력/실제 정상 반환의 근거를 저장한다."""
    oracle=Oracle();rows=[]
    for value in cases():rows.append([*columns(value),*oracle.run(value)])
    FIXTURE.write_text('# PATCH Tutorial2 실제 몸체·Advance/Pause/TotalMade/Genus 독립 관찰. GUI/OS 실행0.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    FOLDER.mkdir(parents=True,exist_ok=True);decoder=common.capstone.Cs(common.capstone.CS_ARCH_X86,common.capstone.CS_MODE_32);assembly=[]
    # 새 몸체만 실제 바이트로 분해한다. jump table 데이터는 명령 범위에서 제외한다.
    for low,high in oracle.extra:
        for item in decoder.disasm(oracle.data[low-oracle.base:high-oracle.base],low):assembly.append(f'{item.address:08x} {item.bytes.hex():<24} {item.mnemonic} {item.op_str}')
    assembly_path=FOLDER/'assembly.txt';assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),Path(common.__file__),common.BINARY,FIXTURE,assembly_path,oracle.exports,ROOT/'extracted/refined/originals/Netstorm.c']
    report=dict(host=common.socket.gethostname(),date='2026-10-15',edition='10.78',total=len(rows),returns=oracle.returns,native=dict(oracle.calls),boundaries=dict(oracle.boundaries),os_calls=0,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in paths},
        limits=['Tutorial2 A~H·공유Advance/Pause·현재 선택/Genus/TotalMade는 실제 기계어다. 함수바이트 수정은 없다.',
            '옵션 범위 갱신·선택 초기화/해제·생산 목록 조회·Tell/팝업·시계 정지깊이는 명시 입력/관찰 경계다.',
            '실제 게임/GUI/Ready/login/건설/생산/수확/완주 및 CD10.72는 이번 검사 범위가 아니다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Tutorial2 실제 PE',len(rows),'정상 반환',dict(oracle.calls),flush=True)


def verify():
    """저장 근거 해시·입력 계획과 원본 정상 반환 수를 기대값 재계산 없이 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    for path,sha in report['files'].items():
        if digest(ROOT/path)!=sha:raise RuntimeError('Tutorial2 근거 SHA 불일치 '+path)
    plan=cases()
    if len(rows)!=len(plan) or report['returns']!=len(rows) or report['native']['Two']!=len(rows) or report['os_calls']:raise RuntimeError('Tutorial2 실행 합계 불일치')
    for row,value in zip(rows,plan):
        if len(row)!=27 or row[:19]!=list(map(str,columns(value))):raise RuntimeError('Tutorial2 명시 입력 계획 불일치')
    print('Tutorial2 독립 근거 감사 통과',len(rows),flush=True)


def main():
    """요청에 따라 독립 관찰 생성 또는 저장 근거 감사를 수행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate()


if __name__=='__main__':main()
