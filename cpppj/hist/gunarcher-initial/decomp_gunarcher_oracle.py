#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""SunArcher 실제회전·각속도Fire·helper·Firector와생성직전인자를두PE로독립관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
import capstone
from decomp_gunanimation_oracle import GunAnimationOracle,OBJECT,VTABLE,ALLOC,UNPOP,REPOP,bits
from decomp_owner_oracle import ROOT,TYPES,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 현재정밀전용몸체와실제SunArcher 가상표/현재delta·출력통지다.
ARCHER={
 'originals':dict(entries=[0x467940,0x467c80,0x4676c0,0x467b20,0x467b90,0x468140,0x466f50],outer=[0x50b450,0x50b1c0],inner=[0x50b448,0x50b1bc],attach=0x41bd20,notify=0x4214a0,delta=0x55b4e0,checking=0x5e4794,fpuBug=0x5e4acc,source=0x541174),
 'originalCD':dict(entries=[0x4a26c0,0x4a2a10,0x4a2640,0x4a28a0,0x4a2910,0x4a3960,0x4a12f0],outer=[0x505570,0x5054d0],inner=[0x50556c,0x5054cc],attach=0x48f590,notify=0x448c10,delta=0x548430,checking=0x531454,fpuBug=0x546778,source=0x51ca60),
}
# 네이티브float 반환의저장수신자와원본Form 부모조회가상경계다.
OUTPUT,STORE_FLOAT,PARENT_QUERY=0x1131b000,STOP+0x800,STOP+0x180
KEYS=('kind','frame','delta','position','velocity','acceleration','fired','sprung','targetstate',
      'launch','due','aim','tx','ty','directions','frames','hp')
FIXTURE=ROOT/'cpppj/tests/fixtures/gunarcher-x86.tsv'
REPORT=ROOT/'cpppj/gunarcher.json'


