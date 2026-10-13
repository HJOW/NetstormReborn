#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""소유 사제 조회 전체를 세 실제 PE의 제한 x86으로 실행한다. 대체 함수/게임/OS는 없다."""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,POOL,LISTS,STACK,STOP,CAPACITY,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 전체 조회와 공유 목록의 전역 포인터/개수 주소다.
SPEC={'originals':dict(entry=0x4921f0,list=0x5954c4,count=0x5954cc),
      'originalCD':dict(entry=0x40d940,list=0x549188,count=0x549190)}
SPEC['original1037']=dict(SPEC['originalCD'])
# 두 x87 정밀도와 저장할 독립 관찰 파일이다. 조회 자체는 부동소수점 명령을 쓰지 않는다.
CONTROLS=(639,895)
FIXTURE=ROOT/'cpppj/tests/fixtures/priestlookup-x86.tsv'
REPORT=ROOT/'cpppj/priestlookup.json'


def inputs():
    """소유자 DWORD 폭·모든 주요 상태 비트·signed 개수·순서/중복/예약 0을 교차한다."""
    cases=[]
    # 비활성 꼬리의 일치 항목도 입력에 남겨 개수 밖 접근을 검출한다.
    for player,state,count,entries in itertools.product((0,1,3,8,255,259,0xffffffff),
            (0,1,2,3,4,5,6,8,9,10,255),(0,1,2,3,4,0x80000000,0xffffffff),
            ((50,51,52,50),(52,50,51,50),(0,50,51,52))):
        cases.append(dict(player=player,count=count,entries=entries,nodes=(
            (0,1,0,3,255,255),(50,82,state,0 if state==8 else 3,1,255),(51,158,0,1,3,0),(52,6,4,255,3,9))))
    return cases


def input_text(case):
    """기대 결과를 계산하지 않고 입력의 고정 열만 직렬화한다."""
    return [str(case['player']),str(case['count']),','.join(map(str,case['entries'])),
        ';'.join(','.join(map(str,node)) for node in case['nodes'])]


class PriestLookupOracle(OwnerOracle):
    """원본 조회의 한 몸체와 스택 쓰기만 허용한다."""
    def __init__(self,edition):
        """공통 PE 로더를 쓰되 실제 실행 범위는 새 전체 조회로 교체한다."""
        super().__init__(edition);self.loader_exports=list(self.exports);self.r=SPEC[edition];self.returns=0
        self.exports=[ROOT/f'extracted/priestlookup/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra가 계산한 각 불연속 범위를 포함 끝 주소에서 반열린 구간으로 바꾼다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 함수 몸체 밖 실행은 공통 instruction 훅이 거부한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        if self.entries!={self.r['entry']}:raise RuntimeError('소유 사제 내보내기 진입점 오류')
        self.mu.mem_write(self.r['list'],struct.pack('<I',LISTS))

    def run_lookup(self,case,control):
        """합성 입력에서 정상 반환까지 실행해 DWORD 결과와 입력 불변/ABI를 검사한다."""
        mu=self.mu;raw=bytearray(CAPACITY*self.stride)
        # 원래 owner WORD·타입·extra도 공급해 원본이 이들을 검사하지 않는지 대조한다.
        for sid,typ,state,owner,original,extra in case['nodes']:
            offset=sid*self.stride;raw[offset+10]=typ;raw[offset+11]=state;raw[offset+12]=original
            raw[offset+self.o['owner']]=owner;raw[offset+self.o['extra']]=extra
        mu.mem_write(POOL,bytes(raw));storage=struct.pack('<4I',*case['entries']);mu.mem_write(LISTS,storage)
        mu.mem_write(self.r['count'],struct.pack('<I',case['count']))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 보존 레지스터는 서로 다른 비트로 준비해 잘못된 복구를 검출한다.
        for register,value in preserved.items():mu.reg_write(register,value)
        # 휘발 레지스터는 앞 호출 결과에 의존하지 않도록 초기화한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_ESP,STACK)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.mem_write(STACK,struct.pack('<II',STOP,case['player']))
        mu.emu_start(self.r['entry'],STOP,count=20000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('소유 사제 반환/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('소유 사제 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('소유 사제 x87 균형 오류')
        if bytes(mu.mem_read(POOL,len(raw)))!=raw or bytes(mu.mem_read(LISTS,16))!=storage:raise RuntimeError('소유 사제 조회 입력 변경')
        self.returns+=1
        return [mu.reg_read(UC_X86_REG_EAX),zlib.adler32(raw)&0xffffffff,zlib.adler32(storage)&0xffffffff]


def generate(smoke=False):
    """세 PE를 각각 실행하고 두 정밀도 결과가 일치할 때만 fixture와 SHA 근거를 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',FIXTURE,ROOT/'tools/ghidra/priestlookup-functions.json'}
    # 각 실제 PE의 전체 조회를 독립적으로 실행한다.
    for edition in SPECS:
        oracle=PriestLookupOracle(edition);plan=inputs()[::97] if smoke else inputs()
        # 출력과 checksum은 입력 생성기가 아니라 실제 명령 실행기에서 얻는다.
        for case in plan:
            observed=[oracle.run_lookup(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('소유 사제 두 정밀도 관찰 차이')
            rows.append([edition,*input_text(case),*observed[0]])
        editions[edition]=dict(cases=len(plan),returns=oracle.returns,native_calls=dict(oracle.native_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.update(oracle.loader_exports);paths.add(ROOT/oracle.spec['binary'])
        print(f'{edition}: priestlookup {len(plan)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 소유 사제 조회 전체. 대체 함수 없음. 게임/OS 미실행.\n'
        '# 판본 player count entries nodes(sid,type,state,owner,original,extra) 결과 rawAdler listAdler\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',controls=list(CONTROLS),
        os_calls=0,substitutions={},editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},
        limits=['실제 전체 함수, 스택/보존 레지스터/x87 정상 반환, 허용 실행/쓰기와 raw/목록 입력 불변',
            '합성 128슬롯·유효 주소의 DWORD 목록. 손상된 양수 개수/풀 밖 SID는 C++ 안전 계약 검사',
            '소유 사제 조회만 실행. 기준점·확정·통지의 조합은 C++ 통합 검사이며 기존 독립 근거를 재사용']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'total: {len(rows)}',flush=True)


def verify():
    """결과를 재계산하지 않고 SHA·입력 순서·DWORD 폭·진입/정상 반환을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든 근거 파일이 생성 시점 그대로인지 검사한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or report['substitutions']:raise RuntimeError('소유 사제 합계/경계 오류')
    # 각 판본의 원본 행을 입력 계획과 비교하고 실제 결과값은 폭만 검사한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];plan=inputs();entry=f"{SPEC[edition]['entry']:08x}"
        if len(chosen)!=len(plan) or item['cases']!=len(plan) or item['returns']!=2*len(plan) or item['assertions']:raise RuntimeError('소유 사제 반환 합계 오류')
        if item['native_calls']!={entry:2*len(plan)}:raise RuntimeError('소유 사제 실제 몸체 진입 오류')
        # 저장 출력에서 기대 선택을 다시 만들지 않는다.
        for row,case in zip(chosen,plan,strict=True):
            if len(row)!=8 or row[1:5]!=input_text(case) or any(not 0<=int(v)<=0xffffffff for v in row[5:]):raise RuntimeError('소유 사제 입력/폭 오류')
    print(f'priestlookup 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사 중 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
