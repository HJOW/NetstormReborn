#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dude 실제 factory/owner/Pop/최초 I 프레임과 population을 두 보호 PE로 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from decomp_postpop_oracle import PostOracle,ARENA,POST
from decomp_pop_oracle import HEADS,SPOTS,HEAD_COUNT,bits
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL,SidOracle
from decomp_oracle import ROOT,STACK,STOP
from decomp_owner_oracle import digest
from shp import load_type_texts,read_clusters
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_FPSW

# 실제 population 전역과 Dude 생성/후처리 주소다.
SPEC={
 'originals':dict(birth=0x450a20,post=0x450cd0,ctor=0x451450,vtable=0x508cc8,frame=0x4acee0,population=0x55a4f0,capacity=0x55a4f4,count=0x55a4f8,blocked=0x55a4e8,type=0x541170,bridge=0x5411a0),
 'originalCD':dict(birth=0x4e8b30,post=0x4e9230,ctor=0x4e8b10,vtable=0x5072f0,frame=0x4acbc0,population=0x5b72b8,capacity=0x5b72bc,count=0x5b72c0,blocked=0x5417c8,type=0x51ca5c,bridge=0x51ca8c),
}
# 전체 보호 SHP와 population의 별도 격리 주소다.
FULL_SHAPE,POPULATION=0x14000000,ARENA+0x8000
KEYS=('capacity','count','blocked','x','y','owner','repeat')
FIXTURE=ROOT/'cpppj/tests/fixtures/dudespawn-x86.tsv'
REPORT=ROOT/'cpppj/dudespawn.json'


def inputs():
    """한도/차단·분수좌표·owner와 최초 뒤 재등록 입력을 기대값 없이 교차한다."""
    rows=[]
    # cap0, 여유1/3칸, 마지막칸, 가득 찬 목록을 구분한다.
    for pair,blocked,point,owner,repeat in itertools.product(((0,0),(1,0),(1,1),(3,0),(3,2),(3,3)),(0,1),((20.75,21.9),(50.5,50.5)),(0,1,8),(0,1)):
        rows.append(dict(zip(KEYS,(*pair,blocked,bits(point[0]),bits(point[1]),owner,repeat))))
    return rows


