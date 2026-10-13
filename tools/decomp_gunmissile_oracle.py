#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Missile ctor와 최초 썬 캐논 frame을 제한 x86으로 관찰한다. 게임/OS 실행 없음."""
import argparse
import collections
import csv
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, POOL, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 각 판본 실제 몸체와 외부 월드 효과/자료 경계다.
SPEC={
 'originals':dict(ctor=0x481a50,frame=0x482340,vtable=0x50d630,now=0x55b4d0,types=0x59ab20,type=0x49a840,slotType=0x4ac550,
  shape=0x419850,dimensions=0x49a900,ray=0x481750,group=0x4ac950,create=0x4af530,sound=0x4a9c30,
  attach=0x41bd20,center=0x4adea0,impact=0x45a750,refresh=0x4d5000,hit=0x4d4f00,bounds=0x49af10,hitEffect=0x470ac0,validType=0x436d00,impactType=0x541278,
  scaleX=0x59a92c,scaleY=0x59a930,capacity=0x541348,checking=0x5e4794,sun=0x541258,disc=0x5411a4,
  direct=0x541174,process=0x5410e0,ftol=0x4e49c0,fpuBug=None,boxPoint=0x431f90,
  end=104,start=112,origin=56,endpoint=72,previous=120,size=128),
 'originalCD':dict(ctor=0x44ac70,frame=0x44b260,vtable=0x501ca0,now=0x5484a8,types=0x51c960,type=None,slotType=0x4abf50,
  shape=None,dimensions=0x4442b0,ray=0x44a960,group=0x4ac420,create=0x4ab390,sound=0x438ae0,
  attach=0x48f590,center=0x4aef30,impact=0x47c8a0,refresh=0x4433e0,hit=0x4432f0,bounds=0x444b50,hitEffect=0x455b40,validType=None,impactType=0x51cb64,
  scaleX=0x565c44,scaleY=0x565c48,capacity=0x51cbf0,checking=0x531454,sun=0x51cb44,disc=0x51ca90,
  direct=0x51ca60,process=0x51c9d0,ftol=0x4f161c,fpuBug=0x546778,boxPoint=None,
  end=80,start=88,origin=40,endpoint=56,previous=96,size=104),
}
# 실제 외부 가상 경계와 독립 raw 객체의 실행기 메모리다.
OBJECT,VTABLE,HEADER=0x11318000,0x11319000,0x1131a000
MUZZLE,OWNER,POP,UNPOP,DESTROY,EFFECT=STOP+0x100,STOP+0x110,STOP+0x120,STOP+0x130,STOP+0x140,STOP+0x150
KEYS=('kind','x','y','hotx','hoty','typehotx','typehoty','endx','endy','fraction','targetstate','previous','corner')
FIXTURE=ROOT/'cpppj/tests/fixtures/gunmissile-x86.tsv'
REPORT=ROOT/'cpppj/recovery-gunmissile-evidence.json'


