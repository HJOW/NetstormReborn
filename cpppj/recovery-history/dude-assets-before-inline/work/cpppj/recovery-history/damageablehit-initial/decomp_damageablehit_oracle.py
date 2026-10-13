#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 두 PE의 일반 Base/Damageable 피격과 HP·레벨·사망·섬 경고를 독립 관찰한다."""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_priestdamage_oracle import DamageOracle,CREATED
from decomp_owner_oracle import ROOT,TYPES,PLAYERS,STACK,STOP,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 현재 정밀 주몸체와 실제 경고/사망 전역 및 외부 UI/시계 경계다.
HIT={
 'originals':dict(base=0x4add10,derived=0x44c8e0,clock=0x460d70,warning=0x55a450,dead=0x5c845c,server=0x540bc0,local=0x540c70,primary=0x78,islandAt=0x40eaf0,event=0x4d3090,warningSound=0x4a9cb0,format=0x4a89c0,text=0x4cf960),
 'originalCD':dict(base=0x4aeda0,derived=0x462300,clock=0x4011b0,warning=0x566ff0,dead=0x5395d4,server=0x540a28,local=0x50f6c8,primary=0x70,islandAt=None,event=None,warningSound=0x438b50,format=None,text=None),
}
# 실행기 전용 가상 판정/삭제/공간 경계와 CD 지면 SID 표의 독립 입력 저장소다.
VTABLE=CREATED+0x800
BLOCK,DIE,UNREALIZE=STOP+0x500,STOP+0x520,STOP+0x540
GROUND=0x11500000
KEYS=('kind','current','damage','arank','vrank','attackerOwner','victimOwner','disabled','nodamage','server','state','extra','reject','hasHP','dais','attackerGenus','effect','existing','created','checking','localOwner','islandGenus','islandId','primary','clock','warning')
FIXTURE=ROOT/'cpppj/tests/fixtures/damageablehit-x86.tsv'
REPORT=ROOT/'cpppj/recovery-damageablehit-evidence.json'


