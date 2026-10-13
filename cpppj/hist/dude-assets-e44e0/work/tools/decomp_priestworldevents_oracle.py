#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 월드 이동 프로세스 선택을 두 보호 PE의 실제 코드로 실행한다. 게임/GUI는 실행하지 않는다."""
import argparse
import collections
import csv
import json
from decomp_reward_oracle import RewardOracle, ROOT, TYPES, ARENA, ARENA_SIZE, STACK, digest

# 실제 선택 몸체와 mutable 타입/프로세스 전역의 주소 순서다.
SPEC={
    'originals':dict(entry=0x488fe0,types=[0x54126c,0x541344,0x54127c,0x5412c8,0x541270,0x541274,0x541170,0x5412d0,0x5411fc],
                     processes=[0x541110,0x541108,0x541118,0x5410cc,0x541114,0x5410d0,0x541148,0x54110c]),
    'originalCD':dict(entry=0x47d020,types=[0x51cb58,None,0x51cb68,0x51cbb4,0x51cb5c,0x51cb60,0x51ca5c,0x51cbbc,0x51cae8],
                      processes=[0x51ca00,0x51c9f8,0x51ca08,0x51c9bc,0x51ca04,0x51c9c0,0x51ca38,0x51c9fc],assertion=0x490040),
}
# 기존 보호 PE의 초기 DWORD를 읽어 확인한 자산/프로세스 입력이다. 결과 선택은 계산하지 않는다.
TYPE_DEFAULTS=(133,187,137,156,134,135,70,158,105)
PROCESS_DEFAULTS=(40,38,42,23,41,24,54,39)
FIXTURE=ROOT/'cpppj/tests/fixtures/priestworldevents-x86.tsv'
EVIDENCE=ROOT/'cpppj/priestworldevents.json'
SOURCES=['tools/decomp_priestworldevents_oracle.py','tools/decomp_reward_oracle.py','tools/ghidra/priestworldevents-functions.json',
         'cpppj/src/client/RawPriestWorldBindings.h','cpppj/src/client/RawPriestWorldBindings.cpp',
         'cpppj/src/o/RiftType.h','cpppj/src/o/RiftTypeTable.cpp','cpppj/tests/RawPriestWorldBindingsTests.cpp']


def cases(count):
    """전체 타입/balloon genus/현재 프로세스 전역과 우선순위 충돌 입력만 만든다."""
    # 프로세스 전역 변형과 사제/windWalker 타입 충돌을 모든 유효 타입에 교차한다.
    for profile in (0,1):
        for shift in (0,17):
            for genus in (0,0x20000):
                for number in range(count):
                    yield profile,shift,genus,number


