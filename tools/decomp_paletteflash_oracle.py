#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 번개 생성/초기화/프레임/중단을 제한 x86으로 실행한다. 메모리·부모·OS 팔레트 경계만 대체하며 게임은 실행하지 않는다."""
import argparse
import collections
import csv
import json
import math
import random
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,SPECS,OwnerOracle,TYPES,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EBP,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 함수·현재 팔레트/모드/실시간 전역과 메모리·부모·표시 경계의 판본별 주소다.
SPEC={
 'originals':dict(entry=0x470fa0,init=0x470e30,frame=0x471650,cancel=0x471820,read=0x4a2800,
    mode=0x5c78d4,wall=0x55b4d8,palette=0x59afe0,first=0x540a34,last=0x540a38,ready=0x595344,challenge=0x594fc8,
    new=0x4e4391,parent=0x48fc60,attach=0x41bd20,apply=0x4a2640,kill=0x41bf10,base=0x41bd00),
 'originalCD':dict(entry=0x4b10d0,init=0x4b1310,frame=0x4b1450,cancel=0x4b15d0,read=0x4246c0,
    mode=0x516af0,wall=0x548490,palette=0x5529a0,first=0x53be38,last=0x53be3c,ready=0x50f828,challenge=0x540a24,
    new=0x4f1650,parent=0x4b10b0,attach=0x48f590,apply=0x424500,kill=0x48f7b0,base=0x48f570)
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 합성 객체·입력 시드·원본 x87 정밀도 두 가지와 새 근거 파일이다.
OBJECT=TYPES+0x10000
SEED=0x471650
CONTROLS=(0x027f,0x037f)
FIXTURE=ROOT/'cpppj/tests/fixtures/paletteflash-x86.tsv'
REPORT=ROOT/'cpppj/recovery-paletteflash-evidence.json'


def bits(value):
    """double의 저장 바이트를 little-endian hex로 기록한다. NaN/무한대도 문자열 파싱 없이 보존한다."""
    return struct.pack('<d',value).hex()


def inputs():
    """색 포화/감김·예약 바이트·정확한 간격·지연/역행/NaN·모드·중단 입력만 만든다. 결과를 계산하지 않는다."""
    rng=random.Random(SEED);cases=[]
    # 실제 공개 생성 함수의 0→64 치환과 모드별 수명 선택 뒤 전체 프레임을 실행한다.
    for full in (0,1):
        for offset in (0,1,63,64,65,255,-1,-256,0x7fffffff,-0x80000000):
            for elapsed in (0.0,1/64/60,0.003,0.51):
                cases.append(['entry',full,offset,0.0,228,18,0.0,elapsed,64,0])
    # 공개 생성자 외 일반 색 전환 경로와 부분/빈 배열도 원본 초기화+프레임으로 확인한다.
    for full in (0,1):
        for offset in (0,64,-7,256):
            for elapsed in (0.0,1/128,math.nextafter(1/128,math.inf),0.125,0.5,0.51,-1.0,math.nan,math.inf):
                cases.append(['init',full,offset,0.5,5,7,0.0,elapsed,64,0x917f03fe])
    # 초기화된 효과 중단, 이미 끝난 단계/잘못된 단계, 0/음수 간격과 전체 색 범위를 관찰한다.
    for full in (0,1):
        for start,count in ((0,0),(0,256),(228,18),(255,1)):
            cases.append(['cancel',full,64,0.5,start,count,4.0,4.0,64,0])
            for duration,step in ((0.0,64),(-0.5,64),(0.5,0),(0.5,65)):
                cases.append(['init',full,64,duration,start,count,4.0,4.001,step,0])
    # 0이 아닌 시작 시각·마지막 시각 누적 반올림과 다양한 예약 바이트를 추가한다.
    for index in range(64):
        cases.append(['init',index%2,rng.randint(-300,300),rng.choice((0.5,1/60,0.125)),
                      17,23,rng.choice((3.25,1000000.0)),rng.choice((3.2501,3.28,1000000.2)),64,rng.getrandbits(32)])
    # 팔레트 결과는 실제 명령이 계산한다. 입력 RGB에는 포화 경계와 임의 예약 바이트를 섞는다.
    result=[]
    for case in cases:
        count=case[5];palette=bytes(rng.randrange(256) for _ in range(count*4))
        if count:palette=bytes((0,191,255,137))+palette[4:]
        result.append((*case,palette.hex()))
    return result


class FlashOracle(OwnerOracle):
    """번개 몸체/현재 색 읽기는 실제 실행하고 명시한 메모리·부모·표시 경계만 기록한다."""
    def __init__(self,edition):
        """읽기 전용 PE와 동일 PC의 함수 범위·합성 SEH 저장소를 준비한다."""
        super().__init__(edition);self.b=SPEC[edition];self.mu.mem_map(0,0x1000)
        self.exports=[ROOT/f'extracted/paletteflash/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.returns=0;self.boundaries=collections.Counter()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 실제 내보낸 불연속 몸체만 허용한다. OS 코드는 범위에 포함하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.stubs={self.b[name]:name for name in ('new','parent','attach','apply','kill','base')}
        self.write_ranges=[(0,4),(OBJECT,OBJECT+0x840)]

    def read(self,address,format):
        """합성 x86 메모리의 정해진 크기 인자를 읽는다."""
        return struct.unpack(format,self.mu.mem_read(address,struct.calcsize(format)))

    def ret(self,purge=0,value=0):
        """명시한 경계의 호출 규약대로 반환한다. 원본 몸체 내부를 건너뛰는 데 사용하지 않는다."""
        stack=self.mu.reg_read(UC_X86_REG_ESP);target,=self.read(stack,'<I')
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,stack+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def on_instruction(self,mu,address,size,data):
        """명시한 경계는 인자를 검증/기록하고 나머지는 부모 클래스의 실제 몸체 감사로 보낸다."""
        name=self.stubs.get(address)
        if not name:return super().on_instruction(mu,address,size,data)
        self.boundaries[name]+=1;sp=mu.reg_read(UC_X86_REG_ESP)
        if name=='new':
            if self.read(sp+4,'<I')!=(0x840,):raise RuntimeError('번개 할당 크기 오류')
            self.ret(value=OBJECT)
        elif name=='parent':self.ret(value=1)
        elif name=='attach':
            _,parent,sid,flags=self.read(sp+4,'<4I')
            if (parent,sid,flags)!=(1,0,16) or mu.reg_read(UC_X86_REG_ECX)!=OBJECT:raise RuntimeError('번개 등록 인자 오류')
            target,=self.read(sp,'<I');mu.reg_write(UC_X86_REG_ESP,sp+16)
            mu.mem_write(sp+16,struct.pack('<I',target));mu.reg_write(UC_X86_REG_EIP,self.b['init'])
        elif name=='apply':
            start,count,pointer,apply=self.read(sp+4,'<4I')
            if apply!=1:raise RuntimeError('번개 팔레트 적용 인자 오류')
            if pointer==0:
                if (start,count)!=(0,256):raise RuntimeError('번개 전체 복구 인자 오류')
                self.trace.append('restore')
            else:
                if pointer!=OBJECT+0x40c:raise RuntimeError('번개 변경 색 주소 오류')
                self.trace.append(f'apply:{start}:{count}:'+bytes(mu.mem_read(pointer,count*4)).hex())
            self.ret()
        elif name=='kill':
            if mu.reg_read(UC_X86_REG_ECX)!=OBJECT or self.read(sp+4,'<I')!=(0,):raise RuntimeError('번개 종료 인자 오류')
            self.trace.append('kill');self.ret(4)
        else:self.ret()

    def call(self,entry,args,control):
        """전체 진입/정상 반환·스택·보존 레지스터·SEH·x87 균형과 허용 쓰기를 확인한다."""
        mu=self.mu
        # CD 프레임은 아직 쓰지 않은 STACK-8의 double을 읽는다. 이 입력 경계를 매 호출 0으로 고정하여 앞 관찰의 스택이 섞이지 않게 한다.
        mu.mem_write(STACK-0x100,bytes(0x100));mu.mem_write(STACK,struct.pack('<I',STOP)+args)
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_EBP:0x23456789,UC_X86_REG_ESI:0x34567890,UC_X86_REG_EDI:0x45678901}
        # 네 호출 보존 레지스터에 서로 다른 표식을 넣는다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.instructions=0
        mu.emu_start(entry,STOP,count=2000000)
        purge=4 if entry==self.b['cancel'] and self.edition!='originals' else 0
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+purge:
            raise RuntimeError(f'번개 반환/스택 오류: {self.edition} {entry:08x} EIP={mu.reg_read(UC_X86_REG_EIP):08x} ESP={mu.reg_read(UC_X86_REG_ESP):08x}')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('번개 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800 or self.read(0,'<I')!=(0,):raise RuntimeError('번개 x87/SEH 오류')
        self.returns+=1

    def run(self,case,control):
        """생성/초기화 뒤 한 프레임 또는 중단의 원본 상태·색·경계 호출 순서를 관찰한다."""
        op,full,offset,duration,start,count,wall,now,step,solid,palette=case;mu=self.mu;b=self.b;self.trace=[]
        mu.mem_write(OBJECT,bytes(0x840));mu.mem_write(b['palette'],bytes(1024));mu.mem_write(b['palette']+start*4,bytes.fromhex(palette))
        mu.mem_write(b['mode'],struct.pack('<I',full));mu.mem_write(b['wall'],struct.pack('<d',wall))
        mu.mem_write(b['first'],struct.pack('<I',start));mu.mem_write(b['last'],struct.pack('<I',(start+count-1)&0xffffffff))
        mu.mem_write(b['ready'],struct.pack('<I',1));mu.mem_write(b['challenge'],bytes(4))
        if op=='entry':self.call(b['entry'],struct.pack('<i',offset),control)
        else:
            mu.mem_write(OBJECT+0x820,struct.pack('<IIiId',start,count,offset,solid,duration));self.call(b['init'],b'',control)
            mu.mem_write(OBJECT+0x838,struct.pack('<i',step))
        initial_current=bytes(mu.mem_read(OBJECT+0x40c,count*4)).hex()
        if op=='cancel':self.call(b['cancel'],bytes(4) if self.edition!='originals' else b'',control)
        else:mu.mem_write(b['wall'],struct.pack('<d',now));self.call(b['frame'],b'',control)
        if bytes(mu.mem_read(b['palette']+start*4,count*4)).hex()!=palette:raise RuntimeError('번개 입력 팔레트 변경')
        original=bytes(mu.mem_read(OBJECT+0xc,count*4)).hex();current=bytes(mu.mem_read(OBJECT+0x40c,count*4)).hex()
        interval,last=self.read(OBJECT+0x810,'<dd');remaining,initialized=self.read(OBJECT+0x838,'<ii')
        # 소멸 뒤 원본 flag는 읽을 수 없는 상태다. 안전 API의 중복 취소 방지를 위해 cancel 결과는 종료 상태로 정규화한다.
        if op=='cancel':initialized=0
        return [initial_current or '-',original or '-',current or '-',bits(interval),bits(last),remaining,initialized,'|'.join(self.trace) or '-']


def fields(case):
    """기대값 없이 입력만 TSV로 직렬화한다."""
    op,full,offset,duration,start,count,wall,now,step,solid,palette=case
    return [op,full,offset,bits(duration),start,count,bits(wall),bits(now),step,solid,palette or '-']


def generate(smoke=False):
    """각 원본을 두 x87 정밀도로 실행해 일치하는 실제 관찰만 fixture로 저장한다."""
    cases=inputs()[:8] if smoke else inputs();rows=[];editions={}
    paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/paletteflash-functions.json',FIXTURE}
    # 세 판본에서 같은 입력을 원래 기계어로 각각 실행한다.
    for edition in SPECS:
        oracle=FlashOracle(edition)
        for case in cases:
            first=oracle.run(case,CONTROLS[0]);second=oracle.run(case,CONTROLS[1])
            if first!=second:raise RuntimeError(f'번개 x87 정밀도 차이: {edition} {fields(case)} {first} {second}')
            rows.append([edition,*fields(case),*first])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 번개 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 원본 번개 생성/초기화/프레임/중단의 실제 x86 관찰. 게임/OS 실행 아님.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-10',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},
        limits=['메모리 할당·부모 조회/등록·OS 팔레트 적용·프로세스 해제/기반 소멸자는 명시 경계로 대체',
                '등록 경계는 원본 초기화 몸체를 실제 호출; 실제 Squid/SpStore 연결과 DirectDraw 장치는 실행하지 않음',
                'CD/10.37 프레임의 초기화되지 않은 STACK-8 입력을 매 호출 0으로 고정; 복원 목표 10.78에는 이 버그가 없음',
                '중단 뒤 소멸된 객체의 initialized는 안전 API 종료 상태 0으로 정규화']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """입력 순서·SHA·실제 몸체 진입/정상 반환과 명시 경계 수를 재감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 입력 PE/도구/내보내기/fixture의 바이트를 모두 감사한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    cases=inputs()
    if report['controls']!=list(CONTROLS) or report['os_calls'] or len(rows)!=report['total'] or set(report['editions'])!=set(SPECS):raise RuntimeError('번개 근거 전체 계수 오류')
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];b=SPEC[edition]
        if len(selected)!=len(cases) or item['cases']!=len(cases) or item['returns']!=len(cases)*4 or item['assertions']:raise RuntimeError('번개 입력/반환 수 오류')
        expected=collections.Counter();boundary=collections.Counter()
        for row,case in zip(selected,cases):
            if row[1:12]!=list(map(str,fields(case))) or len(row)!=20:raise RuntimeError('번개 입력/순서 오류')
            op=case[0];expected[f'{b["init"]:08x}']+=2;expected[f'{b["read"]:08x}']+=4
            expected[f'{b["cancel"] if op=="cancel" else b["frame"]:08x}']+=2
            if op=='entry':
                expected[f'{b["entry"]:08x}']+=2
                if edition=='originals':expected['00470d40']+=2
                boundary.update(new=2,parent=2,attach=2)
            if op=='cancel':
                boundary['base']+=2
                if edition!='originals':expected['004b165b']+=2
            # 표시/해제 횟수는 실제 관찰 trace의 각각의 경계를 센다. 순서는 fixture 자체의 SHA로 보호한다.
            for event in row[-1].split('|'):
                if event.startswith('apply:') or event=='restore':boundary['apply']+=2
                elif event=='kill':boundary['kill']+=2
        if dict(expected)!=item['native_calls'] or dict(boundary)!=item['boundaries']:raise RuntimeError('번개 실제 몸체/경계 호출 수 오류')
    print(f"번개 팔레트 감사 통과: {report['total']}개")


def main():
    """전체 생성·짧은 x86 실행·근거 재감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