def double_bits(value):
    """실제 wall-clock 입력 double의 원시 비트다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def sample(**values):
    """복원 판단 없이 현재 raw/타입/소유자 입력의 기본값을 만든다."""
    result=dict(zip(KEYS,(0,150,24,0,0,2,1,0,0,1,0,0,0,1,0,0,0,0,1,0,0,2,3,3,double_bits(10.1),double_bits(0))));result.update(values);return result


def inputs():
    """실제 HP/권한/held/free/방어/레벨과 dais/burning·주 섬 경고를 교차한다."""
    rows=[]
    # dead 진단은 정상 반환 격자와 구별하므로 두 판본 공통 정상 입력에는 넣지 않는다.
    for hp,damage,server,state,extra,reject,ranks in itertools.product((1,150,32768),(-1,0,24,200),(0,1),(0,1),(0,1,4,9),(0,1),((0,0),(3,1),(1,3))):
        rows.append(sample(current=hp,damage=damage,server=server,state=state,extra=extra,reject=reject,arank=ranks[0],vrank=ranks[1]))
    # Damageable의 actual HAS_HP/dais와 burning 생성/유지를 명시 입력으로 방문한다.
    for hp,damage,hasHP,dais,extra,reject,effect,existing,server in itertools.product((1,99,150),(0,24,200),(0,1),(0,1),(0,1,4),(0,1),(0,1),(0,1),(0,1)):
        rows.append(sample(kind=1,current=hp,damage=damage,hasHP=hasHP,dais=dais,extra=extra,reject=reject,effect=effect,existing=existing,server=server))
    # 현재 주 섬인지, 30초 재경고 마감 전/동률/후인지에 따른 판본별 UI 순서를 관찰한다.
    for deadline,genus,primary,local in itertools.product((0.0,10.1,10.2),(2,0x1000002,4),(3,4),(0,1)):
        rows.append(sample(kind=1,warning=double_bits(deadline),islandGenus=genus,primary=primary,localOwner=local))
    # NoDamage와 공격자 bomb genus의 별도 서버 사망 flags도 방문한다.
    for nodamage,disabled,owner in itertools.product((0,1),(0,1),(0,2)):
        rows.append(sample(current=1,damage=200,nodamage=nodamage,disabled=disabled,attackerOwner=owner,attackerGenus=0x100))
    unique={}
    # 실제 같은 원시 입력의 alarm 교차 재방문을 신규 고유 관찰로 세지 않는다.
    for row in rows:unique.setdefault(tuple(row[key] for key in KEYS),row)
    return list(unique.values())


class GeneralHitOracle(DamageOracle):
    """이미 복원한 실제 레벨/burning 및 새 일반 피해 helper를 같은 PE에서 실행한다."""
    def __init__(self,edition):
        """현재 readOnly helper와 명시 가상 경계 입력만 추가한다."""
        super().__init__(edition);self.h=HIT[edition];self.mu.mem_map(GROUND,0x20000)
        paths=[ROOT/f'extracted/damageablehit-helper/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as file:
            # 현재 정밀 함수의 실제 불연속 범위만 실행 허용한다.
            for row in csv.DictReader(file,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for segment in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in segment.split('-'));self.allowed.append((low,high+1))
        for offset,address in ((0x9c,BLOCK),(0x10,DIE),(0x88,UNREALIZE)):
            self.mu.mem_write(VTABLE+offset,struct.pack('<I',address))

    def on_instruction(self,mu,address,size,data):
        """방어/삭제/UI와 실제 wall clock만 관찰하고 HP·레벨·사망context는 원본을 실행한다."""
        h=self.h;esp=mu.reg_read(UC_X86_REG_ESP);args=[self.read(esp+4+i*4) for i in range(6)];ret=self.read(esp);purge=0;value=0
        if address==BLOCK:self.events.append('R:50:'+str(args[0]));value=self.input['reject'];purge=4
        elif address==DIE:self.events.append('D:50:'+str(args[0]));purge=4
        elif address==UNREALIZE:self.events.append('U:50')
        elif address==h['clock']:
            self.events.append('W:'+str(self.input['clock']));mu.mem_write(args[0],struct.pack('<Q',self.input['clock']));value=args[0]
        elif address==h['islandAt']:self.events.append('I:'+':'.join(map(str,args[:2])));value=70
        elif address==h['event']:self.events.append('V:12')
        elif address==h['warningSound']:self.events.append('S:islandUnderAttack.wav')
        elif address==h['format']:self.events.append('F:islandUnderAttack');mu.mem_write(args[0],b'islandUnderAttack\0');value=args[0]
        elif address==h['text']:self.events.append('L:islandUnderAttack')
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_body(self,case):
        """독립 현재 raw와 두 실제 일반 피해 진입에서 정상 반환/전체 부작용을 기록한다."""
        self.input=case;self.case=dict(kind='Set',after='-');self.events=[];mu=self.mu;patch=self.edition=='originals';p=self.d;h=self.h
        raw=bytearray((i*17+3)&255 for i in range(self.stride));struct.pack_into('<I',raw,0,VTABLE);struct.pack_into('<HH',raw,4,0,0);raw[10]=71;raw[11]=case['state'];raw[self.o['extra']]=case['extra'];raw[self.o['owner']]=case['victimOwner'];struct.pack_into('<ff',raw,14,20.75,21.9);struct.pack_into('<I' if patch else '<H',raw,26,case['current']&(0xffffffff if patch else 0xffff));struct.pack_into('<I',raw,36,0) if patch else raw.__setitem__(34,0);mu.mem_write(self.slot(50),bytes(raw));mu.mem_write(self.slot(60),bytes(self.stride));mu.mem_write(self.slot(60)+10,bytes([128]));mu.mem_write(self.slot(70),bytes(self.stride));mu.mem_write(self.slot(70)+10,bytes([90]));mu.mem_write(self.slot(70)+8,struct.pack('<H',case['islandId']))
        for number,flags,genus in ((71,0x10 if case['hasHP'] else 0,0x400000 if case['dais'] else 0),(128,0,case['attackerGenus']),(90,0,case['islandGenus'])):
            source=TYPES+number*self.type_stride;mu.mem_write(source,bytes(self.type_stride));mu.mem_write(source,struct.pack('<i',300));mu.mem_write(source+0xe8,struct.pack('<II',flags,genus));mu.mem_write(source+0xd4,struct.pack('<I',case['effect']))
        mu.mem_write(self.p['quarter'],bytes(4));mu.mem_write(self.p['priest'],struct.pack('<I',158));mu.mem_write(p['disabled'],struct.pack('<I',case['disabled']));mu.mem_write(p['noDamage'],struct.pack('<I',case['nodamage']));mu.mem_write(p['checking'],struct.pack('<I',case['checking']));mu.mem_write(p['burningType'],struct.pack('<I',14));mu.mem_write(p['bulf'],struct.pack('<I',135));mu.mem_write(h['server'],struct.pack('<I',case['server']));mu.mem_write(h['local'],struct.pack('<I',case['localOwner']));mu.mem_write(h['dead'],bytes(8));mu.mem_write(h['warning'],struct.pack('<Q',case['warning']))
        for owner,rank in ((case['attackerOwner'],case['arank']),(case['victimOwner'],case['vrank'])):
            mu.mem_write(PLAYERS+owner*self.player_stride+p['player_level'],bytes([rank&255]))
            if patch and 1<=owner<=8:mu.mem_write(p['challenge']+0x4f+owner-1,bytes([rank&255]))
        mu.mem_write(PLAYERS+case['victimOwner']*self.player_stride+h['primary'],struct.pack('<i',case['primary']))
        if not patch:mu.mem_write(0x546778,bytes(4));mu.mem_write(0x5670cc,struct.pack('<I',GROUND));mu.mem_write(GROUND+(21*256+20)*2,struct.pack('<H',70))
        self.write_ranges=[(self.slot(50),self.slot(50)+self.stride),(h['dead'],h['dead']+8),(h['warning'],h['warning']+8)]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,self.slot(50));mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<IIII',STOP,case['damage']&0xffffffff,case['attackerOwner'],60));mu.emu_start(h['derived'] if case['kind'] else h['base'],STOP,count=100000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+16:raise RuntimeError('일반 피해 반환/스택 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('일반 피해 x87 스택 오류')
        self.returns+=1;return [str(mu.reg_read(UC_X86_REG_EAX)),';'.join(self.events),bytes(mu.mem_read(self.slot(50),self.stride)).hex(),bytes(mu.mem_read(h['dead'],8)).hex(),bytes(mu.mem_read(h['warning'],8)).hex()]


def generate(smoke):
    """독립 PE 관찰을 저장하고 정상 반환/하위 경계·현재 source SHA를 구분한다."""
    rows=[];editions={};cases=inputs()[::211] if smoke else inputs();files={Path(__file__),ROOT/'tools/decomp_priestdamage_oracle.py',ROOT/'tools/decomp_prieststate_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/damageablehit-helper-functions.json',FIXTURE}
    for edition in HIT:
        oracle=GeneralHitOracle(edition)
        # 같은 독립 입력을 두 실제 일반 피해 몸체에서 관찰한다.
        for case in cases:
            try:out=oracle.run_body(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*out])
        editions[edition]=dict(returns=oracle.returns,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),assertions=oracle.assertions,instructions=oracle.instructions);files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,oracle.returns,flush=True)
    if smoke:return
    for name in ('cpppj/src/o/RawDamageableHit.cpp','cpppj/src/o/RawDamageableHit.h','cpppj/src/o/RawSquidHitPoints.cpp','cpppj/src/o/RawSquidHitPoints.h','cpppj/src/o/RawPriestDamage.cpp','cpppj/src/o/RawPriestDamage.h','cpppj/src/o/RawDamageableBurning.cpp','cpppj/src/o/RawDamageableBurning.h','cpppj/src/o/SquidReward.cpp','cpppj/src/o/SquidReward.h','cpppj/tests/DamageableHitTests.cpp','cpppj/recovery-priestdamage-evidence.json'):files.add(ROOT/name)
    FIXTURE.write_text('# 실제 두 PE Base/Damageable 피격. 가상방어/삭제/UI/시계/burning 생성·삭제는 명시 관찰 경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),host='HJOW-Athlon',editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},os_calls=0,limitations=['실제 Base/Damageable Hit와 레벨/HP저장/HP보유/섬genus·번호/경고마감/사망context 몸체를 실행한다.','가상 방어·삭제·unrealize/소유자·UI·wall clock·burning 생성/삭제는 명시 관찰 경계다.','서버 사망의 실제 corpse/효과/월드 삭제 하위와 다른 파생 virtual Hit는 별도 경계다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """현재 SHA와 두 실제 진입의 고유 정상 입력/OS0을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total'] or report['os_calls']:raise RuntimeError('일반 피해 관찰 수/OS 오류')
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('일반 피해 SHA 불일치 '+name)
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if item['returns']!=len(selected) or item['assertions'] or len({tuple(row[1:27]) for row in selected})!=len(selected):raise RuntimeError('일반 피해 고유/정상/assert 오류')
    print(json.dumps(dict(verified=True,total=len(rows)),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
