#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""썬 캐논 회전/발사/원위치와 Missile 생성 직전 인자를 실제 두 PE에서 관찰한다.

Missile ctor는 명시 관찰 경계에서 멈춘다. 경계 정지를 전체 launch 완료나 투사체 실행으로 세지 않는다.
게임/OS/창 실행 없음. CD의 잘못 소유된 짧은 virtual thunk는 PE 명령/ret 폭을 별도 감사한다.
"""
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
import capstone

# 판본별 실제 원본 진입점과 명시 월드 효과 경계다.
SPEC={
 'originals':dict(entries=[0x467130,0x467330,0x467240,0x466f50],outer=[0x50b260,0x50b318,0x50b2c8],inner=[0x50b258,0x50b310,0x50b2c4],
  parent=0x41bfb0,center=0x4adea0,direction=0x41ce90,random=0x4558f0,format=0x4a89c0,sound=0x4a9d70,
  fromform=0x4ae2d0,processType=0x41bff0,kill=0x41bf10,flags=0x4ac1e0,point=0x4571b0,new=0x4e4391,missile=0x481a50,
  now=0x55b4d0,sun=0x541258,display=0x541198,impact=0x54109c,gun=0x5410f8,special=0x5412a8,
  helpers=[(0x468330,1),(0x468320,1),(0x468440,1),(0x468490,1),(0x4684a0,1),(0x4684b0,2),(0x468640,1),(0x466a40,1),(0x466ab0,1)]),
 'originalCD':dict(entries=[0x4a1fc0,0x4a21b0,0x4a20c0,0x4a12f0],outer=[0x505520,0x505480,0x505660],inner=[0x505518,0x505478,0x505658],
  parent=0x48f870,center=0x4aef30,direction=0x43f620,random=0x48cc10,format=0x420560,sound=0x438c00,
  fromform=0x4af4e0,processType=0x48f8b0,kill=0x48f7b0,flags=0x4abac0,point=0x4793d0,new=0x4f1650,missile=0x44ac70,
  now=0x5484a8,sun=0x51cb44,display=0x51ca84,impact=0x51c98c,gun=0x51c9e8,special=0x51cb94,
  helpers=[(0x4a3460,1),(0x4a3450,1),(0x4a3170,1),(0x4a3180,1),(0x4a31a0,2),(0x4a3190,1),(0x4a2cd0,1),(0x4a0c80,1),(0x4a3a80,1),(0x4a3a90,1),(0x436e50,1),(0x4a3f00,1),(0x4a3f20,1)]),
}
# 실행기 내부 객체/가상 함수/할당 경계 주소와 저장 자료다.
OBJECT,ALLOC,VTABLE,NOTIFY,UNPOP,REPOP=0x11318000,0x11319000,0x1131a000,STOP+0x100,STOP+0x110,STOP+0x120
KEYS=('kind','frame','direction','rest','targetstate','hp','due','dirty')
FIXTURE=ROOT/'cpppj/tests/fixtures/gunanimation-x86.tsv'
REPORT=ROOT/'cpppj/recovery-gunanimation-evidence.json'


def bits(value):
    """입력 float의 raw 비트이며 기대 결과를 계산하지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """모든 28물리프레임/4방향, 시각 지연, 표적 free/dead 및 HP 유무를 방문한다."""
    result=[]
    # 회전은 모든 방향·물리프레임, 원위치는 마지막 8번 방향도 포함한다.
    for kind,directions in ((0,range(4)),(2,range(5))):
        for frame in range(28):
            for direction in directions:
                for due in (0,1):result.append(dict(zip(KEYS,(kind,frame,direction,direction*2,0,1,due,0x20))))
    # 발사 정상 종료는 발사 완료 24~27 및 invalid target 분기로 검사한다.
    for frame in range(48):
        for targetstate in (0,1,2,3):
            for hp in (0,1):
                for due in (0,1):result.append(dict(zip(KEYS,(1,frame,0,0,targetstate,hp,due,0x20))))
    # launch descriptor는 실제 Missile ctor 진입에서 정지한다.
    for targetstate in (0,1,2,3):
        for hp in (0,1):result.append(dict(zip(KEYS,(3,0,0,0,targetstate,hp,1,0x20))))
    return result


