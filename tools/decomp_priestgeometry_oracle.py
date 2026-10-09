#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 비패턴 CanonDecoder 생성/진행/범위와 한 모양의 finder 인자를 대조한다.

ctor/Advance/Bounds/snap/CRT는 실제 명령이다. 충돌 사각형 구간은 중간 지역 변수를 공급한다.
finder 호출 직전 인자 관찰로 종료하며 전체 MayPlace/후보/지형/게임/OS를 실행하지 않는다.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,STACK,STOP,digest
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 분석 전용 decoder/출력/프레임 코드 주소와 새 관찰 파일이다.
DEC,OUT,CODES=0x14200000,0x14300000,0x14400000
FIXTURE=ROOT/'cpppj/tests/fixtures/priestgeometry-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestgeometry-evidence.json'
# 실제 진입점·한 모양 중간 구간·현재 패턴 전역이다.
GEOMETRY={
 'originals':dict(begin=0x425c20,next=0x425860,bounds=0x425b90,area=0x49b8c0,finder=0x4b16d0,
    snap=0x41d770,ftol=0x4e49c0,foot=0x1d4,point=0xf4,self=0x14,
    patterns=(0x541204,0x5411a0,0x5411d0,0x5412cc,0x541264,0x54125c,0x541288,0x541290),predicate=0x425830),
 'originalCD':dict(begin=0x41fcf0,next=0x41feb0,bounds=0x4202e0,area=0x4455ab,finder=0x4eae20,
    snap=0x440a50,ftol=0x4f161c,foot=0x1b4,point=0xf8,
    patterns=(0x51caf0,0x51ca8c,0x51cabc,0x51cbb8,0x51cb50,0x51cb48,0x51cb74,0x51cb7c),predicate=0x41fcc0),
}
GEOMETRY['original1037']=dict(GEOMETRY['originalCD'])
# 고정 입력 열 순서다. 기대 사각형과 frame/Valid는 입력 생성기에서 계산하지 않는다.
KEYS=('type','argument','direction','x','y','explicit','count','default','footX','footY')


def inputs(edition):
    """음수 좌표/절삭 경계·단일/여러 발자국·누락/default/명시 프레임을 교차한다."""
    positions=((20,21),(20.5,21.5),(20.000001907348633,21.999998092651367),(19.999998092651367,20.000001907348633),
        (0.99999,1.00001),(-1.25,-0.75),(255.99999,254.5),(0,0))
    footprints=((1,1),(3,2),(2,4),(10,10))
    profiles=((5,0,158,0),(5,4,158,0),(5,-1,158,0),(0,-1,158,0),(1,4,158,0),(5,0,0,1),(5,0,2,0xffffffff),(5,0,4,1))
    directions=(0,1,2,3,4,5,6,7,0xffffffff) if edition=='originals' else (0,2,4,6)
    cases=[]
    # 원본 assert에 도달하는 CD 홀수/잘못된 명시 프레임은 정상 fixture에서 제외한다.
    for pos,foot,profile,direction in itertools.product(positions,footprints,profiles,directions):
        count,default,argument,explicit=profile
        cases.append(dict(type=158,argument=argument,direction=direction,x=bits(pos[0]),y=bits(pos[1]),explicit=explicit,
            count=count,default=default,footX=foot[0],footY=foot[1]))
    return cases


