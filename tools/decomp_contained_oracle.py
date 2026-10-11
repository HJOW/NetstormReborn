#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""종속 체인 타입/순번 조회·권한별 삭제 전체를 세 실제 PE의 명령으로 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_process_oracle import (ProcessOracle,ROOT,BINARIES,SPECS,CONTROLS,POOL,VTABLE,STUBS,STACK,
    digest,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EDI,
    UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_ESP)

# 실제 루트·명시 가상 삭제 경계와 고정 부모 번호다. 경계 주소는 격리 메모리 안에만 존재한다.
ENTRIES={'originals':0x4b1a90,'originalCD':0x4eb310,'original1037':0x4eb310}
DELETE=STUBS+0x400
PARENT=50
# 입력 열의 순서와 관찰/근거 저장 경로다.
KEYS=('server','pattern','type','ordinal','remove','mutate')
FIXTURE=ROOT/'cpppj/tests/fixtures/contained-x86.tsv'
REPORT=ROOT/'cpppj/recovery-contained-evidence.json'


def plan():
    """타입 DWORD·일치 순번/감김·후보 상태·양쪽 권한과 삭제 중 변경의 입력만 만든다."""
    rows=[]
    # 모든 조합은 유효한 유한 체인이다. 경계값과 존재하지 않는 큰 순번도 포함한다.
    for server,pattern,kind,ordinal,remove in itertools.product((0,1),range(5),(47,46,303,255,0xffffffff),
            (0,1,2,5,6,7,0xffffffff,0x80000000),(0,1)):
        rows.append(dict(server=server,pattern=pattern,type=kind,ordinal=ordinal,remove=remove,mutate=0))
    # 가상 삭제는 현재 next·다음 후보 상태·부모 head를 바꿀 수 있다.
    for mutate in range(1,6):
        rows.append(dict(server=1,pattern=0,type=47,ordinal=0,remove=1,mutate=mutate))
    return rows


def children(edition):
    """예약/client/서버/예측 번호 경계의 합성 종속 번호를 반환한다."""
    spec=SPECS[edition]
    return (1,4,5,6,spec['server_first']-1,spec['server_first'],23002 if spec['patch'] else 14001)


def initial(edition,c):
    """기대값을 계산하지 않고 입력 raw 바이트를 구성한다. 부모와 주변 필드에 표식 바이트를 넣는다."""
    ids=children(edition);stride=SPECS[edition]['stride'];result={}
    # 모든 지정 슬롯의 입력은 실행기와 C++ 장면이 함께 사용하는 고정 자료다.
    for index,sid in enumerate((PARENT,*ids)):
        raw=bytearray((i*29+index*17+7)&255 for i in range(stride))
        struct.pack_into('<I',raw,0,VTABLE);struct.pack_into('<H',raw,4,0)
        struct.pack_into('<H',raw,6,ids[0] if index==0 and c['pattern']!=4 else 0)
        raw[10]=100 if index==0 else 47;raw[11]=0 if index==0 else 8
        if index:
            struct.pack_into('<H',raw,4,ids[index] if index<len(ids) else 0)
            if c['pattern']==1:raw[10]=(46,47,47,46,47,47,47)[index-1]
            if c['pattern']==2:raw[11]=(2,10,8,1,4,0,8)[index-1]
            if c['pattern']==3:
                raw[10]=(0,255,46,47,47,255,0)[index-1];raw[11]=(0,8,2,10,1,4,0)[index-1]
        result[sid]=raw
    return result


