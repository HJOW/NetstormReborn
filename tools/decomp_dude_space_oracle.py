#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Dude Birth 뒤 발자국0의 Unpop/재등록과 같은 공간/장부를 두 PE에서 관찰한다."""
import argparse
import csv
import itertools
import json
import struct
import zlib
from decomp_dudespawn_oracle import DudeSpawnOracle
from decomp_pop_oracle import HEADS,SPOTS,HEAD_COUNT,bits
from decomp_sid_oracle import POOL
from decomp_owner_oracle import ROOT,digest

# 현재 생성/제거/재등록 입력이며 좌표는 원본 float 저장비트다.
KEYS=('x','y','owner','targetX','targetY','repop')
FIXTURE=ROOT/'cpppj/tests/fixtures/dude-space-x86.tsv'
REPORT=ROOT/'cpppj/recovery-dude-space-evidence.json'


def inputs():
    """두 최초위치/두 목적지/owner0·1·8과 Unpop만/실제Repop을 교차한다."""
    # 생성번호/헤더/재등록 결과는 입력에서 계산하지 않는다.
    return [dict(zip(KEYS,(bits(point[0]),bits(point[1]),owner,bits(target[0]),bits(target[1]),repop))) for point,target,owner,repop in itertools.product(((20.75,21.9),(50.5,50.5)),((0.5,0.5),(25.125,24.75)),(0,1,8),(0,1))]


class DudeSpaceOracle(DudeSpawnOracle):
    """새 실제 Birth/Unpop/Pop 사슬을 실행하며 모든 하위 몸체를 대체하지 않는다."""
    def run_space(self,case):
        """실제 생성 후 제거시점 전체 raw와 최종 전체 공간/등록/장부를 관찰한다."""
        initial=super().run(dict(capacity=3,count=0,blocked=0,x=case['x'],y=case['y'],owner=case['owner'],repeat=0));born=int(initial[0])
        if born!=6:raise RuntimeError('Dude 공간 관찰 실제 생성 번호 오류')
        self.invoke(self.creation['unpop'],[0],POOL+born*self.stride,4);removed=self.raw()
        if case['repop']:self.invoke(self.space['pop'],[case['targetX'],case['targetY'],0x50],POOL+born*self.stride,12)
        population=','.join(map(str,struct.unpack('<3I',self.mu.mem_read(self.read(self.p['population']),12))))
        final=[str(born),str(self.read(self.p['count'])),population,self.raw(),str(zlib.adler32(self.mu.mem_read(HEADS,HEAD_COUNT*2))),str(zlib.adler32(self.mu.mem_read(SPOTS,65536))),*map(str,self.output())]
        return [removed,*final]


def generate(smoke):
    """새48공간입력을 앞288생성/414Notice와 구분하고 실제 전체 출처를 기록한다."""
    rows=[];editions={};files={ROOT/'tools/decomp_dude_space_oracle.py',ROOT/'tools/decomp_dudespawn_oracle.py'}
    # 각 실제 주소 공간에서 독립 입력을 한 번씩 실행한다.
    for edition in ('originals','originalCD'):
        oracle=DudeSpaceOracle(edition);cases=inputs()[:2] if smoke else inputs()
        for case in cases:
            try:result=oracle.run_space(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*result])
        editions[edition]=dict(returns=len(cases),assertions=oracle.assertions,births=oracle.births,actual_unpop=len(cases),actual_repop=sum(case['repop'] for case in cases),actual_calls=dict(oracle.calls),external_replaced=0);files.update(oracle.exports);files.add(oracle.shapeFile);files.add(ROOT/edition/('Netstorm.exe' if edition=='originals' else 'NETSTORM.EXE'));files.add(ROOT/edition/'netstorm.tarc');print(edition,editions[edition],flush=True)
    if smoke:return
    # 공유 Unpop의 현재 미연결 크기검사와 실제 새 소비자의 원문을 당시 SHA로 남긴다.
    for name in ('cpppj/src/o/SquidUnpop.cpp','cpppj/src/o/SquidPop.cpp','cpppj/src/o/SquidPostPop.cpp','cpppj/src/o/RawDudeSpawn.cpp','cpppj/tests/DudeSpaceTests.cpp','cpppj/tests/DudeSpaceSupport.h','tools/decomp_postpop_oracle.py','tools/decomp_display_oracle.py','tools/decomp_creation_oracle.py','tools/decomp_sid_oracle.py','tools/decomp_oracle.py','tools/decomp_owner_oracle.py','cpppj/recovery-dudespawn-evidence.json','cpppj/recovery-dude-postpop-binding-lineage-evidence.json'):files.add(ROOT/name)
    FIXTURE.write_text('# 실제 Dude Birth→foot0 Unpop→선택 실제Repop50. 전체raw/해시/spot/장부/표시, 하위대체0.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',primary_target='10.78',cd_target='10.72',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},cpp_tests_completed=0,pending_shared_foot0=True,os_calls=0,limitations=['실제Dude 생성/owner/I최초등록 뒤 발자국0의 실제Unpop/일부Repop50을 실행했다.','Unpop만 반환한 raw와 최종 전체hash/spot/표시/장부를 각각 관찰한다.','모든 하위몸체는 실제원본이며 외부대체0이다. 기존Spawn288/Notice414와 중복 합산하지않는다.','현재sharedUnpop은foot0를거부하므로기준13전체보존/정확역변환successor연결뒤C++대조해야한다.','실제DudePath24·연속이동/삭제부모/전체Host수명과캠페인월드는후속이다.'],),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """새48고유입력과 실제Birth/Unpop/재등록 및 당시 SHA를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError('Dude 공간 SHA 오류 '+name)
    if len(rows)!=report['total'] or report['os_calls']:raise RuntimeError('Dude 공간 고유/OS 분류 오류')
    # 최초생성과 실제재등록의 반환/깊이를 직접몸체 repeat50과 혼동하지 않는다.
    for edition,data in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=24 or len({tuple(row[1:7]) for row in selected})!=24 or data['assertions'] or data['births']!=24 or data['actual_unpop']!=24 or data['actual_repop']!=12 or data['external_replaced']:raise RuntimeError('Dude 공간 실제 사슬 분류 오류')
    print(json.dumps(dict(verified=True,total=len(rows),cpp_completed=False,pending_shared_foot0=True),ensure_ascii=False))


# 명시 CLI에서 새 원본 격자를 만들거나 고정된 현재 출처를 검증한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