def wide(value):
    """double입력의실제raw비트다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def sample(**values):
    """원본기대값계산없이각독립입력의공통기본필드를넣는다."""
    row=dict(zip(KEYS,(0,0,wide(0.1),bits(360),bits(180),bits(180),0,0,0,1,1,0,bits(60),bits(50),20,40,1)));row.update(values);return row


def inputs():
    """회전/현재표적·due·각상태·발사/끝경계와정수/각도helper를방문한다."""
    rows=[]
    # 현재타입방향수는16/20을별도공급하며프레임끝과target0 방향getter를포함한다.
    for frame,point,due,targetstate in itertools.product((0,1,10,15,16,31,32),((60,50),(50,40),(50,60),(40,50),(60,60)),(0,1),(0,2)):
        rows.append(sample(frame=frame,tx=bits(point[0]),ty=bits(point[1]),due=due,targetstate=targetstate))
    for aim,frame in itertools.product(range(8),(0,10,32)):rows.append(sample(frame=frame,aim=aim,hp=0))
    # Fire몸체의역회전/sprung/발사와delta0/2 자연분기를교차한다.
    for position,velocity,flags,delta,targetstate,launch in itertools.product((0,360,1230,1410,1590,-1),(180,-180),((0,0),(0,1),(1,1)),(0.0,0.1,0.5,2.0),(0,2),(0,1)):
        rows.append(sample(kind=1,position=bits(position),velocity=bits(velocity),acceleration=bits(-180 if velocity<0 else 180),fired=flags[0],sprung=flags[1],delta=wide(delta),targetstate=targetstate,launch=launch,hp=launch))
    # 두판본각도표helper의양쪽중간경계와wrap을독립방문한다.
    for angle in (-1,0,10.5,21,28,35,148,164,180,345,352.5,359.9999,360):rows.append(sample(kind=2,position=bits(angle)))
    # 실제Firector각getter는0..32만정상이며33은원본assert로분리한다.
    for frame in range(33):rows.append(sample(kind=3,frame=frame));rows.append(sample(kind=5,frame=frame))
    for angle,sprung in itertools.product((0,1,22.5,360,720,1230,1410,1590,1799.75),(0,1)):rows.append(sample(kind=4,position=bits(angle),sprung=sprung))
    for targetstate,hp in itertools.product((0,1,2,3),(0,1)):rows.append(sample(kind=6,targetstate=targetstate,hp=hp))
    return rows


class ArcherOracle(GunAnimationOracle):
    """기존공통경계위에실제Archer/기하/CRT몸체를독립추가한다."""
    def __init__(self,edition):
        """동일원본PE·현재정밀함수와실제짧은가상thunk를검사한다."""
        super().__init__(edition);self.a=ARCHER[edition];self.p=dict(self.p);self.p['entries']=self.a['entries'];self.p['outer']=self.a['outer'];self.p['inner']=self.a['inner'];self.boundaries=collections.Counter()
        for name in ('gunarcher','gunarcher-helper','gunmissile-helper'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 정밀export의불연속실제몸체만허용한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for segment in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in segment.split('-'));self.allowed.append((low,high+1))
        decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
        # 실제virtual getter/setter는첫RET까지PE바이트를별도감사해CD 함수소유오류를분리한다.
        for table in self.a['outer']:
            for offset in range(0x20,0x48,4):
                entry=self.read(table+offset)
                if any(low<=entry<high for low,high in self.allowed):continue
                end=None
                for item in decoder.disasm(bytes(self.mu.mem_read(entry,512)),entry):
                    if item.mnemonic.startswith('ret'):end=item.address+item.size;break
                if end is None:raise RuntimeError('Archer virtual RET누락')
                self.allowed.append((entry,end));self.entries.add(entry);self.thunks[f'{entry:08x}']=bytes(self.mu.mem_read(entry,end-entry)).hex()
        # 원본float 반환을저장할뿐계산하지않는외부수신자명령이다.
        receiver=b'\xd9\x1d'+struct.pack('<I',OUTPUT)+b'\xe9'+struct.pack('<i',STOP-(STORE_FLOAT+11));self.mu.mem_write(STORE_FLOAT,receiver);self.allowed.append((STORE_FLOAT,STORE_FLOAT+len(receiver)))

    def on_instruction(self,mu,address,size,data):
        """실제Fire의launch반환경계와생성/출력/회전format만추가관찰한다."""
        p=self.p;a=self.a;esp=mu.reg_read(UC_X86_REG_ESP);args=[self.read(esp+4+i*4) for i in range(4)];ret=self.read(esp);this=mu.reg_read(UC_X86_REG_ECX);purge=0;value=0
        if address==p['missile'] and self.case['kind']==1:
            # Fire의실제launch 몸체는계속실행한다.호출한Missile ctor만별도관찰단위경계다.
            arguments=[self.read(esp+4+i*4) for i in range(12)];self.events.append('J:'+','.join(map(str,arguments)));value=ALLOC;purge=48
        elif address==a['attach']:self.events.append('B:'+':'.join(map(str,args)));mu.mem_write(this+4,struct.pack('<II',81,50));purge=16
        elif address==a['notify']:self.events.append(f'Q:{args[0]}')
        elif address==PARENT_QUERY:value=50
        elif address==p['format']:
            self.events.append(f'F:{args[2]}');mu.mem_write(args[0],f'sunDiscThrowerRatchet0{args[2]}.wav\0'.encode());value=args[0]
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_body(self,case):
        """실제필드/부모전체raw와helper반환·정상/생성접두를기록한다."""
        self.case=case;self.events=[];p=self.p;a=self.a;mu=self.mu;patch=self.edition=='originals';kind=case['kind'];fire=kind in (1,3,4,5,6);fields=bytearray((i*13+7)&255 for i in range(88));struct.pack_into('<III',fields,0,a['outer'][int(fire)],81,50);struct.pack_into('<ffff',fields,12,60,60,59.5,59.5);struct.pack_into('<I',fields,32,a['inner'][int(fire)]);fields[40]=0x20;struct.pack_into('<HH',fields,42,70,60 if case['hp'] or fire else 0);struct.pack_into('<dI',fields,48,10 if case['due'] else 10.25,0);struct.pack_into('<IIIII',fields,64,case['position'],case['velocity'],case['acceleration'],case['sprung'],case['fired']);mu.mem_write(OBJECT,bytes(fields));mu.mem_write(OUTPUT,bytes(4));self.point=struct.unpack('<ff',struct.pack('<II',case['tx'],case['ty']))
        for sid,typeid in ((50,71),(60,158),(70,34)):
            raw=bytearray(self.stride);struct.pack_into('<I',raw,0,VTABLE);struct.pack_into('<H',raw,6,81 if sid==50 else 0);raw[10]=typeid;raw[11]=case['targetstate'] if sid==60 else 0;raw[12]=0xa5;raw[34 if patch else 32]=1;struct.pack_into('<ff',raw,14,50.5 if sid==50 else 60,50.5 if sid==50 else 60);struct.pack_into('<I',raw,36,case['frame']) if patch else raw.__setitem__(34,case['frame']);mu.mem_write(self.slot(sid),bytes(raw))
        mu.mem_write(VTABLE,struct.pack('<I',PARENT_QUERY));mu.mem_write(VTABLE+0x48,struct.pack('<I',UNPOP));mu.mem_write(VTABLE+0x90,struct.pack('<I',REPOP));mu.mem_write(OBJECT+0x100,struct.pack('<I',VTABLE+0x200));mu.mem_write(VTABLE+0x21c,struct.pack('<I',STOP+0x100));source=TYPES+71*self.type_stride;mu.mem_write(source,bytes(self.type_stride));mu.mem_write(source+0xe8,struct.pack('<I',0x800));mu.mem_write(source+0x110,struct.pack('<ii',case['directions'],case['frames']));mu.mem_write(source+0x118,struct.pack('<i',0))
        mu.mem_write(p['now'],struct.pack('<d',10));mu.mem_write(a['delta'],struct.pack('<Q',case['delta']));mu.mem_write(a['checking'],bytes(4));mu.mem_write(a['fpuBug'],bytes(4));mu.mem_write(a['source'],struct.pack('<I',71));mu.mem_write(p['gun'],struct.pack('<I',34));mu.mem_write(0,bytes(4));self.write_ranges=[(0,4),(OBJECT,OBJECT+0x100),(ALLOC,ALLOC+0x200),(OUTPUT,OUTPUT+4),(self.slot(50),self.slot(50)+self.stride)]
        if kind==0 and not case['hp']:
            # target0 aimgetter의원본현재추적방향BYTE는004ad390/CD004adbc0 몸체가입력에서읽는다.
            mu.mem_write(self.slot(50)+ (30 if patch else 28),bytes([case['aim']]))
        args=[];return_address=STOP
        if kind==2:args=[case['position']]
        elif kind==3:return_address=STORE_FLOAT
        elif kind==4:
            angle=struct.unpack('<f',struct.pack('<I',case['position']))[0];value=wide(angle);args=[value&0xffffffff,value>>32]
        elif kind==5:args=[70,60]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),return_address,*args));before=self.prefix_stops;mu.emu_start(a['entries'][kind],STOP,count=100000);prefix=self.prefix_stops!=before
        if not prefix:
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+(8 if kind in (4,5) else 0):raise RuntimeError('Archer 반환/스택 오류')
            if mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Archer x87 스택 오류')
            self.returns+=1
        result='prefix' if prefix else str(mu.reg_read(UC_X86_REG_EAX)) if kind in (2,4,6) else 'return';return [result,';'.join(self.events),bytes(mu.mem_read(OBJECT,88)).hex(),bytes(mu.mem_read(self.slot(50),self.stride)).hex(),bytes(mu.mem_read(OUTPUT,4)).hex()]


def generate(smoke):
    """정상완료와실제Missile 생성직전접두를분리해독립관찰을저장한다."""
    cases=inputs();cases=cases[::97] if smoke else cases;rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_gunanimation_oracle.py',ROOT/'tools/decomp_owner_oracle.py',FIXTURE,ROOT/'tools/ghidra/gunarcher-functions.json',ROOT/'tools/ghidra/gunarcher-helper-functions.json'}
    for edition in ARCHER:
        oracle=ArcherOracle(edition)
        # 같은입력의재방문을새관찰수에더하지않는다.
        for case in cases:
            try:output=oracle.run_body(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*output])
        editions[edition]=dict(returns=oracle.returns,prefix=oracle.prefix_stops,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.stub_calls),extra_boundaries=dict(oracle.boundaries),pe_thunks=oracle.thunks,assertions=oracle.assertions,instructions=oracle.instructions);files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,oracle.returns,oracle.prefix_stops,flush=True)
    if smoke:return
    for relative in ('cpppj/src/o/RawArcherAnimation.cpp','cpppj/src/o/RawArcherAnimation.h','cpppj/tests/ArcherAnimationTests.cpp'):files.add(ROOT/relative)
    FIXTURE.write_text('# 실제두PE Archer 회전/발사/helper/ctor. prefix는Missile생성전인자이며생성완료가아니다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),host='HJOW-Athlon',editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},os_calls=0,limitations=['중심/공간/소리/출력/부착/완료는명시관찰경계다.','Fire의실제launch도실행하며호출한Missile ctor는별도archermissile 단위경계다.별도launch만ctor직전에서정지한다.','각도기하/fmod/float절삭은정밀원본몸체를직접실행한다.','현재합성타입방향수/물리프레임은실제게임메타데이터조회완료로세지않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """현재SHA와정상반환/접두/고유입력및실제CRT와주몸체실행을감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('Archer 관찰행수 오류')
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('Archer SHA 불일치 '+path)
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({tuple(row[1:18]) for row in selected})!=len(selected) or item['assertions'] or sum(row[18]=='prefix' for row in selected)!=item['prefix'] or len(selected)!=item['returns']+item['prefix']:raise RuntimeError('Archer 고유입력/반환/assert 오류')
    print(json.dumps(dict(verified=True,total=len(rows)),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