class ContainedOracle(ProcessOracle):
    """공통 읽기 전용 PE 로더에 종속 조회 몸체와 가상 삭제 응답 경계만 더한다."""
    def __init__(self,edition):
        """Ghidra의 실제 불연속 범위를 허용하며 함수 사이의 명령은 실행하지 않는다."""
        super().__init__(edition);self.edition=edition;self.returns=collections.Counter();self.deletes=0
        self.extra=[ROOT/f'extracted/contained/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as fp:
            # 목록의 함수 진입점과 실제 몸체 범위만 추가한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """가상 삭제 인자/this와 지정 입력 변경만 처리하며 조회/권한/순번은 실제 명령으로 실행한다."""
        if address==DELETE:
            esp=mu.reg_read(UC_X86_REG_ESP);flag=self.u32(esp+4);sid=self.sid_of(mu.reg_read(UC_X86_REG_ECX))
            if flag!=0 or sid not in children(self.edition):raise RuntimeError('contained 가상 삭제 인자 오류')
            self.events.append(f'D:{sid}:{flag}');self.deletes+=1
            ids=children(self.edition);raw=POOL+sid*self.s['stride'];mutation=self.case['mutate']
            if mutation==1:self.put(raw+4,ids[3],2)
            if mutation==2:self.put(raw+4,0,2)
            if mutation==3:self.put(POOL+ids[1]*self.s['stride']+11,2,1)
            if mutation==4:
                kind=mu.mem_read(raw+10,1)[0];mu.mem_write(raw,bytes(self.s['stride']));self.put(raw+10,kind,1);self.put(raw+11,3,1)
            if mutation==5:self.put(POOL+PARENT*self.s['stride']+6,0,2)
            mu.reg_write(UC_X86_REG_EAX,0xdecafbad);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));mu.reg_write(UC_X86_REG_ESP,esp+8)
            return
        ProcessOracle.on_instruction(self,mu,address,size,data)

    def run(self,c,control):
        """입력 한 개를 정상 반환까지 실행하고 ABI/보존 레지스터/x87/SEH·전체 지정 raw를 관찰한다."""
        self.begin(dict(server=c['server'],boss=c['server'],rotation=0,now=0),control);self.case=c
        self.put(VTABLE+0x10,DELETE)
        # 입력 자료만 쓰며 타입/순번/삭제 결과를 미리 계산하지 않는다.
        for sid,raw in initial(self.edition,c).items():self.mu.mem_write(POOL+sid*self.s['stride'],bytes(raw))
        result=self.call(ENTRIES[self.edition],[PARENT,c['type'],c['ordinal'],c['remove']])
        if any(self.mu.reg_read(reg) for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)):
            raise RuntimeError('contained 보존 레지스터 오류')
        self.returns[f'{ENTRIES[self.edition]:08x}']+=1
        raw=':'.join(bytes(self.mu.mem_read(POOL+sid*self.s['stride'],self.s['stride'])).hex() for sid in (PARENT,*children(self.edition)))
        return [str(result),';'.join(self.events) or '-',raw]


def generate(smoke):
    """두 x87 정밀도에서 같은 원본 관찰만 fixture로 저장하고 PE/도구/입력/내보내기의 SHA를 기록한다."""
    rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/ghidra/contained-functions.json',FIXTURE}
    # 세 PE는 별도의 가상 메모리와 허용 목록을 사용한다.
    for edition in BINARIES:
        oracle=ContainedOracle(edition);cases=plan()[::53] if smoke else plan()
        # C++ 기대식을 만들지 않고 실제 루트의 두 실행을 비교한다.
        for c in cases:
            observed=[oracle.run(c,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('contained 정밀도 관찰 차이')
            rows.append([edition,','.join(str(c[key]) for key in KEYS),*observed[0]])
        editions[edition]=dict(cases=len(cases),entries=dict(oracle.hits),returns=dict(oracle.returns),deletes=oracle.deletes,assertions=oracle.asserts)
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: contained {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 종속 타입/순번 조회·권한별 삭제 전체. 가상 삭제 내부만 응답 경계.\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),controls=list(CONTROLS),host='HJOW-Athlon',date='2026-10-11',os_calls=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},limits=[
            '조회/타입 DWORD/순번 감김/권한/다음 링크 읽기를 실제 전체 몸체로 실행',
            '가상 삭제만 명시 경계: 실제 파생 삭제·소리 장치·공간·게임/OS는 원본 실행 범위 밖',
            '유한 합성 체인, free/void/contained 비트·예약/client/서버/예측 번호·부모 head/next/상태 경계 변동',
            'ABI·보존 레지스터·x87·SEH·실행/쓰기 허용 목록·SHA/입력/폭/진입/반환/삭제 집계 감사']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """기대 결과를 재계산하지 않고 저장 관찰의 SHA·입력 순서·폭·실제 실행/경계 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 근거 생성에 쓴 모든 파일의 정확한 바이트를 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('contained SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('contained 근거 합계 오류')
    if set(report['editions'])!=set(BINARIES):raise RuntimeError('contained PE 목록 오류')
    # 판본별로 생성 때의 입력과 실제 진입/정상 반환 횟수를 대조한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];cases=plan();count=2*len(cases);deletes=0
        if item['cases']!=len(cases) or len(chosen)!=len(cases) or item['assertions']:raise RuntimeError('contained 입력/assert 오류')
        for row,c in zip(chosen,cases,strict=True):
            if len(row)!=5 or row[1]!=','.join(str(c[key]) for key in KEYS) or not 0<=int(row[2])<=65535:raise RuntimeError('contained 입력/반환 폭 오류')
            raw=row[4].split(':')
            if len(raw)!=8 or any(len(bytes.fromhex(value))!=SPECS[edition]['stride'] for value in raw):raise RuntimeError('contained raw 폭 오류')
            if row[3]!='-':
                for event in row[3].split(';'):
                    letter,sid,flag=event.split(':')
                    if letter!='D' or int(sid) not in children(edition) or flag!='0':raise RuntimeError('contained 삭제 경계 폭 오류')
                    deletes+=2
        entry=f'{ENTRIES[edition]:08x}'
        if item['entries'].get(entry)!=count or item['returns']!={entry:count} or item['deletes']!=deletes:
            raise RuntimeError('contained 실제 진입/반환/삭제 수 오류')
    print(f'contained 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본 실행·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
