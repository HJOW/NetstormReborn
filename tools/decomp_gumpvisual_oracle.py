#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 버튼 생성자/그리기를 제한 x86으로 실행해 팔레트·문맥·선·글자 배치를 관찰한다. 게임 실행 아님."""
import argparse
import collections
import csv
import json
import random
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT, SPECS, OwnerOracle, TYPES, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP,
    UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 실제 전체 생성자/그리기와 기반/자식/장치 경계다. CD/10.37은 동일 배치다.
SPEC={
    'originals':dict(ctor=0x4254b0,draw=0x424ff0,base=0x4253e0,alloc=0x4e4391,child=0x4655d0,
        rect=0x4648e0,line=0x4dade0,text=0x465050,black=0x5a4058,white=0x5bec2c,edge=0x5bec38),
    'originalCD':dict(ctor=0x441310,draw=0x441660,base=0x440bb0,alloc=0x4f1650,child=0x494640,
        rect=0x493670,line=0x4d6850,text=0x493ed0,black=0x55baa8,white=0x55bad0,edge=0x55287c)
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 합성 버튼·배경 자식·가상 표와 등록 경계다. 호스트 메모리와 관계없다.
OBJECT,CHILD,VTABLE,ATTACH=TYPES+0x10000,TYPES+0x11000,TYPES+0x12000,TYPES+0x13000
# 입력만 만드는 시드·두 x87 정밀도·고정 출력 위치다.
SEED,CONTROLS=0x424ff0,(0x027f,0x037f)
FIXTURE=ROOT/'cpppj/tests/fixtures/gumpvisual-x86.tsv'
REPORT=ROOT/'cpppj/recovery-gumpvisual-evidence.json'


def inputs():
    """정상/비활성/외곽선 생략·두 상태·음수/홀수 폭·DWORD 넘침 입력을 만든다. 기대값은 계산하지 않는다."""
    rng=random.Random(SEED);cases=[]
    # 플래그의 낮은 비트와 무관한 높은 비트를 함께 관찰한다.
    for flags in (0,1,8,9,0x100,0x101,0xffffffff,0x108):
        # 독립한 값/마우스 눌림 필드를 각각 설정해 OR 조건을 관찰한다.
        for value,pressed in ((0,0),(1,0),(0,1),(-1,7)):
            cases.append((10,20,85,39,37,14,value,pressed,flags))
    # 화면 밖·빈/작은 사각형과 감기는 중간 계산을 원본 정수 명령으로 관찰한다.
    for rect in ((-7,-11,3,5),(0,0,1,1),(-0x80000000,-3,0x7fffffff,4),(0x7fffffff,0x7fffffff,-0x80000000,-0x80000000)):
        # 부호가 다른 글자 크기로 음수 나눗셈/중간 감김을 관찰한다.
        for width,height in ((0,0),(33,14),(-3,-7),(0x7fffffff,-0x80000000)):
            cases.append((*rect,width,height,0,1,rng.choice((0,1,8,9))))
    # 다양한 홀수/음수 계산과 0이 아닌 상태를 더한다.
    for _ in range(16):
        x,y=rng.randint(-500,500),rng.randint(-500,500)
        cases.append((x,y,x+rng.randint(-50,120),y+rng.randint(-20,60),rng.randint(-10,180),rng.randint(-7,32),
            rng.choice((0,1,-1)),rng.choice((0,0,3)),rng.getrandbits(32)))
    return [(*case,rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32)) for case in cases]