class GunAnimationOracle(OwnerOracle):
    """원본 몸체와 짧은 PE thunk만 실행하며 효과/생성 관찰 경계를 기록한다."""
    def __init__(self,edition):
        """읽기 전용 정밀 몸체와 PE 가상 표/짧은 thunk를 확인한다."""
        super().__init__(edition);self.p=SPEC[edition];self.mu.mem_map(0,0x1000)
        paths=[ROOT/f'extracted/gunanimation/{edition}/{name}' for name in ('creation.c','functions.tsv')];self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # Ghidra가 정상 내보낸 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(n,16) for n in part.split('-'));self.allowed.append((low,high+1))
        self.thunks={};decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
        # CD 정밀 함수 소유 오류를 피하면서 실제 PE thunk의 마지막 정상 RET까지 정확히 허용한다.
        for address,returns in self.p['helpers']:
            count=0;end=None
            for instruction in decoder.disasm(bytes(self.mu.mem_read(address,512)),address):
                if instruction.mnemonic.startswith('ret'):
                    count+=1
                    if count==returns:end=instruction.address+instruction.size;break
            if end is None:raise RuntimeError(f'thunk 반환 경계 없음 {address:08x}')
            self.allowed.append((address,end));self.entries.add(address);self.thunks[f'{address:08x}']=bytes(self.mu.mem_read(address,end-address)).hex()
        self.stub_calls=collections.Counter();self.returns=0;self.prefix_stops=0;self.events=[]
        for index,address in enumerate(self.p['outer']):
            if self.read(address+0x18)!=self.p['entries'][index]:raise RuntimeError('GunAnimation 실제 virtual frame 불일치')

    def read(self,address,width=4):
        """실행기 메모리의 비정렬 raw 필드를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """외부 공간/소리/완료 효과를 관찰하고 Missile ctor 진입에서 접두 실행을 멈춘다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(esp+4+i*4) for i in range(13)];ret=self.read(esp);purge=0;value=0
        if address==p['missile']:
            self.events.append('J:'+','.join(map(str,args[:12])));self.prefix_stops+=1;mu.emu_stop();return
        if address==p['parent']:value=self.slot(50)
        elif address==p['center']:
            sid=(this-POOL)//self.stride;self.events.append(f'C:{sid}');point=(50.0,50.0) if sid==50 else self.point
            mu.mem_write(args[0],struct.pack('<ff',*point));value=args[0];purge=4
        elif address==p['direction']:value=self.case['direction']*2;purge=12
        elif address==p['random']:self.events.append(f'N:{args[0]}:{args[1]}');value=3000
        elif address==p['format']:
            self.events.append(f'F:{args[2]}');mu.mem_write(args[0],f'sunCannonRaiseLower{args[2]}.wav\0'.encode());value=args[0]
        elif address==p['sound']:
            name=bytes(mu.mem_read(args[2],80)).split(b'\0')[0].decode();self.events.append('S:'+name)
        elif address==UNPOP:self.events.append(f'U:50:{args[0]}');purge=4
        elif address==REPOP:self.events.append(f'P:50:{args[2]}');purge=12
        elif address==p['fromform']:value=OBJECT+0x100
        elif address==p['processType']:value=34
        elif address==NOTIFY:self.events.append('M:'+':'.join(map(str,args[:3])));purge=12
        elif address==p['kill']:self.events.append(f'K:81:{args[0]}');purge=4
        elif address==p['flags']:value=0x10 if self.case['hp'] else 0
        elif address==p['point']:
            self.events.append(f'T:{args[1]}:{args[2]}');mu.mem_write(args[0],struct.pack('<ff',52.25,52.75))
        elif address==p['new']:self.events.append(f'A:{args[0]}');value=ALLOC
        else:super().on_instruction(mu,address,size,data);return
        self.stub_calls[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_body(self,case):
        """각 독립 입력에서 원본 raw 필드·부모프레임·효과 순서를 수집한다."""
        self.case=case;self.events=[];mu=self.mu;p=self.p;patch=self.edition=='originals';kind=case['kind'];virtual=min(kind,1) if kind==3 else kind
        self.point=((50.0,40.0),(60.0,50.0),(50.0,60.0),(40.0,50.0))[case['direction']%4]
        fields=bytearray((i*13+7)&255 for i in range(0x40));struct.pack_into('<III',fields,0,p['outer'][virtual],81,50);struct.pack_into('<ffff',fields,12,60.0,60.0,59.5,59.5)
        struct.pack_into('<I',fields,32,p['inner'][virtual]);fields[40]=case['dirty'];struct.pack_into('<HH',fields,42,70,60);struct.pack_into('<dI',fields,48,10.0 if case['due'] else 10.25,case['rest']);mu.mem_write(OBJECT,bytes(fields))
        mu.mem_write(OBJECT+0x100,struct.pack('<I',VTABLE+0x200));mu.mem_write(VTABLE+0x200+0x1c,struct.pack('<I',NOTIFY))
        parent=bytearray(self.stride);struct.pack_into('<I',parent,0,VTABLE);parent[10]=128;struct.pack_into('<ff',parent,14,50.5,50.5)
        parent[34 if patch else 32]=1;parent[36 if patch else 34]=case['frame'];mu.mem_write(self.slot(50),bytes(parent));mu.mem_write(VTABLE+0x48,struct.pack('<I',UNPOP));mu.mem_write(VTABLE+0x90,struct.pack('<I',REPOP))
        target=bytearray(self.stride);target[10]=71;target[11]=case['targetstate'];struct.pack_into('<ff',target,14,60.0,60.0);mu.mem_write(self.slot(60),bytes(target))
        for name,value in (('sun',128),('display',80),('impact',11),('gun',34),('special',148)):mu.mem_write(p[name],struct.pack('<I',value))
        mu.mem_write(p['now'],struct.pack('<d',10.0));mu.mem_write(0,bytes(4));self.write_ranges=[(0,4),(OBJECT,OBJECT+0x100),(ALLOC,ALLOC+0x200),(self.slot(50),self.slot(50)+self.stride)]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<I',STOP))
        prior=self.prefix_stops;mu.emu_start(p['entries'][kind],STOP,count=40000);prefix=self.prefix_stops!=prior
        if not prefix:
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError(f'GunAnimation 반환 오류 {self.edition} {case}')
            if mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('GunAnimation x87 반환 오류')
            self.returns+=1
        return ['prefix' if prefix else 'return',';'.join(self.events),bytes(mu.mem_read(OBJECT,0x40)).hex(),self.read(self.slot(50)+(36 if patch else 34),4 if patch else 1)]


def generate(smoke):
    """정상 프레임 반환과 Missile 생성 직전 접두 관찰을 구분해 저장한다."""
    cases=inputs();cases=cases[::31] if smoke else cases;rows=[];editions={};files=[Path(__file__),FIXTURE,ROOT/'tools/ghidra/gunanimation-functions.json']
    for edition in SPEC:
        oracle=GunAnimationOracle(edition)
        # 같은 입력의 반복은 고유 관찰에 다시 더하지 않는다.
        for case in cases:
            try:output=oracle.run_body(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*output])
        editions[edition]=dict(returns=oracle.returns,missile_prefix_stops=oracle.prefix_stops,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.stub_calls),pe_thunks=oracle.thunks,assertions=oracle.assertions,instructions=oracle.instructions)
        files.extend(oracle.exports);files.append(ROOT/oracle.spec['binary'])
    FIXTURE.write_text('# 실제 두 PE GunAnimation 관찰. prefix는 Missile ctor 진입 정지이며 투사체 완료가 아니다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),host='HJOW-Athlon',editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in files},limitations=['공간/중심/피해점/소리/완료 통지는 명시 관찰 callback이다.','prefix는 실제 Missile ctor 진입 전 인자 관찰이며 그 몸체/투사체 Pop/충돌을 실행하지 않는다.','CD 짧은 가상 thunk는 PE 첫 RET 명령 범위로 별도 감사한다.'],os_calls=0),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(total=len(rows),editions={edition:dict(returns=data['returns'],prefix=data['missile_prefix_stops']) for edition,data in editions.items()}),ensure_ascii=False))


def verify():
    """현재 도구/원본/정밀 몸체/fixture와 반환·접두 행수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('GunAnimation 행 수 오류')
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('GunAnimation SHA 불일치 '+path)
    for edition,data in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if sum(row[9]=='return' for row in selected)!=data['returns'] or sum(row[9]=='prefix' for row in selected)!=data['missile_prefix_stops']:raise RuntimeError('GunAnimation 반환/접두 수 오류')
        if data['assertions']:raise RuntimeError('GunAnimation 원본 assert 도달')
    print(json.dumps(dict(verified=True,total=len(rows)),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)
