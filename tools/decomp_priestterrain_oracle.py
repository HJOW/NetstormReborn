#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 사제의 후보 지형/모양 종료 구간과 지역 조회를 세 PE에서 격리 실행한다.

중간 구간의 지역 변수는 입력으로 공급한다. finder/전체 MayPlace/게임/OS는 실행하지 않는다.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STACK,STOP,digest
from decomp_priestcollision_oracle import SPEC as COLLISION
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 합성 입력 지도/프레임 표와 판본별 실제 전역/지역 변수 주소다.
MAP,CODES=0x14600000,0x14700000
SPEC={
 'originals':dict(next=0x4b1810,nextCall=0x49bca5,end=0x49bcb9,ended=0x49bd1d,final=0x49bde0,returned=0x49bfc3,canGround=0x18,region=0x46f4e0,
    regions=0x59aa88,sentinel=0x50c038,map=0x5c84bc,noIsland=0x5412cc,
    bridge=0x28,only=0x24,ground=0x18,permission=0x10,originX=0x20,originY=0x48,allowedGround=0x64),
 'originalCD':dict(next=0x4eafe0,nextCall=0x445966,end=0x44597e,ended=0x4459d4,final=0x445a9c,returned=0x445ce7,canGround=0x12c,region=0x4bdbf0,
    regions=0x565a30,sentinel=0x505eb0,map=0x52d590,noIsland=0x51cbb8,
    bridge=0x74,only=0x50,ground=0x44,permission=0x20,originX=0x70,originY=0x6c,allowedGround=0x7c),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# fixture에는 입력과 실제 출력만 저장하며 Python 판정 함수는 두지 않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priestterrain-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestterrain-evidence.json'
KEYS=('kind','flags','other','frameFlag','cx','cy','wx','wy','ox','oy','extra','noIsland','regionValue','mapped','seed','sentinel','initialGround')


def seed_bytes(case):
    """기대값과 무관한 144바이트 입력 배열을 만든다."""
    seed=case['seed'];value=(case['sentinel']&255,255,4,127,128,128)[seed]
    data=bytearray([value]*144)
    if seed==3:data[0]=4
    if seed==5:data[13]=4
    return bytes(data)


def inputs():
    """지면 비트/다리/8칸 경계/소수 좌표/지역 WORD와 signed sentinel 입력을 만든다."""
    cases=[]
    positions=((20,21),(19.9,20.9),(18,19),(27,28),(28,29),(29,30),(-0.9,0),(255.9,255.9),(256,256),(0,0))
    # 일반 사제는 클라이언트의 해당 SID 무시를 통과한 뒤에도 후보 지형 처리를 받는다.
    for index,(flags,other,frame,pos,profile,extra) in enumerate(itertools.product((0,2,4,6),(0,2,4,6),(0,4),positions,(0,1,2),(0,1))):
        wx,ox,oy=((1,1,1),(2,3,2),(9,10,10))[profile]
        cases.append(dict(kind='C',flags=flags,other=other,frameFlag=frame,cx=bits(pos[0]),cy=bits(pos[1]),
            wx=wx,wy=wx,ox=ox,oy=oy,extra=extra,noIsland=83+index%2,
            regionValue=(0,1,126,127,128,255,0x1234,0xffff)[index%8],mapped=index%2,seed=index%6,sentinel=127,initialGround=1))
    # 모양 종료의 첫 signed BYTE/DWORD 비교와 발자국 전체 검사, 이전 false 유지다.
    for wx,sentinel,seed,ground in itertools.product((1,2,9),(127,0xffffffff,255,256),range(6),(0,1)):
        cases.append(dict(kind='E',flags=6,other=2,frameFlag=0,cx=bits(20),cy=bits(21),wx=wx,wy=wx,
            ox=1,oy=1,extra=0,noIsland=83,regionValue=1,mapped=1,seed=seed,sentinel=sentinel,initialGround=ground))
    # 좌표별 조회는 thiscall 정상 반환까지 실제 helper 전체를 실행한다.
    for pos,other,mapped,region in itertools.product(((-1,0),(0,0),(255,255),(256,0)),(2,4,6,0x1000000),(0,1),(0,127,255,0xffff)):
        cases.append(dict(kind='R',flags=6,other=other,frameFlag=0,cx=bits(pos[0]),cy=bits(pos[1]),wx=1,wy=1,
            ox=1,oy=1,extra=0,noIsland=83,regionValue=region,mapped=mapped,seed=0,sentinel=127,initialGround=1))
    # 최종 구간에서는 extra/noIsland/mapped 열을 초기 bridge/only/permission 입력으로 사용한다.
    for flags,ground,bridge,only,permission in itertools.product((0,0x400,0x406),(0,1),(0,1),(0,1),(0,1)):
        cases.append(dict(kind='F',flags=flags,other=2,frameFlag=0,cx=bits(20),cy=bits(21),wx=1,wy=1,
            ox=1,oy=1,extra=bridge,noIsland=only,regionValue=1,mapped=permission,seed=0,sentinel=127,initialGround=ground))
    return cases


class TerrainOracle(OwnerOracle):
    """내보낸 실제 명령과 정확한 배열/지역 스택 쓰기만 허용한다."""
    def __init__(self,edition):
        """새 함수 목록/지도/프레임 입력 저장소를 준비한다."""
        super().__init__(edition);self.s=SPEC[edition];self.c=COLLISION[edition]
        self.mu.mem_map(MAP,0x20000);self.mu.mem_map(CODES,0x1000)
        self.exports=[ROOT/f'extracted/priestterrain/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.boundaries=dict(C=0,E=0,R=0,F=0)
        with self.exports[1].open(encoding='utf-8') as fp:
            # 실제 함수 몸체만 허용하여 누락된 호출을 대체하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """finder Next/주변 관계 검사 직전에 관찰을 종료한다. 해당 몸체는 실행하지 않는다."""
        kind=self.case['kind'];s=self.s
        if (kind=='C' and address==s['nextCall']) or (kind=='E' and address==s['ended']) or (kind=='F' and address==s['returned']):
            self.boundaries[kind]+=1;mu.reg_write(UC_X86_REG_EIP,STOP);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """독립 입력에서 실제 배열/다리/지역 상태와 raw/스택/x87를 관찰한다."""
        self.case=case;self.instructions=0;mu=self.mu;s=self.s;c=self.c
        own=TYPES+82*self.type_stride;other=TYPES+83*self.type_stride;foot=0x1d4 if self.edition=='originals' else 0x1b4
        for address,flags,genus,wx,wy in ((own,case['flags'],0x210000,case['wx'],case['wy']),(other,0,case['other'],case['ox'],case['oy'])):
            mu.mem_write(address,bytes(self.type_stride));mu.mem_write(address+0xe8,struct.pack('<II',flags,genus));mu.mem_write(address+foot,struct.pack('<ii',wx,wy))
        mu.mem_write(other+0x124,struct.pack('<I',CODES));mu.mem_write(CODES,bytes((0,0,0,0,0,0,0,case['frameFlag'])))
        raw=bytearray([0xcd]*self.stride);raw[10]=83;raw[8:10]=struct.pack('<H',case['regionValue']);raw[self.o['extra']]=case['extra']
        raw[14:22]=struct.pack('<II',case['cx'],case['cy'])
        if self.edition=='originals':raw[36:40]=struct.pack('<I',1)
        else:raw[34]=1
        mu.mem_write(self.slot(50),bytes(raw));mu.mem_write(MAP,bytes(0x20000));mu.mem_write(s['map'],struct.pack('<I',MAP))
        x,y=(int(struct.unpack('<f',struct.pack('<I',case[key]))[0]) for key in ('cx','cy'))
        if 0<=x<256 and 0<=y<256:mu.mem_write(MAP+2*(y*256+x),struct.pack('<H',50 if case['mapped'] else 0))
        mu.mem_write(s['regions'],seed_bytes(case));mu.mem_write(s['sentinel'],struct.pack('<I',case['sentinel']))
        mu.mem_write(s['noIsland'],struct.pack('<I',case['noIsland']));mu.mem_write(c['client'],struct.pack('<I',1));mu.mem_write(c['mask'],bytes(144))
        self.write_ranges=[(s['regions'],s['regions']+144),(c['mask'],c['mask']+144)]
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        for key,value in (('bridge',0),('only',1),('ground',case['initialGround']),('permission',1),('originX',21-case['wx']),('originY',22-case['wy']),('allowedGround',case['flags']&6)):
            mu.mem_write(STACK+s[key],struct.pack('<i',value))
        if case['kind']=='F':
            for key,value in (('bridge',case['extra']),('only',case['noIsland']),('permission',case['mapped'])):
                mu.mem_write(STACK+s[key],struct.pack('<I',value))
        if case['kind']=='R':
            mu.reg_write(UC_X86_REG_ECX,0x12340000);mu.mem_write(STACK,struct.pack('<Iii',STOP,x,y));entry=s['region']
        else:
            if self.edition=='originals':mu.mem_write(STACK+c['self'],struct.pack('<I',own))
            else:mu.reg_write(UC_X86_REG_EBX,own);mu.reg_write(UC_X86_REG_EAX,POOL)
            if case['kind']=='C':
                # 패치는 후보 진입의 현재 SID를 EAX로 받는다. CD는 지역 변수에서 읽는다.
                if self.edition=='originals':mu.reg_write(UC_X86_REG_EAX,50)
                for key,value in (('mode',0),('qx',bits(20)),('qy',bits(21)),('local',0),('sid',50)):
                    mu.mem_write(STACK+c[key],struct.pack('<I',value))
                entry=c['entry']
            else:entry=s['final'] if case['kind']=='F' else s['end']
        mu.emu_start(entry,STOP,count=25000)
        adjustment=12 if case['kind']=='R' or (case['kind']=='F' and self.edition=='originals') else 0
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+adjustment:raise RuntimeError('지형 구간 반환/스택 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('지형 x87 균형 오류')
        result=mu.reg_read(UC_X86_REG_EAX) if case['kind'] in ('R','F') else 0
        if case['kind']=='R':
            if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('지역 조회 보존 레지스터 오류')
            self.boundaries['R']+=1
        state=[struct.unpack('<I',mu.mem_read(STACK+s[key],4))[0] for key in ('bridge','only','ground','permission')]
        can=0
        if case['kind']=='F':
            can=struct.unpack('<I',mu.mem_read(STACK+s['canGround'],4))[0]
            if self.edition=='originals':state[2]=mu.reg_read(UC_X86_REG_EBP)
        return [result,*state,can,bytes(mu.mem_read(s['regions'],144)).hex(),bytes(mu.mem_read(self.slot(50),self.stride)).hex()]


def generate(smoke=False):
    """두 정밀도의 실제 결과가 일치하면 fixture와 입력 SHA 근거를 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestcollision_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/priestterrain-functions.json',FIXTURE}
    for edition in SPECS:
        oracle=TerrainOracle(edition);cases=inputs()
        if smoke:cases=[case for kind in ('C','E','R','F') for case in [c for c in cases if c['kind']==kind][:32]]
        # 입력 순서대로 실제 명령이 계산한 출력만 수집한다.
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError(f'x87 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*values[0]])
        editions[edition]=dict(cases=len(cases),boundaries=oracle.boundaries,native_calls=dict(oracle.native_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 지형 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 후보 지형/모양 종료/최종 판정 구간 및 지역 조회. 전체 MayPlace/finder/게임 미실행.\n# F 입력의 extra/noIsland/mapped는 초기 bridge/only/permission이다.\n# edition '+' '.join(KEYS)+' result bridge only ground permission canGround regions raw\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['후보/모양 종료는 중간 구간 지역 변수 입력, finder Next/주변 관계 진입 직전 종료','지역 조회는 실제 thiscall 정상 반환/ABI/보존 레지스터','최종 구간은 실제 사제 우회/반환값 설정 뒤 종료, 전체 MayPlace ABI 검증 아님','일반 사제의 초기 permission=true 범위, 전체 MayPlace/패턴/일반 자산 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """PE/내보내기/입력 SHA와 실제 구간/조회 실행 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('지형 근거 오류')
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition]
        if len(selected)!=item['cases'] or item['assertions']:raise RuntimeError('지형 행/assert 오류')
        for kind in ('C','E','R','F'):
            if item['boundaries'][kind]!=2*sum(r[1]==kind for r in selected):raise RuntimeError('실제 지형 구간 누락')
        if item['native_calls'].get(f'{SPEC[edition]["next"]:08x}',0):raise RuntimeError('finder 몸체 실행 범위 오류')
        if not item['native_calls'].get(f'{SPEC[edition]["region"]:08x}',0):raise RuntimeError('실제 지역 조회 누락')
        if not item['native_calls'].get(f'{COLLISION[edition]["ftol"]:08x}',0):raise RuntimeError('실제 후보 지형 좌표 절삭 누락')
    print(f'priestterrain 검증 통과: {len(rows)}개')


def main():
    """전체 생성/표본/저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
