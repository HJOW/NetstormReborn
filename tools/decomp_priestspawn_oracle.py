#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 세 PE의 사제 생성 나선 탐색과 Carrier 검사 래퍼를 대조한다.

스냅·132개 후보·공간/경계 조건·WORD 쓰기·Carrier genus 분기·전체 반환은 실제 실행한다.
배치 검사·생성·가상 소유자/Pop/Carrier 검사·표면 알림만 함수 진입에서 대체한다. 게임/OS 실행은 없다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, POOL, STACK, STOP, digest
from decomp_damageablepredestroy_oracle import CONTROLS, bits
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 독립 공간·관찰 슬롯·합성 가상 표와 세 가상 함수 주소다.
SPOTS,NEW=0x14000000,100
VTABLE,OWNER,POP,CARRIER_CHECK=STOP+0x100,STOP+0x300,STOP+0x400,STOP+0x500
# 실제 함수와 전역의 판본별 주소다.
SPAWN={
 'originals':dict(entry=0x44b2e0,snap=0x41d7a0,ftol=0x4e49c0,place=0x49b510,create=0x4af530,
    word=0x4ac220,notify=0x4214a0,finish=0x426230,genus=0x4ac200,spots=0x5c7c44,bridge=0x5411a0),
 'originalCD':dict(entry=0x461430,snap=0x440a80,ftol=0x4f161c,place=0x445200,create=0x4ab390,
    word=0x4abb00,notify=0x448c10,finish=0x4e38f0,genus=0x4abae0,spots=0x52fe48,bridge=0x51ca8c),
}
SPAWN['original1037']=dict(SPAWN['originalCD'])
# 기존 fixture/감사와 분리한 새 관찰 경로다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priestspawn-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestspawn-evidence.json'


def inputs():
    """기대 위치를 계산하지 않고 독립 배치 반환/지도/좌표 입력을 만든다."""
    cases=[]
    patterns=((0,0),(6,0),(16,1),(22,0xffffffff),(6,1),(6,3),(6,132),(6,-2))
    # 0 지도에서는 검사 생략, 그 밖에서는 첫/세 번째/마지막 성공과 전체 실패를 교차한다.
    for pos,word,pattern,typ in itertools.product(((20,21),(20.00001,21.00001),(20.0001,21.0001),(20.75,21.9),(0,0),(255,255)),
        (0,8,0x8108,0xffff),patterns,(158,82)):
        cases.append(dict(x=bits(pos[0]),y=bits(pos[1]),type=typ,word=word,spot=pattern[0],pattern=pattern[1],change=0))
    # 현재 spot을 검사 콜백에서 지우거나 생성/Pop 중 다리 전역·새 타입을 바꾼다.
    for i in range(24):
        cases.append(dict(x=bits(20.75),y=bits(21.9),type=(158,82)[i%2],word=(0x8081,0xffff)[i%2],
            spot=(16,6,22)[i%3],pattern=(1,3,132,-2)[i%4],change=1+i%3))
    # Pop 뒤 타입의 genus를 현재 값으로 읽는 Carrier 검사 분기를 별도로 관찰한다.
    for case in cases:case['after_genus']=0x210000
    # Carrier genus 없는 상태 검사는 패치에서 무조건 assert다. 정상 CD 두 판본만 관찰한다.
    for word in (0,1,8,0x8081,0x8108,0xffff):
        cases.append(dict(x=bits(20.75),y=bits(21.9),type=158,word=word,spot=0,pattern=0,change=3,after_genus=0))
    return cases


def placement_result(pattern,count):
    """배치 경계의 외부 반환 입력이다. 나선/공간/생성의 기대 판단은 하지 않는다."""
    if pattern==0xffffffff:return pattern
    if pattern==-2:return count%2
    return int(pattern>0 and count>=pattern)


