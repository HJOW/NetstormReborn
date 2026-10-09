#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 후보 권한 helper를 실행한다. Player locator만 진입에서 명시 대체한다.

패치의 방향 관계 helper도 독립 호출한다. 전체 MayPlace/Player 몸체/게임/OS는 실행하지 않는다.
"""
import argparse
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,STACK,STOP,digest
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 후보 helper·명시 대체 경계·현재 전역과 원본 관계 표 주소다.
SPEC={
 'originals':dict(entry=0x462cb0,related=0x4629e0,anchor=0x48fdb0,ready=0x540414,
    editor=0x5c85a4,allies=0x540cb0,anyOwner=0x59ab28,relations=0x595200),
 'originalCD':dict(entry=0x45bae0,anchor=0x406f80,ready=0x5207f8,
    editor=0x518904,allies=0x50f824,anyOwner=0x51cbf8,relations=0x50f6e0),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 독립 기대값·근거 파일과 고정 입력 순서다. 기대 판정 함수는 없다.
FIXTURE=ROOT/'cpppj/tests/fixtures/canonpermission-x86.tsv'
REPORT=ROOT/'cpppj/recovery-canonpermission-evidence.json'
KEYS=('kind','ready','editor','allies','anyOwner','current','owner','forward','reverse','selected','sid','type','x','y','mutation')


def inputs(edition):
    """조기 반환·방향 관계·DWORD 감김·locator 결과/인자/진입 중 변화 입력을 만든다."""
    cases=[]
    # 전역·소유자·방향 표·조회 반환의 독립 조합을 열거한다.
    for index,values in enumerate(itertools.product((0,1),(0,1),(0,1),(0,1),(0,1,2,8,255),
            (0,1,2,8,0xffffffff,128),((0,0),(1,0),(0,1)),(0,1,50,0xffffffff))):
        ready,editor,allies,any_owner,current,owner,relation,selected=values
        slot=(current*9+owner)&0xffffffff
        # 원본이 실제로 읽는 관계 주소만 81개 배열 계약 안에 제한한다. 조기 반환은 제한하지 않는다.
        if ready and owner and not editor and current!=owner and allies and slot>=81:continue
        sid=65535 if (not ready or not owner) and index%7==0 else (0 if index%5==0 else 50)
        x,y=((0x41a40000,0x41ac0000),(0x80000000,0xbf666666),(0x7fc12345,0x7f7fffff))[index%3]
        cases.append(dict(kind='C',ready=ready,editor=editor,allies=allies,anyOwner=any_owner,current=current,
            owner=owner,forward=relation[0],reverse=relation[1],selected=selected,sid=sid,
            type=(82,83,255)[index%3],x=x,y=y,mutation=index%2))
    if edition=='originals':
        # 방향 관계 helper는 중립/다른 소유자 허용 없이 독립 정상 반환까지 실행한다.
        for editor,allies,current,owner,relation in itertools.product((0,1),(0,1),(0,1,2,8,0xffffffff),
                (0,1,2,8,0xffffffff,128),((0,0),(1,0),(0,1))):
            slot=(current*9+owner)&0xffffffff
            if not editor and current!=owner and allies and slot>=81:continue
            cases.append(dict(kind='R',ready=0,editor=editor,allies=allies,anyOwner=1,current=current,owner=owner,
                forward=relation[0],reverse=relation[1],selected=0,sid=50,type=82,x=0,y=0,mutation=0))
    return cases


class PermissionOracle(OwnerOracle):
    """helper의 실제 몸체/ABI와 명시적인 locator 경계만 실행한다."""
    def __init__(self,edition):
        """새 함수 목록의 실제 범위를 받아 기존 게임의 원본 포인터를 분석용 풀로 재배치한다."""
        super().__init__(edition);self.s=SPEC[edition]
        self.exports=[ROOT/f'extracted/canonpermission/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.substitutions=0;self.related_calls=0;self.candidate_calls=0
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra가 내보낸 함수의 실제 명령만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 함수의 분리된 모든 코드 범위를 실행 허용 목록에 넣는다.
                for part in row['ranges'].split(';'):
                    lo,hi=(int(value,16) for value in part.split('-'));self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """locator 진입에서 값 인자를 관찰하고 지정한 반환/변화만 대체한다. 몸체는 실행하지 않는다."""
        if address==self.s['anchor']:
            sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<4I',mu.mem_read(sp+4,16));self.observed.append(args)
            if self.case['mutation']:
                slot=self.slot(self.raw_sid);mu.mem_write(slot+self.o['owner'],b'\x03');mu.mem_write(slot+10,b'\x54')
                mu.mem_write(slot+14,struct.pack('<II',0x41f00000,0x41f80000))
                mu.mem_write(self.s['editor'],struct.pack('<I',self.case['editor']^1))
            self.substitutions+=1;mu.reg_write(UC_X86_REG_EAX,self.case['selected'])
            mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',mu.mem_read(sp,4))[0]);mu.reg_write(UC_X86_REG_ESP,sp+4);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """입력에서 실제 반환값·경계 인자·raw/전역·스택/x87를 관찰한다."""
        self.case=case;self.instructions=0;self.observed=[];s=self.s;mu=self.mu
        self.raw_sid=case['sid'] if case['sid']<128 else 50
        raw=bytearray([0xcd]*self.stride);raw[10]=case['type'];raw[self.o['owner']]=case['current']&255
        raw[14:22]=struct.pack('<II',case['x'],case['y']);mu.mem_write(self.slot(self.raw_sid),bytes(raw))
        table=[0]*81;forward=(case['current']*9+case['owner'])&0xffffffff;reverse=(case['owner']*9+case['current'])&0xffffffff
        if forward<81:table[forward]=case['forward']
        if reverse<81 and reverse!=forward:table[reverse]=case['reverse']
        mu.mem_write(s['relations'],struct.pack('<81I',*table))
        # 실제 주소에 각 입력 전역의 DWORD를 넣는다.
        for key in ('ready','editor','allies','anyOwner'):mu.mem_write(s[key],struct.pack('<I',case[key]))
        self.write_ranges=[(self.slot(self.raw_sid),self.slot(self.raw_sid)+self.stride),(s['editor'],s['editor']+4)]
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 반환 뒤 보존할 레지스터에 식별 가능한 값을 넣는다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        # 호출자 임시 레지스터는 앞선 입력의 값을 남기지 않는다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.mem_write(STACK,struct.pack('<III',STOP,case['current'] if case['kind']=='R' else case['sid'],case['owner']))
        entry=s['related'] if case['kind']=='R' else s['entry'];mu.emu_start(entry,STOP,count=1000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('권한 helper ABI/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('권한 helper 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('권한 helper x87 오류')
        if case['kind']=='R':self.related_calls+=1
        else:self.candidate_calls+=1
        args=self.observed[0] if self.observed else (0,0,0,0)
        return [mu.reg_read(UC_X86_REG_EAX),len(self.observed),*args,bytes(mu.mem_read(self.slot(self.raw_sid),self.stride)).hex(),
            struct.unpack('<I',mu.mem_read(s['editor'],4))[0],zlib.adler32(mu.mem_read(s['relations'],324))]


def generate(smoke=False):
    """두 정밀도의 실제 관찰이 같으면 새 fixture/근거에만 기록한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/canonpermission-functions.json',FIXTURE}
    # 세 PE 각각에 별도 실행기를 만들어 실제 판본별 관찰을 모은다.
    for edition in SPECS:
        oracle=PermissionOracle(edition);cases=inputs(edition)
        if smoke:cases=cases[::max(1,len(cases)//100)]
        # 같은 입력을 두 정밀도로 실행하고 일치한 원본 관찰만 기록한다.
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError(f'x87 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*values[0]])
        editions[edition]=dict(cases=len(cases),candidates=oracle.candidate_calls,related=oracle.related_calls,
            substitutions=oracle.substitutions,native_calls=dict(oracle.native_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 권한 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 후보 권한/패치 관계 helper 실제 정상 반환. Player locator만 명시 대체. 게임/OS 미실행.\n# edition '+
        ' '.join(KEYS)+' result calls argOwner argType argX argY raw editorOut relationsAdler\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},
        limits=['후보 helper/패치 방향 관계 helper 정상 반환/ABI/보존 레지스터·실제 조기 분기/현재 raw/관계 읽기',
            'Player 작업장/그래프 locator 진입에서 인자/호출 횟수 관찰 후 지정 반환/변화 대체, 몸체 미실행',
            '실제 관계 읽기가 81개 표 밖인 입력 제외, 조기 반환의 큰 SID/owner는 유지',
            '전체 MayPlace/지형/주변 finder/최종 관계·Player locator/게임/OS 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """저장 SHA/행/실제 helper 진입/명시 대체 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 입력 도구·PE·내보내기·fixture의 기록 당시 바이트 동일성을 확인한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('권한 근거 오류')
    # 판본별 실제 helper 진입과 명시 대체 횟수를 저장 행과 대조한다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];s=SPEC[edition]
        if len(selected)!=item['cases'] or item['assertions']:raise RuntimeError('권한 행/assert 오류')
        if item['candidates']!=2*sum(r[1]=='C' for r in selected) or item['native_calls'].get(f'{s["entry"]:08x}',0)!=item['candidates']:raise RuntimeError('실제 후보 helper 누락')
        if item['substitutions']!=2*sum(int(r[17]) for r in selected) or item['native_calls'].get(f'{s["anchor"]:08x}',0):raise RuntimeError('locator 대체 경계 오류')
        if edition=='originals' and (item['related']!=2*sum(r[1]=='R' for r in selected) or item['native_calls'].get(f'{s["related"]:08x}',0)!=item['related']):raise RuntimeError('실제 방향 관계 helper 누락')
    print(f'canonpermission 검증 통과: {len(rows)}개')


def main():
    """전체 생성/표본/저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
