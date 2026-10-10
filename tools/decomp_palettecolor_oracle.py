#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""팔레트 색 검색 몸체를 세 실제 PE의 제한 x86에서 실행한다. 파일 로더/창/게임/OS는 실행하지 않는다."""
import argparse
import collections
import csv
import json
import random
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT, SPECS, OwnerOracle, TYPES, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 실제 검색 함수와 논리 팔레트 포인터 전역이다. 10.37은 CD와 주소 배치가 같다.
SPEC={'originals':(0x4a2820,0x5c7954),'originalCD':(0x4246e0,0x516b7c),'original1037':(0x4246e0,0x516b7c)}
# 입력 난수 시드·두 x87 제어값·합성 논리 팔레트 저장소다. 호스트 주소와 무관하다.
SEED=0x4a2820
CONTROLS=(0x027f,0x037f)
PALETTE=TYPES+0x10000
# 앞 단계 fixture/근거와 분리한 새 관찰 파일이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/palettecolor-x86.tsv'
REPORT=ROOT/'cpppj/recovery-palettecolor-evidence.json'


def inputs():
    """동률·끝 번호·플래그·거리 상한·signed 감김·float32 반올림 입력만 만든다. 결과는 계산하지 않는다."""
    rng=random.Random(SEED);cases=[]
    # 모든 색이 같아지는 동률과 큰 거리의 저장 반올림 뒤 동률을 관찰한다.
    for word in (0,0x00112233,0xffffffff):
        for query in ((0,0,0),(255,255,255),(22,22,255),(181,140,111),(4097,0,0),
                      (0x7fffffff,-0x80000000,65537),(50000,50000,50000),(-4097,4097,1)):
            cases.append(([word]*256,*query))
    ramp=[i|(i<<8)|(i<<16)|(rng.randrange(256)<<24) for i in range(256)]
    # 0~255 각 채널의 원본 값과 RGB 채널 순서를 독립적으로 확인한다.
    for i in range(0,256,7):cases.append((ramp.copy(),i,i,i))
    for index in (0,1,7,8,31,127,128,254,255):
        palette=[0xffffffff]*256;palette[index]=0x55112233
        cases.append((palette,0x33,0x22,0x11))
    # byte 범위 안 RGB와 범위 밖 32비트 입력을 섞는다. 플래그는 임의 값이며 색 검색에 영향이 없어야 한다.
    for index in range(192):
        palette=[rng.getrandbits(32) for _ in range(256)]
        query=[rng.randrange(256) if index%3==0 else rng.randint(-0x80000000,0x7fffffff) for _ in range(3)]
        cases.append((palette,*query))
    return cases


class PaletteOracle(OwnerOracle):
    """검색 함수 한 몸체만 허용하고 스택 밖의 실제 쓰기를 전부 거부한다."""
    def __init__(self,edition):
        """같은 PC에서 내보낸 검색 함수 범위와 원본 PE를 읽기 전용으로 준비한다."""
        super().__init__(edition);self.entry,self.global_palette=SPEC[edition]
        self.exports=[ROOT/f'extracted/palettecolor/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries={self.entry};self.returns=0
        with self.exports[1].open(encoding='utf-8') as fp:
            # 로더도 내보냈지만 실제 실행 허용 범위에는 검색 몸체만 넣는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                if int(row['entry'],16)!=self.entry:continue
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        if not self.allowed:raise RuntimeError('검색 함수 내보내기 누락')
        self.mu.mem_write(self.global_palette,struct.pack('<I',PALETTE));self.write_ranges=[]

    def run(self,case,control):
        """cdecl 전체 진입/반환·레지스터/스택·x87·팔레트 불변을 관찰하고 EAX 색 번호를 반환한다."""
        palette,r,g,b=case;mu=self.mu;raw=struct.pack('<256I',*palette)
        mu.mem_write(PALETTE,raw);mu.mem_write(STACK,struct.pack('<Iiii',STOP,r,g,b))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_EBP:0x23456789,
                   UC_X86_REG_ESI:0x34567890,UC_X86_REG_EDI:0x45678901}
        # 호출 규약이 보존해야 하는 네 레지스터에 서로 다른 표식을 넣는다.
        for register,value in preserved.items():mu.reg_write(register,value)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.instructions=0
        mu.emu_start(self.entry,STOP,count=20000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('검색 반환/스택 불일치')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('검색 보존 레지스터 불일치')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('검색 x87 균형 불일치')
        if bytes(mu.mem_read(PALETTE,1024))!=raw:raise RuntimeError('검색이 논리 팔레트를 변경함')
        result=mu.reg_read(UC_X86_REG_EAX)
        if result>255:raise RuntimeError('검색 결과 범위 오류')
        self.returns+=1;return result


def generate(smoke=False):
    """두 x87 정밀도의 원본 결과를 기록하며 호스트나 C++ 계산으로 기대값을 만들지 않는다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/palettecolor-functions.json',FIXTURE}
    cases=inputs()[:12] if smoke else inputs()
    # 실제 세 PE에서 동일한 입력을 각각 실행한다.
    for edition in SPECS:
        oracle=PaletteOracle(edition)
        for palette,r,g,b in cases:
            actual=[oracle.run((palette,r,g,b),control) for control in CONTROLS]
            if actual[0]!=actual[1]:raise RuntimeError('팔레트 검색 정밀도 차이')
            rows.append([edition,r,g,b,struct.pack('<256I',*palette).hex(),actual[0]])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native_calls=dict(oracle.native_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 색 검색 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 색 검색 전체 몸체의 두 x87 관찰. 파일 로더/창/게임 실행 아님.\n# edition red green blue logical_rgba_hex index\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-10',controls=list(CONTROLS),
        os_calls=0,stubs=0,editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},
        limits=['검색 전체 몸체만 실제 실행; 로더/경로/Win32 팔레트 적용은 정적 판독 및 클론 창 검사',
                'x87 기본 반올림(nearest)의 두 정밀도만 관찰']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """입력 전체·PE/내보내기/도구/fixture SHA·실제 호출/정상 반환·스텁/OS 0을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if report['controls']!=list(CONTROLS) or report['os_calls'] or report['stubs'] or len(rows)!=report['total']:raise RuntimeError('근거 전체 계수 오류')
    if set(report['editions'])!=set(SPECS):raise RuntimeError('판본 누락')
    # 저장한 입력이 생성기의 입력/순서와 정확히 같아야 한다. 기대값 범위도 확인한다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];cases=inputs()
        if len(selected)!=len(cases) or item['cases']!=len(cases) or item['returns']!=len(cases)*2 or item['assertions']:raise RuntimeError('검색 진입/반환 수 오류')
        if item['native_calls']!={f'{SPEC[edition][0]:08x}':len(cases)*2}:raise RuntimeError('검색 실제 몸체 호출 수 오류')
        for row,(palette,r,g,b) in zip(selected,cases):
            if row[1:5]!=[str(r),str(g),str(b),struct.pack('<256I',*palette).hex()] or not 0<=int(row[5])<=255:raise RuntimeError('검색 입력/결과 기록 오류')
    print(f"팔레트 색 검색 감사 통과: {report['total']}개")


def main():
    """전체 생성·짧은 원본 실행·저장 근거 감사 중 요청한 작업을 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
