#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 실제 PE의 Carrier preDestroy와 contained 조회를 전체 정상 반환까지 대조한다.

Carrier·조회·finder·기본 true 필터는 실제 명령이다. Damageable와 전역 후처리만 대체한다.
게임/OS/창 실행은 없다.
"""
import argparse
import collections
import csv
import itertools
import json
import random
import struct
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle, ROOT, SPECS, POOL, STOP, STACK, TARGET, digest,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 실행 진입·외부 두 경계·실제 finder 타입/권한 전역이다.
PRE={
 'originals':dict(pre=0x426890,query=0x4ac9f0,begin=0x4b2270,next=0x4b1b20,filter=0x44daa0,
    damageable=0x44b4b0,found=0x485e40,boss=0x540bc4,type=0x541088),
 'originalCD':dict(pre=0x4e43b0,query=0x4ac560,begin=0x4eb8d0,next=0x4eb900,filter=0x40f230,
    damageable=0x4615e0,found=0x4151d0,boss=0x540a2c,type=0x51c978),
}
PRE['original1037']=dict(PRE['originalCD'])
# 관찰 슬롯과 입력 시드·독립 저장 파일이다.
IDS=(50,60,61,62,63)
SEED=0x426890
FIXTURE=ROOT/'cpppj/tests/fixtures/carrierpredestroy-x86.tsv'
REPORT=ROOT/'cpppj/recovery-carrierpredestroy-evidence.json'


def inputs():
    """조건별 체인과 권한/머리/WORD 변경을 만든다. 기대 판단은 만들지 않는다."""
    scenes=[(0,''),(60,'60,6,8,0,7,1'),(60,'60,46,8,61,7,1;61,6,1,62,7,0;62,6,6,63,8,3;63,6,9,0,7,1'),
        (60,'60,6,0,61,7,1;61,6,2,62,7,3;62,6,4,63,8,65535;63,6,8,0,7,2')]
    cases=[]
    # 조회 조건의 생략·WORD 상한·순번 소진과 권한 분기를 교차한다.
    for scene,mask,kind,index,boss in itertools.product(scenes,(0,1,65536,0xffffffff),(0,7,65536),(0,1,3,0xffffffff),(0,1)):
        cases.append(dict(state=2,boss=boss,type=6,mask=mask,kind=kind,index=index,flags=0x200000,after='-',head=scene[0],nodes=scene[1]))
    rng=random.Random(SEED)
    # Damageable 이후 권한/종속 상태를 새로 읽는지 관찰한다.
    for i in range(96):
        nodes=';'.join(f'{sid},{rng.choice((6,6,46,167))},{rng.choice((0,1,2,4,8,9,15))},{sid+1 if sid<63 else 0},{rng.choice((0,7,8,65535))},{rng.choice((0,1,2,3,65535))}' for sid in IDS[1:])
        cases.append(dict(state=rng.choice((0,2,4,6)),boss=i%2,type=rng.choice((6,46,167,256,0xffffffff)),
            mask=rng.getrandbits(32),kind=rng.choice((0,7,8,65535,65536)),index=rng.choice((0,1,2,0xffffffff)),flags=rng.getrandbits(32),
            after=f'{(i//2)%2},{rng.choice((0,60,63))},{rng.choice((0,1,2,65535))}',head=60,nodes=nodes))
    return cases


class CarrierOracle(OwnerOracle):
    """같은 격리 PE 메모리에서 Carrier와 실제 contained finder만 허용한다."""
    def __init__(self,edition):
        """현재 PC의 Ghidra 몸체 범위와 두 외부 대체 주소를 준비한다."""
        super().__init__(edition);self.p=PRE[edition];base=list(self.exports);self.stub_calls=collections.Counter()
        self.exports=[ROOT/f'extracted/carrierpredestroy/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 허용 코드는 내보낸 불연속 범위로 제한한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.exports=base+self.exports
        if edition=='originals':self.mu.mem_write(0x5e4794,bytes(4))

    def ret(self,purge):
        """대체한 외부 몸체의 thiscall 반환만 모사한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);address=struct.unpack('<I',self.mu.mem_read(esp,4))[0]
        self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,address)

    def on_instruction(self,mu,address,size,data):
        """Damageable/전역 후처리만 대체하고 조회와 필터는 실제 실행한다."""
        if address==self.p['damageable']:
            flags=struct.unpack('<I',mu.mem_read(mu.reg_read(UC_X86_REG_ESP)+4,4))[0]
            if mu.reg_read(UC_X86_REG_ECX)!=self.slot(TARGET) or flags!=self.case['flags']:raise RuntimeError('Damageable 인자 오류')
            boss=struct.unpack('<I',mu.mem_read(self.p['boss'],4))[0]
            self.events.append(f'D:{TARGET}:{flags}:{boss}');self.stub_calls['D']+=1
            if self.case['after']!='-':
                boss,head,mask=map(int,self.case['after'].split(','))
                mu.mem_write(self.p['boss'],struct.pack('<I',boss));mu.mem_write(self.slot(TARGET)+6,struct.pack('<H',head))
                mu.mem_write(self.slot(63)+20,struct.pack('<H',mask))
            self.ret(4);return
        if address==self.p['found']:
            self.events.append('G');self.stub_calls['G']+=1;self.ret(0);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,mode):
        """직접 조회 또는 Carrier 전체 호출의 반환/레지스터/raw를 관찰한다."""
        self.case=case;self.events=[];self.instructions=0;self.write_ranges=[]
        self.mu.mem_write(self.p['boss'],struct.pack('<I',case['boss']));self.mu.mem_write(self.p['type'],struct.pack('<I',case['type']))
        # 입력하지 않은 바이트도 전체 슬롯 비교로 보존 여부를 확인한다.
        for sid in IDS:
            raw=bytearray([0xab]*self.stride);raw[10]=158;raw[11]=case['state'] if sid==TARGET else 0
            raw[4:8]=bytes(4);self.mu.mem_write(self.slot(sid),bytes(raw))
        self.mu.mem_write(self.slot(TARGET)+6,struct.pack('<H',case['head']))
        # 다음 링크·두 WORD·타입/상태를 입력 그대로 쓴다.
        for node in filter(None,case['nodes'].split(';')):
            sid,typ,state,next_sid,kind,mask=map(int,node.split(','));slot=self.slot(sid)
            self.mu.mem_write(slot+4,struct.pack('<H',next_sid));self.mu.mem_write(slot+10,bytes((typ,state)))
            self.mu.mem_write(slot+18,struct.pack('<HH',kind,mask))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x76543210,UC_X86_REG_EDI:0x23456789,UC_X86_REG_EBP:0x34567890}
        for register,value in preserved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_EDX,0);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(TARGET))
        args=[case['mask'],case['kind'],case['index']] if mode=='Q' else [case['flags']]
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        self.mu.emu_start(self.p['query' if mode=='Q' else 'pre'],STOP,count=20000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4*(len(args)+1):raise RuntimeError('전체 반환/스택 오류')
        if any(self.mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('보존 레지스터 오류')
        result=self.mu.reg_read(UC_X86_REG_EAX) if mode=='Q' else '-'
        return [result,struct.unpack('<I',self.mu.mem_read(self.p['boss'],4))[0],';'.join(self.events),
            '|'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in IDS)]


def generate(smoke=False):
    """원본 관찰을 독립 fixture와 입력 파일별 SHA 근거로 저장한다."""
    cases=inputs();rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/carrierpredestroy-functions.json',FIXTURE}
    if smoke:cases=cases[::61]
    # 세 실제 파일의 직접 조회와 Carrier 호출을 별도로 실행한다.
    for edition in SPECS:
        oracle=CarrierOracle(edition)
        for case in cases:
            for mode in ('Q','P'):
                rows.append([edition,mode,case['state'],case['boss'],case['type'],case['mask'],case['kind'],case['index'],case['flags'],case['after'],case['head'],case['nodes'],*oracle.run(case,mode)])
        editions[edition]=dict(cases=2*len(cases),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: Carrier/조회 {2*len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 Carrier/contained 조회 전체 반환. Damageable와 전역 후처리만 대체.\n# edition mode state boss type mask kind index flags after head nodes result bossOut events slots\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-08',normal_return=True,os_calls=0,editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['Carrier·contained 조회·finder·기본 true 필터만 실제 실행','Damageable pre와 인자 없는 전역 후처리는 외부 대체',
            'assert/손상 체인은 생성하지 않으며 전체 파생 삭제·공간/GUI 완성을 뜻하지 않음']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """파일 SHA·행 수·실제 실행/대체 경계의 사건 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    if len(rows)!=report['total'] or not report['normal_return'] or report['os_calls']:raise RuntimeError('Carrier 증거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];p=PRE[edition];direct=sum(row[1]=='Q' for row in selected)
        if len(selected)!=item['cases'] or item['assertions']:raise RuntimeError('Carrier 행/실행 오류')
        if item['native_calls'].get(f'{p["pre"]:08x}')!=direct or item['substitutions'].get('D')!=direct:raise RuntimeError('Carrier/Damageable 호출 오류')
        queries=direct+sum(row[1]=='P' and int(row[13])!=0 for row in selected)
        if item['native_calls'].get(f'{p["query"]:08x}')!=queries or item['native_calls'].get(f'{p["begin"]:08x}')!=queries:raise RuntimeError('실제 조회/Begin 호출 오류')
        if not item['native_calls'].get(f'{p["next"]:08x}') or not item['native_calls'].get(f'{p["filter"]:08x}'):raise RuntimeError('실제 Next/필터 누락')
        if any(item['native_calls'].get(f'{p[key]:08x}',0) for key in ('damageable','found')):raise RuntimeError('대체 몸체의 실행 기록 혼합')
        if item['substitutions'].get('G')!=sum(row[14].split(';').count('G') for row in selected):raise RuntimeError('전역 후처리 사건 오류')
        if any(row[14] for row in selected if row[1]=='Q'):raise RuntimeError('직접 조회에 외부 대체 발생')
    print(f'carrierpredestroy 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
