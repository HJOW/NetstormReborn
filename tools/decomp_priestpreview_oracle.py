#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 실제 PE의 사제 배치 접두와 로컬 미리보기/표면/관계를 실행한다.

모양 조회·CanonDecoder 생성/범위 조회는 함수 진입에서 대체한다.
미리보기 뒤 충돌/지역 구간만 명시 대체하며 게임/OS는 실행하지 않는다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,STACK,STOP,digest
from decomp_priestplacement_oracle import PlacementOracle,PLACEMENT,KEYS as PREFIX_KEYS
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 분석 전용 지도 주소, 미리보기 크기, 표면 raw SID/타입/소유자 입력이다.
SPOTS,MAP=0x14000000,0x14100000
SIDE,CELLS,MASK=256,65536,144
SURFACES=((50,83,2,1),(51,84,2,2),(52,85,4,0),(53,86,2,8))
# 실제 함수·미리보기·관계 전역 주소다. CD의 표면/관계 조회는 호출자에 인라인되어 있다.
PREVIEW={
 'originals':dict(begin=0x425c20,bounds=0x425b90,later=0x49b825,mask=0x59a9f0,spots=0x5c7c44,map=0x5c84bc,
    editor=0x5c85a4,allies=0x540cb0,relations=0x595200,genus=0x4ac200,surface=0x40da00,related=0x4629e0),
 'originalCD':dict(begin=0x41fcf0,bounds=0x4202e0,later=0x44550e,mask=0x565998,spots=0x52fe48,map=0x52d590,
    editor=0x518904,allies=0x50f824,relations=0x50f6e0,genus=0x4abae0),
}
PREVIEW['original1037']=dict(PREVIEW['originalCD'])
# 기존 접두/나선 생성 근거를 바꾸지 않는 새 파일과 입력 열 순서다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priestpreview-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestpreview-evidence.json'
EXTRA_KEYS=('bx','by','br','bb','mapVariant','editor','allies','forward','mutation')
KEYS=PREFIX_KEYS+EXTRA_KEYS


def maps(variant):
    """기대 판단 없이 지도 입력만 만든다. CD의 x=256 읽기도 실제 linear 입력 그대로다."""
    spots=bytearray(CELLS);surface=[]
    # 가로/세로가 다른 주기로 변하는 입력으로 배열 전치와 순회 경계를 구별한다.
    for index in range(CELLS):
        x,y=index%SIDE,index//SIDE;choice=(x+2*y)%5
        if variant==0:sid,spot=0,0
        elif variant==1:sid,spot=50,6
        elif variant==2:sid,spot=(0,50,51,52,53)[choice],6
        elif variant==3:sid,spot=(0xffff if choice!=3 else 52),(16 if choice!=3 else 6)
        else:sid,spot=(0,50,51,52,53)[choice],(16 if (3*x+y)%7==0 else 6)
        spots[index]=spot;surface.append(sid)
    return bytes(spots),struct.pack('<65536H',*surface)


def inputs():
    """클램프·빈 범위·12×12 최대 범위·판본 가장자리와 로컬/관계/변화를 교차한다."""
    rectangles=((20,21,20,21),(18,19,20,21),(1,1,1,1),(-2,-3,1,1),(3,4,1,1),(20,21,17,21),
        (1,1,10,10),(20,252,20,252),(20,253,20,253),(20,254,20,254),(255,252,255,252),(254,252,254,252))
    contexts=((1,1,0,0,0,0,1),(1,1,0,0,1,1,0),(2,2,0,0,1,1,1),(1,2,0,0,0,0,1),
        (1,1,0,1,0,0,0),(0,0,0,0,1,0,1),(1,1,1,0,0,0,1),(0x101,1,0,0,1,1,1),(255,-1,0,0,1,1,1))
    cases=[]
    # 반환은 외부 충돌 구간의 입력일 뿐 미리보기 기대값은 만들지 않는다.
    for index,(rect,variant,context,mutation) in enumerate(itertools.product(rectangles,range(5),contexts,range(3))):
        owner,local,force,editor,allies,forward,lower=context
        c=dict(type=158,x=bits(20.5),y=bits(21.5),owner=owner,flags=(0,7)[index%2],mode=index%2,
            genus=0x210000,force=force,local=local,left=bits(0),top=bits(0),right=bits(16),bottom=bits(11),lower=lower,change=0,
            bx=rect[0],by=rect[1],br=rect[2],bb=rect[3],mapVariant=variant,editor=editor,allies=allies,forward=forward,mutation=mutation)
        cases.append(c)
    return cases


