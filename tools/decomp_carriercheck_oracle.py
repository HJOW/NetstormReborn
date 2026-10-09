#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Carrier +0xcc와 사제 +0xc8→실제 낙하 요청을 세 PE에서 정상 반환까지 대조한다. 게임/OS 실행은 없다."""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_priestfall_oracle import FallOracle, FALL, CODES, SURFACES, TEMP
from decomp_prieststate_oracle import CONTROLS, SPOTS, bits
from decomp_priestpostpop_oracle import MEMORY, PRIEST_TYPE
from decomp_owner_oracle import ROOT, SPECS, TYPES, LISTS, TARGET, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 원본 조회/가상 낙하와 새 결과 경로다. 합성 가상 주소는 명시 callback 입력에만 사용한다.
CHECK = {'originals':(0x426fc0,0x494e80),'originalCD':(0x4e50d0,0x40d770)}
CHECK['original1037'] = CHECK['originalCD']
VTABLE, VIRTUAL = LISTS+0x3000,STOP+0x200
FIXTURE = ROOT/'cpppj/tests/fixtures/carriercheck-x86.tsv'
REPORT = ROOT/'cpppj/recovery-carriercheck-evidence.json'
KEYS = ('kind','genus','floor','ceil','side','authority','allocated','change','virtual','reply','x','y','extra','state')
# 서로 다른 almost-ceil/단순 절삭 칸 및 요청 자체가 계산하지 않는 지도 밖 좌표다.
COORDS = ((20.75,21.9),(20.000099182128906,21.000099182128906),(-2.0,21.0),
    (255.0,255.0),(256.0,1.0),(-0.0,0.0))


def inputs():
    """조건 격자/외부 반환 입력을 만든다. 낙하 여부/반환/사건 기대값은 계산하지 않는다."""
    rows=[]
    # 제외 genus와 지면 2/4 비트, 가상 호출의 정수 반환, 실제 사제 분배를 교차한다.
    for genus,floor,ceil,virtual,side,reply in itertools.product(
            (0,0x10000,0x20000,0x100000,0x200000,0x320000,0x400000,0x210000),
            (0,6),(0,2,6),(0,1),(65,74),(0,0x80000000)):
        n=len(rows);x,y=COORDS[n%2]
        rows.append(dict(kind='Check',genus=genus,floor=floor,ceil=ceil,virtual=virtual,side=side,
            reply=reply,authority=(n//2)%2,allocated=(n//4)%2,change=0,x=bits(x),y=bits(y),
            extra=(0,1,8,9,32)[n%5],state=(0,2,4,8)[n%4]))
    # 낙하 wrapper는 J일 때 요청하지 않는다. 나머지는 확보 실패와 현재 권한/좌표 변이도 교차한다.
    for side,authority,allocated,change,coord in itertools.product((65,74,255),(0,1),(0,1),range(4),COORDS):
        rows.append(dict(kind='Try',genus=0x210000,floor=0,ceil=0,virtual=1,side=side,reply=0,
            authority=authority,allocated=allocated,change=change,x=bits(coord[0]),y=bits(coord[1]),extra=32,state=0))
    # 제외 조건이 있으면 비유한 좌표/잘못된 프레임의 조회도 생략한다.
    for genus in (0x20000,0x100000,0x200000,0x210000):
        rows.append(dict(kind='Check',genus=genus,floor=0,ceil=0,virtual=1,side=255,reply=0,
            authority=1,allocated=1,change=0,x=0x7fc12345,y=0x7f800000,extra=0,state=0))
    return rows


class CheckOracle(FallOracle):
    """기존 실제 낙하 요청/공유 생성과 최신 +0xcc/+0xc8 원본 몸체를 함께 실행한다."""
    def __init__(self,edition):
        """기존 허용 범위와 최신 읽기 전용 내보내기를 합치며 합성 callback만 별도 대체한다."""
        super().__init__(edition)
        self.exports.extend(ROOT/f'extracted/carriercheck/{edition}/{n}' for n in ('creation.c','functions.tsv'))
        with self.exports[-1].open(encoding='utf-8') as fp:
            # 최신 Ghidra 몸체의 불연속 범위를 실행 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_write(VTABLE,bytes(0xd0));self.mu.mem_write(VTABLE+0xc8,struct.pack('<I',VIRTUAL))

    def on_instruction(self,mu,address,size,data):
        """합성 타입의 미복원 +0xc8 반환만 공급하며 실제 사제 낙하는 부모 실행기를 그대로 쓴다."""
        if address==VIRTUAL:
            if mu.reg_read(UC_X86_REG_ECX)!=self.slot(TARGET):raise RuntimeError('가상 낙하 this 오류')
            self.stub_calls['virtual_c8']+=1;self.events.append(f'V:{TARGET}:{self.case["reply"]}')
            self.ret(result=self.case['reply']);return
        super().on_instruction(mu,address,size,data)

    def run_check(self,case,control):
        """정상 정수 반환/스택/SEH/x87와 전체 raw·사건·현재 권한을 관찰한다."""
        # 낙하 요청의 외부 변이는 기존 실행기와 같은 입력 계약을 쓴다.
        self.case=dict(case,kind='Begin');self.events=[];raw=self.slot(TARGET);f,s=self.f,self.s
        data=bytearray(self.stride);struct.pack_into('<I',data,0,s['vtable'] if case['virtual'] else VTABLE)
        data[10],data[11],data[self.o['extra']],data[self.o['owner']]=PRIEST_TYPE,case['state'],case['extra'],1
        struct.pack_into('<II',data,14,case['x'],case['y']);struct.pack_into('<I' if self.stride==50 else '<H',data,26,20)
        self.mu.mem_write(raw,bytes(data));self.mu.mem_write(MEMORY,bytes(0x28))
        base=TYPES+PRIEST_TYPE*self.type_stride;self.mu.mem_write(base,bytes(self.type_stride))
        self.mu.mem_write(base,struct.pack('<I',200));self.mu.mem_write(base+0xec,struct.pack('<I',case['genus']))
        self.mu.mem_write(base+0x124,struct.pack('<I',CODES));self.mu.mem_write(CODES,bytes([case['side'],80,1,0]))
        self.mu.mem_write(self.s['spot'],struct.pack('<I',SPOTS));self.mu.mem_write(SPOTS,bytes(65536))
        self.mu.mem_write(SPOTS+21*256+20,bytes([case['floor']]))
        self.mu.mem_write(SPOTS+22*256+21,bytes([case['ceil']]))
        self.mu.mem_write(s['boss'],struct.pack('<I',case['authority']));self.mu.mem_write(f['shared'],struct.pack('<I',61))
        self.mu.mem_write(f['now'],struct.pack('<d',12.5));self.mu.mem_write(0x5c8488 if self.stride==50 else 0x539600,bytes(4))
        self.mu.mem_write(0,struct.pack('<I',0xffffffff))
        self.write_ranges=[(0,4),(raw,raw+self.stride),(MEMORY,MEMORY+0x28)]
        saved=(0x1234,0x2345,0x3456,0x4567)
        # 비휘발 레지스터는 이 실행기 입력값을 함수 반환 때까지 보존해야 한다.
        for reg,value in zip((UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP),saved):self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,raw);self.mu.mem_write(STACK,struct.pack('<I',STOP))
        entry=CHECK[self.edition][case['kind']=='Try'];self.mu.emu_start(entry,STOP,count=30000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('정상 반환/스택 오류')
        if tuple(self.mu.reg_read(r) for r in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP))!=saved:raise RuntimeError('보존 레지스터 오류')
        if self.u32(0)!=0xffffffff or self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('SEH/x87 복구 오류')
        if any(e.startswith('R:') for e in self.events):
            if self.u32(MEMORY+24)!=0x25b or self.u32(MEMORY+28)!=bits(0.01) or self.u32(MEMORY+32)!=0:raise RuntimeError('실제 공유 생성 필드 오류')
        self.returns+=1
        return [self.mu.reg_read(UC_X86_REG_EAX),';'.join(self.events) or '-',bytes(self.mu.mem_read(raw,self.stride)).hex(),self.u32(s['boss'])]