class MovementOracle(RewardOracle):
    """기존 PE 메모리 로더만 재사용하며 실행 목록을 이동 선택/타입 getter로 제한한다."""
    def __init__(self,edition):
        """고유 정밀 내보내기와 실제 PE 초기 전역 값을 검증한다."""
        super().__init__(edition);self.m=SPEC[edition];self.allowed=[];self.entry_names={}
        export=ROOT/f'extracted/priestworldevents/{edition}/functions.tsv'
        with export.open(encoding='utf-8') as stream:
            # 실제 선택/타입 getter 이외의 exported body는 이번 검사의 실행에 허용하지 않는다.
            for row in csv.DictReader(stream,delimiter='\t'):
                if int(row['entry'],16) not in (self.m['entry'],0x49a840):continue
                self.entry_names[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        # CD의 미지원 타입 진단만 명시적으로 가로챈다. 선택/분기 결과는 실제 원본이 계산한다.
        self.stub_addresses={self.m['assertion']} if 'assertion' in self.m else set()
        self.s=dict(self.s);self.s['stubs']=dict(self.s['stubs'])
        if 'assertion' in self.m:self.s['stubs']['assert_']=self.m['assertion']
        self.writes=[(ARENA,ARENA+ARENA_SIZE),(STACK-0xf000,STACK+0x1000)]
        # 두 PE의 실제 보호 이미지가 사용한 초기 번호와 도구 입력의 일치를 먼저 검사한다.
        for address,expected in zip(self.m['types'],TYPE_DEFAULTS):
            if address is not None:assert self.u32(address)==expected,(edition,hex(address))
        for address,expected in zip(self.m['processes'],PROCESS_DEFAULTS):
            assert self.u32(address)==expected,(edition,hex(address))

    def run(self,profile,shift,genus,number):
        """전역/타입 표를 합성 입력으로 준비하고 원본 반환/진단 횟수만 관찰한다."""
        self.mu.mem_write(ARENA,bytes(ARENA_SIZE));self.put(self.s['type_ptr'],TYPES)
        self.put(TYPES+number*self.s['type_stride']+0xec,genus)
        values=list(TYPE_DEFAULTS)
        if profile:values[7]=values[2]
        # 실제 전역 주소를 매 실행마다 원래 값 또는 명시 입력 변형으로 설정한다.
        for address,value in zip(self.m['types'],values):
            if address is not None:self.put(address,value)
        for address,value in zip(self.m['processes'],PROCESS_DEFAULTS):self.put(address,value+shift)
        self.asserts=0;self.hits.clear();self.stub_hits.clear()
        result=self.call(self.m['entry'],[number],0x027f)
        return result,self.asserts,dict(self.hits)


def generate():
    """두 PE의 독립 결과와 원본/실행 body/구현 SHA를 fixture·근거에 보존한다."""
    rows=[];hits=collections.Counter();counts={}
    for edition in SPEC:
        oracle=MovementOracle(edition);count=0
        # 기대 프로세스 번호를 계산하지 않고 실제 PE를 매 입력에서 호출한다.
        for profile,shift,genus,number in cases(oracle.s['count']):
            result,assertions,called=oracle.run(profile,shift,genus,number)
            rows.append('\t'.join(map(str,(edition,profile,shift,genus,number,result,assertions))));hits.update(called);count+=1
        counts[edition]=count;print(f'{edition} 실제 이동 선택 {count}개 완료',flush=True)
    FIXTURE.write_text('# edition profile processShift genus type result assertions\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    files=list(SOURCES)
    for edition in SPEC:
        files.extend([f'{edition}/{"Netstorm.exe" if edition=="originals" else "NETSTORM.EXE"}',
                      f'extracted/priestworldevents/{edition}/creation.c',f'extracted/priestworldevents/{edition}/functions.tsv'])
    report=dict(host='VM-W11-CODEX',date='2026-10-13',cases=len(rows),edition_cases=counts,entries=dict(hits),
                type_defaults=TYPE_DEFAULTS,process_defaults=PROCESS_DEFAULTS,fixture_sha256=digest(FIXTURE),
                files={path:digest(ROOT/path) for path in files},
                limits=['CD 미지원 타입의 진단 보고만 대체; C++는 동일 경로를 명시 예외로 진단','게임/GUI 실행 없음','종속 조회는 기존 독립 containedlookup 근거를 재사용'])
    EVIDENCE.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """현재 보호 PE/정밀 body/소스/fixture SHA와 전체 타입/분기 입력을 감사한다."""
    report=json.loads(EVIDENCE.read_text(encoding='utf-8'));assert digest(FIXTURE)==report['fixture_sha256']
    for path,expected in report['files'].items():assert digest(ROOT/path)==expected,path
    rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['cases']==(188+171)*8
    assert set(report['entries'])=={'00488fe0','0049a840','0047d020'}
    for edition,count in (('originals',188),('originalCD',171)):
        assert report['edition_cases'][edition]==len(list(cases(count)))
    print(f'이동 선택 SHA/실제 body/전체 타입 입력 {len(rows)}개 감사 통과')


def main():
    """fixture 생성과 현재 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()


if __name__=='__main__':main()
