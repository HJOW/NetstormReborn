#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 실제 anim 생성·Regular ctor·프레임 지정/삭제·지면 충돌 사슬을 격리 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_setframe_oracle import FrameOracle,SHP,UNPOP,UPDATE88
from decomp_bridgeevent_oracle import ROOT,TYPES,VTABLE,RESULT,RETURN_STUB,RETURN_END,STACK,STOP,DESTROY,POP,float_bits,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 현재 현호스트 정밀 몸체와 실제 anim/Regular/난수·억제 전역 주소다.
SPEC={
 'originals':dict(entries=(0x45a750,0x45a6e0,0x41d0d0,0x45ae00,0x471ab0,0x470ac0),table=0x509758,
    create=0x4af530,new=0x4e4391,regular=0x496e80,attach=0x41bd20,dudes=0x451470,
    type=0x5412d8,process=0x541128,now=0x55b4d0,rng=0x532710,suppressTime=0x5c85f0,suppress=0x5c89b8,
    slots=0x56b808,capacity=40000,limit=30001,processTable=0x50f420),
 'originalCD':dict(entries=(0x47c8a0,0x47c810,0x43f950,0x47c880,0x4ed650,0x455b40),table=0x503b30,
    create=0x4ab390,new=0x4f1650,regular=0x48efd0,attach=0x48f590,dudes=0x4e8e70,
    type=0x51cbc4,process=0x51ca18,now=0x5484a8,rng=0x5308ec,suppressTime=0x564db8,suppress=0x5178d4,
    slots=0x5b7bb0,capacity=4000,limit=3001,processTable=0x5037c0),
}
# 합성 외부 heap과 좌표 수신자는 실제 원본 몸체의 쓰기만 받아 관찰한다.
OBJECT,POINT=0x11250000,0x11250100
KEYS=('kind','x','y','event','count','payload','frame','level','suppressedTime','suppressed','rng','processes')
FIXTURE=ROOT/'cpppj/tests/fixtures/gunimpact-x86.tsv'
REPORT=ROOT/'cpppj/gunimpact.json'


