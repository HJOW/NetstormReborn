#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 세 PE의 사제 preDestroy를 정상 반환까지 실행한다.

목록 압축·보호막 SID 범위 검사·호출 순서는 원본 명령이다.
보호막 조회/가상 삭제와 Carrier 몸체만 외부 기록으로 대체한다. 게임/OS 실행은 없다.
"""
import argparse
import collections
import csv
import itertools
import json
import random
import struct
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle, ROOT, SPECS, POOL, STOP, STACK, LISTS, TARGET, digest,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 실제 월드 크기와 목록 용량, 격리된 가상 destroy/가상 표 기록 주소다.
CAPACITY, LIST_CAPACITY, DESTROY, VTABLE = 24000, 6, STOP+0x300, STOP+0x100
# 결과는 원본 PE가 결정한다. 이 시드는 추가 입력만 만든다.
SEED = 0x4919b0
# 판본별 preDestroy·외부 조회/Carrier·목록/풀 크기 전역·실제 사제 표다.
PRE = {
 'originals': dict(entry=0x4919b0, find=0x4918e0, carrier=0x426890, head=0x5954c4, capacity=0x5c847c, table=0x50f210),
 'originalCD': dict(entry=0x40c330, find=0x40bf00, carrier=0x4e43b0, head=0x549188, capacity=0x5395f4, table=0x5003e0),
}
PRE['original1037'] = dict(PRE['originalCD'])
# 독립 C++ 회귀 자료와 감사 기록이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/priestdestroy-x86.tsv'
REPORT = ROOT/'cpppj/recovery-priestdestroy-evidence.json'


def inputs():
    """활성/비활성 중복·상태·플래그·SID 양끝과 조회 중 raw 변경 입력을 만든다."""
    lists=[(0,[50,60,50,70,50,80]),(6,[50]*6),(6,[60,70,80,90,100,110]),
        (6,[50,60,50,70,50,80]),(3,[60,50,70,50,50,50]),(1,[50,60,70,50,80,90])]
    cases=[]
    # 같은 외부 인자를 여러 분기에서 관찰한다. 기대 판단은 여기서 만들지 않는다.
    for extra,state,flags,items,found in itertools.product((0,1,8,9,32),(0,2,6),(0,0x200000,0xffffffff),lists,(0,1,4,5,50,23999,24000,65535)):
        cases.append(dict(extra=extra,state=state,flags=flags,count=items[0],entries=items[1],found=found,after='-'))
    rng=random.Random(SEED)
    # 조회 뒤 extra가 바뀌어도 이미 선택한 보호막 삭제 경로와 Carrier 호출을 유지한다.
    for i in range(24):
        cases.append(dict(extra=0 if i<12 else rng.randrange(256),state=rng.choice((0,2,4,6)),flags=rng.getrandbits(32),
            count=rng.randrange(7),entries=[rng.choice((50,50,60,0,0xffffffff)) for _ in range(6)],
            found=rng.choice((0,1,4,5,50,23999,24000,65535)),after=rng.choice((0,1,8,9,32))))
    return cases


class PriestDestroyOracle(OwnerOracle):
    """기존 격리 메모리에 사제 삭제 준비의 정확한 실행/쓰기 범위만 더한다."""
    def __init__(self,edition):
        """현재 PC의 읽기 전용 Ghidra 내보내기를 불연속 명령 범위로 사용한다."""
        super().__init__(edition);self.p=PRE[edition];self.stub_calls=collections.Counter()
        base_exports=list(self.exports)
        size=(CAPACITY*self.stride+4095)&~4095
        self.mu.mem_map(POOL+0x2000,size-0x2000)
        self.exports=[ROOT/f'extracted/priestdestroy/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 몸체 범위 밖의 코드 호출은 여전히 거부한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_write(self.p['capacity'],struct.pack('<I',CAPACITY))
        self.mu.mem_write(VTABLE+0x10,struct.pack('<I',DESTROY))
        self.mu.mem_write(self.p['table']+0x10,struct.pack('<I',DESTROY))
        self.exports=base_exports+self.exports

    def event(self,marker,sid,flags=None):
        """각 외부 경계에서 이미 압축된 count와 현재 extra를 관찰한다."""
        count=struct.unpack('<I',self.mu.mem_read(self.p['head']+8,4))[0]
        extra=self.mu.mem_read(self.slot(TARGET)+self.o['extra'],1)[0]
        self.events.append(f'{marker}:{sid}'+('' if flags is None else f':{flags}')+f':{count}:{extra}')

    def on_instruction(self,mu,address,size,data):
        """조회/가상 삭제/Carrier만 기록 대체하며 ret 인자 폭도 원본대로 복구한다."""
        if address not in (self.p['find'],self.p['carrier'],DESTROY):
            super().on_instruction(mu,address,size,data);return
        esp=mu.reg_read(UC_X86_REG_ESP);args=0
        sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//self.stride
        if address==self.p['find']:
            if sid!=TARGET:raise RuntimeError('보호막 조회 this 오류')
            marker='F';self.event(marker,sid);mu.reg_write(UC_X86_REG_EAX,self.case['found'])
            if self.case['after']!='-':mu.mem_write(self.slot(TARGET)+self.o['extra'],bytes([self.case['after']]))
        else:
            args=4;flags=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
            marker='D' if address==DESTROY else 'C';self.event(marker,sid,flags)
            if marker=='D' and (sid!=self.case['found'] or flags):raise RuntimeError('보호막 삭제 인자 오류')
            if marker=='C' and (sid!=TARGET or flags!=self.case['flags']):raise RuntimeError('Carrier 인자 오류')
        self.stub_calls[marker]+=1
        ret=struct.unpack('<I',mu.mem_read(esp,4))[0];mu.reg_write(UC_X86_REG_ESP,esp+4+args);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,case):
        """목록 전체/raw 슬롯·정상 반환·thiscall 및 보존 레지스터를 관찰한다."""
        self.case=case;self.events=[];slot=self.slot(TARGET)
        raw=bytearray([0xab]*self.stride);struct.pack_into('<I',raw,0,self.p['table'])
        raw[10]=158;raw[11]=case['state'];raw[self.o['extra']]=case['extra'];self.mu.mem_write(slot,bytes(raw))
        found=case['found']
        if 0<found<CAPACITY and found!=TARGET:self.mu.mem_write(self.slot(found),struct.pack('<I',VTABLE))
        self.mu.mem_write(self.p['head'],struct.pack('<III',LISTS,LIST_CAPACITY,case['count']))
        self.mu.mem_write(LISTS,struct.pack('<6I',*case['entries']))
        self.write_ranges=[(LISTS,LISTS+24),(self.p['head']+8,self.p['head']+12)]
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # thiscall는 callee 보존 레지스터를 되돌려야 한다.
        for register,value in preserved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_EDX,0);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,slot)
        self.mu.mem_write(STACK,struct.pack('<II',STOP,case['flags']))
        self.mu.emu_start(self.p['entry'],STOP,count=20000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+8:raise RuntimeError('사제 pre 반환/스택 오류')
        if any(self.mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('사제 pre 보존 레지스터 오류')
        count=struct.unpack('<I',self.mu.mem_read(self.p['head']+8,4))[0]
        entries=struct.unpack('<6I',self.mu.mem_read(LISTS,24))
        return [count,','.join(map(str,entries)),';'.join(self.events),bytes(self.mu.mem_read(slot,self.stride)).hex()]


def generate(smoke=False):
    """원본 관찰만 UTF-8 fixture와 파일별 SHA 증거로 저장한다."""
    cases=inputs();rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/priestdestroy-functions.json',FIXTURE}
    if smoke:cases=cases[::73]
    # 세 실제 PE를 별도로 실행한다.
    for edition in SPECS:
        oracle=PriestDestroyOracle(edition)
        for case in cases:
            rows.append([edition,case['extra'],case['state'],case['flags'],case['count'],','.join(map(str,case['entries'])),case['found'],case['after'],*oracle.run(case)])
        editions[edition]=dict(cases=len(cases),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: preDestroy {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 사제 preDestroy 전체 반환. 조회/보호막 가상 삭제/Carrier만 대체.\n# edition extra state flags count entries found after countOut entriesOut events slot\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-08',normal_return=True,os_calls=0,editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['사제 preDestroy 본문과 목록 압축·보호막 SID 범위 검사만 실제 실행',
            '보호막 공간 조회/가상 삭제 및 Carrier→Damageable 몸체는 외부 기록 대체',
            '사제 전체 삭제·보호막 파생 수명·GUI/Pop의 완성을 뜻하지 않음']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """행/SHA·실제 진입·분기/대체 호출 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    if len(rows)!=report['total'] or not report['normal_return'] or report['os_calls']:raise RuntimeError('사제 pre 증거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=item['cases'] or item['assertions'] or item['native_calls'].get(f'{PRE[edition]["entry"]:08x}')!=len(selected):raise RuntimeError('사제 pre 실제 실행 수 오류')
        ordinary=sum((int(row[1])&9)==0 for row in selected)
        helper=0x40ea00 if edition=='originals' else 0x40c0d0
        if item['native_calls'].get(f'{helper:08x}')!=ordinary or item['substitutions'].get('F')!=ordinary or item['substitutions'].get('C')!=len(selected):
            raise RuntimeError('실제 목록 제거/보호막 helper 분기 수 오류')
        if any(not item['substitutions'].get(marker) for marker in ('F','D','C')):raise RuntimeError('사제 pre 외부 분기 누락')
        for marker,count in item['substitutions'].items():
            if count!=sum(sum(part.startswith(marker+':') for part in row[10].split(';')) for row in selected):raise RuntimeError('사제 pre 대체 수 오류')
    print(f'priestdestroy 검증 통과: {len(rows)}개')


def main():
    """전체 생성/무저장 표본 검사/저장 증거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