class GumpOracle(OwnerOracle):
    """실제 버튼 생성/상태 분기/배치 몸체를 실행하고 GUI 기반/자식/래스터 경계만 기록한다."""
    def __init__(self,edition):
        """읽기 전용 PE와 동일 호스트에서 내보낸 전체 몸체 범위 및 쓰기 범위를 준비한다."""
        super().__init__(edition);self.b=SPEC[edition];self.mu.mem_map(0,0x1000)
        self.exports=[ROOT/f'extracted/gumpvisual/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed,self.entries=[],set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 전체 원본 함수의 불연속 범위만 실행을 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.stubs={self.b[name]:name for name in ('base','alloc','child','rect','line','text')};self.stubs[ATTACH]='attach'
        self.write_ranges=[(0,4),(OBJECT,OBJECT+0x170),(CHILD,CHILD+0xb8)]
        self.mu.mem_write(VTABLE+0x20,struct.pack('<I',ATTACH));self.boundaries=collections.Counter();self.returns=0

    def read(self,address,format):
        """에뮬레이터 메모리에서 정확한 인자/상태 배치를 읽는다."""
        return struct.unpack(format,self.mu.mem_read(address,struct.calcsize(format)))

    def ret(self,purge=0,value=0):
        """명시 경계의 호출 규약에 맞춰 반환한다. 원본 내부 분기를 건너뛰는 용도로 사용하지 않는다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);target,=self.read(sp,'<I')
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def on_instruction(self,mu,address,size,data):
        """기반/자식/사각형 공급과 선/글자 출력 호출을 확인하고 실제 몸체는 부모의 실행 감사를 거친다."""
        name=self.stubs.get(address)
        if not name:return super().on_instruction(mu,address,size,data)
        self.boundaries[name]+=1;sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        if name=='base':
            if this!=OBJECT or self.read(sp+4,'<3I')!=(75,19,0):raise RuntimeError('버튼 기반 인자 오류')
            mu.mem_write(OBJECT+0x38,struct.pack('<f',1));self.ret(12,OBJECT)
        elif name=='alloc':
            if self.read(sp+4,'<I')!=(0xb8,):raise RuntimeError('자식 할당 크기 오류')
            self.ret(value=CHILD)
        elif name=='child':
            if this!=CHILD or self.read(sp+4,'<ffII')!=(73.0,17.0,0x3e00000,1):raise RuntimeError('배경 자식 인자 오류')
            mu.mem_write(CHILD,struct.pack('<I',VTABLE));self.ret(24,CHILD)
        elif name=='attach':
            if this!=CHILD or self.read(sp+4,'<4I')!=(0x3f800000,0x3f800000,OBJECT,0):raise RuntimeError('자식 부착 인자 오류')
            self.ret(16)
        elif name=='rect':
            if this!=OBJECT:raise RuntimeError('사각형 공급 대상 오류')
            destination,=self.read(sp+4,'<I');mu.mem_write(destination,struct.pack('<4i',*self.case[:4]));self.ret(4,destination)
        elif name=='line':
            x1,y1,x2,y2=self.read(sp+4,'<4i');color,=self.read(sp+20,'<I')
            self.lines.append(f'{x1},{y1},{x2},{y2},{color}');self.ret()
        else:
            pointer,x,y,context=self.read(sp+4,'<IiiI')
            if this!=OBJECT or pointer!=OBJECT+0xc0 or context not in (OBJECT+0x104,OBJECT+0x124,OBJECT+0x144):
                raise RuntimeError('버튼 글자 인자 오류')
            self.text=(x,y,(context-OBJECT-0x104)//32);self.ret(16)

    def call(self,entry,args,control,purge):
        """전체 진입/정상 반환·스택·보존 레지스터·SEH·x87 균형을 확인한다."""
        mu=self.mu;mu.mem_write(STACK-0x100,bytes(0x100));mu.mem_write(STACK,struct.pack('<I',STOP)+args)
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_EBP:0x23456789,UC_X86_REG_ESI:0x34567890,UC_X86_REG_EDI:0x45678901}
        # 매 호출 새 보존 레지스터 값을 넣어 경계의 호출 규약 오류도 검출한다.
        for register,value in preserved.items():mu.reg_write(register,value)
        mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.instructions=0
        mu.emu_start(entry,STOP,count=200000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+purge:raise RuntimeError('버튼 반환/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('버튼 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800 or self.read(0,'<I')!=(0,):
            raise RuntimeError('버튼 x87/SEH 오류')
        self.returns+=1

    def run(self,case,control):
        """전체 생성자가 만든 문맥을 실제 그리기에 공급하고 선 순서·상대 위치·선택 문맥을 반환한다."""
        self.case=case;self.lines=[];self.text=None;mu=self.mu;b=self.b
        mu.mem_write(0,bytes(4));mu.mem_write(OBJECT,bytes(0x170));mu.mem_write(CHILD,bytes(0xb8))
        # 실제 원본 색 전역만 입력한다. 임의 DWORD로 색 번호의 중간 절단을 검출한다.
        for name,value in zip(('black','white','edge'),case[-3:]):mu.mem_write(b[name],struct.pack('<I',value))
        self.call(b['ctor'],struct.pack('<5I',75,19,0,0,0),control,20)
        if mu.reg_read(UC_X86_REG_EAX)!=OBJECT or self.read(OBJECT+0x100,'<I')!=(CHILD,):raise RuntimeError('버튼 생성 결과 오류')
        contexts=bytes(mu.mem_read(OBJECT+0x104,96)).hex()
        width,height,value,pressed,flags=case[4:9]
        mu.mem_write(OBJECT+0x164,struct.pack('<2i',width,height));mu.mem_write(OBJECT+0xa0,struct.pack('<2i',value,pressed))
        # 상태 둘 다음의 +a8은 별도 필드다. 실제 그리기가 검사하는 버튼 플래그는 +ac에 기록한다.
        mu.mem_write(OBJECT+0xac,struct.pack('<I',flags))
        self.call(b['draw'],b'',control,0)
        if not self.text:raise RuntimeError('버튼 글자 출력 누락')
        if bytes(mu.mem_read(OBJECT+0x104,96)).hex()!=contexts:raise RuntimeError('그리기가 문맥을 변경함')
        return [contexts,*self.text,'|'.join(self.lines) or '-']


def generate(smoke=False):
    """두 정밀도가 일치하는 세 실제 PE 관찰만 저장하고 원본/도구/몸체/fixture SHA를 기록한다."""
    cases=inputs()[:2] if smoke else inputs();rows=[];editions={}
    paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/gumpvisual-functions.json',FIXTURE}
    # 각 판본에서 같은 입력을 두 정밀도로 독립 실행한다.
    for edition in SPECS:
        oracle=GumpOracle(edition)
        # 표본마다 실제 몸체를 두 번 실행하며 결과가 같을 때만 저장한다.
        for number,case in enumerate(cases):
            first=oracle.run(case,CONTROLS[0]);second=oracle.run(case,CONTROLS[1])
            if first!=second:raise RuntimeError(f'버튼 x87 정밀도 차이: {edition} {number}')
            rows.append([edition,number,*case,*first])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native_calls=dict(oracle.native_calls),
            boundaries=dict(oracle.boundaries),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 버튼 시각 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 전체 버튼 생성/그리기 x86 관찰. 기반/자식/사각형/선/글자 출력은 명시 경계.\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-10',controls=list(CONTROLS),os_calls=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},
        limits=['GUI 기반/자식 생성·할당·가상 부착·부모 사각형 공급·선/글자 래스터 출력은 명시 경계',
                '생성 입력의 객체 메모리는 0으로 준비; 원본이 초기화하지 않는 비사용 문맥 필드를 원본의 항상 0 값으로 주장하지 않음',
                '생성자 라벨 null/기본 문맥 경로만 실행; strncpy·사용자 문맥 복사·할당 실패/예외 제외',
                '자식의 질감/명암 변환표·전체 Gump 계층/입력·실제 원본 창 출력은 범위 밖']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """저장한 SHA·입력 순서·전체 진입/정상 반환·명시 경계 수를 다시 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 원본 바이너리와 도구/몸체/fixture가 생성 시점과 동일한지 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError(f'SHA 불일치: {name}')
    cases=inputs()
    if report['controls']!=list(CONTROLS) or report['os_calls'] or report['total']!=len(rows) or set(report['editions'])!=set(SPECS):raise RuntimeError('버튼 근거 계수 오류')
    # 실제 전체 생성/그리기 진입과 경계 호출을 판본마다 확인한다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];b=SPEC[edition];n=len(cases)*2
        expected={f'{b["ctor"]:08x}':n,f'{b["draw"]:08x}':n}
        boundary={name:n for name in ('base','alloc','child','attach','rect','text')}
        boundary['line']=sum(8 for case in cases if not case[8]&8)
        if len(selected)!=len(cases) or item['cases']!=len(cases) or item['returns']!=n*2 or item['assertions']:raise RuntimeError('버튼 표본/반환 계수 오류')
        if item['native_calls']!=expected or item['boundaries']!=boundary:raise RuntimeError('버튼 몸체/경계 계수 오류')
        # fixture의 입력 순서/개수와 생성 시드의 입력을 행마다 대조한다.
        for number,(row,case) in enumerate(zip(selected,cases)):
            if len(row)!=19 or row[1:14]!=list(map(str,(number,*case))):raise RuntimeError('버튼 입력/순서 오류')
    print(f"버튼 시각 감사 통과: {report['total']}개")


def main():
    """전체 생성·소량 직접 실행·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
