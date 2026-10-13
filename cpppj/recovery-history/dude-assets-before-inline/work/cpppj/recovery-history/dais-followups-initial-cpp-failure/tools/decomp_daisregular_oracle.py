#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dais 초기 하위 몸체를 두 보호 PE의 제한 x86에서 관찰한다. 전체 희생/실제 게임 검사는 후속이다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_templeritual_oracle import TempleOracle,SPEC as TEMPLE,SHP,ROOT,POOL,HEAP,VTABLE,bits,sha
from decomp_runepostpop_oracle import RuneOracle
from decomp_bridgedecay_oracle import TYPES,CODES,STACK,STOP,FPU_CONTROLS,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 새 실제 entry와 외부 요청의 ABI만 선언한다. 판단/기대 결과는 원본이 계산한다.
SPEC={
 'originals':dict(stop=0x491b60,collapse=0x448080,start=0x4483c0,advance=0x44a200,handle305=0x44b150,can=0x449710,find=0x4afb40,kill=0x41bf10,sacrifice=0x5412fc,altar=0x5412e8,contained=0x541088,alliances=0x595200),
 'originalCD':dict(stop=0x40c840,collapse=0x482370,start=0x482cf0,advance=0x482df0,handle305=0x482c40,can=0x481910,find=0x4ac890,kill=0x48f7b0,sacrifice=0x51cbe8,altar=0x51cbd4,contained=0x51c978,alliances=0x50f6e0),
}
CALLER=HEAP+0xa000 # 실제 float10 반환을 caller의 원본 fstp float 저장 경계로 읽는다.
TEMP=HEAP+0xa100 # 위 caller가 쓰는 단정도 반환 비트 저장소다.
PROCESS=HEAP+0xd000 # 명시 FindEvent 경계가 반환하는 opaque 프로세스다.
FIXTURE=ROOT/'cpppj/tests/fixtures/daisregular-core-x86.tsv' # 첫 하위 몸체 관찰을 전체 희생 결과와 구분한다.
REPORT=ROOT/'cpppj/recovery-daisregular-core-evidence.json' # 현재 단계의 범위/반환/출처만 기록한다.


def inputs():
    """초기 하위 몸체의 frame/count/방향/예약/포획/동맹 입력만 구성한다."""
    result=[]
    # 여섯 모드는 scope가 다르므로 아직 수행하지 않은 Progress/Finish/Bridge를 합산하지 않는다.
    def add(mode,frame=3,side='A',priest=0,event_exists=0,reserve=1,altar=1,captured=1,genus=0x210000,owner=1,alliance=0):
        """실제 입력 tuple을 중복 없는 순서로 저장한다."""
        result.append((mode,frame,side,priest,event_exists,reserve,altar,captured,genus,owner,alliance))
    # K..O/P와604 존재/부재를 독립 검사한다.
    for frame,event_exists in itertools.product(range(9),(0,1)):add('stop',priest=frame,event_exists=event_exists)
    # 정상 stage0..4만 collapse 목표점에 전달한다. 단계5의 원본 미초기화 출력은 여기서 관찰하지 않는다.
    for frame,side,reserve in itertools.product((3,14,25,36,47),('A','N','M','L'),(0,1)):add('collapse',frame,side,reserve=reserve)
    # 실제 count1/2/3의 시작 프레임과 altar 존재/예약실패를 분리한다.
    for frame,side,altar,reserve in itertools.product((3,64,120,164),('A','N','M','L'),(0,1),(0,1)):add('start',frame,side,altar=altar,reserve=reserve)
    # 첫 단계는 wrap 없이 실제169 구간의 진행을 읽는다. wrap→Finish는 후속 전체 범위다.
    for frame in (0,1,3,43,45,46,48,178):add('advance',frame)
    # explicit 다음frame도 현재 count로 판정하는305 경계를 검사한다.
    for frame,side in itertools.product((0,2,3,13,14,24,25,35,36,46,47,57,58,63,64,118,119,120,164),('A','N','M','L')):add('handle305',frame,side)
    # 현재 stage/altar방향/포획/사제genus·owner/패치동맹 조건이다.
    for frame,altar_frame,captured,genus,owner,alliance in itertools.product((3,47),range(4),(0,1),(0,0x210000),(1,2),(0,1)):
        add('can',frame,priest=altar_frame,captured=captured,genus=genus,owner=owner,alliance=alliance)
    return list(dict.fromkeys(result))


