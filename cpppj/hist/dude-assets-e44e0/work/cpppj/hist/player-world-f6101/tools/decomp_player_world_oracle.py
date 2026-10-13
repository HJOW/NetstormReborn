#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Player152 실제 생성/Owner/공간/Post/Technology getter와 Regular 필드를 두 PE로 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_postpop_oracle import PostOracle,ARENA
from decomp_pop_oracle import HEADS,SPOTS,HEAD_COUNT,bits
from decomp_display_oracle import DISPLAY_OBJECT,RECT
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL,SidOracle
from decomp_oracle import ROOT,STACK,STOP
from decomp_owner_oracle import digest
from shp import load_type_texts,read_clusters,read_blocks,TYPE_LOAD_ORDER
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 새 부모 네 몸체와 기존 실제 공통/Regular 경계 및 원본 현재 전역이다.
PLAN={
 'originals':dict(create=0x490250,post=0x4905d0,current=0x48fc60,owner=0x48fc90,ctor=0x491660,vtable=0x50ee08,
   known=0x595344,local=0x540c70,template=0x540cac,type=0x5412b8,fort=0x594fb8,battle=0x594fbc,special=0x55a480,
   new=0x4e4391,regular=0x496e80,attach=0x41bd20,now=0x55b4d0,regularType=0x541128,assert_=0x4e0620),
 'originalCD':dict(create=0x407400,post=0x407cc0,current=0x406d90,owner=0x406dc0,ctor=0x407950,vtable=0x5002a8,
   known=0x50f828,local=0x50f6c8,template=0x50f6d4,type=0x51cba4,fort=0x540a1c,battle=0x540a20,special=0x537e8c,
   new=0x4f1650,regular=0x48efd0,attach=0x48f590,now=0x5484a8,regularType=0x51ca18,assert_=0x490040)}
# Player의 보호 SHP 전체/실제 Regular0x28과 선택 Form 부착의 격리 주소다.
SHAPE,REGULAR=0x14000000,ARENA+0x8000
CONTROLS=(0x027f,0x037f)
KEYS=('kind','owner','local','template','known','fort','special','reserve','flags','now')
FIXTURE=ROOT/'cpppj/tests/fixtures/player-world-x86.tsv'
REPORT=ROOT/'cpppj/player-world.json'


def inputs():
    """실제 생성·직접Post·두getter를 현재 두owner/모드/진단/힙 실패와 교차한다."""
    rows=[]
    # 실제 Factory flags2/Owner/Pop과 최초 local 예약을 관찰한다.
    for owner,local,template,known,fort,special,reserve in itertools.product((0,1,8),(1,8),(2,8),(0,1),(0,1),(0,1),(0,1)):
        rows.append((0,owner,local,template,known,fort,special,reserve,0,struct.unpack('<Q',struct.pack('<d',16777217.125))[0]))
    # 재Pop/직접flags에서 actual 현재 Player 기록과 frame DWORD/BYTE 차이를 확인한다.
    for owner,local,template,fort,special,reserve,flags in itertools.product((0,1,8),(1,8),(2,8),(0,1),(0,1),(0,1),(0,1,2,3,0x50,0xffffffff)):
        rows.append((1,owner,local,template,1,fort,special,reserve,flags,struct.unpack('<Q',struct.pack('<d',17.25))[0]))
    # getter는 SID DWORD 전체를 읽는다. PlayerKnown0의 Current는 실제 owner 접근을 생략한다.
    for kind,owner,local,template,known in itertools.product((2,3),(0,1,8),(0,1,8),(2,8),(0,1)):
        rows.append((kind,owner,local,template,known,0,0,0,0,0))
    return rows