def bits(value):
    """직접 입력 float32 비트를 만든다. 기대값을 계산하지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """포구/핫스폿/게임시각과 실제 이동·표적충돌·범위 종료 경계를 방문한다."""
    result=[]
    # 숫자 생성자는 픽셀 hot spot 및 같은 위치의 타입 보정에 대해 교차한다.
    for x,y in ((50.0,50.0),(80.25,64.75),(1.25,1.25)):
        for hotx,hoty in ((0,0),(16,11),(-8,-5)):
            for typehotx,typehoty in ((0,0),(16,11)):
                for corner in (0,1):result.append(dict(zip(KEYS,(0,bits(x),bits(y),hotx,hoty,typehotx,typehoty,bits(100.0),bits(90.0),0,0,0,corner))))
    # frame 입력은 ctor 기대값을 사용하지 않으며 2초의 독립 경로를 만든다.
    for fraction in (0,1,3,4,5,8,12):
        for targetstate in (0,1,2,3):
            for previous in (0,1):
                for corner in (0,1):result.append(dict(zip(KEYS,(1,bits(50.0),bits(50.0),0,0,0,0,bits(50.0),bits(90.0),fraction,targetstate,previous,corner))))
    return result


class MissileOracle(OwnerOracle):
    """실제 숫자 생성/프레임 몸체를 실행하고 외부 자산/effect/피해 경계를 기록한다."""
    def __init__(self,edition):
        """Ghidra 정상 몸체만 허용하며 실제 virtual frame을 확인한다."""
        super().__init__(edition);self.p=SPEC[edition];self.mu.mem_map(0,0x1000)
        # 주몸체와 CRT/rectangle helper의 정밀 내보내기를 별도로 감사한다.
        for name in ('gunmissile','gunmissile-helper'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 정밀 내보낸 불연속 함수 몸체만 허용한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        if self.read(self.p['vtable']+0x18)!=self.p['frame']:raise RuntimeError('Missile 실제 virtual Frame 불일치')
        self.boundaries=collections.Counter();self.returns=0;self.events=[]

    def read(self,address,width=4):
        """비정렬 원본 실행기 메모리를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """원본 외부 가상 생성/Pop/비행효과/사운드/삭제를 정확한 인자로 관찰한다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(esp+4+i*4) for i in range(6)];ret=self.read(esp);purge=0;value=0
        if address==MUZZLE:mu.mem_write(args[0],struct.pack('<II',self.case['x'],self.case['y']));value=args[0];purge=4;self.events.append('M:50')
        elif address==p['slotType']:value=TYPES+self.read(this+10,1)*self.type_stride
        elif p['type'] is not None and address==p['type']:value=TYPES+args[0]*self.type_stride
        elif p['shape'] is not None and address==p['shape']:value=HEADER+0x600;purge=8;self.events.append('H:50')
        elif address==p['dimensions']:
            mu.mem_write(args[0],struct.pack('<i',2));mu.mem_write(args[1],struct.pack('<i',3));purge=12;self.events.append('D:60')
        elif address==p['ray']:
            # 사정선 함수는 출력 좌표를 쓴 뒤 그 포인터를 EAX로 반환하며 호출자가 즉시 역참조한다.
            self.events.append('R:'+','.join(map(str,args[1:6])));mu.mem_write(args[0],struct.pack('<II',self.case['endx'],self.case['endy']));value=args[0]
        elif address==p['group']:value=1
        elif address==p['create']:self.events.append(f'A:{args[0]}:{args[1]}');value=self.slot(90)
        elif address==OWNER:mu.mem_write(this+(34 if self.edition=='originals' else 32),bytes([args[0]&255]));self.events.append(f'O:90:{args[0]}');purge=4
        elif address==POP:
            self.events.append('P:90:'+':'.join(map(str,args[:3])));mu.mem_write(this+14,struct.pack('<II',args[0],args[1]));purge=12
        elif address==UNPOP:self.events.append(f'U:90:{args[0]}');purge=4
        elif address==DESTROY:self.events.append(f'X:90:{args[0]}');purge=4
        elif address==EFFECT:self.events.append(f'E:11:{args[0]}:{args[1]}')
        elif address==p['attach']:
            self.events.append('B:'+':'.join(map(str,args[:4])));mu.mem_write(this+4,struct.pack('<II',81,args[1]));purge=16
        elif address==p['center']:mu.mem_write(args[0],struct.pack('<ff',50,50));value=args[0];purge=4;self.events.append('C:50')
        elif address==p['impact']:self.events.append('I:'+':'.join(map(str,args[:4])))
        elif address==p['sound']:self.events.append('S:'+':'.join(map(str,args[:5])))
        elif address==p['refresh']:self.events.append('Q:50')
        elif address==p['hit']:self.events.append(f'G:50:{args[0]}');purge=4
        elif address==p['bounds']:self.events.append('T:'+':'.join(map(str,args[1:3])));mu.mem_write(args[0],struct.pack('<ffff',49,89,51,91));value=args[0];purge=12
        elif address==p['hitEffect']:self.events.append('H:'+':'.join(map(str,args[:2])))
        elif p['validType'] is not None and address==p['validType']:value=1
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_body(self,case):
        """생성자 또는 독립 frame 입력을 실행하고 필드/자산/효과 순서를 반환한다."""
        self.case=case;self.events=[];p=self.p;mu=self.mu;patch=self.edition=='originals';mu.mem_write(0,bytes(4))
        fields=bytearray((i*13+7)&255 for i in range(p['size']));mu.mem_write(OBJECT,bytes(fields));mu.mem_write(VTABLE,bytes(0x400))
        for offset,address in ((0xb4,MUZZLE),(0x74,OWNER),(0x90,POP),(0x48,UNPOP),(0x10,DESTROY)):mu.mem_write(VTABLE+offset,struct.pack('<I',address))
        for sid,typeid in ((50,128),(60,71),(90,80)):
            slot=bytearray(self.stride);struct.pack_into('<I',slot,0,VTABLE);slot[10]=typeid;slot[11]=case['targetstate'] if sid==60 else 0;slot[34 if patch else 32]=1
            struct.pack_into('<ff',slot,14,50,50);mu.mem_write(self.slot(sid),bytes(slot));mu.mem_write(TYPES+typeid*self.type_stride,bytes(self.type_stride))
        source=TYPES+128*self.type_stride;struct.pack_into('<II',fields,20,50,60);mu.mem_write(source+0x9c,struct.pack('<I',1));mu.mem_write(source+0x44,struct.pack('<f',40));mu.mem_write(source+(0x1dc if patch else 0x1bc),struct.pack('<ii',case['typehotx'],case['typehoty']))
        mu.mem_write(source+0xdc,struct.pack('<I',HEADER+0x400));mu.mem_write(HEADER+0x408,struct.pack('<I',64));mu.mem_write(HEADER+0x43c,struct.pack('<hh',case['hotx'],case['hoty']));mu.mem_write(HEADER+0x620,struct.pack('<hh',case['hotx'],case['hoty']))
        mu.mem_write(TYPES+11*self.type_stride+(0x1d0 if patch else 0x1b0),struct.pack('<I',EFFECT));mu.mem_write(p['scaleX'],struct.pack('<f',16));mu.mem_write(p['scaleY'],struct.pack('<f',11));mu.mem_write(p['capacity'],struct.pack('<I',188 if patch else 171));mu.mem_write(p['checking'],bytes(4))
        for name,value in (('sun',128),('disc',83),('direct',71),('process',28)):mu.mem_write(p[name],struct.pack('<I',value))
        if p['fpuBug']:mu.mem_write(p['fpuBug'],bytes(4))
        if case['kind']==0:
            mu.mem_write(p['now'],struct.pack('<d',10.25));corner=(bits(100),bits(90)) if case['corner'] else (bits(-10000),bits(-10000));args=[50,60,bits(20),80,11,bits(100),bits(90),*corner,0,0,0]
        else:
            fields=bytearray((i*13+7)&255 for i in range(p['size']));struct.pack_into('<III',fields,0,p['vtable'],81,90);struct.pack_into('<IIIII',fields,12,0,0,50,60,case['corner'])
            struct.pack_into('<f',fields,48 if patch else 32,20);struct.pack_into('<f',fields,52 if patch else 36,0)
            struct.pack_into('<II',fields,p['origin'],case['x'],case['y']);struct.pack_into('<II',fields,64 if patch else 48,case['endx'],case['endy']);struct.pack_into('<II',fields,p['endpoint'],case['endx'],case['endy'])
            struct.pack_into('<ffff',fields,88 if patch else 64,-10000,-10000,-10000,-10000);struct.pack_into('<dd',fields,p['end'],12,10)
            struct.pack_into('<ff',fields,p['previous'],50,89 if case['previous'] else -10000)
            if patch:struct.pack_into('<IIII',fields,32,0,0,0,0)
            mu.mem_write(OBJECT,bytes(fields));mu.mem_write(p['now'],struct.pack('<d',10+case['fraction']*0.5));args=[]
        self.write_ranges=[(0,4),(OBJECT,OBJECT+p['size']),(self.slot(90),self.slot(90)+self.stride)]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        mu.emu_start(p['ctor'] if case['kind']==0 else p['frame'],STOP,count=100000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+len(args)*4:raise RuntimeError('Missile 반환/스택 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Missile x87 스택 오류')
        self.returns+=1;return [';'.join(self.events),bytes(mu.mem_read(OBJECT,p['size'])).hex(),bytes(mu.mem_read(self.slot(90),self.stride)).hex()]


def generate(smoke):
    """실제 원본 관찰·각 입력 파일의 SHA를 저장한다."""
    cases=inputs();cases=cases[::13] if smoke else cases;rows=[];editions={};files=[Path(__file__),FIXTURE,ROOT/'tools/ghidra/gunmissile-functions.json',ROOT/'tools/ghidra/gunmissile-helper-functions.json']
    for edition in SPEC:
        oracle=MissileOracle(edition)
        # 독립 입력마다 정상 반환과 모든 실제 필드를 관찰한다.
        for case in cases:
            try:output=oracle.run_body(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*output])
        editions[edition]=dict(returns=oracle.returns,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),assertions=oracle.assertions,instructions=oracle.instructions);files.extend(oracle.exports);files.append(ROOT/oracle.spec['binary'])
    FIXTURE.write_text('# 실제 두 PE Missile ctor/frame 관찰. 자산/표시/effect/피해는 명시 외부 관찰 경계다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),host='HJOW-Athlon',editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in files},limitations=['최초 썬 캐논 정상 발사와 별도 교차 레인 배리어 코너 경로를 관찰한다.','포구/SHP/사정선/자산 생성/Pop/효과/피해/삭제는 명시 경계이며 빈 성공의 실제 월드 실행으로 세지 않는다.'],os_calls=0),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(total=len(rows),editions=editions),ensure_ascii=False))


def verify():
    """현재 원본/정밀 몸체/실행기/fixture SHA와 정상 반환 행수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('Missile 행 수 오류')
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('Missile SHA 불일치 '+name)
    for edition,data in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=data['returns'] or data['assertions']:raise RuntimeError('Missile 반환/assert 오류')
        if len({tuple(row[1:14]) for row in selected})!=len(selected):raise RuntimeError('Missile 고유 입력 중복')
        if not data['boundaries'].get(f'{SPEC[edition]["hit"]:08x}'):raise RuntimeError('Missile 정상 직접 표적 hit 경계 관찰 없음')
    print(json.dumps(dict(verified=True,total=len(rows)),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
