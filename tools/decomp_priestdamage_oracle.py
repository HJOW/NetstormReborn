#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 두 PE의 사제Hit·HPsetter·레벨보정·burning 판정을 제한x86으로 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_prieststate_oracle import StateOracle
from decomp_owner_oracle import ROOT,TYPES,PLAYERS,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 현재정밀원본주소와외부회복/프레임/소유자/효과경계다.
SPEC={
 'originals':dict(hit=0x494380,adjust=0x44bfe0,burning=0x44c060,regen=0x4afb40,cancel=0x491ac0,
  frame=0x4acee0,notice=0x416760,find=0x4b1a90,create=0x424090,sound=0x4a9d70,
  disabled=0x594fa4,noDamage=0x55a48c,burningType=0x5410a8,bulf=0x541274,depend=0x541074,
  checking=0x5e4794,challenge=0x52f7e0,player_level=0x68),
 'originalCD':dict(hit=0x40cb10,adjust=0x462280,burning=0x462480,regen=0x40c5a0,cancel=0x40c570,
  frame=0x4acbc0,notice=0x4d89b0,find=0x4eb310,create=0x4a6720,sound=0x438c00,
  disabled=0x52e170,noDamage=0x537e9c,burningType=0x51c998,bulf=0x51cb60,depend=0x51c964,
  checking=0x531454,challenge=None,player_level=0x68),
}
# 관찰출력과외부burning 생성결과를저장하는실행기전용메모리다.
OUTPUT,CREATED=0x11318000,0x11319000
DESTROY=STOP+0x180
KEYS=('kind','current','damage','arank','vrank','attackerOwner','victimOwner','disabled','nodamage',
      'boss','regen','direction','effect','existing','created','checking','type','maximum')
FIXTURE=ROOT/'cpppj/tests/fixtures/priestdamage-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestdamage-evidence.json'


def case(**values):
    """원본판단을사용하지않고독립입력의공통값을준비한다."""
    result=dict(zip(KEYS,(0,150,24,0,0,2,1,0,0,1,0,0,0,0,0,0,158,200)));result.update(values);return result


def inputs():
    """rank변환/부호/보정차단과실제Hit·HP경계·burning유무를방문한다."""
    rows=[]
    # 패치rank11/23은실제level15/100으로바뀌며CD는BYTE11/23을그대로사용한다.
    for ranks,damage,owners,disabled in itertools.product(((0,0),(0,3),(3,0),(10,11),(23,3),(3,23)),
        (-19,-1,0,1,24,201,0x7fffffff),((2,1),(0,1),(2,0),(9,1)),(0,1)):
        rows.append(case(arank=ranks[0],vrank=ranks[1],damage=damage,attackerOwner=owners[0],victimOwner=owners[1],disabled=disabled))
    # held10..14와현재regen의다른취소이유,HP1/절반경계,NoDamage/권한을교차한다.
    for current,damage,boss,nodamage,cancel,ranks in itertools.product((1,99,100,150),(0,24,120,200),(0,1),(0,1),
        ((0,0),(1,0),(0,9),(0,10),(0,14),(0,15)),((0,0),(3,1),(1,3))):
        rows.append(case(kind=1,current=current,damage=damage,boss=boss,nodamage=nodamage,regen=cancel[0],direction=cancel[1],arank=ranks[0],vrank=ranks[1]))
    # nonzero burning은조회/생성/삭제를명시관찰하며zero는실제자연조기반환을실행한다.
    for current,effect,existing,created,checking,typeid in itertools.product((-1,0,99,100,101,200,32768),(0,1),(0,1),(0,1),(0,1),(158,135)):
        rows.append(case(kind=2,current=current,effect=effect,existing=existing,created=created,checking=checking,type=typeid))
    return rows


