#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 타입 로더의 숫자 속성 구간과 CRT ftol을 세 PE에서 제한 실행한다. 게임/OS는 실행하지 않는다."""
import argparse
import csv
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT, SPECS, OwnerOracle, TYPES, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 합성 AST 주소와 두 x87 정밀도다. 기대값은 아래 실제 명령이 계산한다.
AST=0x14800000
CONTROLS=(0x027f,0x037f)
# 로더 중간 진입/다음 속성 경계/타입 기준점 저장 위치다.
SPEC={'originals':dict(X=0x49d6f9,Y=0x49d758,stop=0x49d090,hot=0x1dc,ftol='004e49c0'),
      'originalCD':dict(X=0x447428,Y=0x447487,stop=0x44825a,hot=0x1bc,ftol='004f161c')}
SPEC['original1037']=SPEC['originalCD']
# 기존 fixture/감사 기록과 분리한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/typehotspot-x86.tsv'
REPORT=ROOT/'cpppj/recovery-typehotspot-evidence.json'


def bits(value):
    """속성 노드의 float32 비트를 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """정수 경계 양옆과 float32 반올림/부호/중복 속성 입력을 만든다. 결과는 계산하지 않는다."""
    values={bits(v) for v in (0.0,-0.0,0.5,-0.5,1,-1,1.1,-1.1,0.49999999,0.99999999,16777217/16,16777217/11)}
    # 11/16 배율로 정수에 닿는 각 float32의 인접 표현도 포함한다.
    for factor in (11,16):
        for integer in (-100000001,-16777217,-1025,-33,-11,-1,1,11,33,1025,16777217,100000001):
            center=bits(integer/factor)
            values.update((center-1,center,center+1))
    cases=[]
    # 다른 축의 이전 값 보존과 두 축의 순서/중복 덮어쓰기를 관찰한다.
    for value in sorted(values):
        for sequence in ('X','Y','XY','YX','XXY','YYX'):
            cases.append((sequence,value,bits(-0.75),123456789,-987654321))
    return cases


class HotspotOracle(OwnerOracle):
    """실제 명령 몸체와 타입의 선택된 기준점 8바이트 쓰기만 허용한다."""
    def __init__(self,edition):
        """AST 저장소 및 새 함수 허용 목록을 준비한다."""
        super().__init__(edition);self.s=SPEC[edition];self.boundaries=0;self.mu.mem_map(AST,0x1000)
        self.exports=[ROOT/f'extracted/typehotspot/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 함수의 실제 몸체만 실행한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """다음 속성의 파싱/문자열 비교 전에 종료하며 호출을 대체하지 않는다."""
        if address==self.s['stop']:
            self.boundaries+=1;mu.reg_write(UC_X86_REG_EIP,STOP);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """각 속성의 AST 숫자 입력에서 실제 정수 저장/스택/x87 균형을 관찰한다."""
        sequence,a,b,x,y=case;mu=self.mu;s=self.s;target=TYPES+82*self.type_stride
        mu.mem_write(target,bytes([0xcd])*self.type_stride);mu.mem_write(target+s['hot'],struct.pack('<ii',x,y))
        mu.mem_write(AST,bytes(0x100));mu.mem_write(AST+0xc,struct.pack('<I',AST+0x40));mu.mem_write(AST+0x40,struct.pack('<I',2))
        self.write_ranges=[(target+s['hot'],target+s['hot']+8)]
        # 첫 입력 이후의 속성은 다른 값을 지정하여 중복 대입을 구별한다.
        for index,axis in enumerate(sequence):
            mu.mem_write(AST+0x48,struct.pack('<I',a if index==0 else b));self.instructions=0
            mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EBX,target);mu.reg_write(UC_X86_REG_EBP,STACK+0x100)
            mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
            if self.edition=='originals':mu.mem_write(STACK+0x58,struct.pack('<I',AST))
            else:
                mu.mem_write(STACK+0xf0,struct.pack('<I',AST));mu.mem_write(STACK+0xec,struct.pack('<I',target))
            mu.emu_start(s[axis],STOP,count=2000)
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK:raise RuntimeError('속성 구간 스택/종료 오류')
            if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('속성 구간 x87 균형 오류')
            if mu.reg_read(UC_X86_REG_EBX)!=target or mu.reg_read(UC_X86_REG_EBP)!=STACK+0x100:raise RuntimeError('속성 구간 기준 레지스터 변경')
        return struct.unpack('<ii',mu.mem_read(target+s['hot'],8))


def generate(smoke=False):
    """두 정밀도 결과가 같은 실제 기계어 출력과 SHA/관찰 경계를 기록한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/typehotspot-functions.json',FIXTURE}
    # 각 실제 PE에서 동일한 독립 입력을 실행한다.
    for edition in SPECS:
        oracle=HotspotOracle(edition);cases=inputs()[:12] if smoke else inputs()
        for case in cases:
            actual=[oracle.run(case,c) for c in CONTROLS]
            if actual[0]!=actual[1]:raise RuntimeError('타입 기준점의 x87 정밀도 차이')
            rows.append([edition,*case,*actual[0]])
        editions[edition]=dict(cases=len(cases),boundaries=oracle.boundaries,property_ops=sum(len(c[0]) for c in cases),native_calls=dict(oracle.native_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 기준점 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 숫자 속성 구간/CRT ftol. 전체 타입 로더/AST parser/게임 실행 아님.\n# edition sequence a_bits b_bits initial_x initial_y result_x result_y\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['실제 숫자 속성 중간 진입에서 다음 속성 경계까지, 전체 로더 ABI 검증 아님','AST float32/정수 범위 안 입력만 관찰, 문자열 비교/파싱/잘못된 입력 assert 경로 미실행']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """PE/내보내기/도구/fixture SHA와 실제 속성/ftol 실행 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('기준점 근거 개수/정밀도 오류')
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];ops=sum(len(r[1]) for r in selected)
        if len(selected)!=item['cases'] or len(selected)!=len(inputs()) or ops!=item['property_ops'] or item['boundaries']!=ops*2 or item['assertions']:raise RuntimeError('속성 구간 경계/입력 개수 오류')
        if item['native_calls']!={SPEC[edition]['ftol']:ops*2}:raise RuntimeError('실제 ftol 호출 횟수 오류')
    print(f"타입 기준점 감사 통과: {report['total']}개")


def main():
    """생성/짧은 실행/저장 기록 감사 중 선택한 작업을 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