class DudeSpawnOracle(PostOracle):
    """실제 기존 생성/owner/공간/표시/Frame과 새 Dude 몸체를 실행하며 대체 함수를 두지 않는다."""
    def __init__(self,edition):
        """현호스트 좁은 export와 기존 실제 공통 몸체/보호65프레임 SHP를 연결한다."""
        super().__init__(edition);self.p=SPEC[edition];self.returns=0;self.births=0;self.calls=collections.Counter();self.exports=[];self.mu.mem_map(0,0x1000)
        # 모든 읽은 body/range를 전체 SHA 출처로 남긴다.
        for name in ('sid','creation','pop','display','postpop','owner','setframe','dude-spawn'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')]
            if not paths[0].exists():paths=paths[1:]
            self.exports.extend(paths)
            with paths[-1].open(encoding='utf-8') as fp:
                # 각 실제 함수의 불연속 몸체만 허용한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.exports.extend(path for path in (ROOT/'extracted/pop/helpers-originals/creation.c',ROOT/'extracted/pop/helpers-originals/functions.tsv') if edition=='originals' and path.exists())
        self.shapeFile=ROOT/edition/'d/_shapes.shp';data=self.shapeFile.read_bytes();self.mu.mem_map(FULL_SHAPE,(len(data)+4095)&~4095);self.mu.mem_write(FULL_SHAPE,data)
        self.typeText=load_type_texts(str(ROOT/edition))['dude'];clusters=read_clusters(self.typeText)
        if len(clusters)!=65 or clusters[-1][0]!='I00':raise RuntimeError('실제 Dude65/I00 보호 자산 오류')
        self.basePool=None

    def read(self,address,width=4):
        """현재 원본 비정렬 숫자의 저장 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """실제 Birth/ctor/owner/Pop/Frame/후처리 도달 수만 기록하고 외부 대체를 하지 않는다."""
        if hasattr(self,'p'):
            observed={self.p['birth']:'birth',self.p['ctor']:'ctor',self.p['post']:'dudePostPop',self.p['frame']:'setFrame',self.space['pop']:'Pop',self.creation['entries']['Create']:'Create',0x4adf00 if self.edition=='originals' else 0x4aefa0:'owner'}
            if address in observed:self.calls[observed[address]]+=1
        # CreationOracle의 void Unpop 제한 대신 실제 Pop/Frame 재등록의 현재 몸체를 허용한다.
        SidOracle.on_instruction(self,mu,address,size,data)

    def prepare(self,case):
        """같은 실제 Reset과 명시 현재 TYPE/population/통계/표시 입력만 공급한다."""
        self.case=case;mu=self.mu;p=self.p;patch=self.edition=='originals'
        if self.basePool is None:
            self.start_post(0x027f);self.basePool=bytes(mu.mem_read(POOL,self.capacity*self.stride));self.baseGlobals={address:bytes(mu.mem_read(address,4)) for address in self.spec['globals']};self.baseWrites=list(self.write_ranges)
        else:
            # 이전 실제 Reset/Create 결과 전체를 입력으로 복원하며 재방문을 새 관찰로 더하지 않는다.
            mu.mem_write(POOL,self.basePool)
            for address,value in self.baseGlobals.items():mu.mem_write(address,value)
            self.write_ranges=list(self.baseWrites);mu.mem_write(HEADS,bytes(0x80000))
        # 실제 서버 생성의 공통 HP/owner 초기화를 실행한다. client flags2의 번호 할당은 같다.
        mu.mem_write(self.spec['globals'][8],struct.pack('<I',1))
        self.input_type(70,0x28040010,0x02000400,0);typ=TYPES+70*self.creation['type_stride'];mu.mem_write(typ,struct.pack('<I',20));mu.mem_write(typ+0x104,struct.pack('<i',-20));mu.mem_write(typ+self.creation['ctor_offset'],struct.pack('<I',p['ctor']));mu.mem_write(typ+0xdc,struct.pack('<I',FULL_SHAPE));mu.mem_write(typ+0x114,struct.pack('<I',65));mu.mem_write(typ+self.space['foot'],bytes(8));mu.mem_write(typ+0x150,struct.pack('<I',64))
        self.state(0,0,0,0,0,0,0,1,0,0,0);self.view(1024,768,0,0,65536,(0,0,1024,768),0)
        mu.mem_write(POPULATION,struct.pack('<16I',*[0x60000000+index for index in range(16)]));mu.mem_write(p['population'],struct.pack('<I',POPULATION));mu.mem_write(p['capacity'],struct.pack('<I',case['capacity']));mu.mem_write(p['count'],struct.pack('<I',case['count']));mu.mem_write(p['blocked'],struct.pack('<I',case['blocked']));mu.mem_write(p['type'],struct.pack('<I',70));mu.mem_write(p['bridge'],struct.pack('<I',82))
        self.write_ranges.extend([(0,4),(p['count'],p['count']+4),(POPULATION,POPULATION+64)]);mu.mem_write(0,bytes(4))
        # 공통owner의 실제 정상 모드와 현재 owner 목록은 동일 후처리 상태를 사용한다.
        for address in ((0x594fb8,0x594fbc,0x594fc8) if patch else (0x540a1c,0x540a20,0x540a24)):mu.mem_write(address,bytes(4))

    def run(self,case):
        """실제 Birth 정상 반환/같은 최초 후처리와 전체raw/공간/표시/장부/population을 관찰한다."""
        self.prepare(case);p=self.p;self.invoke(p['birth'],[case['x'],case['y'],case['owner']]);born=self.mu.reg_read(UC_X86_REG_EAX);sid=born;self.returns+=1
        if sid:
            self.births+=1
            if case['repeat']:self.invoke(p['post'],[0x50],POOL+sid*self.stride,4)
        else:sid=6
        self.sid=sid;population=','.join(map(str,struct.unpack('<'+'I'*case['capacity'],self.mu.mem_read(POPULATION,case['capacity']*4)))) if case['capacity'] else '-'
        return [str(born),str(self.read(p['count'])),population,self.raw(),str(zlib.adler32(self.mu.mem_read(HEADS,HEAD_COUNT*2))),str(zlib.adler32(self.mu.mem_read(SPOTS,65536))),*map(str,self.output())]


def generate(smoke):
    """실제 몸체의 독립 출력과 보호 자산/허용 범위 SHA만 저장한다."""
    rows=[];editions={};files={ROOT/'tools/decomp_dudespawn_oracle.py',ROOT/'tools/ghidra/dude-spawn-functions.json'}
    for edition in SPEC:
        oracle=DudeSpawnOracle(edition);cases=inputs();cases=[cases[0],cases[24]] if smoke else cases
        # 각 입력은 두 판본의 별도 원본 주소 공간에서 한 번씩 실행한다.
        for case in cases:
            try:result=oracle.run(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*result])
        editions[edition]=dict(returns=oracle.returns,births=oracle.births,actual_calls=dict(oracle.calls),assertions=oracle.assertions);files.update(oracle.exports);files.add(ROOT/edition/('Netstorm.exe' if edition=='originals' else 'NETSTORM.EXE'));files.add(oracle.shapeFile);files.add(ROOT/edition/'netstorm.tarc');print(edition,editions[edition],flush=True)
    if smoke:return
    # 기존 공통 모듈/도구와 새 소비자의 전체 원문을 연결한다.
    for name in ('decomp_postpop_oracle.py','decomp_display_oracle.py','decomp_pop_oracle.py','decomp_creation_oracle.py','decomp_sid_oracle.py','decomp_oracle.py','decomp_owner_oracle.py','shp.py'):files.add(ROOT/'tools'/name)
    for path in ('cpppj/src/o/RawDudeSpawn.h','cpppj/src/o/RawDudeSpawn.cpp','cpppj/tests/DudeSpawnTests.cpp','cpppj/src/o/SquidFactory.cpp','cpppj/src/o/SquidOwner.cpp','cpppj/src/o/SquidPop.cpp','cpppj/src/o/SquidFrame.cpp','cpppj/src/o/SquidPostPop.cpp','cpppj/src/o/SquidDisplay.cpp'):files.add(ROOT/path)
    FIXTURE.write_text('# 실제 Dude factory/owner/Pop/최초 I 프레임과 population. 실제 보호65클러스터/130물리프레임 SHP, 외부 대체0.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',primary_target='10.78',cd_target='10.72',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},boundaries=[],os_calls=0,cpp_tests_completed=0,limitations=['실제 보호 Dude TYPE에서 확정한 flag/HP/발자국/Run을 명시 TYPE 입력으로 공급한다. 전체 parser를 새 실행한 근거는 아니다.','원본의 기존 확보 SID Reset과 실제 factory/ctor/owner/Pop/표시/프레임/최초population·공통 장부를 직접 실행한다.','DudePath24·작업5103/5104 부모·실제 이동/피해/삭제 및 캠페인 월드 바인딩은 별도 후속이다.','C++ shared Pop 분배 및 발자국0의 실제 빈 순회 연결은 다음단위 전체 출처 lineage가 필요하다.'],),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """현재 출처 SHA와 정상/assert/경계의 분류를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('Dude spawn SHA 불일치 '+path)
    if len(rows)!=report['total'] or report['boundaries'] or report['os_calls']:raise RuntimeError('Dude spawn 입력/효과 경계 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=item['returns'] or len({tuple(row[1:8]) for row in selected})!=len(selected) or item['assertions']:raise RuntimeError('Dude spawn 고유/정상 반환 오류')
    print(json.dumps(dict(verified=True,total=len(rows),boundaries=0,cpp_completed=False),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