def wide(value):
    """double 억제 입력의 raw 비트다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def sample(**values):
    """원본 기대값을 계산하지 않고 독립 입력의 기본 필드를 만든다."""
    row=dict(zip(KEYS,(0,float_bits(20.75),float_bits(21.9),3,0,float_bits(0.02),2,1,0,0,0,0)));row.update(values);return row


def inputs():
    """생성 gate·경계 좌표·글자/끝 count·공유 난수 및 실제 Kernel count를 독립 방문한다."""
    result=[]
    # 각 구간의 존재/끝과 signed count·payload 반환·해시 단계 재등록을 교차한다.
    for event,count,payload,level in itertools.product(range(16),(0xffffffff,0,1,2,3,5,8),(0.02,0.0,-1.0),(0,1,2)):
        result.append(sample(kind=1,event=event,count=count,payload=float_bits(payload),level=level))
    # 실제 producer는 억제 gate와 한도 경계에서 좌표나 자산을 쓰지 않는다.
    for event,processes,suppress,point in itertools.product((0,3,15),range(4),range(3),((-1,400),(20.75,21.9))):
        result.append(sample(event=event,processes=processes,suppressedTime=wide(1.0) if suppress==1 else 0,suppressed=suppress==2,x=float_bits(point[0]),y=float_bits(point[1])))
    # x87 좌표 제한은 음수/하한·상한·무한 및 NaN 원본 비교 분기를 직접 관찰한다.
    for x,y in itertools.product((-float('inf'),-1,0,0.5,0.75,255,256,float('inf'),float('nan')),(0.0,255.0,float('nan'))):
        result.append(sample(kind=2,x=float_bits(x),y=float_bits(y)))
    result.append(sample(kind=3))
    for count in range(5):result.append(sample(kind=4,processes=count))
    # actual ground 몸체의 Dude 호출·난수 소비·producer 연결을 억제 여부와 교차한다.
    for rng,suppress,point in itertools.product((0,1,0xffffffff,0x12345678),range(3),((-1,400),(20.75,21.9))):
        result.append(sample(kind=5,rng=rng,suppressed=suppress,x=float_bits(point[0]),y=float_bits(point[1])))
    return result


class ImpactOracle(FrameOracle):
    """실제 frame 지정/해시 단계/타입 query 위에 효과/ctor/count/난수를 추가한다."""
    def __init__(self,edition):
        """보호 PE 원문과 현호스트 정밀 범위만 실행하고 실제 가상 Handle를 확인한다."""
        super().__init__(edition);self.p=SPEC[edition];self.entries=set();self.returns=0;self.boundaries=collections.Counter();self.pending_count=None;self.mu.mem_map(OBJECT,0x1000)
        for name in ('gunimpact','assetregular'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as stream:
                # 정밀 Ghidra의 각 불연속 함수 몸체만 허용한다.
                for row in csv.DictReader(stream,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for segment in row['ranges'].split(';'):
                        low,high=(int(item,16) for item in segment.split('-'));self.allowed.append((low,high+1))
                        if int(row['entry'],16)==self.p['entries'][4]:self.count_range=(low,high+1)
        if self.read(self.p['table']+0x5c)!=self.p['entries'][1]:raise RuntimeError('anim 실제 Handle 가상 표 오류')
        # 현재 공간/표시/삭제 효과만 경계로 관찰하며 Handle와 SetFrame 몸체는 그대로 실행한다.
        for offset,target in ((0x10,DESTROY),(0x48,UNPOP),(0x88,UPDATE88),(0x90,POP)):
            self.mu.mem_write(self.p['table']+offset,struct.pack('<I',target))

    def read(self,address,width=4):
        """현재 에뮬레이터의 비정렬 원본 필드를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def process_count(self,selector):
        """count 입력의 두 판본별 실제 한도/전체 배열 끝을 공급한다."""
        p=self.p;return (0,2,p['limit']-1,p['limit'],p['capacity']-1)[selector]

    def on_instruction(self,mu,address,size,data):
        """실제 Kernel count의 반환을 관찰하고 외부 Factory/Pop/부착/Dude 경계만 대체한다."""
        # 긴 count 순회도 실제 명령을 실행하며 해당 정밀 범위에는 외부 호출이나 쓰기가 없다.
        if self.count_range[0]<address and address+size<=self.count_range[1]:self.instructions+=1;return
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(esp+4+i*4) for i in range(4)]
        if self.pending_count==address:
            self.events.append('Q:'+str(mu.reg_read(UC_X86_REG_EAX)));self.pending_count=None
        if address==p['entries'][4] and self.case['kind']!=4:self.pending_count=ret
        if address in self.entries:self.native_calls[f'{address:08x}']+=1
        value=0;purge=0
        if address==p['create']:
            self.events.append(f'C:{args[0]}:{args[1]}');value=self.slot(50)
        elif address==p['new']:
            self.events.append(f'A:{args[0]}');value=OBJECT
        elif address==p['attach']:
            self.events.append('B:'+':'.join(map(str,args)));mu.mem_write(this+4,struct.pack('<II',71,args[1]));purge=16
        elif address==p['dudes']:
            self.events.append(f'Qd:{args[0]}:{args[1]}')
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def prepare(self,case):
        """타입 표/프레임 추가 헤더/현재 slot/프로세스 배열의 입력만 공급한다."""
        self.case=case;self.events=[];self.pending_count=None;p=self.p;mu=self.mu;typ=TYPES+160*self.type_stride
        mu.mem_write(typ,bytes(self.type_stride));mu.mem_write(typ+0x114,struct.pack('<I',80));mu.mem_write(typ+0xdc,struct.pack('<I',SHP))
        # 글자 구간은 합성 입력이며 유효 프레임과 빈 구간을 구분한다.
        for event in range(16):
            mu.mem_write(typ+0x130+event*4,struct.pack('<i',event*5 if event%4 else -1));mu.mem_write(typ+0x170+event*4,struct.pack('<i',event%4))
        blob=bytearray(0x3000)
        # 실제 프레임 크기 조회는 각 현재 frame의 동일1×1 추가 헤더를 읽는다.
        for frame in range(80):
            offset=0x400+frame*0x40;struct.pack_into('<I',blob,8+frame*8,offset);struct.pack_into('<ff',blob,offset-36,1.0,1.0)
        mu.mem_write(SHP,bytes(blob));raw=bytearray((i*7+3)&255 for i in range(self.stride));struct.pack_into('<I',raw,0,p['table']);raw[10]=160;raw[11]=0;struct.pack_into('<ff',raw,14,20.75,21.9);raw[self.level_offset]=case['level'];raw[40 if self.spec['patch'] else 35]=0;struct.pack_into('<I' if self.spec['patch'] else '<B',raw,self.o['frame'],case['frame']);mu.mem_write(self.slot(50),bytes(raw))
        mu.mem_write(OBJECT,bytes((i*13+7)&255 for i in range(40)));mu.mem_write(POINT,struct.pack('<II',case['x'],case['y']));mu.mem_write(RESULT,bytes(4));mu.mem_write(0,bytes(4))
        mu.mem_write(p['type'],struct.pack('<I',160));mu.mem_write(p['process'],struct.pack('<I',46));mu.mem_write(p['now'],struct.pack('<d',10.0));mu.mem_write(p['rng'],struct.pack('<I',case['rng']));mu.mem_write(p['suppressTime'],struct.pack('<Q',case['suppressedTime']));mu.mem_write(p['suppress'],struct.pack('<I',case['suppressed']))
        count=self.process_count(case['processes']);mu.mem_write(p['slots'],bytes(4)+struct.pack('<I',0x12340000)*count+bytes((p['capacity']-count-1)*4))
        self.write_ranges=[(self.slot(50),self.slot(50)+self.stride),(OBJECT,OBJECT+40),(POINT,POINT+8),(RESULT,RESULT+4),(p['rng'],p['rng']+4)]

    def run_body(self,case):
        """실제 RET/스택/x87을 확인하고 의미 있는 프로세스 상태·raw·순서를 관찰한다."""
        self.prepare(case);kind=case['kind'];p=self.p;mu=self.mu;args=[];purge=0;this=self.slot(50);until=STOP;ret=STOP
        if kind in (0,5):args=[case['x'],case['y']]+([case['event'],case['payload']] if kind==0 else [])
        elif kind==1:args=[case['event'],case['count'],case['payload']];purge=12;ret=RETURN_STUB;until=RETURN_END
        elif kind==2:this=POINT
        elif kind==3:args=[50]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,this);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),ret,*args));mu.emu_start(p['entries'][kind],until,count=1000000)
        if mu.reg_read(UC_X86_REG_EIP)!=until or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+purge or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Impact 정상 반환/스택/x87 오류')
        self.returns+=1;result=str(self.read(RESULT)) if kind==1 else str(mu.reg_read(UC_X86_REG_EAX)) if kind==4 else '-';process='-'
        if any(event.startswith('B:') for event in self.events):process=','.join(map(str,(self.read(OBJECT+4),self.read(OBJECT+8),self.read(OBJECT+16,8),self.read(OBJECT+24),self.read(OBJECT+28),self.read(OBJECT+32))))
        return [result,';'.join(self.events),bytes(mu.mem_read(self.slot(50),self.stride)).hex(),process,str(self.read(p['rng'])),bytes(mu.mem_read(POINT,8)).hex()]