class DamageOracle(StateOracle):
    """기존실제HPsetter/helper에Hit/rank/burning몸체를추가해함께실행한다."""
    def __init__(self,edition):
        """현호스트정밀export의허용범위와실제virtual 진입을추가한다."""
        super().__init__(edition);self.d=SPEC[edition]
        for name in ('victimdamage','victimdamage-helper','assetregular'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 불연속정밀몸체만허용한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for segment in row['ranges'].split(';'):
                        low,high=(int(x,16) for x in segment.split('-'));self.allowed.append((low,high+1))
        actual=struct.unpack('<I',self.mu.mem_read(self.p['vtable']+0x98,4))[0]
        if actual!=self.d['hit']:raise RuntimeError('실제Priest+98 진입불일치')
        self.mu.mem_write(CREATED+4,struct.pack('<I',90));self.mu.mem_write(self.slot(90),bytes(self.stride));self.mu.mem_write(self.slot(90),struct.pack('<I',CREATED+0x100));self.mu.mem_write(CREATED+0x110,struct.pack('<I',DESTROY))
        self.boundaries=collections.Counter();self.returns=0

    def read(self,address,width=4):
        """비정렬little endian raw를읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """회복유무/취소·프레임·소유자통지·burning 외부생성/삭제를정확한ABI로관찰한다."""
        p=self.d;esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(esp+4+i*4) for i in range(4)];ret=self.read(esp);purge=0;value=0;c=self.input
        if address==p['regen']:value=c['regen'];purge=4 if self.edition=='originals' else 0
        elif address==p['cancel']:self.events.append(f'C:50:{args[0]}');purge=4
        elif address==p['frame']:
            self.events.append(f'F:50:{args[0]}:{args[1]}');mu.mem_write(this+(36 if self.edition=='originals' else 34),struct.pack('<I',args[0]) if self.edition=='originals' else bytes([args[0]&255]));purge=8
        elif address==p['notice']:self.events.append('N:'+':'.join(map(str,args[:3])))
        elif address==p['find']:
            value=90 if c['existing'] and args[2]==0 else 0;self.events.append('B:'+':'.join(map(str,args[:3]))+':'+str(value))
        elif address==p['create']:self.events.append(f'A:{args[0]}');value=CREATED if c['created'] else 0
        elif address==p['sound']:self.events.append(f'S:50:{args[0]}:{args[1]}')
        elif address==DESTROY:self.events.append(f'X:90:{args[0]}');purge=4
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_body(self,values):
        """독립입력에서실제rank/Hit/HPsetter/burning 정상반환을기록한다."""
        self.input=values;self.case=dict(kind='Set',after='-');self.events=[];mu=self.mu;p=self.d;patch=self.edition=='originals';raw=bytearray((i*17+3)&255 for i in range(self.stride));struct.pack_into('<I',raw,0,self.p['vtable']);struct.pack_into('<HH',raw,4,0,0);raw[10]=values['type'];raw[11]=0;raw[self.o['extra']]=0;raw[self.o['owner']]=values['victimOwner'];struct.pack_into('<ff',raw,14,20.75,21.9);struct.pack_into('<I' if patch else '<H',raw,26,values['current']& (0xffffffff if patch else 0xffff));struct.pack_into('<I',raw,36,0) if patch else raw.__setitem__(34,0);mu.mem_write(self.slot(50),bytes(raw))
        source=TYPES+values['type']*self.type_stride;mu.mem_write(source,bytes(self.type_stride));mu.mem_write(source,struct.pack('<i',values['maximum']));mu.mem_write(source+0xe8,struct.pack('<II',0x69012,0x210000));mu.mem_write(source+0xd4,struct.pack('<I',values['effect']));mu.mem_write(source+0x118,struct.pack('<I',0));mu.mem_write(source+0x124,struct.pack('<I',CREATED+0x200));mu.mem_write(CREATED+0x200,bytes([65+values['direction'],0,0,0]))
        mu.mem_write(self.p['quarter'],bytes(4));mu.mem_write(self.p['priest'],struct.pack('<I',158));mu.mem_write(self.p['boss'],struct.pack('<I',values['boss']));mu.mem_write(p['disabled'],struct.pack('<I',values['disabled']));mu.mem_write(p['noDamage'],struct.pack('<I',values['nodamage']));mu.mem_write(p['checking'],struct.pack('<I',values['checking']));mu.mem_write(p['burningType'],struct.pack('<I',14));mu.mem_write(p['bulf'],struct.pack('<I',135));mu.mem_write(p['depend'],struct.pack('<I',1));mu.mem_write(0x546778,bytes(4)) if not patch else None
        # 패치는원본24rank변환getter를실행하고CD는원본Player BYTE를읽는다.
        for owner,rank in ((values['attackerOwner'],values['arank']),(values['victimOwner'],values['vrank'])):
            mu.mem_write(PLAYERS+owner*self.player_stride+p['player_level'],bytes([rank&255]));mu.mem_write(PLAYERS+owner*self.player_stride+self.p['player_flag'],struct.pack('<I',1))
            if patch and 1<=owner<=8:mu.mem_write(p['challenge']+0x4f+owner-1,bytes([rank&255]))
        mu.mem_write(OUTPUT,struct.pack('<I',values['damage']&0xffffffff));self.write_ranges=[(self.slot(50)+14,self.slot(50)+22),(self.slot(50)+26,self.slot(50)+26+(4 if patch else 2)),(self.slot(50)+(36 if patch else 34),self.slot(50)+(40 if patch else 35)),(OUTPUT,OUTPUT+4)]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,self.slot(50));mu.reg_write(UC_X86_REG_ESP,STACK)
        args=([OUTPUT,values['attackerOwner'],values['victimOwner']] if values['kind']==0 else [values['damage']&0xffffffff,values['attackerOwner'],60] if values['kind']==1 else []);entry=(p['adjust'],p['hit'],p['burning'])[values['kind']];mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=100000)
        end=STACK+4+(12 if values['kind']==1 else 0)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=end:raise RuntimeError('반환/스택불일치')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('x87스택불일치')
        self.returns+=1;return ['-' if values['kind']==0 else str(mu.reg_read(UC_X86_REG_EAX)),';'.join(self.events),bytes(mu.mem_read(self.slot(50),self.stride)).hex(),bytes(mu.mem_read(OUTPUT,4)).hex()]


def generate():
    """실제관찰과현재소스/정밀export/fixture/PE의SHA를고정한다."""
    rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_prieststate_oracle.py',FIXTURE,ROOT/'tools/ghidra/victimdamage-functions.json',ROOT/'tools/ghidra/victimdamage-helper-functions.json'}
    for edition in SPEC:
        oracle=DamageOracle(edition)
        # 모든독립입력을같은두원본몸체에서실행한다.
        for values in inputs():
            try:output=oracle.run_body(values)
            except Exception as error:raise RuntimeError(f'{edition} {values}: {error}') from error
            rows.append([edition,*[values[key] for key in KEYS],*output])
        editions[edition]=dict(returns=oracle.returns,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),space=dict(oracle.stub_calls),assertions=oracle.assertions,instructions=oracle.instructions);files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,oracle.returns,flush=True)
    # 복원구현과대조코드도같은고정SHA기록에포함한다.
    for relative in ('cpppj/src/o/RawPriestDamage.h','cpppj/src/o/RawPriestDamage.cpp','cpppj/src/o/RawDamageableBurning.h','cpppj/src/o/RawDamageableBurning.cpp','cpppj/tests/PriestDamageTests.cpp'):files.add(ROOT/relative)
    FIXTURE.write_text('# 실제두PE 사제Hit+HPsetter+rank보정+burning 관찰. 회복/프레임/소유자/가상공간/burning생성·삭제는명시경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),host='HJOW-Athlon',editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},os_calls=0,limitations=['live 사제의실제Hit/HPsetter/rank와Damageable burning 판정몸체를실행한다.','회복조회/취소,frame setter,소유자/가상공간통지,burning find/create/destroy/sound는명시관찰경계다.','HP1 사제의빈감시체인은실제FormFinder로실행하며nonempty 실제의존삭제는C++연결검사및기존모듈근거다.','nonzero burning의실제파생생성/애니메이션몸체는후속이며이경계의성공을완료로세지않는다.'],normal_return=True),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """현재SHA와모든행의고유입력/정상반환/실제Hit/setter/rank/burning실행을확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total'] or report['os_calls']:raise RuntimeError('관찰행수/OS경계불일치')
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA불일치 '+name)
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=item['returns'] or item['assertions'] or len({tuple(row[1:19]) for row in selected})!=len(selected):raise RuntimeError('고유입력/반환/assert불일치')
        for name in ('hit','adjust','burning'):
            if not item['native_calls'].get(f'{SPEC[edition][name]:08x}'):raise RuntimeError('실제몸체실행누락 '+name)
    print(json.dumps(dict(verified=True,total=len(rows)),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