class PreviewOracle(PlacementOracle):
    """앞부분은 기존 실제 접두를 재사용하고 미리보기 본문은 대체하지 않는다."""
    def __init__(self,edition):
        """새 허용 함수/지도와 미리보기 주소를 준비한다."""
        super().__init__(edition);self.v=PREVIEW[edition];self.map_inputs=[maps(i) for i in range(5)]
        self.mu.mem_map(SPOTS,0x10000);self.mu.mem_map(MAP,0x20000)
        self.mu.mem_write(self.v['spots'],struct.pack('<I',SPOTS));self.mu.mem_write(self.v['map'],struct.pack('<I',MAP))
        self.exports=[ROOT/f'extracted/priestpreview/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 실제 함수의 불연속 범위만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 각 몸체 범위를 그대로 재현한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_write(self,mu,access,address,size,value,data):
        """144바이트 미리보기의 실제 쓰기만 추가로 허용한다."""
        if self.v['mask']<=address and address+size<=self.v['mask']+MASK:return
        super().on_write(mu,access,address,size,value,data)

    def ret(self,purge=0):
        """모양 객체의 함수 진입 반환만 대체한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);target=struct.unpack('<I',self.mu.mem_read(esp,4))[0]
        self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def mutate(self):
        """범위 조회 경계에서 원본에 공급하는 현재 지도/관계/genus 변화다."""
        c=self.case;v=self.v
        if c['mutation']==1:
            index=(c['bb']+1)*SIDE+c['br']+1
            if 0<=index<CELLS:self.mu.mem_write(SPOTS+index,bytes((16,)))
        if c['mutation']==2:
            self.mu.mem_write(v['editor'],bytes(4));self.mu.mem_write(v['allies'],struct.pack('<I',1));self.mu.mem_write(v['relations']+19*4,bytes(4))
            self.mu.mem_write(TYPES+83*self.type_stride+0xec,struct.pack('<I',4));self.mu.mem_write(self.slot(50)+self.o['owner'],bytes((8,)))

    def on_instruction(self,mu,address,size,data):
        """접두와 미리보기는 실제 실행하고 모양 진입/이후 충돌 구간만 대체한다."""
        c=self.case;p=self.p;v=self.v;esp=mu.reg_read(UC_X86_REG_ESP)
        if address==p['region']:
            self.local_owner=mu.reg_read(UC_X86_REG_EDI) if self.edition=='originals' else struct.unpack('<I',mu.mem_read(esp+0x5c,4))[0]
            OwnerOracle.on_instruction(self,mu,address,size,data);return
        if address==v['begin']:
            args=struct.unpack('<6I',mu.mem_read(esp+4,24))
            if args!=(c['type'],c['type'],c['flags'],c['x'],c['y'],0):raise RuntimeError('미리보기 CanonDecoder 인자 오류')
            if any(mu.mem_read(v['mask'],MASK)):raise RuntimeError('미리보기 생성 전 전체 초기화 누락')
            self.decoder=mu.reg_read(UC_X86_REG_ECX);self.events.append(f'B:{c["type"]}:{c["x"]}:{c["y"]}:{c["flags"]}');self.stub_calls['decoder']+=1;self.ret(24);return
        if address==v['bounds']:
            if mu.reg_read(UC_X86_REG_ECX)!=self.decoder:raise RuntimeError('미리보기 범위 조회 this 오류')
            out=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
            if not 0x30000000<=out<=STACK-16:raise RuntimeError('미리보기 범위 출력 포인터 오류')
            mu.mem_write(out,struct.pack('<4i',*[c[key] for key in ('bx','by','br','bb')]))
            self.events.append('R:'+':'.join(str(c[key]) for key in ('bx','by','br','bb')));self.stub_calls['bounds']+=1;self.mutate();self.ret(4);return
        if address==v['later']:
            self.events.append(f'C:{self.local_owner}:{c["owner"]}:{c["flags"]}:{c["mode"]}:{c["lower"]}');self.stub_calls['collision_region']+=1
            mu.reg_write(UC_X86_REG_EIP,p['accept'] if c['lower'] else p['reject']);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """전체 배치 반환 이후 현재 미리보기/지도/전역/raw와 ABI를 관찰한다."""
        self.case=case;self.events=[];self.margin=-1;self.instructions=0;self.local_owner=0;p=self.p;v=self.v
        self.write_ranges=[(p['blocked'],p['blocked']+4)]
        self.mu.mem_write(TYPES+case['type']*self.type_stride+0xec,struct.pack('<I',case['genus']))
        # 표면 raw의 상태/extra는 서로 다르게 두며 미리보기에서 추가 필터가 없는지 확인한다.
        for sid,typ,genus,owner in SURFACES:
            raw=bytearray([0xcd]*self.stride);raw[10]=typ;raw[11]=(case['mapVariant']+sid)%16;raw[self.o['owner']]=owner
            self.mu.mem_write(self.slot(sid),bytes(raw));self.mu.mem_write(TYPES+typ*self.type_stride+0xec,struct.pack('<I',genus))
        spots,surface=self.map_inputs[case['mapVariant']];self.mu.mem_write(SPOTS,spots);self.mu.mem_write(MAP,surface)
        self.mu.mem_write(v['mask'],bytes([0xa5])*MASK)
        relations=[0]*81;relations[19]=case['forward']
        if case['owner']&255==255:relations[8]=case['forward']
        self.mu.mem_write(v['relations'],struct.pack('<81I',*relations))
        self.mu.mem_write(v['editor'],struct.pack('<I',case['editor']));self.mu.mem_write(v['allies'],struct.pack('<I',case['allies']))
        self.mu.mem_write(p['force'],struct.pack('<I',case['force']));self.mu.mem_write(p['local'],struct.pack('<i',case['local']));self.mu.mem_write(p['blocked'],struct.pack('<I',0xffffffff))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 호출 전 보존 레지스터·x87 상태·값 인자를 준비한다.
        for reg,value in preserved.items():self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_EDX,0);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_ECX,TYPES+case['type']*self.type_stride);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.mem_write(STACK,struct.pack('<7I',STOP,*[case[key] for key in ('type','x','y','flags','owner','mode')]))
        self.mu.emu_start(p['entry'],STOP,count=50000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+28:raise RuntimeError('미리보기 전체 반환/스택 오류')
        if any(self.mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('미리보기 보존 레지스터 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('미리보기 x87 복구 오류')
        state=[struct.unpack('<I',self.mu.mem_read(v[key],4))[0] for key in ('editor','allies')]
        forward=struct.unpack('<I',self.mu.mem_read(v['relations']+19*4,4))[0]
        genus=struct.unpack('<I',self.mu.mem_read(TYPES+83*self.type_stride+0xec,4))[0]
        return [self.mu.reg_read(UC_X86_REG_EAX),struct.unpack('<I',self.mu.mem_read(p['blocked'],4))[0],*state,forward,genus,
            bytes(self.mu.mem_read(v['mask'],MASK)).hex(),bytes(self.mu.mem_read(self.slot(50),self.stride)).hex(),
            zlib.adler32(bytes(self.mu.mem_read(SPOTS,CELLS))),zlib.adler32(bytes(self.mu.mem_read(MAP,CELLS*2))),';'.join(self.events)]


def generate(smoke=False):
    """두 정밀도 관찰이 같은 입력만 fixture와 SHA 근거에 기록한다."""
    cases=inputs();rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestplacement_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/priestpreview-functions.json',FIXTURE}
    if smoke:cases=cases[::53]
    # 세 실제 PE의 진입/반환과 미리보기 내용을 별도로 관찰한다.
    for edition in SPECS:
        oracle=PreviewOracle(edition)
        # 기대 mask/거부는 Python에서 계산하지 않고 실제 명령으로 얻는다.
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError('미리보기 x87 정밀도 차이')
            rows.append([edition,*[case[key] for key in KEYS],*values[0]])
        editions[edition]=dict(cases=len(cases),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 사제 미리보기 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 사제 배치 접두/미리보기/표면/관계. 모양 진입과 미리보기 뒤 충돌 구간만 대체.\n# edition '+ ' '.join(KEYS)+' result blocked editor allies forward genus mask raw spotsAdler mapAdler events\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',normal_return=True,os_calls=0,controls=list(CONTROLS),editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['진입/여백/로컬 분기/144바이트 초기화/순회/지도/표면 genus/관계/반환은 실제 실행',
            '모양·CanonDecoder 생성/범위 조회 함수 진입 대체; 이후 0049b825 또는 CD 0044550e부터 충돌/지역 구간 대체',
            'CD의 할당 지도 밖 읽기·관계 표 밖 입력·패치 assert는 정상 fixture에서 제외; 실제 모양/충돌/지도 수명 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·정상 반환·행 수·실제 함수/중간 구간 대체 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 현재 입력/내보내기/PE가 저장 근거와 같은지 확인한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls'] or not report['normal_return']:raise RuntimeError('미리보기 근거 오류')
    # 대체 사건 수와 실제 진입/CRT/genus를 감사한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];p=PLACEMENT[edition];v=PREVIEW[edition]
        if len(selected)!=item['cases'] or item['assertions'] or item['native_calls'].get(f'{p["entry"]:08x}')!=2*len(selected):raise RuntimeError('미리보기 실제 진입 오류')
        for marker,key in (('H','shape'),('B','decoder'),('R','bounds'),('C','collision_region')):
            count=2*sum(sum(part.startswith(marker+':') for part in row[-1].split(';')) for row in selected)
            if count!=item['substitutions'].get(key,0):raise RuntimeError(f'미리보기 대체 수 오류: {key}')
        if item['native_calls'].get(f'{p["ftol"]:08x}')!=item['substitutions']['shape']:raise RuntimeError('실제 여백 CRT 누락')
        if any(item['native_calls'].get(f'{address:08x}',0) for address in (p['shape'],v['begin'],v['bounds'])):raise RuntimeError('모양 실제/대체 혼합')
        if not item['native_calls'].get(f'{v["genus"]:08x}',0):raise RuntimeError('실제 표면 genus 누락')
        if edition=='originals' and any(not item['native_calls'].get(f'{v[key]:08x}',0) for key in ('surface','related')):raise RuntimeError('실제 표면/관계 조회 누락')
    print(f'priestpreview 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