def generate(smoke):
    """두 실제 PE 관찰만 기대값으로 저장하며 외부 효과 경계를 별도로 기록한다."""
    cases=inputs();cases=cases[::151] if smoke else cases;rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_setframe_oracle.py',ROOT/'tools/decomp_bridgeevent_oracle.py',ROOT/'tools/decomp_neighbor_oracle.py',ROOT/'tools/ghidra/gunimpact-functions.json',FIXTURE}
    for edition in SPEC:
        oracle=ImpactOracle(edition)
        for case in cases:
            try:output=oracle.run_body(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*output])
        editions[edition]=dict(returns=oracle.returns,assertions=oracle.assertions,instructions=oracle.instructions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),virtual_boundaries=dict(oracle.stub_calls));files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,oracle.returns,flush=True)
    if smoke:return
    for name in ('RawGunImpact.cpp','RawGunImpact.h','SquidFrame.cpp','SquidFrame.h','SquidFactory.cpp','SquidFactory.h','DerivedConstructors.inc','TypeConstructors.inc','SquidProcess.cpp','SquidProcess.h','Kernel.cpp','Kernel.h','SpStore.cpp','SpStore.h','RiftType.cpp','RiftType.h'):files.add(ROOT/'cpppj/src/o'/name)
    for edition in SPEC:files.add(ROOT/edition/'netstorm.tarc')
    files.add(ROOT/'cpppj/tests/GunImpactTests.cpp');FIXTURE.write_text('# 두 실제 PE anim/producer/Regular ctor/Kernel count/ground 관찰. 공간/생성/Dude/삭제는 명시 경계다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},os_calls=0,limitations=['Factory/Pop/heap/부착/Dude/Destroy는 명시 관찰 경계다.','실제 frame 지정/해시 단계/타입 query·Kernel count·Regular ctor·공유 난수 몸체를 실행한다.','현재 프레임 구간/SHP는 합성 입력이며 실제 anim.type 월드 조립 완료로 세지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """현재 SHA/고유 입력/정상 반환과 실제 하위 몸체 실행을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('Impact 관찰 행수 오류')
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('Impact SHA 불일치 '+name)
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({tuple(row[1:13]) for row in selected})!=len(selected) or len(selected)!=item['returns'] or item['assertions']:raise RuntimeError('Impact 고유입력/반환/assert 오류')
    print(json.dumps(dict(verified=True,total=len(rows)),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
