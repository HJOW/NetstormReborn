#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 실제 PE의 배치 무시 함수와 사제 충돌 후보 구간만 격리 실행한다.

후보 구간에는 원본 MayPlace의 지역 변수/현재 SID를 입력으로 공급한다.
모양/미리보기 초기화/finder/지형/전체 MayPlace는 실행하지 않는다. 게임/OS 실행 없음.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STACK,STOP,digest
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 함수/인라인 후보 구간/전역과 지역 변수의 판본별 주소다.
SPEC={
 'originals':dict(ignore=0x49ade0,entry=0x49b9c4,accept=0x49bb1a,reject=0x49bcd1,
    mask=0x59a9f0,globalMask=0x5325b4,client=0x540bc0,mode=0x294,qx=0x284,qy=0x288,local=0x68,sid=0xc8,self=0x14,getType=0x4ac550,ftol=0x4e49c0),
 'originalCD':dict(ignore=0x444900,entry=0x4456ad,accept=0x4457f7,reject=0x445ad4,
    mask=0x565998,globalMask=0x53fc28,client=0x540a28,mode=0x244,qx=0x234,qy=0x238,local=0x5c,sid=0xcc,getType=0x4abf50,ftol=0x4f161c),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 새 관찰 파일과 고정 입력 열 순서다. 기대 결과는 실제 명령만 계산한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priestcollision-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestcollision-evidence.json'
KEYS=('kind','own','ownFlags','other','mode','globalMask','client','sid','local','qx','qy','cx','cy','wx','wy','ox','oy','extra','seed')


def inputs(edition):
    """분기 마스크·반환 비트값과 SID 경계/발자국/부분 표시 입력을 생성한다."""
    cases=[]
    selves=(0,4,0x20000,0x10000,0x200000,0x210000,0x100,0x8000,0x02000000,0x200004)
    others=(0,2,4,0x10000,0x20000,0x100000,0x200000,0x8000,0x02000000,0x208000,0x218000,0x4000)
    # helper의 일반 타입 분기도 독립 호출로 실행한다.
    for own,other,mode,flags,mask in itertools.product(selves,others,(0,1,0xffffffff),(0,0x800),(0,0x200000,0xffffffff)):
        cases.append(dict(kind='I',own=own,ownFlags=flags,other=other,mode=mode,globalMask=mask,
            client=0,sid=50,local=0,qx=bits(20),qy=bits(21),cx=bits(20),cy=bits(21),wx=3,wy=2,ox=2,oy=3,extra=0,seed=0))
    end=15000 if edition=='originals' else 6000
    profiles=((0,50,0,0,1),(0,50,0,1,1),(0,50,1,1,1),(0,50,0xffffffff,1,1),
        (0,50,1,0,0),(1,4,1,0,1),(1,5,1,0,1),(1,end-1,1,0,1),(1,end,1,0,1),(2,end+1,1,1,1))
    positions=((20,21),(20.5,21.5),(19.5,22.5),(23,18),(20.000001907348633,21.999998092651367),(19.999998092651367,20.000001907348633))
    # 0/1/전체 DWORD mode·extra·비로컬 조건을 같은 좌표/발자국과도 조합한다.
    for index,(own,other,profile,pos,mask) in enumerate(itertools.product((0x200000,0x210000),(0,2,4,0x10000,0x8000,0x200000,0x02000000),profiles,positions,(0,0x200000))):
        client,sid,mode,extra,local=profile
        cases.append(dict(kind='C',own=own,ownFlags=0x800,other=other,mode=mode,globalMask=mask,
            client=client,sid=sid,local=local,qx=bits(20.5),qy=bits(21.5),cx=bits(pos[0]),cy=bits(pos[1]),
            wx=(1,3,4)[index%3],wy=(1,2,4)[index%3],ox=(1,2,3)[index%3],oy=(3,1,2)[index%3],extra=extra,seed=index%2))
    return cases


class CollisionOracle(OwnerOracle):
    """실제 helper와 후보 분기/표시/CRT의 허용 범위만 실행한다."""
    def __init__(self,edition):
        """원본 포인터 산술의 서버 SID 경계까지 분석 전용 풀을 확장한다."""
        super().__init__(edition);self.s=SPEC[edition]
        size=(24000*self.stride+4095)&~4095;old=(128*self.stride+4095)&~4095
        self.mu.mem_map(POOL+old,size-old)
        self.exports=[ROOT/f'extracted/priestcollision/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.candidate_entries=0;self.region_exits=0
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra가 보고한 실제 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 주소 범위를 반열린 구간으로 변환한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_write(self,mu,access,address,size,value,data):
        """지역 스택과 정확히 144바이트 표시 쓰기만 허용한다."""
        if self.s['mask']<=address and address+size<=self.s['mask']+144:return
        super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """한 후보의 지형 처리 이전 성공과 실제 거부 분기에서 관찰을 마친다."""
        if self.case['kind']=='C' and address in (self.s['accept'],self.s['reject']):
            self.result=int(address==self.s['accept']);self.region_exits+=1;mu.reg_write(UC_X86_REG_EIP,STOP);return
        if self.case['kind']=='C' and address==self.s['entry']:self.candidate_entries+=1
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """helper ABI 또는 후보 스택/x87 균형과 전체 표시/raw를 관찰한다."""
        self.case=case;self.instructions=0;self.write_ranges=[];s=self.s;mu=self.mu;self.result=None
        own=TYPES+82*self.type_stride;other=TYPES+83*self.type_stride
        # 타입과 raw 입력에는 예측값을 넣지 않는다.
        for address,flags,genus,wx,wy in ((own,case['ownFlags'],case['own'],case['wx'],case['wy']),(other,0,case['other'],case['ox'],case['oy'])):
            mu.mem_write(address+0xe8,struct.pack('<II',flags,genus));foot=0x1d4 if self.edition=='originals' else 0x1b4
            mu.mem_write(address+foot,struct.pack('<ii',wx,wy))
        raw=bytearray([0xcd]*self.stride);raw[10]=83;raw[self.o['extra']]=case['extra']
        raw[14:22]=struct.pack('<II',case['cx'],case['cy']);mu.mem_write(self.slot(case['sid']),bytes(raw))
        mu.mem_write(s['mask'],bytes([0xa5 if case['seed'] else 0])*144)
        mu.mem_write(s['globalMask'],struct.pack('<I',case['globalMask']));mu.mem_write(s['client'],struct.pack('<I',case['client']))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 독립 helper 호출에서 보존 레지스터도 확인한다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_EAX,case['sid']);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_ECX,own);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        if case['kind']=='I':mu.mem_write(STACK,struct.pack('<3I',STOP,83,case['mode']));entry=s['ignore']
        else:
            # 실제 후보 진입 시의 필요한 지역 변수/레지스터만 공급한다. 전체 MayPlace 실행으로 해석하지 않는다.
            for key in ('mode','qx','qy','local','sid'):mu.mem_write(STACK+s[key],struct.pack('<I',case[key]))
            if self.edition=='originals':mu.mem_write(STACK+s['self'],struct.pack('<I',own))
            else:mu.reg_write(UC_X86_REG_EBX,own);mu.reg_write(UC_X86_REG_EAX,POOL)
            entry=s['entry']
        mu.emu_start(entry,STOP,count=20000)
        wanted=STACK+12 if case['kind']=='I' else STACK
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=wanted:raise RuntimeError('후보/helper 종료 스택 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('후보/helper x87 균형 오류')
        if case['kind']=='I':
            if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('무시 helper 보존 레지스터 오류')
            self.result=mu.reg_read(UC_X86_REG_EAX)
        if self.result is None:raise RuntimeError('후보 관찰 종료 누락')
        return [self.result,bytes(mu.mem_read(s['mask'],144)).hex(),bytes(mu.mem_read(self.slot(case['sid']),self.stride)).hex()]


def generate(smoke=False):
    """두 x87 정밀도가 일치하는 실제 관찰을 새 fixture로 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/priestcollision-functions.json',FIXTURE}
    # 세 PE를 각각 읽고 독립 실행한다.
    for edition in SPECS:
        oracle=CollisionOracle(edition);cases=inputs(edition)
        if smoke:cases=cases[:4]+[c for c in cases if c['kind']=='C'][:40]
        # 기대값은 Python 판정 함수가 아니라 기계어의 반환/메모리다.
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError(f'x87 정밀도 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*values[0]])
        editions[edition]=dict(cases=len(cases),helper=sum(c['kind']=='I' for c in cases),candidates=sum(c['kind']=='C' for c in cases),native_calls=dict(oracle.native_calls),candidate_entries=oracle.candidate_entries,region_exits=oracle.region_exits,assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 무시/후보 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 무시 함수 및 한 후보 인라인 구간. 모양/finder/지형/전체 MayPlace 미실행.\n# edition '+' '.join(KEYS)+' result mask raw\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},limits=['무시 helper 실제 thiscall 정상 반환/ABI/전체 DWORD 반환 대조','후보는 MayPlace 중간 구간에 지역 변수/현재 SID를 공급하며 helper/타입/CRT/표시/거부 분기를 실제 실행','후보 구간 끝에서 관찰 종료: 전체 MayPlace ABI/모양/preview/finder/지형 실행 검증 아님','C++ 실제 finder 연결은 별도 통합 검사이며 전체 모양/지형은 외부 경계']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """입력/PE/내보내기 SHA와 실제 helper/후보 관찰 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    # 근거의 원본 파일을 전부 검사한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('후보 근거 오류')
    # 실제 native 함수와 종료 구간의 횟수를 대조한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];s=SPEC[edition]
        helper=sum(row[1]=='I' for row in selected);candidate=sum(row[1]=='C' for row in selected)
        if (len(selected),helper,candidate)!=(item['cases'],item['helper'],item['candidates']) or item['assertions']:raise RuntimeError('후보 행/분류 오류')
        if item['candidate_entries']!=2*candidate or item['region_exits']!=2*candidate:raise RuntimeError('실제 후보 구간 누락')
        if item['native_calls'].get(f'{s["getType"]:08x}')!=2*candidate or item['native_calls'].get(f'{s["ignore"]:08x}',0)<2*helper:raise RuntimeError('실제 타입/무시 helper 누락')
        if not item['native_calls'].get(f'{s["ftol"]:08x}',0):raise RuntimeError('실제 CRT 표시 누락')
    print(f'priestcollision 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