class DaisOracle(RuneOracle):
    """현재 Temple의 충돌 없는 지도/실제 helper와 Dais18/22 몸체만 실행 허용한다."""
    def __init__(self,edition):
        """현재 호스트 읽기 전용 범위와 명시 ABI 경계를 연결한다."""
        super().__init__(edition);self.d=SPEC[edition];self.calls=0;self.manifest=ROOT/'tools/ghidra/daisregular-functions.json';table=ROOT/f'extracted/daisregular/{edition}/functions.tsv';self.paths.extend((self.manifest,table,table.with_name('creation.c')))
        selected={int(entry,16) for entry in json.loads(self.manifest.read_text(encoding='utf-8'))['editions'][edition]}
        with table.open(encoding='utf-8') as stream:
            # 새 함수의 불연속 범위 외에는 PE 코드를 허용하지 않는다.
            for row in csv.DictReader(stream,delimiter='\t'):
                if int(row['entry'],16) not in selected:continue
                self.entries[int(row['entry'],16)]=row['entry']
                for span in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in span.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        self.spec=dict(self.spec,entries=dict(self.spec['entries'],**{name:self.p[name] for name in ('stage','count','anchor','point','rune','strong','abstract')},**{name:self.d[name] for name in ('stop','collapse','start','advance','handle305','can')}))
        self.boundaries={self.p['frame']:'frame',self.p['new']:'new',self.p['shared']:'shared',self.p['sound']:'sound',self.d['find']:'find',self.d['kill']:'kill'}
        self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',TEMP)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)))

    def on_write(self,mu,access,address,size,value,data):
        """Dais 실제extra2 쓰기와 반환 저장 외에는 기존 readonly/스택 제한을 유지한다."""
        extra=self.slot(5)+(40 if self.patch else 35)
        if address==extra and size==1:return
        return super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """actual body/조회는 실행하고 프레임/new/Shared/음향/Host 요청만 기록한다."""
        if address in (CALLER,CALLER+6):return
        boundary=self.boundaries.get(address)
        if not boundary:return super().on_instruction(mu,address,size,data)
        if address in getattr(self,'entries',{}):self.hits[self.entries[address]]+=1
        self.stub_calls[boundary]+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(sp+4*i) for i in range(1,5)];purge=0;result=0
        if boundary=='frame':
            sid=self.sid_of(this);purge=8;assert args[1]==0;self.events.append(f'F:{sid},{args[0]},0');self.put(this+self.o['frame'],args[0],self.o['frame_size'])
        elif boundary=='new':assert args[0]==40;self.events.append('N:40');result=HEAP+0x6000 if self.case[5] else 0
        elif boundary=='shared':assert this==HEAP+0x6000;purge=12;result=this;self.events.append('S:'+','.join(map(str,args[:3])))
        elif boundary=='sound':
            name=bytes(mu.mem_read(args[2],64)).split(b'\0')[0].decode('ascii');self.events.append('A:'+str(args[0])+','+str(args[1])+','+name+','+str(args[3]))
        elif boundary=='find':assert this==self.slot(6) and args[0]==604;purge=4;result=PROCESS if self.case[4] else 0
        elif boundary=='kill':assert this==PROCESS and args[0]==0;purge=4;self.events.append('K:604,6,0');self.put(self.slot(6)+6,0,2)
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def table(self,typ,pattern):
        """코드/첫번호/구간길이는 합성 자산 입력으로만 준비한다."""
        base=TYPES+typ*self.type_stride;codes=CODES+(typ-158)*0x1000;self.put(base+0x124,codes);self.put(base+0x114,len(pattern));self.mu.mem_write(codes,b''.join(bytes((ord(letter),80,index+1,0)) for index,letter in enumerate(pattern)))
        # 기존RiftTypeFrames와같은각방향의입력표이며출력은이표를읽는원본이계산한다.
        for index,letter in enumerate('ABCDEFGHIJKLMNOP'):
            self.put(base+0x130+index*4,pattern.find(letter));self.put(base+0x170+index*4,pattern.count(letter))

    def run(self,case,control):
        """현재 raw/자산/포획/동맹을 준비하고 실제 정상 반환/전체 raw/효과를 읽는다."""
        mode,frame,side,priest,event_exists,reserve,altar,captured,genus,owner,alliance=case
        # 기존 helper로 자산 입력만 준비하며 first/count/좌표 출력은 새 관찰에서 직접 읽는다.
        super().prepare((0,3,'A',0,1,1,0,2,0),control);self.case=case
        objects=[(5,165,0,0,bits(40.75),bits(41.25),3,3,0,1),(6,158,0,0,bits(40.75),bits(41.25),1,1,0,1)]
        if altar:objects.append((7,164,0,0,bits(40.75),bits(41.25),1,1,0,1))
        self.setup(objects);self.table(165,side*180);self.table(158,'AEKLMNOP'+'P'*24);self.table(164,'AEMLP'+'P'*11);self.table(169,'A'*3+'B'*42+'C'*3+'D'*132)
        self.put(TYPES+158*self.type_stride+0xec,genus);self.put(self.slot(5)+self.o['frame'],frame,self.o['frame_size']);self.put(self.slot(6)+self.o['frame'],priest if mode=='stop' else 0,self.o['frame_size']);self.put(self.slot(7)+self.o['frame'],priest if mode=='can' else 1,self.o['frame_size'])
        self.put(self.slot(5)+(34 if self.patch else 32),1,1);self.put(self.slot(6)+(34 if self.patch else 32),owner,1);self.put(self.slot(7)+(34 if self.patch else 32),1,1);self.put(self.slot(6)+26,100,4 if self.patch else 2)
        self.put(self.slot(5)+6,10 if captured else 0,2);self.put(self.slot(10),VTABLE);self.put(self.slot(10)+10,6,1);self.put(self.slot(10)+12,2,2);self.put(self.slot(10)+18,158,2);self.put(self.slot(10)+20,32,2)
        if mode=='stop' and event_exists:self.put(self.slot(6)+6,12,2)
        self.put(self.d['sacrifice'],169);self.put(self.d['altar'],164);self.put(self.d['contained'],6);self.mu.mem_write(self.d['alliances'],bytes(81*4));self.put(self.d['alliances']+(2+9)*4,alliance)
        floating=mode in ('advance','handle305');args=[0x305,1,bits(9.25)] if mode=='handle305' else [6] if mode=='can' else [];purge=len(args)*4
        if not floating:returned=self.call(mode,args,self.slot(6 if mode=='stop' else 5),purge,control)
        else:
            # float10을실제caller저장으로단정도화하며스택/FS/FPCW/TOP정상복구를확인한다.
            for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
            self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(5));self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.mem_write(0,bytes(4));self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),CALLER,*args));self.events=[];self.write_ranges=[(HEAP,HEAP+0x10000)];self.mu.emu_start(self.spec['entries'][mode],STOP,count=4000000)
            assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge and self.mu.reg_read(UC_X86_REG_FPCW)==control and not(self.mu.reg_read(UC_X86_REG_FPSW)&0x3800) and self.read(0)==0;returned=self.read(TEMP)
        self.calls+=1
        return [str(returned) if mode in ('advance','handle305','can') else '-', ';'.join(self.events) or '-', ';'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in (5,6,7,10))]


