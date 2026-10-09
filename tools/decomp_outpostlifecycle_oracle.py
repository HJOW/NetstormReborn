#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""outpost 목록 접두를 실제 wrapper/Array 명령으로 대조한다. 지역 통지와 부모 몸체만 대체한다."""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle,ROOT,SPECS,POOL,LISTS,STACK,STOP,TARGET,digest,
    UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,
    UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS)
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 실제 두 목록의 Array 전역과 파생 진입/외부 경계다.
SPEC={
 'originals':dict(post=0x4557c0,pre=0x455840,additional=0x55a70c,nearest=0x59a9d8,region=0x455380,basePost=0x4538f0,basePre=0x44b4b0),
 'originalCD':dict(post=0x47b680,pre=0x47b790,additional=0x5670e0,nearest=0x565ae0,region=0x47b3c0,basePost=0x47a4b0,basePre=0x4615e0),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 작은 목록의 물리 용량·비활성 흔적과 독립 저장 파일이다.
CAPACITY=8
FILL=0xabababab
FIXTURE=ROOT/'cpppj/tests/fixtures/outpostlifecycle-x86.tsv'
REPORT=ROOT/'cpppj/recovery-outpostlifecycle-evidence.json'
KEYS=('mode','extra','flags','scene','mutate','x','y')


def layout(scene,second=False):
    """기대 추가/삭제를 계산하지 않고 빈·중복·가득 찬 두 목록 입력만 공급한다."""
    values=([],[60],[50],[60,50,61,50],[60,61,62,63,64,65,66,67],[50,60,50,61,50,62,50,63],[50]*7,[61,60,50])
    selected=values[(scene+3)%len(values) if second else scene]
    return len(selected),selected+[FILL]*(CAPACITY-len(selected))


def inputs():
    """첫 Pop·삭제·extra 비트·독립 용량·외부 변경을 교차하며 결과는 만들지 않는다."""
    rows=[]
    # 주소 표/원본 좌표 비트와 flags를 서로 독립적으로 순회한다.
    for mode,extra,flags,scene,mutate in itertools.product(('P','D'),(0,1,2,8,9,128),(0,1,5,0xffffffff),range(8),(0,1)):
        rows.append(dict(mode=mode,extra=extra,flags=flags,scene=scene,mutate=mutate,x=0x41a20000 if scene%2==0 else 0x80000000,y=0x41ac0000 if scene%2==0 else 0x7fc12345))
    return rows


class OutpostOracle(OwnerOracle):
    """목록 변경/ABI는 원본 명령이며 세 외부 몸체의 인자/현재 목록만 관찰한다."""
    def __init__(self,edition):
        """이번 PC의 wrapper와 Array helper의 실제 범위만 허용한다."""
        super().__init__(edition);self.p=SPEC[edition];self.substitutions={'R':0,'B':0};self.returns=0
        self.exports=[ROOT/f'extracted/outpostlifecycle/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 각 함수의 불연속 몸체를 그 범위대로 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 범위의 끝 주소는 포함한다.
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """지역/부모만 대체하고 그 호출 전에 두 목록의 현재 상태를 기록한다."""
        p=self.p;c=self.case
        if address in (p['region'],p['basePost'],p['basePre']):
            sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<I',mu.mem_read(p['additional']+8,4))[0];b=struct.unpack('<I',mu.mem_read(p['nearest']+8,4))[0]
            if address==p['region']:
                x,y=struct.unpack('<2I',mu.mem_read(sp+4,8))
                if (x,y)!=(c['x'],c['y']) or (self.edition=='originals' and struct.unpack('<I',mu.mem_read(sp+12,4))[0]):raise RuntimeError('지역 통지 인자 오류')
                self.events.append(f'R:{x}:{y}:{a}:{b}');self.substitutions['R']+=1;purge=0
                if c['mutate']:mu.mem_write(self.slot(TARGET)+self.o['extra'],b'\x09');mu.mem_write(self.slot(TARGET)+14,struct.pack('<I',0x42c80000))
            else:
                expected=p['basePost'] if c['mode']=='P' else p['basePre'];flags=struct.unpack('<I',mu.mem_read(sp+4,4))[0]
                if address!=expected or flags!=c['flags'] or mu.reg_read(UC_X86_REG_ECX)!=self.slot(TARGET):raise RuntimeError('outpost 부모 인자 오류')
                self.events.append(f'B:{flags}:{a}:{b}');self.substitutions['B']+=1;purge=4
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',mu.mem_read(sp,4))[0]);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """실제 wrapper 정상 반환·보존 레지스터/x87와 목록 전체/비활성 꼬리를 관찰한다."""
        self.case=case;self.events=[];self.instructions=0;mu=self.mu;p=self.p;raw=bytearray([0xab]*self.stride);raw[10]=126;raw[11]=2;raw[self.o['extra']]=case['extra']
        struct.pack_into('<II',raw,14,case['x'],case['y']);mu.mem_write(self.slot(TARGET),bytes(raw))
        # 두 전역은 별도 저장소이며 각각 현재 count/용량을 가진다.
        for second,key in enumerate(('additional','nearest')):
            count,values=layout(case['scene'],bool(second));pointer=LISTS+second*0x100
            mu.mem_write(p[key],struct.pack('<3I',pointer,CAPACITY,count));mu.mem_write(pointer,struct.pack('<8I',*values))
        self.write_ranges=[(self.slot(TARGET),self.slot(TARGET)+self.stride),(LISTS,LISTS+0x120),(p['additional']+8,p['additional']+12),(p['nearest']+8,p['nearest']+12)]
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 진입마다 서로 다른 보존 값과 두 x87 제어 워드를 공급한다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_ECX,self.slot(TARGET));mu.reg_write(UC_X86_REG_ESP,STACK)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.mem_write(STACK,struct.pack('<2I',STOP,case['flags']))
        mu.emu_start(p['post'] if case['mode']=='P' else p['pre'],STOP,count=20000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+8:raise RuntimeError('outpost 정상 반환/ESP 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()) or mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('outpost 보존 레지스터/x87 오류')
        self.returns+=1;result=[]
        # count와 비활성 꼬리를 포함한 저장소 전체를 원본에서 직접 읽는다.
        for second,key in enumerate(('additional','nearest')):
            result.extend((struct.unpack('<I',mu.mem_read(p[key]+8,4))[0],','.join(map(str,struct.unpack('<8I',mu.mem_read(LISTS+second*0x100,32))))))
        return [*result,';'.join(self.events),bytes(mu.mem_read(self.slot(TARGET),self.stride)).hex()]


def generate(smoke=False):
    """독립 PE 관찰과 SHA/진입/대체 내역만 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/outpostlifecycle-functions.json',FIXTURE}
    # 세 PE의 같은 입력을 두 정밀도로 정상 반환까지 실행한다.
    for edition in SPECS:
        oracle=OutpostOracle(edition);cases=inputs()[::61] if smoke else inputs()
        for case in cases:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('outpost 두 정밀도 불일치')
            rows.append([edition,*[case[k] for k in KEYS],*observed[0]])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native_calls=dict(oracle.native_calls),substitutions=oracle.substitutions,assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: outpost {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 outpost 목록 접두/정상 반환. 지역 통지·작업장 post/Damageable pre 몸체만 대체.\n# edition '+' '.join(KEYS)+' additionalCount additional nearestCount nearest events raw\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['outpost 두 목록 등록/삭제 wrapper와 Array helper 정상 반환/ABI/x87','지역 소유 투표·작업장 postPop·Damageable preDestroy 몸체는 명시 대체','합성 목록/raw 입력; 일반 Pop/실제 맵/전체 파생 삭제/GUI/게임 미검증']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·전체 입력 순서·행/진입·외부 사건 수와 두 정밀도 반환을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if len(rows)!=3*len(inputs()) or report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('outpost 근거 오류')
    # 사건은 저장된 원본 출력에서 세고 입력별 기대 목록을 다시 구현하지 않는다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];n=len(selected);p=SPEC[edition]
        if n!=len(inputs()) or item['returns']!=2*n or item['assertions']:raise RuntimeError('outpost 반환/행 오류')
        for key,mode in (('post','P'),('pre','D')):
            if item['native_calls'].get(f'{p[key]:08x}')!=2*sum(r[1]==mode for r in selected):raise RuntimeError('실제 wrapper 진입 오류')
        if item['substitutions']['B']!=2*n or item['substitutions']['R']!=2*sum(r[12].startswith('R:') for r in selected):raise RuntimeError('외부 경계 사건 오류')
        if edition=='originals':
            # 실제 Array helper는 원본 통지 사건에서 관찰한 두 목록 처리 횟수와 같다.
            for address,mode in ((0x414450,'P'),(0x40ea00,'D')):
                if item['native_calls'].get(f'{address:08x}')!=4*sum(r[1]==mode and r[12].startswith('R:') for r in selected):raise RuntimeError('실제 Array helper 진입 오류')
        if any(item['native_calls'].get(f'{p[key]:08x}',0) for key in ('region','basePost','basePre')):raise RuntimeError('외부 몸체 실행/대체 기록 혼합')
        for row,case in zip(selected,inputs(),strict=True):
            if len(row)!=14 or row[1:8]!=[str(case[k]) for k in KEYS]:raise RuntimeError('outpost 입력/열 오류')
            if any(len(row[index].split(','))!=CAPACITY for index in (9,11)) or any(int(row[index])>CAPACITY for index in (8,10)):raise RuntimeError('물리 목록 크기 오류')
    print(f'outpostlifecycle 검증 통과: {len(rows)}개')


if __name__=='__main__':
    # 게임 실행 없이 정상 생성/무저장 표본/저장 근거 감사를 선택한다.
    parser=argparse.ArgumentParser();parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