class PlayerOracle(PostOracle):
    """기존 실제 공통 몸체와 새 Player를 같은 원본 Pool/Hash/Player 전체기록에 연결한다."""
    def __init__(self,edition):
        """현재PC 좁은/전체 정밀 범위와 보호 Player TYPE/SHP를 준비한다."""
        super().__init__(edition);self.p=PLAN[edition];self.mu.mem_map(0,4096);self.exports=[];self.calls=collections.Counter();self.boundaries=collections.Counter();self.returns=0;self.base=None
        # 동일PC의 기존 완료 몸체 범위는 재사용하며 새 완료 관찰 수에 합산하지 않는다.
        for name in ('sid','creation','pop','display','postpop','owner','finder','player-world','dude-task'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')]
            if not paths[0].exists():paths=paths[1:]
            self.exports.extend(paths)
            with paths[-1].open(encoding='utf-8-sig') as stream:
                # 허용 함수의 불연속 몸체만 사용하고 사이의 PE 명령은 거부한다.
                for row in csv.DictReader(stream,delimiter='\t'):
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        if edition=='originals':self.exports.extend(ROOT/f'extracted/pop/helpers-originals/{part}' for part in ('creation.c','functions.tsv'))
        # 실제152 ctor/단일type finder만 현재 전체 정밀 자료에서 선택한다.
        selected={self.p['ctor'],0x4b1fa0 if edition=='originals' else 0x4eb4b0,0x41d770 if edition=='originals' else 0x440a50,
          0x4b16d0 if edition=='originals' else 0x4eae20,0x4b1810 if edition=='originals' else 0x4eafe0}
        paths=[ROOT/f'extracted/refined/{edition}/{part}' for part in ('raw.c','functions.tsv')];self.exports.extend(paths)
        with paths[1].open(encoding='utf-8-sig') as stream:
            # 기존 완료ctor/finder를 source 주소별로 재사용하며 새152생성자 복원으로 다시 세지 않는다.
            for row in csv.DictReader(stream,delimiter='\t'):
                if int(row['entry'],16) not in selected:continue
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.shapeFile=ROOT/edition/'d/_shapes.shp';blob=self.shapeFile.read_bytes();self.mu.mem_map(SHAPE,(len(blob)+4095)&~4095);self.mu.mem_write(SHAPE,blob)
        self.typeText=load_type_texts(str(ROOT/edition))['player'];assert len(read_clusters(self.typeText))==2
        self.shapeBlock=read_blocks(blob)[TYPE_LOAD_ORDER.index('player')];assert len(self.shapeBlock.frames)==2

    def read(self,address,width=4):
        """비정렬 원본 정수의 전체 저장 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """힙/Regular Form부착/진단만 명시 관찰 경계이며 공통 공간/장부는 실제 실행한다."""
        if not hasattr(self,'p'):return super().on_instruction(mu,address,size,data)
        p=self.p;sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        if address in (p['create'],p['post'],p['ctor'],p['current'],p['owner'],p['regular'],self.space['pop'],self.creation['entries']['Create']):self.calls[f'{address:08x}']+=1
        if address==p['assert_']:
            self.events.append('A:'+str(self.read(sp+12)));self.diagnostics+=1;value=purge=0
        elif address==p['new']:
            assert self.read(sp+4)==40;self.events.append('N:40');value=REGULAR if self.case[7] else 0;purge=0
        elif address==p['attach']:
            args=[self.read(sp+4+index*4) for index in range(4)];assert args==[46,self.sid,0,0x50]
            self.events.append('B:'+':'.join(str(value) for value in args));value=0;purge=16
            # 부착 경계는 실제 Regular this의 Form/parent 필드만 명시 공급한다. ctor의time/payload/event/count는 대체하지 않는다.
            mu.mem_write(this+4,struct.pack('<II',83,self.sid))
        else:SidOracle.on_instruction(self,mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,self.read(sp))

    def prepare(self,case,control):
        """첫 실제 Reset 결과를 동일 입력으로 복원하고 실제 보호 blank 프레임/현재 Player를 공급한다."""
        self.case=case;self.events=[];self.diagnostics=0;mu=self.mu;p=self.p;patch=self.edition=='originals'
        if self.base is None:
            # 부모 전에 가짜 객체를 만들지 않는다. 첫 flags2 Player는 실제client SID5다.
            self.setup_space(32768,control);self.write_ranges.extend([(ARENA,ARENA+0x10000),(self.post['cost'],self.post['cost']+4),(self.post['dirty'],self.post['dirty']+4),
                (DISPLAY_OBJECT,DISPLAY_OBJECT+0x900),(RECT,RECT+16),(self.space['display'],self.space['display']+4)])
            self.write_ranges.extend((address,address+1024) for address in self.post['counts'])
            self.write_ranges.extend((self.post[key][2],self.post[key][2]+4) for key in ('providers','factories'))
            if patch:self.write_ranges.append((self.post['cost_table'],self.post['cost_table']+1024))
            self.base=bytes(mu.mem_read(POOL,self.capacity*self.stride));self.baseGlobals={address:bytes(mu.mem_read(address,4)) for address in self.spec['globals']};self.baseWrites=list(self.write_ranges)
        mu.mem_write(POOL,self.base)
        # 현재 source 입력의 전체 SID 관리 카운터를 재사용하고 새 실행 관찰로 세지 않는다.
        for address,value in self.baseGlobals.items():mu.mem_write(address,value)
        self.write_ranges=list(self.baseWrites);mu.mem_write(HEADS,bytes(0x80000));self.control=control;self.sid=5
        self.input_type(152,2,0,0);typ=TYPES+152*self.creation['type_stride'];mu.mem_write(typ,bytes(4));mu.mem_write(typ+0x104,struct.pack('<I',-20&0xffffffff));mu.mem_write(typ+self.creation['ctor_offset'],struct.pack('<I',p['ctor']))
        mu.mem_write(typ+0xdc,struct.pack('<I',SHAPE+self.shapeBlock.offset));mu.mem_write(typ+0x114,struct.pack('<I',2));mu.mem_write(typ+self.space['foot'],struct.pack('<II',1,1))
        self.state(0,0,0,0,0,0,0,case[2],0,0,0);self.view(1024,768,0,0,65536,(0,0,1024,768),0)
        # 각 실제 Player의 Technology DWORD는 서로 다른 명시 비트이며 두owner 조회를 구분한다.
        for owner in range(9):mu.mem_write(ARENA+owner*self.post['stride']+(0x9c if patch else 0x94),struct.pack('<I',0xfedc0000+owner))
        for address,value in ((p['known'],case[4]),(p['local'],case[2]),(p['template'],case[3]),(p['type'],152),(p['fort'],case[5]),(p['battle'],1-case[5]),(p['special'],case[6]),(p['regularType'],46),(self.spec['globals'][8],1)):
            mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(p['now'],struct.pack('<Q',case[9]));mu.mem_write(0,bytes(4));mu.mem_write(REGULAR,bytes(80));self.write_ranges.extend(((0,4),(REGULAR,REGULAR+80)))

    def run(self,case,control):
        """네 actualbody의 정상 반환/진단 및 전체raw/지도/Player/장부/Regular를 관찰한다."""
        self.prepare(case,control);mu=self.mu;p=self.p;kind=case[0];result=0
        if kind==0:self.invoke(p['create'],[case[1]]);result=mu.reg_read(UC_X86_REG_EAX);self.sid=result
        elif kind==1:
            self.sid=self.creation_step('Create',152,2)[0];raw=POOL+self.sid*self.stride
            mu.mem_write(raw+(34 if self.edition=='originals' else 32),bytes((case[1],)));mu.mem_write(self.space['depth'],struct.pack('<I',1));self.invoke(p['post'],[case[8]],raw,4)
        else:self.invoke(p['current'] if kind==2 else p['owner'],[] if kind==2 else [case[1]]);result=mu.reg_read(UC_X86_REG_EAX)
        self.returns+=1;assert mu.reg_read(UC_X86_REG_FPCW)==control and mu.reg_read(UC_X86_REG_FPSW)&0x3800==0 and self.read(0)==0
        regular=','.join(str(self.read(REGULAR+field,width)) for field,width in ((16,8),(24,4),(28,4),(32,4))) if any(item.startswith('B:') for item in self.events) else '-'
        return [str(result),';'.join(self.events) or '-',bytes(mu.mem_read(ARENA,9*self.post['stride'])).hex(),
            bytes(mu.mem_read(POOL+self.sid*self.stride,self.stride)).hex(),str(zlib.adler32(mu.mem_read(HEADS,HEAD_COUNT*2))),str(zlib.adler32(mu.mem_read(SPOTS,65536))),
            *map(str,self.output()[:8]),regular,str(self.diagnostics)]


def generate(smoke):
    """기대 결과를 계산하지 않고 두 x87의 일치 및 정상/진단 입력 수를 별도로 고정한다."""
    rows=[];editions={};files={Path(__file__).resolve(),ROOT/'tools/ghidra/player-world-functions.json',ROOT/'cpppj/src/o/RawPlayerWorld.h',ROOT/'cpppj/src/o/RawPlayerWorld.cpp'}
    # 각 실제판본의 입력은 같은원본 초기 저장소에서 독립 실행한다.
    for edition in PLAN:
        oracle=PlayerOracle(edition);cases=inputs();normal=diagnostic=0
        if smoke:cases=[cases[0],cases[15],cases[73],cases[96],cases[191],cases[192],cases[250],cases[-1]]
        # x87 반복은 고유 입력 수에 더하지 않는다.
        for case in cases:
            try:observed=[oracle.run(case,control) for control in CONTROLS];assert observed[0]==observed[1]
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            normal+=observed[0][-1]=='0';diagnostic+=observed[0][-1]!='0';rows.append([edition,','.join(str(value) for value in case),*observed[0]])
        editions[edition]=dict(unique=len(cases),normal_unique=normal,assert_unique=diagnostic,returns=oracle.returns,native_calls=dict(oracle.calls),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.update((ROOT/edition/'Netstorm.exe' if edition=='originals' else ROOT/'originalCD/NETSTORM.EXE',ROOT/edition/'netstorm.tarc',oracle.shapeFile));print(edition,editions[edition],flush=True)
    if smoke:return
    # 재사용 actualbody/합성 준비 도구와 실제 자료의 전체 출처를 연결한다.
    for name in ('decomp_owner_oracle','decomp_postpop_oracle','decomp_display_oracle','decomp_pop_oracle','decomp_creation_oracle','decomp_sid_oracle','decomp_oracle','shp','taff'):files.add(ROOT/f'tools/{name}.py')
    FIXTURE.write_text('# edition\t'+','.join(KEYS)+'\treturn\tevents\twholePlayer9\traw\thashAdler\tspotsAdler\tcost\trevision\tdepth\tglobal/local/secondaryAdler\tcounts\tlistsAdler\tregularFields\tassertCount\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-14',total=len(rows),normal_total=sum(item['normal_unique'] for item in editions.values()),assert_total=sum(item['assert_unique'] for item in editions.values()),controls=list(CONTROLS),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},game_runs=0,os_calls=0,limitations=[
        '새wrapper/Post/두getter와 기존실제Factory/152ctor/Owner/Pop/Display/CommonPost/Regularctor를 실행한다. 기존body는 새완료수에 다시 더하지 않는다.',
        '힙new·Regular FormAttach·assert 진단보고는 명시 관찰 경계다. 생성자time/event/payload/count는 실제 몸체가 쓴다.',
        '같은CPPHost/Kernel 실제 소비자 및 실제campaign173 설치는 후속이다. 게임/GUI/네트워크 실행은 하지 않았다.',
        '보호 player.type의2개 blank frame/foot1 및 보호SHP 블록을 사용하며 전체TYPEparser 실행은 새 관찰로 세지 않는다.',
        '유효현재9기록/owner0..8,유한currenttime,실제playerType152 입력 범위다. 다른body로부류를대체하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """정상/assert 분리와 fixture·모든 source/native 출처를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 보호 PE/자산·정밀 범위·완료 재사용몸체·현재 초안의 전체 SHA를 확인한다.
    for path,sha in report['files'].items():assert digest(ROOT/path)==sha,path
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['total'] and sum(int(row[-1])==0 for row in rows)==report['normal_total']
    print('PlayerWorld 전체 source/native/정상·assert 감사 통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