def generate(smoke=False):
    """초기 하위범위만 독립 관찰하며 미수행 전체 Dais를 완료로 세지 않는다."""
    cases=inputs();cases=cases[::17] if smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE}
    # 동일고유입력을두PE와두정밀도에서실행하고반복반환은합산하지않는다.
    for edition in SPEC:
        oracle=DaisOracle(edition)
        for index,case in enumerate(cases):
            try:result=oracle.run(case,FPU_CONTROLS[0]);assert oracle.run(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,','.join(map(str,case)),*result])
        editions[edition]=dict(cases=len(cases),returns=oracle.calls,entries=dict(oracle.hits),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions);files.update(oracle.paths);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    FIXTURE.write_text('# edition mode,frame,side,priestFrame,eventExists,reserve,altar,captured,genus,owner,alliance returnBits effects slots5,6,7,10\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');files.update(ROOT/name for name in ('cpppj/src/o/RawDaisRegular.h','cpppj/src/o/RawDaisRegular.cpp','cpppj/recovery-dais-shared-lineage-evidence.json','cpppj/recovery-dais-contained-lineage-evidence.json','tools/decomp_templeritual_oracle.py','tools/decomp_runepostpop_oracle.py','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',decompile_date='2026-10-14',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},smoke=smoke,game_runs=0,scope='초기 Stop604/Collapse/StartSacrifice/169의비wrapAdvance/305/CanBridge만. 전체Progress/Finish/Bridge/Rune304/Altar24/실제소비자/C++대조는미수행.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Dais 초기 하위 native 입력:',len(rows))


def verify():
    """저장된 초기 범위의 현재 출처/행수/정상반환만 감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==data['total'] and data['game_runs']==0
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    for item in data['editions'].values():assert item['returns']==2*item['cases'] and not item['asserts']
    print('Dais 초기 하위 현재SHA/반환 감사:',len(rows))


# 명시호출에서만초기관찰을만들거나현재출처를감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