def generate(smoke=False):
    """세 PE/두 정밀도 관찰과 명시 대체·정상 반환·SHA 근거를 새 파일에 저장한다."""
    cases=inputs();cases=cases[::73] if smoke else cases;rows=[];reports={}
    paths={Path(__file__),ROOT/'tools/decomp_priestfall_oracle.py',ROOT/'tools/decomp_owner_oracle.py',
        ROOT/'tools/decomp_prieststate_oracle.py',ROOT/'tools/decomp_priestpostpop_oracle.py',
        ROOT/'tools/ghidra/carriercheck-functions.json',ROOT/'tools/ghidra/priestfall-functions.json',FIXTURE}
    for edition in SPECS:
        oracle=CheckOracle(edition)
        # 동일 입력을 원본 명령으로 두 번 실행하며 결과가 같을 때 한 행을 저장한다.
        for case in cases:
            result=[oracle.run_check(case,c) for c in CONTROLS]
            if result[0]!=result[1]:raise RuntimeError('x87 정밀도별 관찰 차이')
            rows.append([edition,*[case[k] for k in KEYS],*result[0]])
        reports[edition]=dict(binary=oracle.spec['binary'],cases=len(cases),executions=oracle.returns,
            native_calls=dict(oracle.native_calls),stub_calls=dict(oracle.stub_calls),assertions=oracle.assertions,instructions=oracle.instructions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        print(f'{edition}: {len(cases)}개 입력 / {oracle.returns}회 정상 반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# Carrier +0xcc / 사제 +0xc8 및 실제 낙하 요청 원본 관찰. 미복원 공간/소리/부착만 명시 대체.\n'
        '# edition '+' '.join(KEYS)+' result events raw authority_after\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(schema=1,primary_target='10.78',last_decompiled_host='HJOW-Athlon',date='2026-10-10',total=len(rows),
        controls=list(CONTROLS),editions=reports,os_calls=0,stopped_before=None,
        stubbed=['synthetic nonpriest virtual C8','new(0x28)','BaseProcess Attach','ensure shield','Unpop/Repop','positional sound'],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·행/입력 순서·반환/실제 실행 수를 검사한다. C++ 생산 코드를 기대값으로 쓰지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for path,sha in report['files'].items():
        if digest(ROOT/path)!=sha:raise RuntimeError(f'SHA 불일치: {path}')
    rows=[l.split('\t') for l in FIXTURE.read_text(encoding='utf-8').splitlines() if l and not l.startswith('#')]
    expected=[[e,*map(str,(c[k] for k in KEYS))] for e in SPECS for c in inputs()]
    if len(rows)!=report['total'] or [r[:15] for r in rows]!=expected or any(len(r)!=19 for r in rows):raise RuntimeError('입력/행/폭 오류')
    for edition,r in report['editions'].items():
        if r['assertions'] or r['executions']!=len(inputs())*2 or not all(r['native_calls'].get(f'{a:08x}') for a in CHECK[edition]):raise RuntimeError('정상 반환/실제 함수 실행 근거 오류')
    print(f'Carrier 조회/사제 가상 낙하 감사: {len(rows)}개 입력, 세 PE·두 정밀도·SHA 통과')


def main():
    """일반 생성/읽기 전용 감사/축소 실제 명령 확인을 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
