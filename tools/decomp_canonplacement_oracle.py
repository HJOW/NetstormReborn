#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 실제 PE의 일반 배치 접두를 실행한다. 게임/OS는 실행하지 않는다.

전역 초기화·강제 허용·타입 번호·여백/x87·지도 경계·정상 반환은 실제 명령이다.
모양 조회는 함수 진입 대체, 미리보기/충돌/관계 본체는 명시한 중간 구간 대체다.
"""
import argparse
import collections
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

# 실제 접두·모양/CRT·중간 대체/반환 지점·전역의 판본별 주소다.
PLACEMENT={
 'originals':dict(entry=0x49b510,shape=0x43cbd0,ftol=0x4e49c0,margin=0x49b5fd,region=0x49b661,
    accept=0x49bfbb,reject=0x49bcd1,force=0x55a488,local=0x540c70,blocked=0x59ab2c),
 'originalCD':dict(entry=0x445200,shape=0x4ed7c0,ftol=0x4f161c,margin=0x44531a,region=0x44537c,
    accept=0x445ce2,reject=0x445cde,force=0x537e98,local=0x50f6c8,blocked=0x51cbfc),
}
PLACEMENT['original1037']=dict(PLACEMENT['originalCD'])
# 저장 관찰과 SHA 근거는 기존 사제 생성 감사와 분리한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/canonplacement-x86.tsv'
REPORT=ROOT/'cpppj/recovery-canonplacement-evidence.json'
# 입력/TSV의 고정 순서다. 기대 여백이나 허용 여부는 입력 생성기에서 계산하지 않는다.
KEYS=('type','argument','x','y','owner','flags','mode','genus','force','local','left','top','right','bottom','lower','change')


def inputs():
    """모양·경계·상위 소유자 비트·허용 전역과 변경 입력을 교차한다."""
    shapes=((0,0,16,11),(-8,-5.5,8,5.5),(0,0,31.99999,21.99999),(0,0,32,22),
        (0,0,32.00001,22.00001),(2.5,7.25,50.5,40.25),(0,0,0,0),(0,0,64,11),
        (0.00000095367431640625,0,32,0),(1000,1000,0,0))
    positions=((-1,20),(0,20),(0.99999,20),(1,20),(1.99999,20),(2,20),(3,20),(4,20),
        (252,252),(254,254),(255,255),(256,256),(257,257),(20,0.99999),(20,2),(20,254))
    contexts=((8,8,0,0,0),(0x108,8,0,1,1),(255,-1,0,0,2),(128,-128,0,1,0),
        (8,264,1,0,0),(8,8,0,1,3),(8,264,0,1,0))
    cases=[]
    # 모든 모양/좌표는 강제 허용·부호 BYTE·로컬 비교·구간 거부/허용과 조합한다.
    for index,(shape,position,context) in enumerate(itertools.product(shapes,positions,contexts)):
        owner,local,force,lower,change=context
        cases.append(dict(type=(70,82,94,107,129,131,140,142,157,158)[index%10],argument=(0,1,25,67,0xffffffff,158)[index%6],x=bits(position[0]),y=bits(position[1]),owner=owner,
            flags=(0,0x80000000,7)[index%3],mode=(0,1,0xffffffff)[index%3],genus=(0,2,4,0x100,0x20000,0x210000,0x10202000)[index%7]|(0x02000000 if change==3 else 0),
            force=force,local=local,left=bits(shape[0]),top=bits(shape[1]),right=bits(shape[2]),bottom=bits(shape[3]),lower=lower,change=change))
    return cases


class CanonPlacementOracle(OwnerOracle):
    """배치 접두와 실제 반환을 관찰하고 후반 구간 대체를 명시한다."""
    def __init__(self,edition):
        """읽기 전용 내보내기의 허용 코드만 다시 지정한다."""
        super().__init__(edition);self.p=PLACEMENT[edition];self.stub_calls=collections.Counter()
        self.exports=[ROOT/f'extracted/canonplacement/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 실제 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 각 함수의 Ghidra 범위를 그대로 쓴다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """모양 진입과 후반 구간만 대체하며 여백/지도 판단은 실제 실행한다."""
        p=self.p;c=self.case;esp=mu.reg_read(UC_X86_REG_ESP)
        if address==p['shape']:
            out,typ,arg,flags,out1,out2,last=struct.unpack('<7I',mu.mem_read(esp+4,28))
            if (typ,arg,flags,last)!=(c['type'],c['argument'],c['flags'],0):raise RuntimeError('모양 조회 인자 오류')
            if not all(0x30000000<=ptr and ptr+length<=STACK for ptr,length in ((out,16),(out1,4),(out2,4))):raise RuntimeError('모양 출력 포인터 범위 오류')
            if struct.unpack('<I',mu.mem_read(p['blocked'],4))[0]:raise RuntimeError('모양 조회 전 차단값 초기화 누락')
            mu.mem_write(out,struct.pack('<4I',*[c[key] for key in ('left','top','right','bottom')]))
            mu.mem_write(out1,bytes(4));mu.mem_write(out2,bytes(4))
            self.events.append(f'H:{typ}:{arg}:{flags}');self.stub_calls['shape']+=1
            if c['change']==1:
                mu.mem_write(p['local'],struct.pack('<i',17));mu.mem_write(p['force'],struct.pack('<I',1));mu.mem_write(TYPES+typ*self.type_stride+0xec,bytes(4))
            target=struct.unpack('<I',mu.mem_read(esp,4))[0];mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,target);return
        if address==p['margin']:
            offset=0x10 if self.edition=='originals' else 0x12c
            self.margin=struct.unpack('<I',mu.mem_read(esp+offset,4))[0]
        if address==p['region']:
            if struct.unpack('<I',mu.mem_read(p['blocked'],4))[0]:raise RuntimeError('후반 구간 진입 전 차단값 초기화 누락')
            local=mu.reg_read(UC_X86_REG_EDI) if self.edition=='originals' else struct.unpack('<I',mu.mem_read(esp+0x5c,4))[0]
            self.events.append(f'G:{local}:{c["owner"]}:{c["flags"]}:{c["mode"]}:{c["lower"]}');self.stub_calls['geometry_region']+=1
            mu.mem_write(p['blocked'],struct.pack('<I',0x77))
            if c['change']==2:mu.mem_write(p['force'],struct.pack('<I',9));mu.mem_write(p['local'],struct.pack('<i',-7))
            mu.reg_write(UC_X86_REG_EIP,p['accept'] if c['lower'] else p['reject']);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """thiscall 전체 반환·레지스터/x87·전역/사건/여백을 관찰한다."""
        self.case=case;self.events=[];self.margin=-1;self.instructions=0;p=self.p
        self.write_ranges=[(p['blocked'],p['blocked']+4)]
        self.mu.mem_write(TYPES+case['type']*self.type_stride+0xec,struct.pack('<I',case['genus']))
        self.mu.mem_write(p['force'],struct.pack('<I',case['force']));self.mu.mem_write(p['local'],struct.pack('<i',case['local']))
        self.mu.mem_write(p['blocked'],struct.pack('<I',0xffffffff))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 호출 전 보존 레지스터와 x87 상태를 서로 다른 기록값으로 준비한다.
        for reg,value in preserved.items():self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_EDX,0);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_ECX,TYPES+case['type']*self.type_stride);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.mem_write(STACK,struct.pack('<7I',STOP,*[case[key] for key in ('argument','x','y','flags','owner','mode')]))
        self.mu.emu_start(p['entry'],STOP,count=10000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+28:raise RuntimeError('배치 thiscall 반환/스택 오류')
        if any(self.mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('배치 보존 레지스터 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('배치 x87 복구 오류')
        state=[struct.unpack('<I',self.mu.mem_read(p[key],4))[0] for key in ('blocked','force','local')]
        genus=struct.unpack('<I',self.mu.mem_read(TYPES+case['type']*self.type_stride+0xec,4))[0]
        return [self.mu.reg_read(UC_X86_REG_EAX),self.margin,*state,genus,';'.join(self.events)]


def generate(smoke=False):
    """두 정밀도의 실제 관찰을 분리된 fixture/SHA 근거에 저장한다."""
    cases=inputs();rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/canonplacement-functions.json',FIXTURE}
    if smoke:cases=cases[::31]
    # 세 PE에서 독립 정상 반환을 관찰한다.
    for edition in SPECS:
        oracle=CanonPlacementOracle(edition)
        # 기대 허용/여백은 실제 명령으로만 얻는다.
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError('배치 접두 x87 정밀도 차이')
            rows.append([edition,*[case[key] for key in KEYS],*values[0]])
        editions[edition]=dict(cases=len(cases),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 일반 배치 접두 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 배치 접두/여백/지도 조건. 모양 진입 및 후반 미리보기/충돌/관계 구간 대체.\n# edition '+ ' '.join(KEYS)+' result margin blocked finalForce finalLocal finalGenus events\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',normal_return=True,os_calls=0,controls=list(CONTROLS),editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['전역 초기화/강제 허용/타입 번호/여백/지도 조건/반환은 실제 실행; 타입 번호와 패턴 argument 구별',
            '모양 함수 진입 대체; 후반 0049b661~0049bfbb / CD 0044537c~00445ce2 중간 구간 대체',
            '미리보기/충돌/지역/관계 본체 또는 실제 모양/지도 수명 검증이 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·행 수·실제 진입·모양/중간 구간 대체 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 근거에 포함한 모든 입력/내보내기/바이너리의 변화를 확인한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls'] or not report['normal_return']:raise RuntimeError('배치 접두 근거 오류')
    # 모양/구간 사건을 실제 호출/대체 수와 비교한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];p=PLACEMENT[edition]
        if len(selected)!=item['cases'] or item['assertions'] or item['native_calls'].get(f'{p["entry"]:08x}')!=2*len(selected):raise RuntimeError('배치 실제 진입 오류')
        for marker,key in (('H','shape'),('G','geometry_region')):
            count=2*sum(sum(part.startswith(marker+':') for part in row[-1].split(';')) for row in selected)
            if count!=item['substitutions'].get(key,0):raise RuntimeError('배치 경계 대체 수 오류')
        if item['native_calls'].get(f'{p["ftol"]:08x}')!=item['substitutions']['shape']:raise RuntimeError('배치 실제 CRT 누락')
        if item['native_calls'].get(f'{p["shape"]:08x}',0):raise RuntimeError('모양 실제/대체 혼합')
        if edition!='originals' and item['native_calls'].get('004448e0')!=item['substitutions']['shape']:raise RuntimeError('CD 실제 타입 번호 조회 누락')
    print(f'canonplacement 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