class GeometryOracle(OwnerOracle):
    """비패턴 실제 함수와 사각형 인라인 구간의 허용 범위만 실행한다."""
    def __init__(self,edition):
        """새 범위/분석 전용 객체와 현재 타입 메타 자료를 준비한다."""
        super().__init__(edition);self.s=GEOMETRY[edition];self.area_entries=0;self.finder_boundaries=0
        # 객체/출력/코드 표는 서로 다른 가상 페이지다.
        for address in (DEC,OUT,CODES):self.mu.mem_map(address,0x1000)
        self.exports=[ROOT/f'extracted/priestgeometry/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 실제 함수의 불연속 몸체를 그대로 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 범위를 반열린 구간으로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        # 모든 특수 패턴 번호는 0으로 공급한다. 사제 158은 비패턴 경로다.
        for address in self.s['patterns']:self.mu.mem_write(address,bytes(4))
        if edition=='originals':self.mu.mem_write(0x5e4794,bytes(4))
        self.mu.mem_write(CODES,bytes(32))

    def on_write(self,mu,access,address,size,value,data):
        """정확한 decoder/출력과 스택 쓰기만 허용한다."""
        if any(low<=address and address+size<=low+length for low,length in ((DEC,84),(OUT,16))):return
        super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """사각형의 실제 계산 뒤 finder 함수 몸체 진입 전에 인자만 관찰한다."""
        if address==self.s['finder']:
            esp=mu.reg_read(UC_X86_REG_ESP);self.area_values=list(struct.unpack('<4i',mu.mem_read(esp+4,16)))
            self.finder_boundaries+=1;mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EIP,STOP);return
        if address==self.s['area']:self.area_entries+=1
        super().on_instruction(mu,address,size,data)

    def prepare_registers(self,control):
        """메서드의 보존 레지스터와 x87 상태를 서로 다른 고정값으로 준비한다."""
        self.preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 독립 메서드 호출마다 보존 레지스터를 다시 준비한다.
        for reg,value in self.preserved.items():self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_EDX,0);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)

    def check(self,control,esp,preserved=True):
        """정상 종료/스택/메서드 보존 레지스터/x87 균형을 검사한다."""
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=esp:raise RuntimeError('사제 모양 종료/스택 오류')
        if preserved and any(self.mu.reg_read(reg)!=value for reg,value in self.preserved.items()):raise RuntimeError('사제 모양 보존 레지스터 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('사제 모양 x87 균형 오류')

    def call(self,address,args,control):
        """실제 thiscall의 정상 반환을 검사한다."""
        self.prepare_registers(control);self.mu.reg_write(UC_X86_REG_ECX,DEC)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*[value&0xffffffff for value in args]))
        self.mu.emu_start(address,STOP,count=20000);self.check(control,STACK+4+4*len(args));return self.mu.reg_read(UC_X86_REG_EAX)

    def snapshot(self):
        """호스트 포인터/원본 미초기화 필드를 제외한 공개 frame/Valid/좌표를 관찰한다."""
        return [struct.unpack('<i',self.mu.mem_read(DEC+16,4))[0],struct.unpack('<I',self.mu.mem_read(DEC+20,4))[0],*struct.unpack('<2I',self.mu.mem_read(DEC+32,8))]

    def run(self,case,control):
        """생성→첫 범위→충돌 인자→다음/끝 범위를 각각 실제 실행한다."""
        self.instructions=0;self.write_ranges=[];s=self.s;mu=self.mu;typ=TYPES+case['type']*self.type_stride
        mu.mem_write(typ+0x114,struct.pack('<Ii',case['count'],case['default']));mu.mem_write(typ+0x124,struct.pack('<I',CODES))
        mu.mem_write(typ+s['foot'],struct.pack('<2i',case['footX'],case['footY']));mu.mem_write(DEC,bytes([0xa5])*84)
        result=self.call(s['begin'],[case[key] for key in ('type','argument','direction','x','y','explicit')],control)
        if result!=DEC:raise RuntimeError('실제 decoder 생성 반환 this 오류')
        first=self.snapshot();self.call(s['bounds'],[OUT],control);bounds=list(struct.unpack('<4i',mu.mem_read(OUT,16)))
        self.area_values=None
        if first[1]:
            # 이 구간은 전체 MayPlace 호출이 아니다. 현재 decoder 좌표/자기 타입의 지역 변수만 공급한다.
            self.prepare_registers(control);mu.mem_write(STACK+s['point'],struct.pack('<2I',first[2],first[3]))
            if self.edition=='originals':mu.mem_write(STACK+s['self'],struct.pack('<I',typ))
            else:mu.reg_write(UC_X86_REG_EBX,typ)
            mu.emu_start(s['area'],STOP,count=20000);self.check(control,STACK,False)
        area='none' if self.area_values is None else ':'.join(map(str,self.area_values))
        self.call(s['next'],[],control);last=self.snapshot();self.call(s['bounds'],[OUT],control);end_bounds=list(struct.unpack('<4i',mu.mem_read(OUT,16)))
        return [*first,':'.join(map(str,bounds)),area,*last,':'.join(map(str,end_bounds))]


def generate(smoke=False):
    """두 x87 정밀도의 같은 관찰만 새 fixture/SHA 근거로 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/priestgeometry-functions.json',FIXTURE}
    # 세 실제 PE에서 각 입력을 독립 실행한다.
    for edition in SPECS:
        oracle=GeometryOracle(edition);cases=inputs(edition)
        if smoke:cases=cases[:80]
        # 기대 frame/범위/좌표는 실제 기계어가 계산한다.
        for case in cases:
            observations=[oracle.run(case,control) for control in CONTROLS]
            if observations[0]!=observations[1]:raise RuntimeError(f'x87 정밀도 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*observations[0]])
        editions[edition]=dict(cases=len(cases),native_calls=dict(oracle.native_calls),area_entries=oracle.area_entries,finder_boundaries=oracle.finder_boundaries,assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 비패턴 사제 모양 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 비패턴 decoder/범위와 한 모양 충돌 인자. finder/전체 MayPlace/지형 미실행.\n# edition '+' '.join(KEYS)+' frame valid x y bounds area endFrame endValid endX endY endBounds\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},limits=['비패턴 ctor/Advance/Bounds/타입·발자국 조회/snap/CRT 실제 실행; 메서드 thiscall 정상 반환/ABI 대조','충돌 사각형은 MayPlace 중간 구간에 지역 변수를 공급하고 finder 진입에서 관찰 종료: 전체 MayPlace/finder/후보/지형 미실행','패턴 전역 번호=0·패치 프레임 디버그 검사 전역=0; CD 홀수 회전/잘못된 명시 프레임 assert는 정상 fixture에서 제외','초기 픽셀 모양/SHP 조회와 실제 후보별 지형/관계 몸체는 외부 경계']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """입력/PE/내보내기 SHA와 실제 생성/진행/범위/구간 관찰 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 근거 파일을 전부 검사한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('사제 모양 근거 오류')
    # 실제 코드와 finder 경계의 관찰 횟수를 대조한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];s=GEOMETRY[edition];n=len(selected);valid=sum(int(row[12])!=0 for row in selected)
        if n!=item['cases'] or item['assertions'] or item['area_entries']!=2*valid or item['finder_boundaries']!=2*valid:raise RuntimeError('사제 모양 실제 구간 누락')
        calls=item['native_calls']
        if calls.get(f'{s["begin"]:08x}')!=2*n or calls.get(f'{s["next"]:08x}')!=4*n or calls.get(f'{s["bounds"]:08x}')!=4*n:raise RuntimeError('실제 생성/진행/범위 누락')
        if calls.get(f'{s["predicate"]:08x}')!=2*n or calls.get(f'{s["snap"]:08x}',0)!=2*valid or calls.get(f'{s["finder"]:08x}',0):raise RuntimeError('비패턴/스냅/경계 감사 오류')
        if calls.get(f'{s["ftol"]:08x}')!=8*n+12*valid:raise RuntimeError('실제 CRT 횟수 오류')
    print(f'priestgeometry 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