class SpawnOracle(OwnerOracle):
    """실제 사제 생성 및 Carrier 검사 래퍼의 전체 명령/반환을 관찰한다."""
    def __init__(self,edition):
        """PE/타입/공간과 허용 코드 범위 및 가상 호출 표를 준비한다."""
        super().__init__(edition);self.p=SPAWN[edition];self.stub_calls=collections.Counter();self.exports=[]
        self.mu.mem_map(SPOTS,0x10000);self.mu.mem_write(self.p['spots'],struct.pack('<I',SPOTS))
        self.mu.mem_write(VTABLE+0x74,struct.pack('<I',OWNER));self.mu.mem_write(VTABLE+0x90,struct.pack('<I',POP));self.mu.mem_write(VTABLE+0xcc,struct.pack('<I',CARRIER_CHECK))
        self.exports=[ROOT/f'extracted/priestspawn/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra의 실제 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 각 몸체 범위를 독립적으로 검사한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def ret(self,purge=0):
        """하위 함수 경계의 정상 반환/thiscall 인자 정리만 대체한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);address=struct.unpack('<I',self.mu.mem_read(esp,4))[0]
        self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,address)

    def on_instruction(self,mu,address,size,data):
        """정해진 하위 경계만 기록하고 나선/WORD/Carrier 검사 래퍼는 그대로 실행한다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP)
        if address==p['place']:
            typ,x,y,flags,owner,mode=struct.unpack('<6I',mu.mem_read(esp+4,24));self.queries+=1
            if typ!=self.case['type'] or flags or mode or owner!=(self.case['word']&0x7f) or mu.reg_read(UC_X86_REG_ECX)!=TYPES+typ*self.type_stride:raise RuntimeError('배치 검사 인자/this 오류')
            value=placement_result(self.case['pattern'],self.queries)
            self.events.append(f'Q:{typ}:{x}:{y}:{flags}:{owner}:{mode}:{value}');self.stub_calls['Q']+=1
            if self.case['change']==1 and self.queries==1:
                xf,yf=struct.unpack('<ff',struct.pack('<II',x,y));cell=int(yf)*256+int(xf)
                mu.mem_write(SPOTS+cell,bytes((0,)));mu.mem_write(SPOTS+cell-256,bytes((0,)))
            if self.case['change']==2:mu.mem_write(p['bridge'],struct.pack('<I',typ))
            mu.reg_write(UC_X86_REG_EAX,value);self.ret(24);return
        if address==p['create']:
            typ,flags=struct.unpack('<II',mu.mem_read(esp+4,8))
            if typ!=self.case['type'] or flags:raise RuntimeError('생성 인자 오류')
            raw=bytearray([0xcd]*self.stride);raw[:4]=struct.pack('<I',VTABLE);raw[10]=typ;raw[11]=0
            mu.mem_write(self.slot(NEW),bytes(raw));mu.reg_write(UC_X86_REG_EAX,self.slot(NEW))
            self.events.append(f'N:{typ}:{flags}:{NEW}');self.stub_calls['N']+=1
            if self.case['change']==2:mu.mem_write(p['bridge'],struct.pack('<I',82))
            self.ret();return
        if address==p['notify']:
            sid=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
            if sid!=NEW:raise RuntimeError('표면 검사 SID 오류')
            self.events.append(f'S:{sid}');self.stub_calls['S']+=1;self.ret();return
        if address in (OWNER,POP,CARRIER_CHECK):
            if mu.reg_read(UC_X86_REG_ECX)!=self.slot(NEW):raise RuntimeError('가상 호출 this 오류')
            if address==OWNER:
                owner=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
                if owner!=(self.case['word']&0x7f):raise RuntimeError('고정 소유자 오류')
                self.events.append(f'O:{NEW}:{owner}');self.stub_calls['O']+=1;self.ret(4)
            elif address==POP:
                x,y,flags=struct.unpack('<3I',mu.mem_read(esp+4,12))
                if flags:raise RuntimeError('Pop flags 오류')
                self.events.append(f'L:{NEW}:{x}:{y}:{flags}');self.stub_calls['L']+=1
                if self.case['change']==3:
                    mu.mem_write(self.slot(NEW)+10,bytes((82,)))
                    mu.mem_write(TYPES+82*self.type_stride+0xec,struct.pack('<I',self.case['after_genus']))
                self.ret(12)
            else:self.events.append(f'K:{NEW}');self.stub_calls['K']+=1;self.ret()
            return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """전체 cdecl 반환·보존 레지스터/x87·허용 쓰기와 raw/map을 관찰한다."""
        self.case=case;self.events=[];self.queries=0;self.instructions=0;self.write_ranges=[(self.slot(NEW)+12,self.slot(NEW)+14)]
        self.mu.mem_write(SPOTS,bytes((case['spot'],))*0x10000);self.mu.mem_write(self.slot(NEW),bytes([0xcd])*self.stride)
        self.mu.mem_write(self.p['bridge'],struct.pack('<I',82))
        if self.edition=='originals':self.mu.mem_write(0x5e4794,bytes(4))
        # 정상 Carrier 검사 입력의 Carrier genus다. Pop 뒤 타입 변경도 현재 값을 다시 읽는다.
        for typ in (158,82):self.mu.mem_write(TYPES+typ*self.type_stride+0xec,struct.pack('<I',0x210000))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 원본이 보존해야 하는 네 레지스터에 서로 다른 기록값을 넣는다.
        for reg,value in preserved.items():self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_ECX,0);self.mu.reg_write(UC_X86_REG_EDX,0);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.mem_write(STACK,struct.pack('<5I',STOP,case['x'],case['y'],case['type'],case['word']))
        self.mu.emu_start(self.p['entry'],STOP,count=30000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('사제 생성 cdecl 반환/스택 오류')
        if any(self.mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('사제 생성 보존 레지스터 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('사제 생성 x87 복구 오류')
        bridge=struct.unpack('<I',self.mu.mem_read(self.p['bridge'],4))[0]
        return [bridge,';'.join(self.events),bytes(self.mu.mem_read(self.slot(NEW),self.stride)).hex(),zlib.adler32(bytes(self.mu.mem_read(SPOTS,0x10000)))]


def generate(smoke=False):
    """두 정밀도의 실제 관찰이 같은 입력을 fixture/SHA 근거에 저장한다."""
    cases=inputs();rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/priestspawn-functions.json',FIXTURE}
    if smoke:cases=cases[::19]
    # 세 실제 PE에서 각 입력의 정상 반환을 독립적으로 관찰한다.
    for edition in SPECS:
        oracle=SpawnOracle(edition)
        count=0
        # 패치의 무조건 assert 입력은 정상 반환/OS 없는 fixture에서 제외한다.
        for case in cases:
            if edition=='originals' and not case['after_genus']:continue
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError('사제 생성 x87 정밀도 차이')
            rows.append([edition,*[case[key] for key in ('x','y','type','word','spot','pattern','change','after_genus')],*values[0]]);count+=1
        editions[edition]=dict(cases=count,native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 사제 생성 {count}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 사제 나선/스냅/WORD/Carrier 검사 래퍼. 배치/생성/가상 효과만 대체.\n# edition x y type word spot pattern change afterGenus bridge events slot mapAdler\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',normal_return=True,os_calls=0,controls=list(CONTROLS),editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['스냅/132개 나선 후보/WORD/Carrier genus 래퍼 실제 실행; 중간 구간 대체 없음',
            '배치 검사/생성/가상 소유자/Pop/Carrier 검사/표면 알림 함수 진입 대체',
            '패치 정상 탐색 실패 입력은 debug 전역 0; 일반 Pop/배치 검사 공간 효과와 GUI 완성 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/행 수/실제 함수 진입과 외부 경계의 호출 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or not report['normal_return'] or report['os_calls']:raise RuntimeError('사제 생성 근거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];p=SPAWN[edition];n=len(selected)
        if n!=item['cases'] or item['assertions'] or item['native_calls'].get(f'{p["entry"]:08x}')!=2*n:raise RuntimeError('사제 생성 실제 진입 오류')
        if item['native_calls'].get(f'{p["snap"]:08x}')!=2*n or item['native_calls'].get(f'{p["ftol"]:08x}')!=8*n:raise RuntimeError('실제 스냅/절삭 누락')
        for marker in ('Q','N','S','O','L','K'):
            count=2*sum(sum(part.split(':')[0]==marker for part in row[10].split(';')) for row in selected)
            if count!=item['substitutions'].get(marker,0):raise RuntimeError(f'대체 수 오류: {marker}')
        created=item['substitutions'].get('N')
        if any(item['native_calls'].get(f'{p[key]:08x}')!=created for key in ('word','finish')):raise RuntimeError('생성 후 WORD/Carrier 검사 래퍼 누락')
        if any(item['substitutions'].get(marker)!=created for marker in ('O','L')):raise RuntimeError('생성 후 가상 호출 누락')
        if item['native_calls'].get(f'{p["genus"]:08x}')!=created*(2 if edition=='originals' else 1):raise RuntimeError('현재 Carrier genus 조회 누락')
        if not item['substitutions'].get('K') or item['substitutions']['K']>created:raise RuntimeError('Carrier 검사 분기 오류')
        if any(item['native_calls'].get(f'{p[key]:08x}',0) for key in ('place','create','notify')):raise RuntimeError('하위 대체/실제 실행 혼합')
    print(f'priestspawn 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
